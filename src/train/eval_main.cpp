#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <cstdlib>

#include "../arch/glia.h"
#include "../arch/neuron.h"
#include "../arch/output_detection.h"
#include "../arch/input_sequence.h"
#include "training_config.h"
#include "trainer.h"

struct Args {
    std::string net_path;
    std::string scenario;        // xor | 3class | (empty)
    bool use_baseline = false;   // if scenario is provided and net_path is empty
    int warmup = 20;
    int window = 80;
    float alpha = 0.05f;
    float threshold = 0.01f;
    std::string default_id;      // optional default when below threshold
    float noise = 0.0f;          // for 3class only
};

static void print_usage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " --net <path> [--warmup U --window W --alpha A --threshold T --default ID]\n"
              << "  " << prog << " --scenario xor|3class [--baseline] [--warmup U --window W --alpha A --threshold T --default ID --noise P]\n"
              << "\nExamples:\n"
              << "  " << prog << " --scenario xor --baseline --default O0\n"
              << "  " << prog << " --net ..\\src\\testing\\3class\\3class_network.net --noise 0.1\n";
}

static bool parse_args(int argc, const char* argv[], Args &a) {
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&](std::string &out){ if (i+1>=argc) return false; out = argv[++i]; return true; };
        if (k == "--net") { if (!next(a.net_path)) return false; }
        else if (k == "--scenario") { if (!next(a.scenario)) return false; }
        else if (k == "--baseline") { a.use_baseline = true; }
        else if (k == "--warmup") { std::string v; if (!next(v)) return false; a.warmup = std::atoi(v.c_str()); }
        else if (k == "--window") { std::string v; if (!next(v)) return false; a.window = std::atoi(v.c_str()); }
        else if (k == "--alpha") { std::string v; if (!next(v)) return false; a.alpha = std::atof(v.c_str()); }
        else if (k == "--threshold") { std::string v; if (!next(v)) return false; a.threshold = std::atof(v.c_str()); }
        else if (k == "--default") { if (!next(a.default_id)) return false; }
        else if (k == "--noise") { std::string v; if (!next(v)) return false; a.noise = std::atof(v.c_str()); }
        else { std::cerr << "Unknown arg: " << k << "\n"; return false; }
    }
    if (a.net_path.empty() && a.scenario.empty()) return false;
    return true;
}

// Build an InputSequence that applies fixed XOR inputs across all ticks
static InputSequence build_xor_sequence(int bit0, int bit1, int total_ticks) {
    InputSequence seq;
    for (int t = 0; t < total_ticks; ++t) {
        if (bit0) seq.addEvent(t, "S0", 200.0f);
        if (bit1) seq.addEvent(t, "S1", 200.0f);
    }
    return seq;
}

// Build an InputSequence for 3-class with noise probability on off-classes
static InputSequence build_3class_sequence(int cls, int total_ticks, float noise) {
    InputSequence seq;
    std::mt19937 rng(12345u);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (int t = 0; t < total_ticks; ++t) {
        seq.addEvent(t, std::string("S") + char('0' + cls), 200.0f);
        for (int c = 0; c < 3; ++c) {
            if (c == cls) continue;
            if (dist(rng) < noise) seq.addEvent(t, std::string("S") + char('0' + c), 200.0f);
        }
    }
    return seq;
}

static void print_metrics(const EpisodeMetrics &m) {
    std::cout << "Winner: " << (m.winner_id.empty() ? std::string("<none>") : m.winner_id) << "\n";
    std::cout << "Margin: " << m.margin << "\n";
    std::cout << "Rates:" << "\n";
    for (const auto &kv : m.rates) {
        std::cout << "  " << kv.first << ": " << kv.second << "\n";
    }
}

int main(int argc, const char* argv[]) {
    Args args;
    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 1;
    }

    // If scenario is provided and net not specified, map to crafted/baseline file paths
    if (args.net_path.empty() && !args.scenario.empty()) {
        if (args.scenario == "xor") {
            args.net_path = args.use_baseline
                ? "../../testing/xor/xor_baseline.net"
                : "../../testing/xor/xor_network.net";
            if (args.default_id.empty()) args.default_id = "O0"; // convenient default for XOR
        } else if (args.scenario == "3class") {
            args.net_path = args.use_baseline
                ? "../../testing/3class/3class_baseline.net"
                : "../../testing/3class/3class_network.net";
        } else {
            std::cerr << "Unknown scenario: " << args.scenario << "\n";
            return 1;
        }
    }

    // Build training config
    TrainingConfig cfg;
    cfg.warmup_ticks = args.warmup;
    cfg.decision_window = args.window;
    cfg.detector.alpha = args.alpha;
    cfg.detector.threshold = args.threshold;
    cfg.detector.type = "ema";
    cfg.detector.default_id = args.default_id;
    cfg.topology = TopologyPolicy();

    // Load network
    Glia net;
    net.configureNetworkFromFile(args.net_path);

    // Prepare trainer
    Trainer trainer(net);

    const int total_ticks = cfg.warmup_ticks + cfg.decision_window;

    if (args.scenario == "xor") {
        std::cout << "=== Evaluating XOR ===\n";
        std::vector<std::pair<int,int>> tests = {{0,0},{0,1},{1,0},{1,1}};
        for (auto &p : tests) {
            InputSequence seq = build_xor_sequence(p.first, p.second, total_ticks);
            EpisodeMetrics m = trainer.evaluate(seq, cfg);
            std::cout << "\nInput: " << p.first << p.second << "\n";
            print_metrics(m);
            int expected = (p.first != p.second) ? 1 : 0;
            std::cout << "Expected: " << (expected ? "TRUE (O1)" : "FALSE (O0)") << "\n";
        }
    } else if (args.scenario == "3class") {
        std::cout << "=== Evaluating 3-Class ===\n";
        for (int c = 0; c < 3; ++c) {
            InputSequence seq = build_3class_sequence(c, total_ticks, args.noise);
            EpisodeMetrics m = trainer.evaluate(seq, cfg);
            std::cout << "\nClass: " << c << " (noise " << args.noise << ")\n";
            print_metrics(m);
            std::cout << "Expected: O" << c << "\n";
        }
    } else {
        // No scenario: just evaluate whatever net was provided with an empty/no-input sequence
        std::cout << "=== Evaluating Custom Net ===\n";
        InputSequence seq; // no events
        EpisodeMetrics m = trainer.evaluate(seq, cfg);
        print_metrics(m);
    }

    return 0;
}
