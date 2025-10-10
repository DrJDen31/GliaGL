#include "network_graph.h"
#include "glia.h"
#include "neuron.h"
#include "meshdata.h"
#include <iostream>
#include <algorithm>
#include <queue>
#include <set>
#include <cmath>

extern MeshData *mesh_data;

// =====================================================================================
// Constructor
// =====================================================================================

NetworkGraph::NetworkGraph(Glia* network, ArgParser *_args) {
    std::cout << "NetworkGraph constructor starting..." << std::endl;
    args = _args;
    glia = network;
    training_mode = false;  // Start in inference mode
    
    // Initialize vertex counts
    connection_vertex_count = 0;
    neuron_vertex_count = 0;
    
    // Initialize default output index (will be set after outputs are identified)
    default_output_index = 0;

    // Spatial layout parameters
    x_left = -5.0f;
    x_right = 5.0f;
    y_span = 4.0f;
    z_span = 4.0f;
    rest_length = 2.5f;  // Increased for better spacing (was 1.0)

    // Physics parameters
    k_connection = 50.0;          // Increased spring stiffness (was 10.0)
    damping = 0.8;                // Increased damping for faster convergence (was 0.5)
    provot_structural_correction = 1.05;  // Tighter constraint (was 1.1)
    timestep = 0.01;              // Physics timestep

    std::cout << "Building network graph..." << std::endl;
    // Build network graph from Glia
    buildFromGlia(network);
    std::cout << "Initializing spatial layout..." << std::endl;
    initializeSpatialLayout();
    std::cout << "Computing bounding box..." << std::endl;
    computeBoundingBox();
    
    // Default output index can be configured:
    // -1 = no default (nothing selected when network is quiet)
    // 0+ = specific output index to select as default
    // For now, default to -1 (no selection when quiet)
    default_output_index = -1;
    std::cout << "Default output index: " << default_output_index 
              << " (use setDefaultOutputIndex() to configure)" << std::endl;
    
    std::cout << "NetworkGraph constructor complete!" << std::endl;
}

// =====================================================================================
// Destructor
// =====================================================================================

NetworkGraph::~NetworkGraph() {
    for (auto* p : particles) {
        delete p;
    }
    particles.clear();
    particle_map.clear();
}

// =====================================================================================
// Build network graph from Glia
// =====================================================================================

void NetworkGraph::buildFromGlia(Glia* network) {
    // Create particles for each neuron in the network
    
    // 1. Create sensory neurons
    std::vector<Neuron*> sensory_neurons;
    network->forEachNeuron([&](Neuron& n) {
        std::string id = n.getId();
        if (id[0] == 'S') {  // Sensory neurons start with 'S'
            sensory_neurons.push_back(&n);
        }
    });

    for (Neuron* neuron : sensory_neurons) {
        std::string id = neuron->getId();
        NeuronParticle* p = new NeuronParticle(id, neuron, NeuronType::SENSORY);
        particles.push_back(p);
        particle_map[id] = p;
    }

    // 2. Create interneurons and outputs (N* and O* neurons)
    std::vector<Neuron*> interneurons;
    network->forEachNeuron([&](Neuron& n) {
        std::string id = n.getId();
        if (id[0] == 'N' || id[0] == 'O') {  // Both N* and O* neurons
            interneurons.push_back(&n);
        }
    });

    for (Neuron* neuron : interneurons) {
        std::string id = neuron->getId();
        NeuronParticle* p = new NeuronParticle(id, neuron, NeuronType::INTERNEURON);
        particles.push_back(p);
        particle_map[id] = p;
    }

    // 3. Build connections between particles
    // We need to extract connection information from Glia
    // For now, we'll use the network structure to build connections

    for (auto* p : particles) {
        Neuron* neuron = p->getNeuron();
        const auto& connections = neuron->getConnections();
        
        for (const auto& conn : connections) {
            std::string target_id = conn.first;
            float weight = conn.second.first;
            
            NeuronParticle* target = getParticleById(target_id);
            if (target) {
                p->addConnection(target, weight);
            }
        }
    }

    // 4. Identify output neurons
    // Use improved heuristics:
    // - Neurons with ID starting with 'O' (explicit output naming convention)
    // - OR neurons with no outgoing connections (leaf nodes)
    
    std::set<std::string> potential_outputs;
    
    for (auto* p : particles) {
        if (p->getType() == NeuronType::INTERNEURON) {
            std::string id = p->getId();
            
            // Heuristic 1: Explicit naming - O* prefix (scalable convention)
            if (!id.empty() && id[0] == 'O') {
                potential_outputs.insert(id);
            }
            // Heuristic 2: No outgoing connections (leaf nodes)
            else if (p->getConnections().empty()) {
                potential_outputs.insert(id);
            }
        }
    }
    
    // Convert to output neurons
    for (const auto& id : potential_outputs) {
        auto* p = getParticleById(id);
        if (p) {
            p->setType(NeuronType::OUTPUT);
            output_neuron_ids.push_back(id);
        }
    }

    std::cout << "Built network graph: " 
              << sensory_neurons.size() << " sensory, "
              << (interneurons.size() - output_neuron_ids.size()) << " interneurons, "
              << output_neuron_ids.size() << " output" << std::endl;
}

// =====================================================================================
// Initialize spatial layout
// =====================================================================================

void NetworkGraph::computeLayerDepths() {
    // Compute connectivity depth for each neuron using BFS from sensory neurons
    std::map<std::string, int> depths;
    std::queue<std::string> to_process;
    std::set<std::string> visited;

    // Start with sensory neurons at depth 0
    for (auto* p : particles) {
        if (p->getType() == NeuronType::SENSORY) {
            depths[p->getId()] = 0;
            to_process.push(p->getId());
            visited.insert(p->getId());
        }
    }

    // BFS to compute depths
    while (!to_process.empty()) {
        std::string current_id = to_process.front();
        to_process.pop();

        NeuronParticle* current = getParticleById(current_id);
        if (!current) continue;

        int current_depth = depths[current_id];

        // Process all outgoing connections
        for (const auto& conn : current->getConnections()) {
            std::string target_id = conn.target->getId();
            
            if (visited.find(target_id) == visited.end()) {
                depths[target_id] = current_depth + 1;
                to_process.push(target_id);
                visited.insert(target_id);
            }
        }
    }

    layer_depths = depths;
}

void NetworkGraph::initializeSpatialLayout() {
    // Compute layer depths for layered layout
    computeLayerDepths();

    // Find max depth for X positioning
    int max_depth = 0;
    for (const auto& kv : layer_depths) {
        max_depth = std::max(max_depth, kv.second);
    }
    if (max_depth == 0) max_depth = 1;  // Avoid division by zero

    // Count neurons by type
    int sensory_count = 0;
    int output_count = 0;
    int interneuron_count = 0;

    for (auto* p : particles) {
        switch (p->getType()) {
            case NeuronType::SENSORY: sensory_count++; break;
            case NeuronType::OUTPUT: output_count++; break;
            case NeuronType::INTERNEURON: interneuron_count++; break;
        }
    }

    // Position neurons
    int sensory_idx = 0;
    int output_idx = 0;
    int interneuron_idx = 0;

    for (auto* p : particles) {
        Vec3f pos;

        switch (p->getType()) {
            case NeuronType::SENSORY:
                pos = computeSensoryPosition(sensory_idx, sensory_count);
                sensory_idx++;
                break;

            case NeuronType::OUTPUT:
                pos = computeOutputPosition(output_idx, output_count);
                output_idx++;
                break;

            case NeuronType::INTERNEURON: {
                int depth = layer_depths[p->getId()];
                pos = computeInterneuronPosition(interneuron_idx, interneuron_count, depth);
                interneuron_idx++;
                break;
            }
        }

        p->setOriginalPosition(pos);
        p->setPosition(pos);
        p->setVelocity(Vec3f(0, 0, 0));
    }
}

Vec3f NetworkGraph::computeSensoryPosition(int index, int total) {
    // Distribute vertically on left plane
    float y = -y_span / 2.0f + (y_span / float(total + 1)) * float(index + 1);
    float z = 0.0f;  // Center depth
    return Vec3f(x_left, y, z);
}

Vec3f NetworkGraph::computeOutputPosition(int index, int total) {
    // Distribute vertically on right plane
    float y = -y_span / 2.0f + (y_span / float(total + 1)) * float(index + 1);
    float z = 0.0f;  // Center depth
    return Vec3f(x_right, y, z);
}

Vec3f NetworkGraph::computeInterneuronPosition(int index, int total, int layer_depth) {
    // Position based on layer depth (X) with random Y/Z spread within layer
    
    // Find max depth
    int max_depth = 0;
    for (const auto& kv : layer_depths) {
        max_depth = std::max(max_depth, kv.second);
    }
    if (max_depth == 0) max_depth = 1;

    // X position based on layer depth
    float x_range = x_right - x_left;
    float x = x_left + (x_range / float(max_depth + 1)) * float(layer_depth);

    // Y and Z spread randomly within bounds
    float y = -y_span / 2.0f + (float(rand()) / RAND_MAX) * y_span;
    float z = -z_span / 2.0f + (float(rand()) / RAND_MAX) * z_span;

    return Vec3f(x, y, z);
}

// =====================================================================================
// Physics Simulation (Training Mode)
// =====================================================================================

Vec3f NetworkGraph::computeSpringForce(NeuronParticle* p1, NeuronParticle* p2, double weight) {
    // F = k * (L0 - L) * direction
    // k is proportional to |weight| (stronger weights = stronger spring)
    
    double k = k_connection * std::abs(weight) / 120.0;  // Normalize by max weight
    Vec3f diff = p1->getPosition() - p2->getPosition();
    double length = diff.Length();
    
    if (length < 0.001) return Vec3f(0, 0, 0);  // Avoid division by zero
    
    Vec3f direction = diff;
    direction /= float(length);
    Vec3f spring_force = direction * float(k * (rest_length - length));
    
    return spring_force;
}

void NetworkGraph::animatePhysics() {
    if (!training_mode) return;  // Only update physics in training mode

    double h = timestep;

    // Compute forces and update positions for all free particles
    for (auto* p : particles) {
        if (p->isFixed()) continue;  // Skip anchored neurons

        Vec3f total_force(0, 0, 0);

        // Spring forces from all connections
        for (const auto& conn : p->getConnections()) {
            Vec3f spring_force = computeSpringForce(p, conn.target, conn.weight);
            total_force += spring_force;
        }

        // Also consider incoming connections (mutual attraction)
        for (auto* other : particles) {
            for (const auto& conn : other->getConnections()) {
                if (conn.target == p) {
                    Vec3f spring_force = computeSpringForce(p, other, conn.weight);
                    total_force += spring_force;
                }
            }
        }

        // Damping force
        Vec3f damping_force = p->getVelocity() * float(-damping);
        total_force += damping_force;

        // Euler integration
        Vec3f acceleration = total_force * float(1.0 / p->getMass());
        Vec3f new_velocity = p->getVelocity() + acceleration * float(h);
        Vec3f new_position = p->getPosition() + new_velocity * float(h);

        // Apply layer constraints for interneurons (keep them in middle layer)
        if (p->getType() == NeuronType::INTERNEURON) {
            float x_margin = 0.8f;  // Keep interneurons at least 0.8 units from edges
            if (new_position.x() < x_left + x_margin) {
                new_position.setx(x_left + x_margin);
                new_velocity.setx(0);  // Stop horizontal movement at boundary
            }
            if (new_position.x() > x_right - x_margin) {
                new_position.setx(x_right - x_margin);
                new_velocity.setx(0);
            }
        }

        p->setAcceleration(acceleration);
        p->setVelocity(new_velocity);
        p->setPosition(new_position);
    }

    // Apply Provot correction
    applyProvotCorrection();
}

void NetworkGraph::applyProvotCorrection() {
    // Prevent springs from over-stretching
    for (auto* p : particles) {
        for (const auto& conn : p->getConnections()) {
            NeuronParticle* target = conn.target;
            
            Vec3f diff = p->getPosition() - target->getPosition();
            float length = diff.Length();
            float max_length = rest_length * provot_structural_correction;

            if (length > max_length) {
                // Pull particles closer
                Vec3f direction = diff;
                direction /= length;
                float excess = length - max_length;
                Vec3f correction = direction * (excess / 2.0f);

                if (!p->isFixed()) {
                    p->setPosition(p->getPosition() - correction);
                }
                if (!target->isFixed()) {
                    target->setPosition(target->getPosition() + correction);
                }
            }
        }
    }
}

// =====================================================================================
// Visualization Updates (Inference Mode)
// =====================================================================================

void NetworkGraph::updateActivationStates() {
    // Update all particles
    for (auto* p : particles) {
        p->updateActivationState();
    }
    
    // Track output neuron firing rates
    for (const auto& output_id : output_neuron_ids) {
        Neuron* n = glia->getNeuronById(output_id);
        if (n) {
            output_tracker.update(output_id, n->didFire());
        }
    }
    
    // Determine winner using firing rate with "sticky" behavior
    // Winner only changes if a different output has higher rate
    std::string candidate = output_tracker.argmax(output_neuron_ids, "", 0.01f);
    
    // If no current winner, use candidate (or default if candidate is empty)
    if (current_winner.empty()) {
        if (!candidate.empty()) {
            current_winner = candidate;
        } else {
            // Use default output if configured
            if (default_output_index >= 0 && default_output_index < (int)output_neuron_ids.size()) {
                current_winner = output_neuron_ids[default_output_index];
            }
        }
    }
    // If we have a current winner, only change if candidate is different AND has higher rate
    else if (!candidate.empty() && candidate != current_winner) {
        // Check if candidate actually has higher rate
        float current_rate = output_tracker.getRate(current_winner);
        float candidate_rate = output_tracker.getRate(candidate);
        if (candidate_rate > current_rate) {
            current_winner = candidate;  // Dethroned!
        }
    }
}

void NetworkGraph::updateColors() {
    // Update winner flags for output neurons
    for (const auto& output_id : output_neuron_ids) {
        NeuronParticle* p = getParticleById(output_id);
        if (p) {
            p->setWinner(output_id == current_winner);
        }
    }
    
    // Update all particle colors
    for (auto* p : particles) {
        p->updateColor();
    }
}

// =====================================================================================
// Helper Functions
// =====================================================================================

NeuronParticle* NetworkGraph::getParticleById(const std::string &id) {
    auto it = particle_map.find(id);
    if (it != particle_map.end()) {
        return it->second;
    }
    return nullptr;
}

NeuronParticle* NetworkGraph::getNeuronParticle(const std::string& id) {
    return getParticleById(id);
}

void NetworkGraph::computeBoundingBox() {
    if (particles.empty()) return;

    box = BoundingBox(particles[0]->getPosition());
    for (auto* p : particles) {
        box.Extend(p->getPosition());
        box.Extend(p->getOriginalPosition());
    }
}

// =====================================================================================
// Rendering (implemented in network_render.cpp)
// =====================================================================================
