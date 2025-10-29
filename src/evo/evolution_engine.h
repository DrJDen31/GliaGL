// Modular Evolution Engine for Glia
// - Operates on Glia+Trainer snapshots (Lamarckian inner training)
// - Pluggable callbacks for fitness and per-generation side effects

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <fstream>
#include <random>

#include "../arch/glia.h"
#include "../arch/neuron.h"
#include "../arch/input_sequence.h"
#include "../train/trainer.h"
#include "../train/training_config.h"

struct EvoMetrics {
    double fitness = -1e9;
    double acc = 0.0;
    double margin = 0.0;
    int edges = 0;
};

class EvolutionEngine {
public:
    struct EdgeRec { std::string from; std::string to; float w; };
    struct NeuronRec { std::string id; float thr; float leak; };
    struct NetSnapshot { std::vector<NeuronRec> neurons; std::vector<EdgeRec> edges; };
    struct Config {
        int population = 8;
        int generations = 10;
        int elite = 2;
        int parents_pool = 4;
        int train_epochs = 3;      // epochs per individual

        // Mutation params (Gaussian jitter)
        float sigma_w = 0.05f;
        float sigma_thr = 0.0f;
        float sigma_leak = 0.0f;

        // Fitness weights (default mapping)
        float w_acc = 1.0f;
        float w_margin = 0.5f;
        float w_sparsity = 0.0f; // penalty on edges/base_edges

        // Random seed
        unsigned int seed = 123456u;

        // Lamarckian: keep trained weights in genome
        bool lamarckian = true;

        // Optional: path to write lineage JSON (evolutionary tree)
        std::string lineage_json;   // if empty, skip writing
    };

    struct Callbacks {
        // Optional: custom fitness function. If unset, default weighted mapping is used.
        // Arguments: metrics, base_edges (for normalization). Return: fitness.
        std::function<double(const EvoMetrics&, int)> fitness_fn;

        // Optional: called after each generation with the best genome and metrics.
        std::function<void(int gen, const NetSnapshot &best_genome, const EvoMetrics &best_metrics)> on_generation;
    };

    struct Result {
        NetSnapshot best_genome;
        std::vector<double> best_fitness_hist;
        std::vector<double> best_acc_hist;
        std::vector<double> best_margin_hist;
    };

    EvolutionEngine(const std::string &net_path,
                    const std::vector<Trainer::EpisodeData> &train_set,
                    const std::vector<Trainer::EpisodeData> &val_set,
                    const TrainingConfig &train_cfg,
                    const Config &evo_cfg,
                    const Callbacks &cbs = {});

    Result run();

private:
    std::string net_path;
    std::vector<Trainer::EpisodeData> train_set;
    std::vector<Trainer::EpisodeData> val_set;
    TrainingConfig train_cfg;
    Config evo_cfg;
    Callbacks cbs;
    std::mt19937 rng;
    int base_edges = 1;

    struct Individual { NetSnapshot genome; EvoMetrics m; int node_id = -1; };

    struct LineageNode {
        int id = -1;
        int parent_id = -1; // -1 for seeds
        int gen = 0;
        EvoMetrics m;       // metrics as evaluated for this node
    };

    int countEdges(Glia &net) const;
    void applyMutation(Glia &net);
    EvoMetrics evaluate(Trainer &tr, Glia &net);
    double mapFitness(const EvoMetrics &m) const;
    NetSnapshot captureNet(Glia &net) const;
    void restoreNet(Glia &net, const NetSnapshot &s) const;
    void writeLineageJson(const std::string &path) const;

    // Lineage bookkeeping
    int next_node_id = 0;
    std::vector<LineageNode> lineage;
    std::unordered_map<int,int> id_to_index; // node_id -> lineage index
};
