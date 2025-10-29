// 3-class Evolution runner using EvolutionEngine and programmatic dataset

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
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
static std::string read_file_all(const std::string &path) { std::ifstream f(path.c_str(), std::ios::in|std::ios::binary); if(!f.is_open()) return {}; std::ostringstream ss; ss<<f.rdbuf(); return ss.str(); }
static bool extract_string_kv(const std::string &s, const std::string &key, std::string &out) { std::regex rgx(std::string("\"")+key+"\"\\s*:\\s*\"([^\"]*)\""); std::smatch m; if(std::regex_search(s,m,rgx)){ out=m[1]; return true;} return false; }
static bool extract_float_kv(const std::string &s, const std::string &key, float &out) { std::regex rgx(std::string("\"")+key+"\"\\s*:\\s*([-+]?([0-9]*\\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)?)"); std::smatch m; if(std::regex_search(s,m,rgx)){ out=std::strtof(m[1].str().c_str(),nullptr); return true;} return false; }
static bool extract_int_kv(const std::string &s, const std::string &key, int &out) { std::regex rgx(std::string("\"")+key+"\"\\s*:\\s*([-+]?[0-9]+)"); std::smatch m; if(std::regex_search(s,m,rgx)){ out=std::atoi(m[1].str().c_str()); return true;} return false; }
static bool extract_uint_kv(const std::string &s, const std::string &key, unsigned int &out) { std::regex rgx(std::string("\"")+key+"\"\\s*:\\s*([0-9]+)"); std::smatch m; if(std::regex_search(s,m,rgx)){ out=(unsigned)std::strtoul(m[1].str().c_str(),nullptr,10); return true;} return false; }
static bool extract_bool_kv(const std::string &s, const std::string &key, bool &out) { std::regex rgx(std::string("\"")+key+"\"\\s*:\\s*(true|false|0|1)"); std::smatch m; if(std::regex_search(s,m,rgx)){ std::string v=m[1]; out=(v=="true"||v=="1"); return true;} return false; }

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

int main(int argc, char** argv) {
    if (argc < 3 || std::string(argv[1]) != std::string("--config")) {
        std::cout << "Usage: " << argv[0] << " --config <path.json>\n";
        return 1;
    }
    const std::string cfg_path = argv[2];
    const std::string s = read_file_all(cfg_path);
    if (s.empty()) { std::cerr << "Could not read config: " << cfg_path << "\n"; return 2; }

    // Essentials
    std::string net_path, out_dir, final_best, metrics_json;
    std::string lineage_json;
    extract_string_kv(s, "net_path", net_path);
    extract_string_kv(s, "out_dir", out_dir);
    extract_string_kv(s, "final_best_net", final_best);
    extract_string_kv(s, "metrics_json", metrics_json);
    if (net_path.empty()) net_path = std::string(".\\examples\\3class\\3class_network.net");
    extract_string_kv(s, "lineage_json", lineage_json);

    // Dataset params
    float train_fraction = 0.8f; int n_per_class = 100; float noise = 0.05f; int timing_jitter = 0;
    extract_float_kv(s,  "train_fraction", train_fraction);
    extract_int_kv(s,    "n_per_class", n_per_class);
    extract_float_kv(s,  "noise", noise);
    extract_int_kv(s,    "timing_jitter", timing_jitter);

    // Episode/detector
    int warmup=20, window=80; float det_alpha=0.05f, det_threshold=0.01f; std::string det_default_id = "O0";
    extract_int_kv(s, "warmup", warmup);
    extract_int_kv(s, "window", window);
    extract_float_kv(s, "alpha", det_alpha);
    extract_float_kv(s, "threshold", det_threshold);
    extract_string_kv(s, "default", det_default_id);
    // nested detector
    size_t pos = s.find("\"detector\""); if (pos!=std::string::npos){ size_t b=s.find('{',pos); if(b!=std::string::npos){ int d=1; size_t i=b+1; for(; i<s.size()&&d>0; ++i){ if(s[i]=='{') d++; else if(s[i]=='}') d--; } if(d==0){ std::string sub=s.substr(b,i-b); extract_float_kv(sub,"alpha",det_alpha); extract_float_kv(sub,"threshold",det_threshold); extract_string_kv(sub,"default_id",det_default_id);} } }

    // Training hyperparams
    TrainingConfig tc; tc.warmup_ticks=warmup; tc.decision_window=window; tc.detector.alpha=det_alpha; tc.detector.threshold=det_threshold; tc.detector.default_id=det_default_id;
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

    // Build programmatic dataset
    std::vector<Trainer::EpisodeData> ds; ds.reserve(3 * std::max(1, n_per_class));
    std::mt19937 rng(ec.seed);
    int total_ticks = warmup + window;
    for (int cls = 0; cls < 3; ++cls) {
        for (int i = 0; i < std::max(1, n_per_class); ++i) {
            Trainer::EpisodeData ex; ex.seq = build_3class_sequence(cls, total_ticks, noise, timing_jitter, rng); ex.target_id = std::string("O") + char('0' + cls);
            ds.push_back(std::move(ex));
        }
    }
    std::shuffle(ds.begin(), ds.end(), rng);
    size_t N = ds.size(); size_t Ntrain = static_cast<size_t>(std::max(0.0f, std::min(1.0f, train_fraction)) * static_cast<float>(N));
    if (Ntrain == 0 || Ntrain >= N) Ntrain = (N > 1 ? N - 1 : N);
    std::vector<Trainer::EpisodeData> train_set(ds.begin(), ds.begin() + Ntrain);
    std::vector<Trainer::EpisodeData> val_set(ds.begin() + Ntrain, ds.end());
    std::cout << "3class dataset: total=" << N << " train=" << train_set.size() << " val=" << val_set.size() << "\n";

    // Callbacks: save best per-generation if out_dir
    EvolutionEngine::Callbacks cbs;
    if (!out_dir.empty()) {
        cbs.on_generation = [&](int gen, const EvolutionEngine::NetSnapshot &best, const EvoMetrics &m){
            (void)m;
            Glia net; net.configureNetworkFromFile(net_path, /*verbose=*/false);
            // restore best
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
                n->setThreshold(r.thr); n->setLeak(r.leak);
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
        // restore and save
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
            n->setThreshold(r.thr); n->setLeak(r.leak);
        }
        net.saveNetworkToFile(final_best);
    }
    if (!metrics_json.empty()) {
        std::ofstream jf(metrics_json.c_str(), std::ios::out | std::ios::trunc);
        if (jf.is_open()) {
            jf << "{\n";
            jf << "  \"generations\": " << res.best_fitness_hist.size() << ",\n";
            jf << "  \"best_fitness\": ["; for (size_t i=0;i<res.best_fitness_hist.size();++i){ jf<<res.best_fitness_hist[i]; if(i+1<res.best_fitness_hist.size()) jf<<","; } jf << "],\n";
            jf << "  \"best_acc\": ["; for (size_t i=0;i<res.best_acc_hist.size();++i){ jf<<res.best_acc_hist[i]; if(i+1<res.best_acc_hist.size()) jf<<","; } jf << "],\n";
            jf << "  \"best_margin\": ["; for (size_t i=0;i<res.best_margin_hist.size();++i){ jf<<res.best_margin_hist[i]; if(i+1<res.best_margin_hist.size()) jf<<","; } jf << "]\n";
            jf << "}\n";
        }
    }
    return 0;
}
