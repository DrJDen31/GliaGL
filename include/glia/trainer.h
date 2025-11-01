#pragma once

#include "network.h"
#include "types.h"
#include <memory>
#include <vector>
#include <functional>

// Forward declarations
class TrainingConfig;
class InputSequence;

namespace glia {

/**
 * @brief Episode data: input sequence paired with target output
 */
struct EpisodeData {
    InputSequence sequence;
    std::string target_id;  ///< Expected output neuron (e.g., "O0")
};

/**
 * @brief Callback function signature for epoch-level monitoring
 * 
 * Called at the end of each training epoch.
 * Args: epoch_number, accuracy, average_margin
 */
using EpochCallback = std::function<void(int, double, double)>;

/**
 * @brief Neural network trainer - public API
 * 
 * Provides gradient-based and eligibility-trace training for spiking networks.
 * Training happens entirely in C++ for performance; Python can provide callbacks.
 */
class Trainer {
public:
    /**
     * @brief Construct trainer for a network
     * @param network Network to train
     */
    explicit Trainer(std::shared_ptr<Network> network);
    
    /**
     * @brief Evaluate network on a single episode
     * @param sequence Input sequence to present
     * @param config Training configuration (warmup, window, etc.)
     * @return Episode metrics
     */
    EpisodeMetrics evaluate(InputSequence& sequence, const TrainingConfig& config);
    
    /**
     * @brief Train for multiple epochs
     * @param dataset Training episodes
     * @param epochs Number of training epochs
     * @param config Training configuration
     * @param callback Optional Python callback for monitoring (called per epoch)
     */
    void train(const std::vector<EpisodeData>& dataset,
               int epochs,
               const TrainingConfig& config,
               EpochCallback callback = nullptr);
    
    /**
     * @brief Train on a single batch
     * @param batch Batch of training episodes
     * @param config Training configuration
     * @return Metrics for each episode in batch
     */
    std::vector<EpisodeMetrics> trainBatch(const std::vector<EpisodeData>& batch,
                                            const TrainingConfig& config);
    
    /**
     * @brief Get training history
     * @return Map of metric name to values over epochs
     */
    std::map<std::string, std::vector<double>> getHistory() const;
    
    /**
     * @brief Revert to last checkpoint (if checkpointing enabled)
     * @return true if revert succeeded
     */
    bool revertCheckpoint();
    
    /**
     * @brief Set random seed for reproducibility
     * @param seed Random seed value
     */
    void setSeed(unsigned int seed);
    
    /**
     * @brief Access internal trainer (for advanced use)
     */
    class ::Trainer& getInternal();

private:
    std::shared_ptr<Network> network_;
    std::shared_ptr<class ::Trainer> impl_;  // Internal trainer implementation
};

} // namespace glia
