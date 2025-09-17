#ifndef __glia_h__
#define __glia_h__

#include <vector>
#include <map>

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

	// flatten neurons for training
	template <class F>
	void forEachNeuron(F f)
	{
		for (auto *n : sensory_neurons)
			f(*n);
		for (auto *n : neurons)
			f(*n);
	}

private:
	// vectors of neuron objects
	std::vector<Neuron *> sensory_neurons;
	std::vector<Neuron *> neurons;

	// mappings from id strings to neuron objects
	std::map<std::string, Neuron *> sensory_mapping;
	std::map<std::string, Neuron *> neuron_mapping;

	// mappings to represent network of neuron relationships
	std::map<std::string, std::map<std::string, float>> sensory_network;
	std::map<std::string, std::map<std::string, float>> neuron_network;

	// helper function for debug print
	void printNeuronMap(std::map<std::string, std::map<std::string, float>> map);

	// helper function for config
	void addConnection(std::string from_id, std::string to_id, float weight);
};

#endif