#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>

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
    bool train = false;
    int epochs = 0;
    float lr = -1.0f;
    float lambda_ = -1.0f;
    float weight_decay = -1.0f;
    float margin_delta = -1.0f;
    float reward_pos = 1.2f;
    float reward_neg = -0.8f;
    float r_target = -1.0f;
    float rate_alpha = -1.0f;
    float eta_theta = 0.0f;
    float eta_leak = 0.0f;
    float prune_eps = -1.0f;
    int prune_patience = -1;
    int grow_edges = -1;
    float init_weight = -1.0f;
    int batch = -1;
    int shuffle = -1; // -1 = default, 0 = false, 1 = true
    std::string reward_mode;   // "binary" | "margin_linear"
    float reward_gain = -9999.0f;
    float reward_min = -9999.0f;
    float reward_max = -9999.0f;
    bool verbose = false;
    int log_every = -1;
    unsigned int seed = 0;
    std::string dataset;       // manifest path: lines "seq_path target_id"
    std::string metrics_json;  // file to write final metrics as JSON
    // Usage-based modulation and pruning
    float usage_boost_gain = 0.0f;
    float inactive_rate_threshold = -1.0f;
    int inactive_rate_patience = -1;
    int prune_inactive_max = -1;
    int prune_inactive_out = -1; // 0/1
    int prune_inactive_in = -1;  // 0/1
    // Checkpoints
    int checkpoints_enable = -1; // 0/1
    int ckpt_l0 = -1;
    int ckpt_l1 = -1;
    int ckpt_l2 = -1;
    // Revert triggers
    int revert_enable = -1;     // 0/1
    std::string revert_metric;  // "accuracy" | "margin"
    int revert_window = -1;     // N
    float revert_drop = -9999.0f; // F
    // Jitter
    float jitter_std = -1.0f;   // weight jitter stddev
    int timing_jitter = -1;     // max onset jitter (ticks)
};

static void print_usage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " [--argfile PATH | --config PATH.json] --net <path> [--warmup U --window W --alpha A --threshold T --default ID] [--train --epochs E [--batch B --shuffle 0|1 --lr L --lambda B --weight_decay D --margin M --reward_mode MODE --reward_gain G --reward_min A --reward_max B --reward_pos RP --reward_neg RN --r_target RT --rate_alpha RA --eta_theta ET --eta_leak EL --prune_eps PE --prune_patience PP --grow_edges G --init_weight IW --usage_boost_gain G --inactive_rate_threshold T --inactive_rate_patience P --prune_inactive_max K --prune_inactive_out 0|1 --prune_inactive_in 0|1 --checkpoints_enable 0|1 --ckpt_l0 N --ckpt_l1 N --ckpt_l2 N --revert_enable 0|1 --revert_metric accuracy|margin --revert_window N --revert_drop F --jitter_std F --timing_jitter N --verbose 0|1 --log_every N --seed S --dataset PATH --metrics_json PATH]]\n"
              << "  " << prog << " --scenario xor|3class [--baseline] [--warmup U --window W --alpha A --threshold T --default ID --noise P] [--train --epochs E --batch B --shuffle 0|1 [...hyperparams...]]\n"
              << "\nExamples:\n"
              << "  " << prog << " --scenario xor --baseline --default O0\n"
              << "  " << prog << " --net ..\\examples\\3class\\3class_network.net --noise 0.1\n";
}

static bool read_argfile(const std::string &path, std::vector<std::string> &tokens) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
    }
    return true;
}

static bool load_manifest(const std::string &path, std::vector<Trainer::EpisodeData> &out) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Could not open dataset manifest: " << path << "\n";
        return false;
    }
    std::string line;
    int line_no = 0;
    while (std::getline(f, line)) {
        line_no++;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string seq_path, target_id;
        if (!(iss >> seq_path >> target_id)) {
            std::cerr << "Malformed dataset line " << line_no << " in " << path << "\n";
            continue;
        }
        InputSequence seq;
        if (!seq.loadFromFile(seq_path)) {
            std::cerr << "Failed to load sequence: " << seq_path << "\n";
            continue;
        }
        Trainer::EpisodeData ex; ex.seq = seq; ex.target_id = target_id; out.push_back(ex);
    }
    return true;
}

// --- Minimal JSON config loader (flat keys; supports nested "detector" object) ---
static std::string read_file_all(const std::string &path) {
    std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f.is_open()) return std::string();
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

static bool extract_string_kv(const std::string &s, const std::string &key, std::string &out) {
    std::regex rgx(std::string("\"") + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch m; if (std::regex_search(s, m, rgx)) { out = m[1]; return true; }
    return false;
}
static bool extract_float_kv(const std::string &s, const std::string &key, float &out) {
    std::regex rgx(std::string("\"") + key + "\"\\s*:\\s*([-+]?([0-9]*\\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)?)");
    std::smatch m; if (std::regex_search(s, m, rgx)) { out = std::strtof(m[1].str().c_str(), nullptr); return true; }
    return false;
}
static bool extract_int_kv(const std::string &s, const std::string &key, int &out) {
    std::regex rgx(std::string("\"") + key + "\"\\s*:\\s*([-+]?[0-9]+)");
    std::smatch m; if (std::regex_search(s, m, rgx)) { out = std::atoi(m[1].str().c_str()); return true; }
    return false;
}
static bool extract_uint_kv(const std::string &s, const std::string &key, unsigned int &out) {
    std::regex rgx(std::string("\"") + key + "\"\\s*:\\s*([0-9]+)");
    std::smatch m; if (std::regex_search(s, m, rgx)) { out = static_cast<unsigned int>(std::strtoul(m[1].str().c_str(), nullptr, 10)); return true; }
    return false;
}
static bool extract_bool_kv(const std::string &s, const std::string &key, bool &out) {
    std::regex rgx(std::string("\"") + key + "\"\\s*:\\s*(true|false|0|1)");
    std::smatch m; if (std::regex_search(s, m, rgx)) {
        std::string v = m[1];
        out = (v == "true" || v == "1");
        return true;
    }
    return false;
}

static bool parse_config_json(const std::string &path, Args &a) {
    std::string s = read_file_all(path);
    if (s.empty()) { std::cerr << "Could not read config: " << path << "\n"; return false; }
    // Top-level keys
    extract_string_kv(s, "net_path", a.net_path);
    extract_string_kv(s, "scenario", a.scenario);
    extract_bool_kv(s,   "baseline", a.use_baseline);
    extract_int_kv(s,    "warmup", a.warmup);
    extract_int_kv(s,    "window", a.window);
    extract_float_kv(s,  "alpha", a.alpha);
    extract_float_kv(s,  "threshold", a.threshold);
    extract_string_kv(s, "default", a.default_id);
    extract_float_kv(s,  "noise", a.noise);
    extract_bool_kv(s,   "train", a.train);
    extract_int_kv(s,    "epochs", a.epochs);
    extract_float_kv(s,  "lr", a.lr);
    extract_float_kv(s,  "lambda", a.lambda_);
    extract_float_kv(s,  "weight_decay", a.weight_decay);
    extract_float_kv(s,  "margin", a.margin_delta);
    extract_string_kv(s, "reward_mode", a.reward_mode);
    extract_float_kv(s,  "reward_gain", a.reward_gain);
    extract_float_kv(s,  "reward_min", a.reward_min);
    extract_float_kv(s,  "reward_max", a.reward_max);
    extract_float_kv(s,  "reward_pos", a.reward_pos);
    extract_float_kv(s,  "reward_neg", a.reward_neg);
    extract_float_kv(s,  "r_target", a.r_target);
    extract_float_kv(s,  "rate_alpha", a.rate_alpha);
    extract_float_kv(s,  "eta_theta", a.eta_theta);
    extract_float_kv(s,  "eta_leak", a.eta_leak);
    extract_float_kv(s,  "prune_eps", a.prune_eps);
    extract_int_kv(s,    "prune_patience", a.prune_patience);
    extract_int_kv(s,    "grow_edges", a.grow_edges);
    extract_float_kv(s,  "init_weight", a.init_weight);
    extract_int_kv(s,    "batch", a.batch);
    { bool b; if (extract_bool_kv(s, "shuffle", b)) a.shuffle = b ? 1 : 0; }
    { bool b; if (extract_bool_kv(s, "verbose", b)) a.verbose = b; }
    extract_int_kv(s,    "log_every", a.log_every);
    extract_uint_kv(s,   "seed", a.seed);
    extract_string_kv(s, "dataset", a.dataset);
    extract_string_kv(s, "metrics_json", a.metrics_json);
    extract_float_kv(s,  "usage_boost_gain", a.usage_boost_gain);
    extract_float_kv(s,  "inactive_rate_threshold", a.inactive_rate_threshold);
    extract_int_kv(s,    "inactive_rate_patience", a.inactive_rate_patience);
    extract_int_kv(s,    "prune_inactive_max", a.prune_inactive_max);
    { bool b; if (extract_bool_kv(s, "prune_inactive_out", b)) a.prune_inactive_out = b ? 1 : 0; }
    { bool b; if (extract_bool_kv(s, "prune_inactive_in", b)) a.prune_inactive_in = b ? 1 : 0; }
    { bool b; if (extract_bool_kv(s, "checkpoints_enable", b)) a.checkpoints_enable = b ? 1 : 0; }
    extract_int_kv(s,    "ckpt_l0", a.ckpt_l0);
    extract_int_kv(s,    "ckpt_l1", a.ckpt_l1);
    extract_int_kv(s,    "ckpt_l2", a.ckpt_l2);
    { bool b; if (extract_bool_kv(s, "revert_enable", b)) a.revert_enable = b ? 1 : 0; }
    extract_string_kv(s, "revert_metric", a.revert_metric);
    extract_int_kv(s,    "revert_window", a.revert_window);
    extract_float_kv(s,  "revert_drop", a.revert_drop);
    extract_float_kv(s,  "jitter_std", a.jitter_std);
    extract_int_kv(s,    "timing_jitter", a.timing_jitter);

    // Nested detector object is optional
    size_t pos = s.find("\"detector\"");
    if (pos != std::string::npos) {
        size_t brace = s.find('{', pos);
        if (brace != std::string::npos) {
            int depth = 1; size_t i = brace + 1;
            for (; i < s.size() && depth > 0; ++i) {
                if (s[i] == '{') depth++; else if (s[i] == '}') depth--;
            }
            if (depth == 0) {
                std::string sub = s.substr(brace, i - brace);
                extract_float_kv(sub,  "alpha", a.alpha);
                extract_float_kv(sub,  "threshold", a.threshold);
                extract_string_kv(sub, "default_id", a.default_id);
            }
        }
    }
    return true;
}

static bool parse_args(int argc, const char* argv[], Args &a) {
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&](std::string &out){ if (i+1>=argc) return false; out = argv[++i]; return true; };
        if (k == "--net") { if (!next(a.net_path)) return false; }
        else if (k == "--scenario") { if (!next(a.scenario)) return false; }
        else if (k == "--config") { std::string p; if (!next(p)) return false; if (!parse_config_json(p, a)) return false; }
        else if (k == "--baseline") { a.use_baseline = true; }
        else if (k == "--warmup") { std::string v; if (!next(v)) return false; a.warmup = std::atoi(v.c_str()); }
        else if (k == "--window") { std::string v; if (!next(v)) return false; a.window = std::atoi(v.c_str()); }
        else if (k == "--alpha") { std::string v; if (!next(v)) return false; a.alpha = std::atof(v.c_str()); }
        else if (k == "--threshold") { std::string v; if (!next(v)) return false; a.threshold = std::atof(v.c_str()); }
        else if (k == "--default") { if (!next(a.default_id)) return false; }
        else if (k == "--noise") { std::string v; if (!next(v)) return false; a.noise = std::atof(v.c_str()); }
        else if (k == "--train") { a.train = true; }
        else if (k == "--epochs") { std::string v; if (!next(v)) return false; a.epochs = std::atoi(v.c_str()); }
        else if (k == "--lr") { std::string v; if (!next(v)) return false; a.lr = std::atof(v.c_str()); }
        else if (k == "--lambda") { std::string v; if (!next(v)) return false; a.lambda_ = std::atof(v.c_str()); }
        else if (k == "--weight_decay") { std::string v; if (!next(v)) return false; a.weight_decay = std::atof(v.c_str()); }
        else if (k == "--margin") { std::string v; if (!next(v)) return false; a.margin_delta = std::atof(v.c_str()); }
        else if (k == "--reward_pos") { std::string v; if (!next(v)) return false; a.reward_pos = std::atof(v.c_str()); }
        else if (k == "--reward_neg") { std::string v; if (!next(v)) return false; a.reward_neg = std::atof(v.c_str()); }
        else if (k == "--r_target") { std::string v; if (!next(v)) return false; a.r_target = std::atof(v.c_str()); }
        else if (k == "--rate_alpha") { std::string v; if (!next(v)) return false; a.rate_alpha = std::atof(v.c_str()); }
        else if (k == "--eta_theta") { std::string v; if (!next(v)) return false; a.eta_theta = std::atof(v.c_str()); }
        else if (k == "--eta_leak") { std::string v; if (!next(v)) return false; a.eta_leak = std::atof(v.c_str()); }
        else if (k == "--prune_eps") { std::string v; if (!next(v)) return false; a.prune_eps = std::atof(v.c_str()); }
        else if (k == "--prune_patience") { std::string v; if (!next(v)) return false; a.prune_patience = std::atoi(v.c_str()); }
        else if (k == "--grow_edges") { std::string v; if (!next(v)) return false; a.grow_edges = std::atoi(v.c_str()); }
        else if (k == "--init_weight") { std::string v; if (!next(v)) return false; a.init_weight = std::atof(v.c_str()); }
        else if (k == "--batch") { std::string v; if (!next(v)) return false; a.batch = std::atoi(v.c_str()); }
        else if (k == "--shuffle") { std::string v; if (!next(v)) return false; a.shuffle = std::atoi(v.c_str()); }
        else if (k == "--reward_mode") { if (!next(a.reward_mode)) return false; }
        else if (k == "--reward_gain") { std::string v; if (!next(v)) return false; a.reward_gain = std::atof(v.c_str()); }
        else if (k == "--reward_min") { std::string v; if (!next(v)) return false; a.reward_min = std::atof(v.c_str()); }
        else if (k == "--reward_max") { std::string v; if (!next(v)) return false; a.reward_max = std::atof(v.c_str()); }
        else if (k == "--verbose") { std::string v; if (!next(v)) return false; a.verbose = (std::atoi(v.c_str()) != 0); }
        else if (k == "--log_every") { std::string v; if (!next(v)) return false; a.log_every = std::atoi(v.c_str()); }
        else if (k == "--seed") { std::string v; if (!next(v)) return false; a.seed = static_cast<unsigned int>(std::strtoul(v.c_str(), nullptr, 10)); }
        else if (k == "--dataset") { if (!next(a.dataset)) return false; }
        else if (k == "--metrics_json") { if (!next(a.metrics_json)) return false; }
        else if (k == "--usage_boost_gain") { std::string v; if (!next(v)) return false; a.usage_boost_gain = std::atof(v.c_str()); }
        else if (k == "--inactive_rate_threshold") { std::string v; if (!next(v)) return false; a.inactive_rate_threshold = std::atof(v.c_str()); }
        else if (k == "--inactive_rate_patience") { std::string v; if (!next(v)) return false; a.inactive_rate_patience = std::atoi(v.c_str()); }
        else if (k == "--prune_inactive_max") { std::string v; if (!next(v)) return false; a.prune_inactive_max = std::atoi(v.c_str()); }
        else if (k == "--prune_inactive_out") { std::string v; if (!next(v)) return false; a.prune_inactive_out = std::atoi(v.c_str()); }
        else if (k == "--prune_inactive_in") { std::string v; if (!next(v)) return false; a.prune_inactive_in = std::atoi(v.c_str()); }
        else if (k == "--checkpoints_enable") { std::string v; if (!next(v)) return false; a.checkpoints_enable = std::atoi(v.c_str()); }
        else if (k == "--ckpt_l0") { std::string v; if (!next(v)) return false; a.ckpt_l0 = std::atoi(v.c_str()); }
        else if (k == "--ckpt_l1") { std::string v; if (!next(v)) return false; a.ckpt_l1 = std::atoi(v.c_str()); }
        else if (k == "--ckpt_l2") { std::string v; if (!next(v)) return false; a.ckpt_l2 = std::atoi(v.c_str()); }
        else if (k == "--revert_enable") { std::string v; if (!next(v)) return false; a.revert_enable = std::atoi(v.c_str()); }
        else if (k == "--revert_metric") { if (!next(a.revert_metric)) return false; }
        else if (k == "--revert_window") { std::string v; if (!next(v)) return false; a.revert_window = std::atoi(v.c_str()); }
        else if (k == "--revert_drop") { std::string v; if (!next(v)) return false; a.revert_drop = std::atof(v.c_str()); }
        else if (k == "--jitter_std") { std::string v; if (!next(v)) return false; a.jitter_std = std::atof(v.c_str()); }
        else if (k == "--timing_jitter") { std::string v; if (!next(v)) return false; a.timing_jitter = std::atoi(v.c_str()); }
        else { std::cerr << "Unknown arg: " << k << "\n"; return false; }
    }
    if (a.net_path.empty() && a.scenario.empty()) return false;
    return true;
}

// Build an InputSequence for XOR with optional timing jitter (delay onsets per sensor)
static InputSequence build_xor_sequence(int bit0, int bit1, int total_ticks, int timing_jitter, std::mt19937 &rng) {
    InputSequence seq;
    int j0 = 0, j1 = 0;
    if (timing_jitter > 0) {
        std::uniform_int_distribution<int> d(0, timing_jitter);
        j0 = d(rng);
        j1 = d(rng);
    }
    for (int t = 0; t < total_ticks; ++t) {
        if (bit0 && t >= j0) seq.addEvent(t, "S0", 200.0f);
        if (bit1 && t >= j1) seq.addEvent(t, "S1", 200.0f);
    }
    return seq;
}

// Build an InputSequence for 3-class with optional timing jitter (delay onset for active class)
static InputSequence build_3class_sequence(int cls, int total_ticks, float noise, int timing_jitter, std::mt19937 &rng) {
    InputSequence seq;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    int j = 0;
    if (timing_jitter > 0) {
        std::uniform_int_distribution<int> d(0, timing_jitter);
        j = d(rng);
    }
    for (int t = 0; t < total_ticks; ++t) {
        if (t >= j) seq.addEvent(t, std::string("S") + char('0' + cls), 200.0f);
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
    // Expand args with optional --argfile content
    std::vector<std::string> extra;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k == "--argfile" && i + 1 < argc) {
            std::string path = argv[++i];
            read_argfile(path, extra);
        } else if (k == "--config" && i + 1 < argc) {
            // We'll parse the JSON later inside parse_args; still keep the token for visibility
            extra.push_back(k);
            extra.push_back(argv[++i]);
        } else {
            extra.push_back(k);
        }
    }
    std::vector<const char*> cargs;
    cargs.push_back(argv[0]);
    for (auto &s : extra) cargs.push_back(s.c_str());

    Args args;
    if (!parse_args((int)cargs.size(), cargs.data(), args)) {
        print_usage(argv[0]);
        return 1;
    }

    // If scenario is provided and net not specified, map to examples paths
    if (args.net_path.empty() && !args.scenario.empty()) {
        if (args.scenario == "xor") {
            args.net_path = std::string("../../examples/xor/") + (args.use_baseline ? "xor_baseline.net" : "xor_network.net");
            if (args.default_id.empty()) args.default_id = "O0";
        } else if (args.scenario == "3class") {
            args.net_path = std::string("../../examples/3class/") + (args.use_baseline ? "3class_baseline.net" : "3class_network.net");
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
    if (args.lr >= 0.0f) cfg.lr = args.lr;
    if (args.lambda_ >= 0.0f) cfg.elig_lambda = args.lambda_;
    if (args.weight_decay >= 0.0f) cfg.weight_decay = args.weight_decay;
    if (args.margin_delta >= 0.0f) cfg.margin_delta = args.margin_delta;
    cfg.reward_pos = args.reward_pos;
    cfg.reward_neg = args.reward_neg;
    if (args.r_target >= 0.0f) cfg.r_target = args.r_target;
    if (args.rate_alpha >= 0.0f) cfg.rate_alpha = args.rate_alpha;
    cfg.eta_theta = args.eta_theta;
    cfg.eta_leak = args.eta_leak;
    if (args.prune_eps >= 0.0f) cfg.prune_epsilon = args.prune_eps;
    if (args.prune_patience >= 0) cfg.prune_patience = args.prune_patience;
    if (args.grow_edges >= 0) cfg.grow_edges = args.grow_edges;
    if (args.init_weight >= 0.0f) cfg.init_weight = args.init_weight;
    if (args.batch > 0) cfg.batch_size = args.batch;
    if (args.shuffle == 0) cfg.shuffle = false; else if (args.shuffle == 1) cfg.shuffle = true;
    if (!args.reward_mode.empty()) cfg.reward_mode = args.reward_mode;
    if (args.reward_gain > -9000.0f) cfg.reward_gain = args.reward_gain;
    if (args.reward_min > -9000.0f) cfg.reward_min = args.reward_min;
    if (args.reward_max > -9000.0f) cfg.reward_max = args.reward_max;
    if (args.log_every >= 0) cfg.log_every = args.log_every;
    cfg.verbose = args.verbose;
    if (args.seed != 0) cfg.seed = args.seed;
    if (args.usage_boost_gain != 0.0f) cfg.usage_boost_gain = args.usage_boost_gain;
    if (args.inactive_rate_threshold >= 0.0f) cfg.inactive_rate_threshold = args.inactive_rate_threshold;
    if (args.inactive_rate_patience >= 0) cfg.inactive_rate_patience = args.inactive_rate_patience;
    if (args.prune_inactive_max >= 0) cfg.prune_inactive_max = args.prune_inactive_max;
    if (args.prune_inactive_out == 0) cfg.prune_inactive_out = false; else if (args.prune_inactive_out == 1) cfg.prune_inactive_out = true;
    if (args.prune_inactive_in == 0) cfg.prune_inactive_in = false; else if (args.prune_inactive_in == 1) cfg.prune_inactive_in = true;
    if (args.checkpoints_enable == 0) cfg.checkpoints_enable = false; else if (args.checkpoints_enable == 1) cfg.checkpoints_enable = true;
    if (args.ckpt_l0 >= 0) cfg.ckpt_l0 = args.ckpt_l0;
    if (args.ckpt_l1 >= 0) cfg.ckpt_l1 = args.ckpt_l1;
    if (args.ckpt_l2 >= 0) cfg.ckpt_l2 = args.ckpt_l2;
    if (args.revert_enable == 0) cfg.revert_enable = false; else if (args.revert_enable == 1) cfg.revert_enable = true;
    if (!args.revert_metric.empty()) cfg.revert_metric = args.revert_metric;
    if (args.revert_window >= 0) cfg.revert_window = args.revert_window;
    if (args.revert_drop > -9000.0f) cfg.revert_drop = args.revert_drop;
    if (args.jitter_std >= 0.0f) cfg.weight_jitter_std = args.jitter_std;
    if (args.timing_jitter >= 0) cfg.timing_jitter = args.timing_jitter;

    // Load network
    Glia net;
    net.configureNetworkFromFile(args.net_path);

    // Prepare trainer
    Trainer trainer(net);
    trainer.reseed(cfg.seed);

    const int total_ticks = cfg.warmup_ticks + cfg.decision_window;
    std::mt19937 rng_local(cfg.seed);

    if (args.train && args.epochs > 0) {
        std::vector<Trainer::EpisodeData> dataset;
        if (!args.dataset.empty()) {
            if (!load_manifest(args.dataset, dataset)) {
                std::cerr << "Failed to load dataset manifest.\n";
                return 1;
            }
        } else if (args.scenario == "xor") {
            std::vector<std::pair<int,int>> items = {{0,0},{0,1},{1,0},{1,1}};
            for (auto &p : items) {
                Trainer::EpisodeData ex;
                ex.seq = build_xor_sequence(p.first, p.second, total_ticks, cfg.timing_jitter, rng_local);
                int expected = (p.first != p.second) ? 1 : 0;
                ex.target_id = expected ? std::string("O1") : std::string("O0");
                dataset.push_back(ex);
            }
        } else if (args.scenario == "3class") {
            for (int c = 0; c < 3; ++c) {
                Trainer::EpisodeData ex;
                ex.seq = build_3class_sequence(c, total_ticks, args.noise, cfg.timing_jitter, rng_local);
                ex.target_id = std::string("O") + char('0' + c);
                dataset.push_back(ex);
            }
        }
        trainer.trainEpoch(dataset, args.epochs, cfg);
    }

    if (args.scenario == "xor") {
        std::cout << "=== Evaluating XOR ===\n";
        std::vector<std::pair<int,int>> tests = {{0,0},{0,1},{1,0},{1,1}};
        int correct = 0;
        std::vector<EpisodeMetrics> evals;
        for (auto &p : tests) {
            InputSequence seq = build_xor_sequence(p.first, p.second, total_ticks, cfg.timing_jitter, rng_local);
            EpisodeMetrics m = trainer.evaluate(seq, cfg);
            evals.push_back(m);
            std::cout << "\nInput: " << p.first << p.second << "\n";
            print_metrics(m);
            int expected = (p.first != p.second) ? 1 : 0;
            std::cout << "Expected: " << (expected ? "TRUE (O1)" : "FALSE (O0)") << "\n";
            std::string target = expected ? std::string("O1") : std::string("O0");
            if (m.winner_id == target) correct++;
        }
        if (!args.metrics_json.empty()) {
            std::ofstream jf(args.metrics_json);
            if (jf.is_open()) {
                jf << "{\n  \"scenario\": \"xor\",\n  \"accuracy\": " << (evals.empty() ? 0.0 : (double)correct / evals.size()) << ",\n  \"details\": [\n";
                for (size_t i = 0; i < evals.size(); ++i) {
                    jf << "    { \"index\": " << i << ", \"winner\": \"" << evals[i].winner_id
                       << "\", \"margin\": " << evals[i].margin << " }" << (i+1<evals.size()?",":"") << "\n";
                }
                jf << "  ]\n}\n";
            }
        }
        std::cout << "\nSummary: accuracy " << correct << "/" << evals.size()
                  << " (" << (evals.empty() ? 0.0 : (100.0 * (double)correct / evals.size())) << "%)\n";
    } else if (args.scenario == "3class") {
        std::cout << "=== Evaluating 3-Class ===\n";
        int correct = 0;
        std::vector<EpisodeMetrics> evals;
        for (int c = 0; c < 3; ++c) {
            InputSequence seq = build_3class_sequence(c, total_ticks, args.noise, cfg.timing_jitter, rng_local);
            EpisodeMetrics m = trainer.evaluate(seq, cfg);
            evals.push_back(m);
            std::cout << "\nClass: " << c << " (noise " << args.noise << ")\n";
            print_metrics(m);
            std::cout << "Expected: O" << c << "\n";
            if (m.winner_id == (std::string("O") + char('0' + c))) correct++;
        }
        if (!args.metrics_json.empty()) {
            std::ofstream jf(args.metrics_json);
            if (jf.is_open()) {
                jf << "{\n  \"scenario\": \"3class\",\n  \"accuracy\": " << (evals.empty() ? 0.0 : (double)correct / evals.size()) << ",\n  \"details\": [\n";
                for (size_t i = 0; i < evals.size(); ++i) {
                    jf << "    { \"index\": " << i << ", \"winner\": \"" << evals[i].winner_id
                       << "\", \"margin\": " << evals[i].margin << " }" << (i+1<evals.size()?",":"") << "\n";
                }
                jf << "  ]\n}\n";
            }
        }
        std::cout << "\nSummary: accuracy " << correct << "/" << evals.size()
                  << " (" << (evals.empty() ? 0.0 : (100.0 * (double)correct / evals.size())) << "%)\n";
    } else {
        // No scenario: just evaluate whatever net was provided with an empty/no-input sequence
        std::cout << "=== Evaluating Custom Net ===\n";
        InputSequence seq; // no events
        EpisodeMetrics m = trainer.evaluate(seq, cfg);
        print_metrics(m);
    }

    return 0;
}
