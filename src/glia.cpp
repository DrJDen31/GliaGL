#include "glia.h"
#include "neuron.h"
#include <vector>
#include <iostream>
#include <fstream>

Glia::Glia()
{
}

Glia::Glia(int num_sensory, int num_neurons)
{
	for (int i = 0; i < num_sensory; i++)
	{
		std::string id = "S" + std::to_string(i);
		Neuron *neuron = new Neuron(id, num_sensory + num_neurons, 70.f, 1, 0, 0, true);
		sensory_neurons.push_back(neuron);
	}

	for (int i = 0; i < num_sensory; i++)
	{
		std::string id = "N" + std::to_string(i);
		Neuron *neuron = new Neuron(id, num_sensory + num_neurons, 70.f, 1, 0, 0, true);
		neurons.push_back(neuron);
	}
}

void Glia::step()
{
	std::cout << "life" << std::endl;

	for (std::vector<Neuron *>::iterator itr = sensory_neurons.begin(); itr != sensory_neurons.end(); itr++)
	{
		(*itr)->tick();
	}

	for (std::vector<Neuron *>::iterator itr = neurons.begin(); itr != neurons.end(); itr++)
	{
		(*itr)->tick();
	}
}