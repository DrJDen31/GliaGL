#ifndef __glia_h__
#define __glia_h__

#include <vector>
#include <map>
#include <string>

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
		for (auto *n : sensory_neurons)
			f(*n);
		for (auto *n : neurons)
			f(*n);
	}

	// apply "stimuli" to sensory neurons
	void injectSensory(const std::string &id, float amt);

	// access neuron by ID (for configuration)
	Neuron* getNeuronById(const std::string &id);
	
	// get all sensory neuron IDs
	std::vector<std::string> getSensoryNeuronIDs() const;

private:
	// vectors of neuron objects
	std::vector<Neuron *> sensory_neurons;
	std::vector<Neuron *> neurons;
	std::map<std::string, Neuron *> sensory_mapping;
	std::map<std::string, Neuron *> neuron_mapping;

	// helper function for config
	void addConnection(std::string from_id, std::string to_id, float weight);
};
#endif
