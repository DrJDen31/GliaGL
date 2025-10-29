#pragma once
#include "hebbian/training_config.h"
#if 0
#include <string>
#include "../arch/topology_policy.h"

struct OutputDetectorConfig {
    // "ema" for now; future: "softmax", "threshold", etc.
    std::string type = "ema";
    float alpha = 0.05f;      // EMA smoothing
    float threshold = 0.01f;  // Minimum activity to select a winner (else abstain)
    std::string default_id;   // Optional default when abstaining
};

#endif
// Legacy wrapper: use hebbian/training_config.h only.
