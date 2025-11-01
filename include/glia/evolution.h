#pragma once

#include "network.h"
#include "trainer.h"
#include "types.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

// Forward declarations
class EvolutionEngine;
class TrainingConfig;

namespace glia {

/**
 * @brief Configuration for evolutionary training
 */
struct EvolutionConfig {
    int population = 8;        ///< Population size
    int generations = 10;      ///< Number of generations
    int elite = 2;             ///< Elite count (preserved each generation)
    int parents_pool = 4;      ///< Parent selection pool size
    int train_epochs = 3;      ///< Training epochs per individual (Lamarckian)
    
    // Mutation parameters (Gaussian noise)
    float sigma_weight = 0.05f;     ///< Weight mutation std dev
    float sigma_threshold = 0.0f;   ///< Threshold mutation std dev
    float sigma_leak = 0.0f;        ///< Leak mutation std dev
    
    // Fitness weights
    float weight_accuracy = 1.0f;   ///< Accuracy weight in fitness
    float weight_margin = 0.5f;     ///< Margin weight in fitness
    float weight_sparsity = 0.0f;   ///< Sparsity penalty weight
    
    unsigned int seed = 123456u;    ///< Random seed
    bool lamarckian = true;         ///< Keep trained weights in genome
    
    std::string lineage_file;       ///< Optional: save evolutionary tree to JSON
};

/**
 * @brief Result from evolutionary training
 */
struct EvolutionResult {
    NetworkSnapshot best_genome;              ///< Best network found
    std::vector<double> fitness_history;      ///< Best fitness per generation
    std::vector<double> accuracy_history;     ///< Best accuracy per generation
    std::vector<double> margin_history;       ///< Best margin per generation
};

/**
 * @brief Callback for generation-level monitoring
 * 
 * Args: generation_number, best_genome, best_metrics
 */
using GenerationCallback = std::function<void(int, const NetworkSnapshot&, const EvolutionMetrics&)>;

/**
 * @brief Custom fitness function signature
 * 
 * Args: metrics, baseline_edge_count
 * Returns: fitness score
 */
using FitnessFunction = std::function<double(const EvolutionMetrics&, int)>;

/**
 * @brief Evolutionary trainer - public API
 * 
 * Evolves network architectures using Lamarckian evolution
 * (trained weights are inherited). Training happens entirely in C++.
 */
class Evolution {
public:
    /**
     * @brief Construct evolution engine
     * @param network_path Path to base network file (.net)
     * @param train_data Training episodes for Lamarckian inner loop
     * @param val_data Validation episodes for fitness evaluation
     * @param trainer_config Configuration for inner training loop
     * @param evolution_config Evolution parameters
     */
    Evolution(const std::string& network_path,
              const std::vector<EpisodeData>& train_data,
              const std::vector<EpisodeData>& val_data,
              const TrainingConfig& trainer_config,
              const EvolutionConfig& evolution_config);
    
    /**
     * @brief Run evolutionary training
     * @param generation_callback Optional Python callback (called per generation)
     * @param fitness_function Optional custom fitness function
     * @return Evolution results with best genome and history
     */
    EvolutionResult run(GenerationCallback generation_callback = nullptr,
                        FitnessFunction fitness_function = nullptr);
    
    /**
     * @brief Load best genome into a network
     * @param result Evolution result containing best genome
     * @return Network with best genome loaded
     */
    static std::shared_ptr<Network> loadBestGenome(const EvolutionResult& result);

private:
    std::shared_ptr<EvolutionEngine> impl_;  // Internal evolution engine
};

} // namespace glia
