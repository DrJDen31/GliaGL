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

	// tick/step function
	void step();

	// file handling to load/save networks
	void configureNetworkFromFile(std::string filepath);
	void saveNetworkToFile(std::string filepath);

	// debug printing
	void printNetwork();

	// keeping the net synced
	void syncBookkeepingFromNeurons();

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
	
	// get configured default output neuron (for winner-take-all when all outputs are silent)
	const std::string& getDefaultOutput() const { return default_output_id; }

private:
	// vectors of neuron objects
	std::vector<Neuron *> sensory_neurons;
	std::vector<Neuron *> neurons;
	std::map<std::string, Neuron *> sensory_mapping;
	std::map<std::string, Neuron *> neuron_mapping;

	// mappings to represent network of neuron relationships
	std::map<std::string, std::map<std::string, float>> sensory_network;
	std::map<std::string, std::map<std::string, float>> neuron_network;
	
	// default output neuron ID (for winner-take-all when outputs are silent)
	std::string default_output_id;

	// helper function for debug print
	void printNeuronMap(std::map<std::string, std::map<std::string, float>> map);

	// helper function for config
	void addConnection(std::string from_id, std::string to_id, float weight);
};

#endif