#ifndef __glia_h__
#define __glia_h__

#include <vector>

class Neuron;

/*
Class representing
*/
class Glia
{
public:
	Glia();
	Glia(int num_sensory, int num_neurons);

	void step();

private:
	std::vector<Neuron *> sensory_neurons;
	std::vector<Neuron *> neurons;
};

#endif