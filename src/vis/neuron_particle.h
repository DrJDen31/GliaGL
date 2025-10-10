#ifndef _NEURON_PARTICLE_H_
#define _NEURON_PARTICLE_H_

#include <string>
#include <vector>
#include "vectors.h"

// Forward declaration
class Neuron;

// =====================================================================================
// Neuron Particle - Represents a neuron in 3D space with physics properties
// =====================================================================================

enum class NeuronType {
    SENSORY,      // Input neurons (left plane, anchored)
    INTERNEURON,  // Hidden neurons (central volume, free to move)
    OUTPUT        // Output neurons (right plane, anchored)
};

class NeuronParticle {
public:
    // CONSTRUCTOR
    NeuronParticle(const std::string &id, Neuron* neuron_ptr, NeuronType type);

    // IDENTIFICATION
    const std::string& getId() const { return neuron_id; }
    Neuron* getNeuron() const { return neuron_ptr; }
    NeuronType getType() const { return type; }
    void setType(NeuronType new_type);

    // SPATIAL ACCESSORS
    const Vec3f& getOriginalPosition() const { return original_position; }
    const Vec3f& getPosition() const { return position; }
    const Vec3f& getVelocity() const { return velocity; }
    const Vec3f& getAcceleration() const { return acceleration; }
    Vec3f getForce() const { return float(mass) * acceleration; }
    double getMass() const { return mass; }
    bool isFixed() const { return fixed; }

    // SPATIAL MODIFIERS
    void setOriginalPosition(const Vec3f &p) { original_position = p; }
    void setPosition(const Vec3f &p) { position = p; }
    void setVelocity(const Vec3f &v) { velocity = v; }
    void setAcceleration(const Vec3f &a) { acceleration = a; }
    void setMass(double m) { mass = m; }
    void setFixed(bool b) { fixed = b; }

    // VISUAL ACCESSORS
    const Vec3f& getBaseColor() const { return base_color; }
    const Vec3f& getActiveColor() const { return active_color; }
    const Vec3f& getCurrentColor() const { return current_color; }
    float getSize() const { return size; }
    float getActivationLevel() const { return activation_level; }
    bool isFiring() const { return is_firing; }

    // VISUAL MODIFIERS
    void setBaseColor(const Vec3f &c) { base_color = c; }
    void setActiveColor(const Vec3f &c) { active_color = c; }
    void setCurrentColor(const Vec3f &c) { current_color = c; }
    void setSize(float s) { size = s; }
    void setActivationLevel(float a) { activation_level = a; }
    void setFiring(bool f) { is_firing = f; }

    // CONNECTION MANAGEMENT
    struct Connection {
        NeuronParticle* target;
        double weight;
        float activation;  // For visualizing spike propagation
        
        Connection(NeuronParticle* t, double w) : target(t), weight(w), activation(0.0f) {}
    };

    void addConnection(NeuronParticle* target, double weight);
    const std::vector<Connection>& getConnections() const { return connections; }
    std::vector<Connection>& getConnections() { return connections; }

    // ACTIVATION UPDATE (for inference mode visualization)
    void updateActivationState();
    void updateColor(float alpha = 0.2f);
    void setWinner(bool is_winner) { is_winner_output = is_winner; }

private:
    // IDENTIFICATION
    std::string neuron_id;        // e.g., "S0", "N5", "O1"
    Neuron* neuron_ptr;           // Pointer to actual Neuron object in the network
    NeuronType type;              // Sensory, Interneuron, or Output

    // SPATIAL PROPERTIES (physics simulation)
    Vec3f original_position;      // Initial position
    Vec3f position;               // Current position
    Vec3f velocity;               // Current velocity
    Vec3f acceleration;           // Current acceleration
    double mass;                  // Mass for physics
    bool fixed;                   // True if position is anchored (sensory/output)

    // VISUAL PROPERTIES (rendering)
    Vec3f base_color;             // Color when inactive
    Vec3f active_color;           // Color when firing
    Vec3f current_color;          // Current interpolated color
    float size;                   // Rendering radius
    float activation_level;       // Smoothed activation (0.0 to 1.0)
    bool is_firing;               // Current firing state
    bool is_winner_output;        // True if this is the winning output (from firing rate)

    // CONNECTIVITY (for spring physics and visualization)
    std::vector<Connection> connections;  // Outgoing connections
};

#endif // _NEURON_PARTICLE_H_
