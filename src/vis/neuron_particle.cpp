#include "neuron_particle.h"
#include "neuron.h"

// =====================================================================================
// Constructor
// =====================================================================================

NeuronParticle::NeuronParticle(const std::string &id, Neuron* neuron, NeuronType t)
    : neuron_id(id), neuron_ptr(neuron), type(t)
{
    // Initialize spatial properties
    original_position = Vec3f(0, 0, 0);
    position = Vec3f(0, 0, 0);
    velocity = Vec3f(0, 0, 0);
    acceleration = Vec3f(0, 0, 0);
    mass = 1.0;  // Default mass

    // Sensory and output neurons are anchored
    fixed = (type == NeuronType::SENSORY || type == NeuronType::OUTPUT);

    // Initialize visual properties based on type
    switch (type) {
        case NeuronType::SENSORY:
            base_color = Vec3f(0.2f, 0.5f, 1.0f);      // Blue
            active_color = Vec3f(0.4f, 0.7f, 1.0f);    // Bright blue
            size = 0.1f;                                // Medium
            break;

        case NeuronType::INTERNEURON:
            base_color = Vec3f(0.5f, 0.5f, 0.5f);      // Light gray (more visible)
            active_color = Vec3f(1.0f, 0.8f, 0.0f);    // Yellow-orange
            size = 0.10f;                               // Medium (same as others)
            break;

        case NeuronType::OUTPUT:
            base_color = Vec3f(0.8f, 0.2f, 0.8f);      // Purple
            active_color = Vec3f(1.0f, 0.3f, 1.0f);    // Bright magenta
            size = 0.1f;                                // Medium
            break;
    }

    current_color = base_color;
    activation_level = 0.0f;
    is_firing = false;
    is_winner_output = false;
}

void NeuronParticle::setType(NeuronType new_type) {
    type = new_type;
    
    // Update fixed flag - sensory and output neurons are anchored
    fixed = (type == NeuronType::SENSORY || type == NeuronType::OUTPUT);
    
    // Update colors based on new type
    switch (type) {
        case NeuronType::SENSORY:
            base_color = Vec3f(0.3f, 0.3f, 0.6f);      // Blue
            active_color = Vec3f(0.3f, 0.8f, 1.0f);    // Cyan
            size = 0.12f;                               // Large
            break;

        case NeuronType::INTERNEURON:
            base_color = Vec3f(0.5f, 0.5f, 0.5f);      // Light gray (more visible)
            active_color = Vec3f(1.0f, 0.8f, 0.0f);    // Yellow-orange
            size = 0.10f;                               // Medium (same as others)
            break;

        case NeuronType::OUTPUT:
            base_color = Vec3f(0.8f, 0.2f, 0.8f);      // Purple
            active_color = Vec3f(1.0f, 0.3f, 1.0f);    // Bright magenta
            size = 0.1f;                                // Medium
            break;
    }
    
    current_color = base_color;
}

// =====================================================================================
// Connection Management
// =====================================================================================

void NeuronParticle::addConnection(NeuronParticle* target, double weight) {
    connections.push_back(Connection(target, weight));
}

// =====================================================================================
// Activation Update (for inference mode visualization)
// =====================================================================================

void NeuronParticle::updateActivationState() {
    // Query the actual neuron's firing state
    if (neuron_ptr) {
        is_firing = neuron_ptr->didFire();
    } else {
        is_firing = false;
    }

    // Update activation level with instant jump on fire, smooth decay otherwise
    if (is_firing) {
        activation_level = 1.0f;  // Instant activation
    } else {
        // Smooth exponential decay
        float decay_alpha = 0.15f;  // Decay rate (higher = faster decay)
        activation_level *= (1.0f - decay_alpha);
        
        // Clamp to zero when very small
        if (activation_level < 0.01f) {
            activation_level = 0.0f;
        }
    }
}

void NeuronParticle::updateColor(float alpha) {
    // For output neurons, show as active if they're the winner (from firing rate)
    float t = activation_level;
    if (type == NeuronType::OUTPUT && is_winner_output) {
        t = 1.0f;  // Always bright if winning
    }
    
    // Interpolate between base and active colors
    current_color = base_color * float(1.0f - t) + active_color * float(t);

    // Update connection activations (for spike propagation visualization)
    for (auto& conn : connections) {
        if (is_firing) {
            conn.activation = 1.0f;  // Spike just sent
        } else {
            conn.activation *= (1.0f - alpha);  // Decay
            if (conn.activation < 0.01f) {
                conn.activation = 0.0f;
            }
        }
    }
}
