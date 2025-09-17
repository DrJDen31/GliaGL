#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <random>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cassert>

namespace glia_training {

struct TrainerConfig {
    float lr_hebb = 0.02f;
    float lr_anti = 0.01f;
    float weight_decay = 0.0005f;
    float pre_tau = 10.0f;
    float post_tau = 10.0f;
    float homeo_target_sum = 1.0f;
    float homeo_rate = 0.005f;
    float w_min = 0.0f;
    float w_max = 1.0f;
    bool  enable_structural = true;
    float prune_threshold = 0.02f;
    float rewire_prob = 0.01f;
    int   max_added_per_step = 2;
    uint64_t rng_seed = 0;
    float dt = 1.0f;
};

struct NetworkIO {
    std::function<size_t()> num_neurons;
    std::function<bool(size_t)> fired;
    std::function<const std::unordered_map<int, float>&(size_t)> out_edges;
    std::function<void(size_t,int,float)> set_weight;
    std::function<void(size_t,int)> remove_edge;
    std::function<void(size_t,int,float)> add_edge;
    std::function<void()> on_after_update = []{};
};

class Trainer {
public:
    explicit Trainer(const TrainerConfig& cfg, NetworkIO io)
        : cfg_(cfg), io_(std::move(io)) {
        seed_rng(); resize_traces();
    }
    void on_step_begin() {
        ensure_sizes();
        float pre_alpha  = std::exp(-1.0f / std::max(1e-6f, cfg_.pre_tau));
        float post_alpha = std::exp(-1.0f / std::max(1e-6f, cfg_.post_tau));
        for (size_t i = 0; i < pre_trace_.size(); ++i) {
            pre_trace_[i]  *= pre_alpha;
            post_trace_[i] *= post_alpha;
        }
        for (size_t i = 0; i < io_.num_neurons(); ++i) if (io_.fired(i)) { pre_trace_[i]+=1.0f; post_trace_[i]+=1.0f; }
    }
    void on_step_end() {
        ensure_sizes();
        apply_synaptic_plasticity();
        if (cfg_.enable_structural) apply_structural_plasticity();
        if (io_.on_after_update) io_.on_after_update();
        step_++;
    }
private:
    TrainerConfig cfg_;
    NetworkIO io_;
    std::vector<float> pre_trace_, post_trace_;
    std::mt19937_64 rng_;
    uint64_t step_ = 0;

    void seed_rng() {
        if (cfg_.rng_seed) rng_.seed(cfg_.rng_seed);
        else rng_.seed((uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }
    void resize_traces() { size_t n = io_.num_neurons(); pre_trace_.assign(n,0.0f); post_trace_.assign(n,0.0f); }
    void ensure_sizes() { if (pre_trace_.size()!=io_.num_neurons()) resize_traces(); }

    void apply_synaptic_plasticity() {
        const size_t N = io_.num_neurons();
        for (size_t i = 0; i < N; ++i) {
            const auto& edges = io_.out_edges(i);
            if (edges.empty()) continue;
            float sum_w = 0.0f; for (auto& kv : edges) sum_w += kv.second;
            float scale = 1.0f;
            if (sum_w > 1e-6f) scale = 1.0f + cfg_.homeo_rate * ((cfg_.homeo_target_sum - sum_w) / std::max(1e-6f, sum_w));
            for (auto& kv : edges) {
                int j = kv.first; float w = kv.second;
                float hebb = cfg_.lr_hebb * pre_trace_[i] * (io_.fired(j) ? 1.0f : 0.0f);
                float anti = cfg_.lr_anti * pre_trace_[i] * (io_.fired(j) ? 0.0f : 1.0f);
                float symmetric = 0.5f * cfg_.lr_hebb * post_trace_[j] * (io_.fired(i) ? 1.0f : 0.0f);
                float delta = hebb - anti + symmetric;
                delta -= cfg_.weight_decay * w;
                w = w * scale + delta;
                w = std::clamp(w, cfg_.w_min, cfg_.w_max);
                io_.set_weight(i, j, w);
            }
        }
    }
    void apply_structural_plasticity() {
        const size_t N = io_.num_neurons();
        std::uniform_real_distribution<float> uni(0.0f,1.0f);
        std::uniform_int_distribution<int> pick(0,(int)N-1);
        int added_total = 0;
        for (size_t i=0;i<N;++i) {
            const auto& edges = io_.out_edges(i);
            std::vector<int> to_prune; to_prune.reserve(edges.size());
            for (auto& kv : edges) if (kv.second <= cfg_.prune_threshold) to_prune.push_back(kv.first);
            for (int j: to_prune) io_.remove_edge(i,j);
            if (added_total >= cfg_.max_added_per_step) continue;
            if (uni(rng_) < cfg_.rewire_prob) {
                int tries = 8, dst=-1;
                while (tries-- > 0) {
                    int cand = pick(rng_); if (cand==(int)i) continue;
                    if (io_.out_edges(i).count(cand)) continue;
                    if (io_.fired(cand) || post_trace_[cand] > 0.2f) { dst=cand; break; }
                    if (dst<0) dst=cand;
                }
                if (dst>=0) { float init_w = 0.05f + 0.05f*uni(rng_); io_.add_edge(i,dst,init_w); ++added_total; }
            }
        }
    }
};

struct PulsePattern { std::vector<std::vector<int>> pulses_per_step; };
class PatternFeeder {
public:
    PatternFeeder(std::vector<int> ids, PulsePattern p, std::function<void(int,float)> inject)
        : input_ids_(std::move(ids)), pattern_(std::move(p)), inject_(std::move(inject)) {}
    void feed_step(size_t step) {
        if (pattern_.pulses_per_step.empty()) return;
        size_t idx = step % pattern_.pulses_per_step.size();
        for (int nid : pattern_.pulses_per_step[idx]) inject_(nid, 1.0f);
    }
private:
    std::vector<int> input_ids_; PulsePattern pattern_; std::function<void(int,float)> inject_;
};

} // namespace glia_training