#include <cstring>
#include <iostream>
#include "network_graph.h"
#include "neuron_particle.h"
#include "meshdata.h"
#include "utils.h"

extern MeshData *mesh_data;

// ================================================================================
// Helper Functions
// ================================================================================

// Add a line segment for connections
void AddLine(float* &current, const Vec3f &start, const Vec3f &end, const Vec3f &color, float alpha) {
    // Line as two vertices with position, normal, and color
    // Normal is the direction for potential thickness rendering
    Vec3f direction = end - start;
    direction.Normalize();
    
    // Start vertex: position + normal + color
    float12 v1 = { 
        float(start.x()), float(start.y()), float(start.z()), 1,
        float(direction.x()), float(direction.y()), float(direction.z()), 0,
        float(color.r()), float(color.g()), float(color.b()), alpha
    };
    memcpy(current, &v1, sizeof(float)*12);
    current += 12;
    
    // End vertex
    float12 v2 = { 
        float(end.x()), float(end.y()), float(end.z()), 1,
        float(direction.x()), float(direction.y()), float(direction.z()), 0,
        float(color.r()), float(color.g()), float(color.b()), alpha
    };
    memcpy(current, &v2, sizeof(float)*12);
    current += 12;
}

// Add a point for neuron (will be rendered as sphere or point sprite)
void AddPoint(float* &current, const Vec3f &pos, const Vec3f &color, float size) {
    // Point as single vertex with position (w=size for shader), normal, color
    Vec3f normal(0, 1, 0);  // Up vector
    
    float12 v = { 
        float(pos.x()), float(pos.y()), float(pos.z()), size,  // Store size in position.w
        float(normal.x()), float(normal.y()), float(normal.z()), 0,
        float(color.r()), float(color.g()), float(color.b()), 1.0f  // Alpha = 1.0
    };
    memcpy(current, &v, sizeof(float)*12);
    current += 12;
}

// ================================================================================
// NetworkGraph Rendering Functions
// ================================================================================

void NetworkGraph::packMesh() {
    // Count connection vertices (lines = 2 vertices per connection)
    connection_vertex_count = 0;
    for (auto* p : particles) {
        connection_vertex_count += p->getConnections().size() * 2;
    }
    
    // Count neuron vertices (points = 1 vertex per neuron)
    neuron_vertex_count = particles.size();
    
    int total_vertex_count = connection_vertex_count + neuron_vertex_count;
    
    // Allocate mesh data
    if (mesh_data->clothTriCount != total_vertex_count) {
        delete[] mesh_data->clothTriData;
        mesh_data->clothTriCount = total_vertex_count;
        if (total_vertex_count == 0) {
            mesh_data->clothTriData = nullptr;
        } else {
            mesh_data->clothTriData = new float[total_vertex_count * 12];
        }
    }
    
    if (total_vertex_count == 0) {
        return;
    }
    
    float *current = mesh_data->clothTriData;
    
    // Pack connections
    if (mesh_data->wireframe && connection_vertex_count > 0) {
        packConnections(current);
    }
    
    // Pack neurons
    if (mesh_data->particles && neuron_vertex_count > 0) {
        packNeurons(current);
    }
}

void NetworkGraph::packTestFloor(float* &current) {
    // Create a gray floor at y = -3, spanning x=[-10,10] z=[-10,10]
    float y = -3.0f;
    float size = 10.0f;
    
    Vec3f v1(-size, y, -size);
    Vec3f v2(size, y, -size);
    Vec3f v3(size, y, size);
    Vec3f v4(-size, y, size);
    
    Vec3f normal(0, 1, 0);  // Up
    Vec3f gray(0.5f, 0.5f, 0.5f);
    
    // Triangle 1: v1, v2, v3
    float12 t1_v1 = { v1.x(), v1.y(), v1.z(), 1, normal.x(), normal.y(), normal.z(), 0, gray.r(), gray.g(), gray.b(), 1 };
    memcpy(current, &t1_v1, sizeof(float)*12); current += 12;
    
    float12 t1_v2 = { v2.x(), v2.y(), v2.z(), 1, normal.x(), normal.y(), normal.z(), 0, gray.r(), gray.g(), gray.b(), 1 };
    memcpy(current, &t1_v2, sizeof(float)*12); current += 12;
    
    float12 t1_v3 = { v3.x(), v3.y(), v3.z(), 1, normal.x(), normal.y(), normal.z(), 0, gray.r(), gray.g(), gray.b(), 1 };
    memcpy(current, &t1_v3, sizeof(float)*12); current += 12;
    
    // Triangle 2: v1, v3, v4
    float12 t2_v1 = { v1.x(), v1.y(), v1.z(), 1, normal.x(), normal.y(), normal.z(), 0, gray.r(), gray.g(), gray.b(), 1 };
    memcpy(current, &t2_v1, sizeof(float)*12); current += 12;
    
    float12 t2_v3 = { v3.x(), v3.y(), v3.z(), 1, normal.x(), normal.y(), normal.z(), 0, gray.r(), gray.g(), gray.b(), 1 };
    memcpy(current, &t2_v3, sizeof(float)*12); current += 12;
    
    float12 t2_v4 = { v4.x(), v4.y(), v4.z(), 1, normal.x(), normal.y(), normal.z(), 0, gray.r(), gray.g(), gray.b(), 1 };
    memcpy(current, &t2_v4, sizeof(float)*12); current += 12;
}

void NetworkGraph::packNeurons(float* &current) {
    for (auto* p : particles) {
        Vec3f pos = p->getPosition();
        Vec3f color = p->getCurrentColor();
        float size = p->getSize();
        
        // Add point for this neuron
        AddPoint(current, pos, color, size);
    }
}

void NetworkGraph::packConnections(float* &current) {
    for (auto* p : particles) {
        Vec3f start_pos = p->getPosition();
        
        for (const auto& conn : p->getConnections()) {
            NeuronParticle* target = conn.target;
            Vec3f end_pos = target->getPosition();
            double weight = conn.weight;
            float activation = conn.activation;
            
            // Color based on weight sign
            Vec3f base_color;
            if (weight > 0) {
                // Excitatory: Green
                base_color = Vec3f(0.0f, 0.8f, 0.0f);
            } else {
                // Inhibitory: Red
                base_color = Vec3f(0.8f, 0.0f, 0.0f);
            }
            
            // Modulate intensity by activation (spike propagation)
            // Base visibility of 0.5, can brighten to 1.0 when active
            Vec3f color = base_color * (0.5f + 0.5f * activation);
            
            // Alpha based on weight magnitude (normalized by typical max)
            float alpha = std::min(1.0f, float(std::abs(weight) / 120.0));
            
            // Add line for this connection
            AddLine(current, start_pos, end_pos, color, alpha);
        }
    }
}

// ================================================================================
// Velocity and Force Visualization (for debug)
// ================================================================================

void NetworkGraph::packVelocities(float* &current) {
    for (auto* p : particles) {
        if (p->isFixed()) continue;  // Skip anchored neurons
        
        Vec3f pos = p->getPosition();
        Vec3f vel = p->getVelocity();
        
        if (vel.Length() < 0.001) continue;  // Skip tiny velocities
        
        // Scale velocity for visibility
        Vec3f end = pos + vel * 0.5f;
        Vec3f color(0, 0, 1);  // Blue for velocity
        
        AddLine(current, pos, end, color, 1.0f);
    }
}

void NetworkGraph::packForces(float* &current) {
    for (auto* p : particles) {
        if (p->isFixed()) continue;  // Skip anchored neurons
        
        Vec3f pos = p->getPosition();
        Vec3f force = p->getForce();
        
        if (force.Length() < 0.001) continue;  // Skip tiny forces
        
        // Scale force for visibility
        Vec3f end = pos + force * 0.1f;
        Vec3f color(1, 1, 0);  // Yellow for force
        
        AddLine(current, pos, end, color, 1.0f);
    }
}
