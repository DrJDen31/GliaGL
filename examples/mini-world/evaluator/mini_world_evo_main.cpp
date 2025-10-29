// Mini-World Evolution runner using EvolutionEngine

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <regex>
#include <cstdio>
#include <unordered_map>

#include "../../../src/arch/glia.h"
#include "../../../src/arch/neuron.h"
#include "../../../src/arch/input_sequence.h"
#include "../../../src/train/trainer.h"
#include "../../../src/train/training_config.h"
#include "../../../src/evo/evolution_engine.h"

// --- simple JSON helpers ---
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
    std::smatch m; if (std::regex_search(s, m, rgx)) { std::string v = m[1]; out = (v == "true" || v == "1"); return true; }
    return false;
}

// --- mini-world dataset loader ---
static bool load_labels_csv(const std::string &labels_csv_path, std::map<int,int> &out) {
    std::ifstream f(labels_csv_path.c_str());
    if (!f.is_open()) { std::cerr << "Could not open labels CSV: " << labels_csv_path << "\n"; return false; }
    std::string line; int line_no = 0;
    while (std::getline(f, line)) {
        line_no++;
        if (line_no == 1) continue; // header
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string a,b,c;
        if (!std::getline(iss, a, ',')) continue;
        if (!std::getline(iss, b, ',')) continue;
        if (!std::getline(iss, c, ',')) continue;
        int clip_id = std::atoi(a.c_str());
        int tick    = std::atoi(b.c_str()); (void)tick;
        int count   = std::atoi(c.c_str());
        if (out.find(clip_id) == out.end()) out[clip_id] = count;
    }
    return true;
}
static std::string make_seq_path(const std::string &root, int clip_id) {
    char buf[64]; std::snprintf(buf, sizeof(buf), "clip_%05d.seq", clip_id);
    std::string p = root; if (!p.empty() && (p.back() != '/' && p.back() != '\\')) p += "/"; p += buf;
    return p;
}
static bool build_miniworld_dataset(const std::string &data_root, int max_class,
                                    std::vector<Trainer::EpisodeData> &out) {
    std::string labels_csv = data_root + (data_root.back()=='/'||data_root.back()=='\\' ? std::string("") : std::string("/")) + "labels/labels_counts.csv";
    std::map<int,int> labels;
    if (!load_labels_csv(labels_csv, labels)) return false;
    std::string seq_dir = data_root + (data_root.back()=='/'||data_root.back()=='\\' ? std::string("") : std::string("/")) + "seq";
    for (const auto &kv : labels) {
        int clip_id = kv.first;
        int count = kv.second; if (count < 0) count = 0; if (count > max_class) count = max_class;
        std::string seq_path = make_seq_path(seq_dir, clip_id);
        std::ifstream test(seq_path.c_str()); if (!test.is_open()) continue;
        test.close();
        InputSequence seq; if (!seq.loadFromFile(seq_path)) continue;
        Trainer::EpisodeData ex; ex.seq = seq; ex.target_id = std::string("O") + std::to_string(count);
        out.push_back(ex);
    }
    return !out.empty();
}

// --- metrics json writer ---
static void write_metrics_json(const std::string &path,
                               const std::vector<double> &fitness,
                               const std::vector<double> &acc,
                               const std::vector<double> &margin) {
    std::ofstream jf(path.c_str(), std::ios::out | std::ios::trunc);
    if (!jf.is_open()) return;
    jf << "{\n";
    jf << "  \"generations\": " << fitness.size() << ",\n";
    jf << "  \"best_fitness\": ["; for (size_t i=0;i<fitness.size();++i){ jf<<fitness[i]; if (i+1<fitness.size()) jf<<","; } jf << "],\n";
    jf << "  \"best_acc\": ["; for (size_t i=0;i<acc.size();++i){ jf<<acc[i]; if (i+1<acc.size()) jf<<","; } jf << "],\n";
    jf << "  \"best_margin\": ["; for (size_t i=0;i<margin.size();++i){ jf<<margin[i]; if (i+1<margin.size()) jf<<","; } jf << "]\n";
    jf << "}\n";
}

int main(int argc, char** argv) {
    if (argc < 3 || std::string(argv[1]) != std::string("--config")) {
        std::cout << "Usage: " << argv[0] << " --config <path.json>\n";
        return 1;
    }
    const std::string cfg_path = argv[2];
    const std::string s = read_file_all(cfg_path);
    if (s.empty()) { std::cerr << "Could not read config: " << cfg_path << "\n"; return 2; }

    // Read essentials
    std::string net_path, data_root, out_dir, final_best, metrics_json;
    std::string lineage_json;
    float train_fraction = 0.9f; int max_class = 4;
    extract_string_kv(s, "net_path", net_path);
    extract_string_kv(s, "data_root", data_root);
    extract_float_kv(s,  "train_fraction", train_fraction);
    extract_int_kv(s,    "max_class", max_class);
    extract_string_kv(s, "out_dir", out_dir);
    extract_string_kv(s, "final_best_net", final_best);
    extract_string_kv(s, "metrics_json", metrics_json);
    extract_string_kv(s, "lineage_json", lineage_json);
    if (net_path.empty() || data_root.empty()) { std::cerr << "Config must set net_path and data_root\n"; return 3; }

    // Detector/Episode params
    int warmup=40, window=140; float det_alpha=0.05f, det_threshold=0.001f; std::string det_default_id;
    extract_int_kv(s, "warmup", warmup);
    extract_int_kv(s, "window", window);
    extract_float_kv(s, "alpha", det_alpha);
    extract_float_kv(s, "threshold", det_threshold);
    extract_string_kv(s, "default", det_default_id);
    // Or nested detector
    size_t pos = s.find("\"detector\"");
    if (pos != std::string::npos) {
        size_t brace = s.find('{', pos); if (brace != std::string::npos) {
            int depth = 1; size_t i = brace + 1;
            for (; i < s.size() && depth > 0; ++i) { if (s[i]=='{') depth++; else if (s[i]=='}') depth--; }
            if (depth == 0) {
                std::string sub = s.substr(brace, i - brace);
                extract_float_kv(sub,  "alpha", det_alpha);
                extract_float_kv(sub,  "threshold", det_threshold);
                extract_string_kv(sub, "default_id", det_default_id);
            }
        }
    }

    // Training hyperparams
    TrainingConfig tc;
    tc.warmup_ticks = warmup; tc.decision_window = window;
    tc.detector.alpha = det_alpha; tc.detector.threshold = det_threshold; tc.detector.default_id = det_default_id;
    extract_float_kv(s,  "lr", tc.lr);
    extract_float_kv(s,  "lambda", tc.elig_lambda);
    extract_float_kv(s,  "weight_decay", tc.weight_decay);
    extract_float_kv(s,  "margin", tc.margin_delta);
    { std::string rm; if (extract_string_kv(s, "reward_mode", rm)) tc.reward_mode = rm; }
    { std::string ug; if (extract_string_kv(s, "update_gating", ug)) tc.update_gating = ug; }
    extract_float_kv(s,  "reward_gain", tc.reward_gain);
    extract_float_kv(s,  "reward_min", tc.reward_min);
    extract_float_kv(s,  "reward_max", tc.reward_max);
    extract_float_kv(s,  "reward_pos", tc.reward_pos);
    extract_float_kv(s,  "reward_neg", tc.reward_neg);
    extract_float_kv(s,  "r_target", tc.r_target);
    extract_float_kv(s,  "rate_alpha", tc.rate_alpha);
    { bool b; if (extract_bool_kv(s, "elig_post_use_rate", b)) tc.elig_post_use_rate = b; }
    { bool b; if (extract_bool_kv(s, "no_update_if_satisfied", b)) tc.no_update_if_satisfied = b; }
    { bool b; if (extract_bool_kv(s, "use_advantage_baseline", b)) tc.use_advantage_baseline = b; }
    extract_float_kv(s,  "baseline_beta", tc.baseline_beta);
    extract_float_kv(s,  "weight_clip", tc.weight_clip);
    int batch = 8; extract_int_kv(s, "batch", batch); tc.batch_size = std::max(1, batch);
    bool shuffle = true; { bool b; if (extract_bool_kv(s, "shuffle", b)) shuffle = b; }
    tc.shuffle = shuffle;

    // Evo config
    EvolutionEngine::Config ec;
    int train_epochs = 3; extract_int_kv(s, "train_epochs", train_epochs); ec.train_epochs = train_epochs;
    extract_int_kv(s, "population", ec.population);
    extract_int_kv(s, "generations", ec.generations);
    extract_int_kv(s, "elite", ec.elite);
    extract_int_kv(s, "parents_pool", ec.parents_pool);
    extract_float_kv(s, "sigma_w", ec.sigma_w);
    extract_float_kv(s, "sigma_thr", ec.sigma_thr);
    extract_float_kv(s, "sigma_leak", ec.sigma_leak);
    extract_float_kv(s, "w_acc", ec.w_acc);
    extract_float_kv(s, "w_margin", ec.w_margin);
    extract_float_kv(s, "w_sparsity", ec.w_sparsity);
    extract_uint_kv(s, "seed", ec.seed);
    ec.lineage_json = lineage_json;

    // Build dataset and split
    std::vector<Trainer::EpisodeData> ds; if (!build_miniworld_dataset(data_root, max_class, ds)) { std::cerr << "No dataset examples found\n"; return 4; }
    std::mt19937 rng(ec.seed); std::shuffle(ds.begin(), ds.end(), rng);
    size_t N = ds.size(); size_t Ntrain = static_cast<size_t>(std::max(0.0f, std::min(1.0f, train_fraction)) * static_cast<float>(N));
    if (Ntrain == 0 || Ntrain >= N) Ntrain = (N > 1 ? N - 1 : N);
    std::vector<Trainer::EpisodeData> train_set(ds.begin(), ds.begin() + Ntrain);
    std::vector<Trainer::EpisodeData> val_set(ds.begin() + Ntrain, ds.end());
    std::cout << "Mini-World dataset: total=" << N << " train=" << train_set.size() << " val=" << val_set.size() << "\n";

    // Callbacks: save best per-generation if out_dir set
    EvolutionEngine::Callbacks cbs;
    if (!out_dir.empty()) {
        cbs.on_generation = [&](int gen, const EvolutionEngine::NetSnapshot &best, const EvoMetrics &m){
            (void)m;
            Glia net; net.configureNetworkFromFile(net_path, /*verbose=*/false);
            // Manual restore (duplicated from engine)
            // Build edge set
            std::unordered_map<std::string, std::unordered_map<std::string,float>> edge_set;
            for (const auto &e : best.edges) edge_set[e.from][e.to] = e.w;
            net.forEachNeuron([&](Neuron &from){
                std::vector<std::string> to_remove;
                const auto &conns = from.getConnections();
                for (const auto &kv : conns) {
                    auto itf = edge_set.find(from.getId());
                    if (itf == edge_set.end() || itf->second.find(kv.first) == itf->second.end()) to_remove.push_back(kv.first);
                }
                for (const auto &tid : to_remove) from.removeConnection(tid);
            });
            for (const auto &e : best.edges) {
                Neuron *from = net.getNeuronById(e.from);
                Neuron *to = net.getNeuronById(e.to);
                if (!from || !to) continue;
                const auto &conns = from->getConnections();
                if (conns.find(e.to) == conns.end()) from->addConnection(e.w, *to);
                else from->setTransmitter(e.to, e.w);
            }
            for (const auto &r : best.neurons) {
                Neuron *n = net.getNeuronById(r.id);
                if (!n) continue;
                n->setThreshold(r.thr);
                n->setLeak(r.leak);
            }
            char fname[256]; std::snprintf(fname, sizeof(fname), "%s/%s_%03d.net", out_dir.c_str(), "best_gen", gen+1);
            net.saveNetworkToFile(fname);
            std::cout << "Saved best genome for gen " << (gen+1) << " -> " << fname << "\n";
        };
    }

    // Run evolution
    EvolutionEngine engine(net_path, train_set, val_set, tc, ec, cbs);
    EvolutionEngine::Result res = engine.run();

    if (!final_best.empty()) {
        Glia net; net.configureNetworkFromFile(net_path, /*verbose=*/false);
        // restore best
        std::unordered_map<std::string, std::unordered_map<std::string,float>> edge_set;
        for (const auto &e : res.best_genome.edges) edge_set[e.from][e.to] = e.w;
        net.forEachNeuron([&](Neuron &from){
            std::vector<std::string> to_remove;
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                auto itf = edge_set.find(from.getId());
                if (itf == edge_set.end() || itf->second.find(kv.first) == itf->second.end()) to_remove.push_back(kv.first);
            }
            for (const auto &tid : to_remove) from.removeConnection(tid);
        });
        for (const auto &e : res.best_genome.edges) {
            Neuron *from = net.getNeuronById(e.from);
            Neuron *to = net.getNeuronById(e.to);
            if (!from || !to) continue;
            const auto &conns = from->getConnections();
            if (conns.find(e.to) == conns.end()) from->addConnection(e.w, *to);
            else from->setTransmitter(e.to, e.w);
        }
        for (const auto &r : res.best_genome.neurons) {
            Neuron *n = net.getNeuronById(r.id);
            if (!n) continue;
            n->setThreshold(r.thr);
            n->setLeak(r.leak);
        }
        net.saveNetworkToFile(final_best);
    }
    if (!metrics_json.empty()) write_metrics_json(metrics_json, res.best_fitness_hist, res.best_acc_hist, res.best_margin_hist);
    return 0;
}
