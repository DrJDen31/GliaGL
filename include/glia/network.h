#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declarations
class Neuron;

namespace glia {

/**
 * @brief Spiking neural network - public API
 * 
 * This is a clean wrapper around the internal Glia class for Python bindings.
 * All neurons are managed via shared_ptr for safe Python/C++ interop.
 */
class Network {
public:
    /**
     * @brief Construct empty network
     */
    Network();
    
    /**
     * @brief Construct network with specified neuron counts
     * @param num_sensory Number of sensory input neurons
     * @param num_neurons Number of internal neurons
     */
    Network(int num_sensory, int num_neurons);
    
    /**
     * @brief Load network from file
     * @param filepath Path to .net configuration file
     * @param verbose Print loading information (default: true)
     */
    void load(const std::string& filepath, bool verbose = true);
    
    /**
     * @brief Save network to file
     * @param filepath Path to output .net file
     */
    void save(const std::string& filepath);
    
    /**
     * @brief Run one simulation timestep
     * 
     * Updates all neurons in the network. This is the main simulation step.
     */
    void step();
    
    /**
     * @brief Inject current into a sensory neuron
     * @param neuron_id ID of the sensory neuron (e.g., "S0")
     * @param amount Current amount to inject
     */
    void inject(const std::string& neuron_id, float amount);
    
    /**
     * @brief Get all sensory neuron IDs
     * @return Vector of sensory neuron ID strings
     */
    std::vector<std::string> getSensoryIds() const;
    
    /**
     * @brief Get neuron by ID
     * @param neuron_id Neuron identifier
     * @return Shared pointer to neuron (nullptr if not found)
     */
    std::shared_ptr<Neuron> getNeuron(const std::string& neuron_id);
    
    /**
     * @brief Access internal Glia instance (for advanced use)
     * @return Reference to internal network
     */
    class Glia& getInternal();
    const class Glia& getInternal() const;

private:
    std::shared_ptr<class Glia> impl_;  // PIMPL pattern for ABI stability
};

} // namespace glia
