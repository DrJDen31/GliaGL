#pragma once
#include <string>
#include <vector>
#include <map>

#include "../arch/glia.h"
#include "../arch/neuron.h"
#include "../arch/output_detection.h"
#include "../arch/input_sequence.h"
#include "training_config.h"

struct EpisodeMetrics {
    std::string winner_id;
    float margin = 0.0f;
    std::map<std::string,float> rates;
    int ticks_run = 0;
};

class Trainer {
public:
    explicit Trainer(Glia &net) : glia(net) {}

    // Evaluate a single episode using the provided input sequence and config.
    EpisodeMetrics evaluate(InputSequence &seq, const TrainingConfig &cfg) {
        // Collect output neuron IDs (O*)
        std::vector<std::string> output_ids = collectOutputIDs();

        // Detector setup
        OutputDetectorOptions opts; opts.threshold = cfg.detector.threshold; opts.default_id = cfg.detector.default_id;
        EMAOutputDetector detector(cfg.detector.alpha, opts);
        detector.reset();

        // Reset sequence
        seq.reset();

        // Warmup (U)
        const int U = cfg.warmup_ticks;
        for (int t = 0; t < U; ++t) {
            injectFromSequence(seq);
            glia.step();
            updateDetector(detector, output_ids);
            seq.advance();
        }

        // Decision window (W)
        const int W = cfg.decision_window;
        for (int t = 0; t < W; ++t) {
            injectFromSequence(seq);
            glia.step();
            updateDetector(detector, output_ids);
            seq.advance();
        }

        // Compile metrics
        EpisodeMetrics m;
        m.winner_id = detector.predict(output_ids);
        m.margin = detector.getMargin(output_ids);
        for (const auto &id : output_ids) {
            m.rates[id] = detector.getRate(id);
        }
        m.ticks_run = U + W;
        return m;
    }

private:
    Glia &glia;

    std::vector<std::string> collectOutputIDs() {
        std::vector<std::string> ids;
        glia.forEachNeuron([&](Neuron &n){
            const std::string &id = n.getId();
            if (!id.empty() && id[0] == 'O') ids.push_back(id);
        });
        return ids;
    }

    void injectFromSequence(InputSequence &seq) {
        auto inputs = seq.getCurrentInputs();
        for (const auto &kv : inputs) {
            glia.injectSensory(kv.first, kv.second);
        }
    }

    void updateDetector(IOutputDetector &detector, const std::vector<std::string> &output_ids) {
        for (const auto &id : output_ids) {
            Neuron *n = glia.getNeuronById(id);
            if (n) detector.update(id, n->didFire());
        }
    }
};
