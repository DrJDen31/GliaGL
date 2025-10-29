#pragma once
#include <string>
#include "../../arch/topology_policy.h"

struct OutputDetectorConfig {
    // "ema" for now; future: "softmax", "threshold", etc.
    std::string type = "ema";
    float alpha = 0.05f;      // EMA smoothing
    float threshold = 0.01f;  // Minimum activity to select a winner (else abstain)
    std::string default_id;   // Optional default when abstaining
};

struct GradConfig {
    std::string loss = "softmax_xent";
    float temperature = 1.0f;
    std::string optimizer = "sgd";
    float momentum = 0.9f;
    float adam_beta1 = 0.9f;
    float adam_beta2 = 0.999f;
    float adam_eps = 1e-8f;
    float clip_grad_norm = 0.0f;
};

struct TrainingConfig {
    int warmup_ticks = 20;          // U
    int decision_window = 50;       // W
    int episode_ticks = 200;        // total ticks for an episode

    OutputDetectorConfig detector;  // output detector parameters
    TopologyPolicy topology;        // structural policy (enforced during training)
    float lr = 0.01f;
    float elig_lambda = 0.95f;
    float weight_decay = 0.0001f;
    float margin_delta = 0.05f;
    float reward_pos = 1.2f;
    float reward_neg = -0.8f;
    std::string reward_mode = "softplus_margin"; // default to smooth margin shaping
    float reward_gain = 1.0f;
    float reward_min = -1.0f;
    float reward_max = 1.0f;
    // Update gating: "none" | "winner_only" | "target_only"
    std::string update_gating = "none";
    // New reward shaping and gating options
    // Modes: "binary", "margin_linear", "softplus_margin"
    bool no_update_if_satisfied = true; // if true and margin>=delta with correct winner, suppress update
    bool use_advantage_baseline = true;  // center reward by subtracting EMA baseline
    float baseline_beta = 0.1f;           // EMA factor for baseline update
    float r_target = 0.05f;
    float rate_alpha = 0.05f;
    // Eligibility variant: if true, use post EMA rate instead of post spike indicator
    bool elig_post_use_rate = true;
    float eta_theta = 0.0f;
    float eta_leak = 0.0f;
    float prune_epsilon = 1e-4f;
    int prune_patience = 3;
    int grow_edges = 0;
    float init_weight = 0.01f;
    // Optional clipping of weights after each update (0 = disabled)
    float weight_clip = 0.0f;

    // Batch/Epoch training
    int batch_size = 1;             // number of episodes per batch
    bool shuffle = true;            // shuffle dataset each epoch

    // Logging and reproducibility
    bool verbose = false;           // print training progress
    int log_every = 1;              // print frequency (epochs)
    unsigned int seed = 123456u;    // random seed
    float weight_jitter_std = 0.0f; // stddev for one-time weight jitter at train start
    int timing_jitter = 0;          // max onset jitter (ticks)

    // Usage-based modulation (optional)
    float usage_boost_gain = 0.0f;  // extra scaling by edge activity (normalized eligibility)
    float inactive_rate_threshold = 0.0f; // neurons below this EMA rate considered inactive
    int inactive_rate_patience = 0; // consecutive batches to tolerate inactivity before prune
    int prune_inactive_max = 0;     // max edges to prune per inactive neuron when triggered
    bool prune_inactive_out = true; // prune outbound edges of inactive neurons
    bool prune_inactive_in = false; // prune inbound edges of inactive neurons

    // In-memory checkpoints (rolling, exponentially decreasing retention)
    bool checkpoints_enable = true; // take snapshots at the end of each epoch
    int ckpt_l0 = 4;                // keep last N at level 0 (most recent)
    int ckpt_l1 = 2;                // keep N at level 1 (less recent)
    int ckpt_l2 = 1;                // keep N at level 2 (oldest)

    // Revert/rollback triggers
    bool revert_enable = false;      // if true, revert when metrics drop
    std::string revert_metric = "accuracy"; // "accuracy" | "margin"
    int revert_window = 1;           // window size for comparison (currently 1 = compare prev vs current)
    float revert_drop = 0.2f;        // drop threshold (e.g., 0.2 accuracy drop or 0.05 margin drop)

    GradConfig grad;
};
