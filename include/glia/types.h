#pragma once

#include <string>
#include <map>
#include <vector>

namespace glia {

/**
 * @brief Metrics from a single episode evaluation
 */
struct EpisodeMetrics {
    std::string winner_id;           ///< ID of winning output neuron
    float margin = 0.0f;              ///< Separation margin between winner and runner-up
    std::map<std::string, float> rates;  ///< Firing rates for output neurons
    int ticks_run = 0;                ///< Number of timesteps executed
};

/**
 * @brief Metrics from evolutionary training
 */
struct EvolutionMetrics {
    double fitness = -1e9;   ///< Overall fitness score
    double accuracy = 0.0;   ///< Classification accuracy on validation set
    double margin = 0.0;     ///< Average margin on validation set
    int edges = 0;           ///< Number of synaptic connections
};

/**
 * @brief Network snapshot for checkpointing/evolution
 * 
 * Stores complete network state including weights and neuron parameters.
 */
struct NetworkSnapshot {
    struct NeuronRecord {
        std::string id;
        float threshold;
        float leak;
    };
    
    struct EdgeRecord {
        std::string from_id;
        std::string to_id;
        float weight;
    };
    
    std::vector<NeuronRecord> neurons;
    std::vector<EdgeRecord> edges;
};

} // namespace glia
