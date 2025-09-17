#include "glia.h"
#include "neuron.h"
#include <vector>
#include <iostream>
#include <fstream>

#include "trainer.hpp"

Glia::Glia()
{
}

Glia::Glia(int num_sensory, int num_neurons)
{
	// sensory neurons
	for (int i = 0; i < num_sensory; i++)
	{
		// ID string
		std::string id = "S" + std::to_string(i);

		// Neuron object
		Neuron *neuron = new Neuron(id, num_sensory + num_neurons, 70.f, 1, 4, 100.f, true);
		sensory_neurons.push_back(neuron);

		// mapping
		sensory_mapping[id] = neuron;

		// network
		std::map<std::string, float> network;
		sensory_network[id] = network;
	}

	// interneurons
	for (int i = 0; i < num_sensory; i++)
	{
		// ID string
		std::string id = "N" + std::to_string(i);

		// Neuron object
		Neuron *neuron = new Neuron(id, num_sensory + num_neurons, 70.f, 1, 4, 100.0f, true);
		neurons.push_back(neuron);

		// mapping
		neuron_mapping[id] = neuron;

		// network
		std::map<std::string, float> network;
		neuron_network[id] = network;
	}
}

void Glia::step()
{
	// debug printing
	std::cout << "tick" << std::endl;

	// call tick on every neuron
	for (std::vector<Neuron *>::iterator itr = sensory_neurons.begin(); itr != sensory_neurons.end(); itr++)
	{
		// testing for sensory
		(*itr)->receive(30.f);

		(*itr)->tick();
	}

	for (std::vector<Neuron *>::iterator itr = neurons.begin(); itr != neurons.end(); itr++)
	{
		(*itr)->tick();
	}
}

void Glia::configureNetworkFromFile(std::string filepath)
{
	// read data from file and create network

	// hardcoding for testing
	addConnection("S0", "N0", 25.0f);
	addConnection("S0", "N3", 9.0f);
}

void Glia::saveNetworkToFile(std::string filepath)
{
	// for each connection, save to file
}

void Glia::printNetwork()
{
	// print each network
	printNeuronMap(sensory_network);
	printNeuronMap(neuron_network);
}

void Glia::printNeuronMap(std::map<std::string, std::map<std::string, float>> map)
{
	// for each neuron
	for (std::map<std::string, std::map<std::string, float>>::iterator itr = map.begin(); itr != map.end(); itr++)
	{
		// print neuron id
		std::cout << itr->first << std::endl;

		// for each connection
		for (std::map<std::string, float>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
		{
			// print connection
			std::cout << "\t" << itr->first << ": --[" << itr2->second << "]--> " << itr2->first << std::endl;
		}
	}
}

void Glia::addConnection(std::string from_id, std::string to_id, float weight)
{
	// get neuron objects
	Neuron *from;
	Neuron *to;

	// gettin "from" object based on type
	if (from_id[0] == 'S')
	{
		from = sensory_mapping[from_id];
	}
	else if (from_id[0] == 'N')
	{
		from = neuron_mapping[from_id];
	}
	else
	{
		return;
	}

	// getting "to" object based on type
	if (to_id[0] == 'S')
	{
		to = sensory_mapping[to_id];
	}
	else if (to_id[0] == 'N')
	{
		to = neuron_mapping[to_id];
	}
	else
	{
		return;
	}

	// add connection to neuron object
	from->addConnection(weight, *to);

	// add connection to network
	if (from_id[0] == 'S')
	{
		sensory_network[from_id][to_id] = weight;
	}
	else if (from_id[0] == 'N')
	{
		neuron_network[from_id][to_id] = weight;
	}
	else
	{
		return;
	}
}

// flatten for training
// template <class F>
// void Glia::forEachNeuron(F f)
// {
// 	for (auto *n : sensory_neurons)
// 		f(*n);
// 	for (auto *n : neurons)
// 		f(*n);
// }