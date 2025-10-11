#include "glia.h"
#include "neuron.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>


Glia::Glia()
{
}

Glia::~Glia()
{
    // Delete dynamically allocated neurons
    for (auto *n : sensory_neurons) delete n;
    for (auto *n : neurons) delete n;
    sensory_neurons.clear();
    neurons.clear();
    sensory_mapping.clear();
    neuron_mapping.clear();
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
	}

	// interneurons (N* prefix by default)
	// Note: O* neurons will be created dynamically
	for (int i = 0; i < num_neurons; i++)
	{
		std::string id = "N" + std::to_string(i);
		Neuron *neuron = new Neuron(id, num_sensory + num_neurons, 70.f, 1, 4, 100.0f, true);
		neurons.push_back(neuron);
		neuron_mapping[id] = neuron;
	}
}

void Glia::step()
{
	// call tick on every neuron
	for (auto itr = sensory_neurons.begin(); itr != sensory_neurons.end(); ++itr)
	{
		(*itr)->tick();
	}
	for (auto itr = neurons.begin(); itr != neurons.end(); ++itr)
	{
		(*itr)->tick();
	}
}

void Glia::configureNetworkFromFile(std::string filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open network file: " << filepath << std::endl;
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
			continue;

		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;

		if (cmd == "NEURON")
		{
			std::string id;
			float threshold, leak, resting;
			iss >> id >> threshold >> leak >> resting;

			Neuron *neuron = getNeuronById(id);
			if (neuron)
			{
				// Neuron already exists, just configure it
				neuron->setThreshold(threshold);
				neuron->setLeak(leak);
				neuron->setResting(resting);
			}
			else
			{
				// Neuron doesn't exist, create it dynamically
				int total_neurons = sensory_neurons.size() + neurons.size();
				Neuron *new_neuron = new Neuron(id, total_neurons + 1, resting, leak, 4, threshold, true);
				
				// Add to appropriate list based on ID prefix
				if (id[0] == 'S')
				{
					sensory_neurons.push_back(new_neuron);
					sensory_mapping[id] = new_neuron;
				}
				else if (id[0] == 'N' || id[0] == 'O')
				{
					neurons.push_back(new_neuron);
					neuron_mapping[id] = new_neuron;
				}
				
				std::cout << "Created neuron " << id << ": threshold=" << threshold
						  << ", leak=" << leak << ", resting=" << resting << std::endl;
            }
        }
        else if (cmd == "CONNECTION")
        {
            std::string from_id, to_id;
            float weight;
            iss >> from_id >> to_id >> weight;
            addConnection(from_id, to_id, weight);
        }
    }

    file.close();
    std::cout << "Network configuration loaded from " << filepath << std::endl;
}

void Glia::saveNetworkToFile(std::string filepath)
{
	std::ofstream file(filepath);
	if (!file.is_open())
	{
		return;
	}

	file << "# Network Configuration\n";
	file << "# Auto-generated file\n\n";

	// Save sensory neurons
	file << "# Sensory neurons\n";
	for (auto *n : sensory_neurons)
	{
		file << "NEURON " << n->getId() << " "
			 << n->getThreshold() << " "
			 << n->getLeak() << " "
			 << n->getResting() << "\n";
	}

	// Save interneurons & outputs
	file << "\n# Interneurons & Outputs\n";
	for (auto *n : neurons)
	{
		file << "NEURON " << n->getId() << " "
			 << n->getThreshold() << " "
			 << n->getLeak() << " "
			 << n->getResting() << "\n";
	}

	// Save connections by iterating neuron objects directly
	file << "\n# Connections\n";
	for (auto *src : sensory_neurons)
	{
		const auto &conns = src->getConnections();
		for (const auto &kv : conns)
		{
			file << "CONNECTION " << src->getId() << " " << kv.first << " " << kv.second.first << "\n";
		}
	}
	for (auto *src : neurons)
	{
		const auto &conns = src->getConnections();
		for (const auto &kv : conns)
		{
			file << "CONNECTION " << src->getId() << " " << kv.first << " " << kv.second.first << "\n";
		}
	}

	file.close();
	std::cout << "Network saved to " << filepath << std::endl;
}

void Glia::printNetwork()
{
	// Print connections by reading directly from neuron objects
	for (auto *n : sensory_neurons)
	{
		std::cout << n->getId() << std::endl;
		for (const auto &kv : n->getConnections())
		{
			std::cout << "\t" << n->getId() << ": --[" << kv.second.first << "]--> " << kv.first << std::endl;
		}
	}
	for (auto *n : neurons)
	{
		std::cout << n->getId() << std::endl;
		for (const auto &kv : n->getConnections())
		{
			std::cout << "\t" << n->getId() << ": --[" << kv.second.first << "]--> " << kv.first << std::endl;
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
	else if (from_id[0] == 'N' || from_id[0] == 'O')
	{
		from = neuron_mapping[from_id];
	}
	else
	{
		std::cerr << "Warning: Unknown neuron type for " << from_id << std::endl;
		return;
	}

	// getting "to" object based on type
	if (to_id[0] == 'S')
	{
		to = sensory_mapping[to_id];
	}
	else if (to_id[0] == 'N' || to_id[0] == 'O')
	{
		to = neuron_mapping[to_id];
	}
	else
	{
		std::cerr << "Warning: Unknown neuron type for " << to_id << std::endl;
		return;
	}

	// add connection to neuron object
	from->addConnection(weight, *to);
}

// apply "stimuli" to sensory neurons
void Glia::injectSensory(const std::string &id, float amt)
{
	auto it = sensory_mapping.find(id);
	if (it != sensory_mapping.end())
	{
		// std::cout << "injecting " << amt << " into " << id << std::endl;
		it->second->receive(amt);
		// std::cout << it->second->getValue() << std::endl;
	}
}

// access neuron by ID (for configuration)
Neuron* Glia::getNeuronById(const std::string &id)
{
	// Check sensory neurons first
	auto it_s = sensory_mapping.find(id);
	if (it_s != sensory_mapping.end())
		return it_s->second;

	// Check interneurons
	auto it_n = neuron_mapping.find(id);
	if (it_n != neuron_mapping.end())
		return it_n->second;

	// Not found - will be created dynamically during config loading
	// (Don't print warning here - config loader will handle it)
	return nullptr;
}

// get all sensory neuron IDs
std::vector<std::string> Glia::getSensoryNeuronIDs() const
{
	std::vector<std::string> ids;
	for (const auto& kv : sensory_mapping)
	{
		ids.push_back(kv.first);
	}
	return ids;
}