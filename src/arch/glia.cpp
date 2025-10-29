#include "glia.h"
#include "neuron.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>


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

void Glia::configureNetworkFromFile(std::string filepath, bool verbose)
{
	std::ifstream file(filepath);
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open network file: " << filepath << std::endl;
		return;
	}

    // Read all lines first
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) { if (!line.empty()) lines.push_back(line); }
    file.close();

    // Check for NEWNET format and collect options
    struct NewNetCfg {
        int S = 0, H = 0, O = 0;
        bool pool = false;
        float dens_SH = 0.6f, dens_SO = 0.2f, dens_HH = 0.1f, dens_HO = 0.6f;
        std::string init = "he"; // he | xavier
        float excit_ratio = 0.7f;
        float w_scale = 1.0f;
        float thr_S = 100.0f, leak_S = 1.0f;
        float thr_H = 45.0f,  leak_H = 0.90f;
        float thr_O = 55.0f,  leak_O = 1.0f;
    } nn;
    bool has_newnet = false;

    for (const auto &ln : lines) {
        if (ln.empty() || ln[0] == '#') continue;
        std::istringstream iss(ln);
        std::string cmd; iss >> cmd;
        if (cmd == "NEWNET") {
            has_newnet = true;
            std::string tok;
            while (iss >> tok) {
                auto eq = tok.find('='); if (eq == std::string::npos) continue;
                std::string k = tok.substr(0, eq);
                std::string v = tok.substr(eq + 1);
                if (k == "S") nn.S = std::atoi(v.c_str());
                else if (k == "H") nn.H = std::atoi(v.c_str());
                else if (k == "O") nn.O = std::atoi(v.c_str());
                else if (k == "POOL" || k == "WTA") nn.pool = (v == "1" || v == "true");
            }
        } else if (cmd == "DENSITY") {
            std::string pair; float p = 0.0f; iss >> pair >> p;
            if (pair == "S->H") nn.dens_SH = p;
            else if (pair == "S->O") nn.dens_SO = p;
            else if (pair == "H->H") nn.dens_HH = p;
            else if (pair == "H->O") nn.dens_HO = p;
        } else if (cmd == "INIT") {
            std::string m; iss >> m; if (!m.empty()) nn.init = m;
        } else if (cmd == "EXCIT_RATIO") {
            float r = 0.0f; iss >> r; if (r > 0.0f) nn.excit_ratio = r;
        } else if (cmd == "W_SCALE") {
            float s = 1.0f; iss >> s; if (s > 0.0f) nn.w_scale = s;
        } else if (cmd == "THRESHOLDS") {
            std::string g; float v;
            while (iss >> g >> v) { if (g=="S") nn.thr_S=v; else if (g=="H") nn.thr_H=v; else if (g=="O") nn.thr_O=v; }
        } else if (cmd == "LEAK") {
            std::string g; float v;
            while (iss >> g >> v) { if (g=="S") nn.leak_S=v; else if (g=="H") nn.leak_H=v; else if (g=="O") nn.leak_O=v; }
        }
    }

    if (has_newnet) {
        auto make_neuron = [&](const std::string &id, float thr, float leak){
            int comp = (int)(sensory_neurons.size() + neurons.size()) + 1;
            Neuron *n = new Neuron(id, comp, /*resting*/0.0f, /*leak*/leak, /*refractory*/4, /*threshold*/thr, /*tick*/true);
            n->setThreshold(thr); n->setLeak(leak); n->setResting(0.0f);
            if (!id.empty() && id[0] == 'S') { sensory_neurons.push_back(n); sensory_mapping[id] = n; }
            else { neurons.push_back(n); neuron_mapping[id] = n; }
            return n;
        };

        std::vector<Neuron*> Svec; Svec.reserve(nn.S);
        std::vector<Neuron*> Hvec; Hvec.reserve(nn.H);
        std::vector<Neuron*> Ovec; Ovec.reserve(nn.O);
        Neuron *Npool = nullptr;

        for (int i=0;i<nn.S;++i) Svec.push_back(make_neuron("S"+std::to_string(i), nn.thr_S, nn.leak_S));
        for (int i=0;i<nn.H;++i) Hvec.push_back(make_neuron("H"+std::to_string(i), nn.thr_H, nn.leak_H));
        for (int i=0;i<nn.O;++i) Ovec.push_back(make_neuron("O"+std::to_string(i), nn.thr_O, nn.leak_O));
        if (nn.pool) Npool = make_neuron("N0", 40.0f, 0.80f);

        std::random_device rd; std::mt19937 rng(rd());
        std::uniform_real_distribution<float> U01(0.0f, 1.0f);

        struct Edge { Neuron* from; Neuron* to; bool is_pool_edge; };
        std::vector<Edge> edges;
        auto maybe_connect = [&](Neuron* a, Neuron* b, float dens){ if (a && b && U01(rng) < dens) { a->addConnection(0.0f, *b); edges.push_back({a,b,false}); } };

        // S->H, S->O
        for (auto *s : Svec) for (auto *h : Hvec) maybe_connect(s,h,nn.dens_SH);
        for (auto *s : Svec) for (auto *o : Ovec) maybe_connect(s,o,nn.dens_SO);
        // H->H (no self), H->O
        for (auto *h1 : Hvec) for (auto *h2 : Hvec) if (h1!=h2) maybe_connect(h1,h2,nn.dens_HH);
        for (auto *h : Hvec) for (auto *o : Ovec) maybe_connect(h,o,nn.dens_HO);

        if (Npool) {
            for (auto *o : Ovec) { o->addConnection(+20.0f, *Npool); edges.push_back({o,Npool,true}); }
            for (auto *o : Ovec) { Npool->addConnection(-25.0f, *o); edges.push_back({Npool,o,true}); }
        }

        // fan-in per target for init scaling
        std::map<Neuron*, int> fanin;
        for (const auto &e : edges) { if (!e.is_pool_edge) fanin[e.to] += 1; }

        // Assign random weights
        for (const auto &e : edges) {
            if (e.is_pool_edge) continue; // keep pool edges fixed
            int fin = std::max(1, fanin[e.to]);
            float limit = std::sqrt(6.0f / (float)fin); // default He-like
            if (nn.init == "xavier") limit = std::sqrt(6.0f / (float)fin);
            limit *= nn.w_scale;
            std::uniform_real_distribution<float> U(-limit, +limit);
            float w = U(rng);
            if (U01(rng) > nn.excit_ratio) w = -std::fabs(w); else w = +std::fabs(w);
            e.from->setTransmitter(e.to->getId(), w);
        }

        // Sanitize: remove any accidental null-pointer connections before use
        int null_edge_count = 0;
        for (auto *src : sensory_neurons) {
            std::vector<std::string> bad;
            const auto &conns = src->getConnections();
            for (const auto &kv : conns) { if (kv.second.second == nullptr) { bad.push_back(kv.first); null_edge_count++; } }
            for (const auto &to_id : bad) src->removeConnection(to_id);
        }
        for (auto *src : neurons) {
            std::vector<std::string> bad;
            const auto &conns = src->getConnections();
            for (const auto &kv : conns) { if (kv.second.second == nullptr) { bad.push_back(kv.first); null_edge_count++; } }
            for (const auto &to_id : bad) src->removeConnection(to_id);
        }

        if (verbose) {
            if (null_edge_count > 0) {
                std::cout << "Sanitized null connections: " << null_edge_count << std::endl;
            }
            std::cout << "NEWNET built: S=" << nn.S << " H=" << nn.H << " O=" << nn.O << (nn.pool?" + pool":"") << std::endl;
            std::cout << "Network configuration loaded from " << filepath << std::endl;
        }
        return;
    }

    // Legacy NEURON/CONNECTION format
    for (const auto &ln : lines) {
        if (ln.empty() || ln[0] == '#') continue;
        std::istringstream iss(ln);
        std::string cmd; iss >> cmd;
        if (cmd == "NEURON") {
            std::string id; float threshold, leak, resting; iss >> id >> threshold >> leak >> resting;
            Neuron *neuron = getNeuronById(id);
            if (neuron) {
                neuron->setThreshold(threshold);
                neuron->setLeak(leak);
                neuron->setResting(resting);
            } else {
                int total_neurons = (int)(sensory_neurons.size() + neurons.size());
                Neuron *new_neuron = new Neuron(id, total_neurons + 1, resting, leak, 4, threshold, true);
                if (!id.empty() && id[0] == 'S') { sensory_neurons.push_back(new_neuron); sensory_mapping[id] = new_neuron; }
                else { neurons.push_back(new_neuron); neuron_mapping[id] = new_neuron; }
                if (verbose) {
                    std::cout << "Created neuron " << id << ": threshold=" << threshold
                              << ", leak=" << leak << ", resting=" << resting << std::endl;
                }
            }
        } else if (cmd == "CONNECTION") {
            std::string from_id, to_id; float weight; iss >> from_id >> to_id >> weight;
            addConnection(from_id, to_id, weight);
        }
    }
    if (verbose) {
        std::cout << "Network configuration loaded from " << filepath << std::endl;
    }
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
    Neuron *from = nullptr;
    Neuron *to = nullptr;

    // get "from" object based on prefix: S* is sensory, anything else is regular neuron
    if (!from_id.empty() && from_id[0] == 'S') {
        auto it = sensory_mapping.find(from_id);
        if (it != sensory_mapping.end()) from = it->second;
    } else {
        auto it = neuron_mapping.find(from_id);
        if (it != neuron_mapping.end()) from = it->second;
    }

    // get "to" object based on prefix
    if (!to_id.empty() && to_id[0] == 'S') {
        auto it = sensory_mapping.find(to_id);
        if (it != sensory_mapping.end()) to = it->second;
    } else {
        auto it = neuron_mapping.find(to_id);
        if (it != neuron_mapping.end()) to = it->second;
    }

    if (!from || !to) {
        std::cerr << "Warning: addConnection skipped for missing neuron(s): " << from_id << " -> " << to_id << std::endl;
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
