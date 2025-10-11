#ifndef _NETWORK_GRAPH_H_
#define _NETWORK_GRAPH_H_

#include <vector>
#include <map>
#include <string>

#include "argparser.h"
#include "boundingbox.h"
#include "vectors.h"
#include "neuron_particle.h"
#include "../arch/output_detection.h"

// Forward declarations
class Glia;
class Neuron;

// =====================================================================================
// Network Graph - Manages 3D visualization and physics of neural network
// =====================================================================================

class NetworkGraph {

public:
    NetworkGraph(Glia* network, ArgParser *args);
    ~NetworkGraph();

    // ACCESSORS
    const BoundingBox& getBoundingBox() const { return box; }
    bool isTrainingMode() const { return training_mode; }
    void setTrainingMode(bool mode) { training_mode = mode; }
    int getConnectionVertexCount() const { return connection_vertex_count; }
    int getNeuronVertexCount() const { return neuron_vertex_count; }
    const std::vector<std::string>& getOutputNeuronIDs() const { return output_neuron_ids; }
    NeuronParticle* getNeuronParticle(const std::string& id);
    const std::string& getCurrentWinner() const { return current_winner; }

    // INITIALIZATION
    void buildFromGlia(Glia* network);
    void initializeSpatialLayout();
    void computeLayerDepths();  // Compute connectivity depth for layered layout
    void computeBoundingBox();

    // PHYSICS SIMULATION (training mode)
    void animatePhysics();
    void applyProvotCorrection();

    // VISUALIZATION UPDATES (inference mode)
    void updateActivationStates();
    void updateColors();

    // RENDERING
    void packMesh();
    void packTestFloor(float* &current);   // Test floor for visibility
    void packNeurons(float* &current);
    void packConnections(float* &current);
    void packVelocities(float* &current);  // For debug visualization
    void packForces(float* &current);      // For debug visualization

private:
    // PRIVATE ACCESSORS
    NeuronParticle* getParticleById(const std::string &id);

    // SPATIAL LAYOUT HELPERS
    Vec3f computeSensoryPosition(int index, int total);
    Vec3f computeOutputPosition(int index, int total);
    Vec3f computeInterneuronPosition(int index, int total, int layer_depth);
    
    // PHYSICS HELPERS
    Vec3f computeSpringForce(NeuronParticle* p1, NeuronParticle* p2, double weight);

    // REPRESENTATION
    Glia* glia;                                      // Pointer to neural network
    ArgParser* args;

    std::vector<NeuronParticle*> particles;          // All neurons as particles
    std::map<std::string, NeuronParticle*> particle_map;  // ID -> particle lookup
    std::map<std::string, int> layer_depths;         // Neuron ID -> connectivity depth

    BoundingBox box;                                 // Bounding box for rendering

    bool training_mode;                              // Training vs inference mode

    // SPATIAL LAYOUT PARAMETERS
    float x_left;                                    // X coordinate for sensory plane
    float x_right;                                   // X coordinate for output plane
    float y_span;                                    // Vertical spread
    float z_span;                                    // Depth spread
    float rest_length;                               // Desired spacing between connected neurons

    // PHYSICS PARAMETERS
    double k_connection;                             // Base spring constant
    double damping;                                  // Velocity damping
    double provot_structural_correction;             // Max stretch before correction
    double timestep;                                 // Physics timestep

    // OUTPUT NEURON TRACKING
    std::vector<std::string> output_neuron_ids;      // IDs of output neurons
    EMAOutputDetector output_detector;               // Tracks firing rates to determine winner
    std::string current_winner;                      // Current winning output (from firing rate)
    
    // RENDERING VERTEX COUNTS (for separate draw calls)
    int connection_vertex_count;                     // Number of vertices for lines
    int neuron_vertex_count;                         // Number of vertices for points
};

#endif // _NETWORK_GRAPH_H_
