#pragma once
#include <string>
#include "../arch/topology_policy.h"

struct OutputDetectorConfig {
    // "ema" for now; future: "softmax", "threshold", etc.
    std::string type = "ema";
    float alpha = 0.05f;      // EMA smoothing
    float threshold = 0.01f;  // Minimum activity to select a winner (else abstain)
    std::string default_id;   // Optional default when abstaining
};

struct TrainingConfig {
    int warmup_ticks = 20;          // U
    int decision_window = 50;       // W
    int episode_ticks = 200;        // total ticks for an episode

    OutputDetectorConfig detector;  // output detector parameters
    TopologyPolicy topology;        // structural policy (enforced during training)
};
