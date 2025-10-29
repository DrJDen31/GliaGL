#include "evolution_engine.h"

#include <algorithm>
#include <cmath>
#include <iostream>

EvolutionEngine::EvolutionEngine(const std::string &net_path,
                                 const std::vector<Trainer::EpisodeData> &train_set,
                                 const std::vector<Trainer::EpisodeData> &val_set,
                                 const TrainingConfig &train_cfg,
                                 const Config &evo_cfg,
                                 const Callbacks &cbs)
    : net_path(net_path), train_set(train_set), val_set(val_set), train_cfg(train_cfg), evo_cfg(evo_cfg), cbs(cbs), rng(evo_cfg.seed)
{
    Glia net; net.configureNetworkFromFile(net_path, /*verbose=*/false);
    base_edges = countEdges(net);
    if (base_edges <= 0) base_edges = 1;
}

int EvolutionEngine::countEdges(Glia &net) const {
    int cnt = 0;
    net.forEachNeuron([&](Neuron &from){ cnt += static_cast<int>(from.getConnections().size()); });
    return cnt;
}

void EvolutionEngine::applyMutation(Glia &net) {
    if (evo_cfg.sigma_w > 0.0f) {
        std::normal_distribution<float> nd(0.0f, evo_cfg.sigma_w);
        net.forEachNeuron([&](Neuron &from){
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                float w = kv.second.first + nd(rng);
                from.setTransmitter(kv.first, w);
            }
        });
    }
    if (evo_cfg.sigma_thr > 0.0f) {
        std::normal_distribution<float> nd(0.0f, evo_cfg.sigma_thr);
        net.forEachNeuron([&](Neuron &n){ n.setThreshold(n.getThreshold() + nd(rng)); });
    }
    if (evo_cfg.sigma_leak > 0.0f) {
        std::normal_distribution<float> nd(0.0f, evo_cfg.sigma_leak);
        net.forEachNeuron([&](Neuron &n){
            float v = n.getLeak() + nd(rng);
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            n.setLeak(v);
        });
    }
}

EvoMetrics EvolutionEngine::evaluate(Trainer &tr, Glia &net) {
    // Accuracy + avg margin on validation set
    size_t total = 0, correct = 0; double sum_margin = 0.0;
    for (const auto &ex : val_set) {
        EpisodeMetrics m = tr.evaluate(const_cast<InputSequence&>(ex.seq), train_cfg);
        total += 1;
        if (m.winner_id == ex.target_id) correct += 1;
        sum_margin += static_cast<double>(m.margin);
    }
    EvoMetrics em;
    em.acc = (total == 0) ? 0.0 : static_cast<double>(correct) / static_cast<double>(total);
    em.margin = (total == 0) ? 0.0 : sum_margin / static_cast<double>(total);
    em.edges = countEdges(net);
    em.fitness = mapFitness(em);
    return em;
}

double EvolutionEngine::mapFitness(const EvoMetrics &m) const {
    if (cbs.fitness_fn) return cbs.fitness_fn(m, base_edges);
    double edge_norm = static_cast<double>(m.edges) / static_cast<double>(base_edges);
    return static_cast<double>(evo_cfg.w_acc) * m.acc
         + static_cast<double>(evo_cfg.w_margin) * m.margin
         - static_cast<double>(evo_cfg.w_sparsity) * edge_norm;
}

EvolutionEngine::Result EvolutionEngine::run() {
    // Initialize population from base net
    std::vector<Individual> pop;
    const int P = std::max(1, evo_cfg.population);
    pop.resize(P);

    // Preamble: configuration summary
    std::cout << "Evolution start\n"
              << "  pop=" << P
              << "  gens=" << std::max(1, evo_cfg.generations)
              << "  elite=" << std::max(0, evo_cfg.elite)
              << "  parents_pool=" << std::max(0, evo_cfg.parents_pool)
              << "  train_epochs=" << std::max(0, evo_cfg.train_epochs)
              << "\n  sigma(w,thr,leak)=(" << evo_cfg.sigma_w << "," << evo_cfg.sigma_thr << "," << evo_cfg.sigma_leak << ")"
              << "  seed=" << evo_cfg.seed
              << "\n  fitness_weights(acc,margin,sparsity)=(" << evo_cfg.w_acc << "," << evo_cfg.w_margin << "," << evo_cfg.w_sparsity << ")"
              << "  lamarckian=" << (evo_cfg.lamarckian ? "1" : "0")
              << "\n";
    for (int i = 0; i < P; ++i) {
        Glia net; net.configureNetworkFromFile(net_path, /*verbose=*/false);
        if (i != 0) applyMutation(net);
        pop[i].genome = captureNet(net);
        pop[i].m.edges = countEdges(net);
        // lineage seed node
        LineageNode node; node.id = next_node_id++; node.parent_id = -1; node.gen = 0; // metrics filled after eval
        lineage.push_back(node); id_to_index[node.id] = (int)lineage.size() - 1;
        pop[i].node_id = node.id;
    }

    Result res;
    double prev_best = -1e9;

    for (int gen = 0; gen < std::max(1, evo_cfg.generations); ++gen) {
        // Evaluate (with inner training)
        for (int i = 0; i < P; ++i) {
            Glia net; net.configureNetworkFromFile(net_path, /*verbose=*/false);
            Trainer tr(net); tr.reseed(evo_cfg.seed + gen * 1000 + i);
            restoreNet(net, pop[i].genome);
            if (!train_set.empty() && evo_cfg.train_epochs > 0) tr.trainEpoch(train_set, evo_cfg.train_epochs, train_cfg);
            pop[i].m = evaluate(tr, net);
            if (evo_cfg.lamarckian) pop[i].genome = captureNet(net);
            // update lineage metrics
            auto it = id_to_index.find(pop[i].node_id);
            if (it != id_to_index.end()) lineage[it->second].m = pop[i].m, lineage[it->second].gen = gen;
        }

        std::sort(pop.begin(), pop.end(), [](const Individual &a, const Individual &b){ return a.m.fitness > b.m.fitness; });
        const Individual &best = pop.front();
        res.best_fitness_hist.push_back(best.m.fitness);
        res.best_acc_hist.push_back(best.m.acc);
        res.best_margin_hist.push_back(best.m.margin);
        res.best_genome = best.genome;
        // Population stats
        std::vector<double> fits; fits.reserve(P);
        std::vector<double> accs; accs.reserve(P);
        std::vector<double> margins; margins.reserve(P);
        std::vector<int> edges; edges.reserve(P);
        double sum_f=0.0, sum_a=0.0, sum_m=0.0; long long sum_e=0;
        for (int i = 0; i < P; ++i) {
            fits.push_back(pop[i].m.fitness);
            accs.push_back(pop[i].m.acc);
            margins.push_back(pop[i].m.margin);
            edges.push_back(pop[i].m.edges);
            sum_f += pop[i].m.fitness; sum_a += pop[i].m.acc; sum_m += pop[i].m.margin; sum_e += pop[i].m.edges;
        }
        auto median_of = [](std::vector<double> v){ if(v.empty()) return 0.0; size_t mid=v.size()/2; return v.size()%2? (std::nth_element(v.begin(), v.begin()+mid, v.end()), v[mid]) : (std::nth_element(v.begin(), v.begin()+mid-1, v.end()), (v[mid-1]+v[mid])*0.5); };
        double mean_f = (P? sum_f / (double)P : 0.0);
        double mean_a = (P? sum_a / (double)P : 0.0);
        double mean_m = (P? sum_m / (double)P : 0.0);
        double mean_e = (P? (double)sum_e / (double)P : 0.0);
        double med_f = median_of(fits);
        double d_best = (gen==0 || prev_best<-1e8) ? 0.0 : (best.m.fitness - prev_best);
        prev_best = best.m.fitness;

        std::cout << "Generation " << (gen+1) << "/" << std::max(1, evo_cfg.generations) << "\n"
                  << "  Best : f=" << best.m.fitness << "  acc=" << best.m.acc << "  margin=" << best.m.margin << "  edges=" << best.m.edges << "\n"
                  << "  Mean : f=" << mean_f          << "  acc=" << mean_a          << "  margin=" << mean_m          << "  edges=" << mean_e          << "\n"
                  << "  Median f=" << med_f << "  Δbest=" << d_best << "\n"
                  << "  Elites=" << std::min(std::max(0, evo_cfg.elite), P) << "  ParentsPool=" << std::min(std::max(0, evo_cfg.parents_pool), P) << "  Children=" << (P - std::min(std::max(0, evo_cfg.elite), P)) << "\n";

        if (cbs.on_generation) cbs.on_generation(gen, best.genome, best.m);

        // Next generation: elites + mutated children from top parents_pool
        int E = std::min(std::max(0, evo_cfg.elite), P);
        int R = std::min(std::max(E, evo_cfg.parents_pool), P);

        std::vector<Individual> next;
        // Elites: copy genomes into new nodes linked to previous self
        for (int i = 0; i < E; ++i) {
            Individual child;
            child.genome = pop[i].genome;
            child.m = {}; // will be evaluated next gen
            child.node_id = next_node_id++;
            LineageNode node; node.id = child.node_id; node.parent_id = pop[i].node_id; node.gen = gen + 1; lineage.push_back(node); id_to_index[node.id] = (int)lineage.size() - 1;
            next.push_back(std::move(child));
        }

        std::uniform_int_distribution<int> dist_parent(0, R - 1);
        while ((int)next.size() < P) {
            const Individual &parent = pop[dist_parent(rng)];
            Glia net; net.configureNetworkFromFile(net_path, /*verbose=*/false);
            Trainer tr(net); (void)tr; restoreNet(net, parent.genome);
            applyMutation(net);
            Individual child; child.genome = captureNet(net); child.m = {}; child.node_id = next_node_id++;
            LineageNode node; node.id = child.node_id; node.parent_id = parent.node_id; node.gen = gen + 1; lineage.push_back(node); id_to_index[node.id] = (int)lineage.size() - 1;
            next.push_back(std::move(child));
        }
        pop.swap(next);
    }

    // Write lineage JSON if requested
    if (!evo_cfg.lineage_json.empty()) writeLineageJson(evo_cfg.lineage_json);
    return res;
}

EvolutionEngine::NetSnapshot EvolutionEngine::captureNet(Glia &net) const {
    NetSnapshot s;
    net.forEachNeuron([&](Neuron &n){
        NeuronRec r{n.getId(), n.getThreshold(), n.getLeak()};
        s.neurons.push_back(r);
    });
    net.forEachNeuron([&](Neuron &from){
        const auto &conns = from.getConnections();
        for (const auto &kv : conns) s.edges.push_back(EdgeRec{from.getId(), kv.first, kv.second.first});
    });
    return s;
}

void EvolutionEngine::restoreNet(Glia &net, const NetSnapshot &s) const {
    // Build edge set for quick lookup
    std::unordered_map<std::string, std::unordered_map<std::string,float>> edge_set;
    for (const auto &e : s.edges) edge_set[e.from][e.to] = e.w;
    // Remove edges not present
    net.forEachNeuron([&](Neuron &from){
        std::vector<std::string> to_remove;
        const auto &conns = from.getConnections();
        for (const auto &kv : conns) {
            auto itf = edge_set.find(from.getId());
            if (itf == edge_set.end() || itf->second.find(kv.first) == itf->second.end()) to_remove.push_back(kv.first);
        }
        for (const auto &tid : to_remove) from.removeConnection(tid);
    });
    // Restore/add edges
    for (const auto &e : s.edges) {
        Neuron *from = net.getNeuronById(e.from);
        Neuron *to = net.getNeuronById(e.to);
        if (!from || !to) continue;
        const auto &conns = from->getConnections();
        if (conns.find(e.to) == conns.end()) from->addConnection(e.w, *to);
        else from->setTransmitter(e.to, e.w);
    }
    // Restore neuron params
    for (const auto &r : s.neurons) {
        Neuron *n = net.getNeuronById(r.id);
        if (!n) continue;
        n->setThreshold(r.thr);
        n->setLeak(r.leak);
    }
}

void EvolutionEngine::writeLineageJson(const std::string &path) const {
    std::ofstream jf(path.c_str(), std::ios::out | std::ios::trunc);
    if (!jf.is_open()) return;
    jf << "{\n  \"nodes\": [\n";
    for (size_t i = 0; i < lineage.size(); ++i) {
        const auto &n = lineage[i];
        jf << "    {\"id\": " << n.id
           << ", \"parent\": " << n.parent_id
           << ", \"gen\": " << n.gen
           << ", \"fitness\": " << n.m.fitness
           << ", \"acc\": " << n.m.acc
           << ", \"margin\": " << n.m.margin
           << ", \"edges\": " << n.m.edges
           << "}";
        if (i + 1 < lineage.size()) jf << ",";
        jf << "\n";
    }
    jf << "  ]\n}\n";
}
