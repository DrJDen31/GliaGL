#ifndef __glia_h__
#define __glia_h__

#include <vector>
#include <map>
#include <string>
#include <memory>

class Neuron;

/*
Class representing
*/
class Glia
{
public:
	// Constructors
	Glia();
	Glia(int num_sensory, int num_neurons);
	~Glia();

	// tick/step function
	void step();

    // file handling to load/save networks
    // verbose: if true (default), prints creation/loading info
    void configureNetworkFromFile(std::string filepath, bool verbose = true);
	void saveNetworkToFile(std::string filepath);

	// debug printing
	void printNetwork();

	// flatten neurons for training
	template <class F>
	void forEachNeuron(F f)
	{
		for (auto &n : sensory_neurons)
			f(*n);
		for (auto &n : neurons)
			f(*n);
	}

	// apply "stimuli" to sensory neurons
	void injectSensory(const std::string &id, float amt);

	// access neuron by ID (for configuration)
	std::shared_ptr<Neuron> getNeuronById(const std::string &id);
	
	// get all sensory neuron IDs
	std::vector<std::string> getSensoryNeuronIDs() const;

	// ========== NumPy-Compatible Data Access (for Python bindings) ==========
	
	/**
	 * @brief Get all neuron IDs in order
	 * @return Vector of neuron IDs (sensory neurons first, then interneurons)
	 */
	std::vector<std::string> getAllNeuronIDs() const;
	
	/**
	 * @brief Get network state as flat arrays
	 * @param ids Output: neuron IDs in order
	 * @param values Output: current voltage values
	 * @param thresholds Output: firing thresholds
	 * @param leaks Output: leak parameters
	 */
	void getState(std::vector<std::string> &ids,
	              std::vector<float> &values,
	              std::vector<float> &thresholds,
	              std::vector<float> &leaks) const;
	
	/**
	 * @brief Set neuron parameters from flat arrays
	 * @param ids Neuron IDs to update
	 * @param thresholds New threshold values (parallel to ids)
	 * @param leaks New leak values (parallel to ids)
	 */
	void setState(const std::vector<std::string> &ids,
	              const std::vector<float> &thresholds,
	              const std::vector<float> &leaks);
	
	/**
	 * @brief Get all synaptic weights as edge list (COO sparse format)
	 * @param from_ids Output: source neuron IDs
	 * @param to_ids Output: target neuron IDs  
	 * @param weights Output: synaptic weights
	 */
	void getWeights(std::vector<std::string> &from_ids,
	                std::vector<std::string> &to_ids,
	                std::vector<float> &weights) const;
	
	/**
	 * @brief Set synaptic weights from edge list
	 * @param from_ids Source neuron IDs
	 * @param to_ids Target neuron IDs
	 * @param weights New weight values
	 * @note Creates connections if they don't exist
	 */
	void setWeights(const std::vector<std::string> &from_ids,
	                const std::vector<std::string> &to_ids,
	                const std::vector<float> &weights);
	
	/**
	 * @brief Get total neuron count
	 * @return Total number of neurons (sensory + internal)
	 */
	int getNeuronCount() const { return static_cast<int>(sensory_neurons.size() + neurons.size()); }
	
	/**
	 * @brief Get total connection count
	 * @return Total number of synaptic connections
	 */
	int getConnectionCount() const;

private:
	// vectors of neuron objects (managed via shared_ptr for Python bindings)
	std::vector<std::shared_ptr<Neuron>> sensory_neurons;
	std::vector<std::shared_ptr<Neuron>> neurons;
	std::map<std::string, std::shared_ptr<Neuron>> sensory_mapping;
	std::map<std::string, std::shared_ptr<Neuron>> neuron_mapping;

	// helper function for config
	void addConnection(std::string from_id, std::string to_id, float weight);
};
#endif
