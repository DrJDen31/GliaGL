#pragma once
#include "hebbian/trainer.h"
#if 0
#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>

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
    explicit Trainer(Glia &net) : glia(net), rng(123456u) {}
    void reseed(unsigned int s) { rng.seed(s); }
    bool revertCheckpoint() { return revertOneCheckpoint(); }
    // Training history getters (copies)
    std::vector<double> getEpochAccHistory() const { return epoch_acc_hist; }
    std::vector<double> getEpochMarginHistory() const { return epoch_margin_hist; }

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

    // Dataset item: an input sequence paired with a target output ID (e.g., "O0").
    struct EpisodeData {
        InputSequence seq;
        std::string target_id;
    };

    // Compute per-edge weight deltas for a single episode without mutating the network.
    // Returns a map keyed by "FROM|TO" -> delta_w. Optionally emits EpisodeMetrics via out.
    // Compute per-edge deltas for one episode. Reward policy is controlled by TrainingConfig.
    std::unordered_map<std::string, float> computeEpisodeDelta(InputSequence &seq,
                                                               const TrainingConfig &cfg,
                                                               const std::string &target_id,
                                                               EpisodeMetrics *out,
                                                               std::unordered_map<std::string,float>* usage_out = nullptr) {
        std::vector<std::string> output_ids = collectOutputIDs();
        OutputDetectorOptions opts; opts.threshold = cfg.detector.threshold; opts.default_id = cfg.detector.default_id;
        EMAOutputDetector detector(cfg.detector.alpha, opts);
        detector.reset();
        seq.reset();

        auto key_for = [](const std::string &a, const std::string &b){ return a + "|" + b; };
        std::unordered_map<std::string, float> elig;

        const int U = cfg.warmup_ticks;
        const int W = cfg.decision_window;

        for (int t = 0; t < U + W; ++t) {
            injectFromSequence(seq);
            glia.step();

            std::unordered_map<std::string, bool> fired;
            glia.forEachNeuron([&](Neuron &n){
                bool f = n.didFire();
                fired[n.getId()] = f;
                float r = neuron_rate[n.getId()];
                neuron_rate[n.getId()] = (1.0f - cfg.rate_alpha) * r + cfg.rate_alpha * (f ? 1.0f : 0.0f);
            });

            glia.forEachNeuron([&](Neuron &from){
                const auto &conns = from.getConnections();
                for (const auto &kv : conns) {
                    const std::string &to_id = kv.first;
                    float &e = elig[key_for(from.getId(), to_id)];
                    float pre = fired[from.getId()] ? 1.0f : 0.0f;
                    float post = cfg.elig_post_use_rate ? neuron_rate[to_id] : (fired[to_id] ? 1.0f : 0.0f);
                    e = cfg.elig_lambda * e + pre * post;
                }
            });

            updateDetector(detector, output_ids);
            seq.advance();
        }

        EpisodeMetrics m;
        m.winner_id = detector.predict(output_ids);
        m.margin = detector.getMargin(output_ids);
        for (const auto &id : output_ids) m.rates[id] = detector.getRate(id);
        m.ticks_run = U + W;
        if (out) *out = m;

        // Reward selection and shaping
        float reward_raw = computeReward(m, cfg, target_id);
        float reward = reward_raw;
        if (cfg.use_advantage_baseline) {
            float adv = reward_raw - reward_baseline;
            reward_baseline = (1.0f - cfg.baseline_beta) * reward_baseline + cfg.baseline_beta * reward_raw;
            reward = adv;
        }
        if (cfg.no_update_if_satisfied && m.winner_id == target_id && m.margin >= cfg.margin_delta) {
            reward = 0.0f;
        }

        // Build delta map for existing edges only.
        std::unordered_map<std::string, float> delta;
        glia.forEachNeuron([&](Neuron &from){
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                const std::string &to_id = kv.first;
                bool take = true;
                if (cfg.update_gating == "winner_only") {
                    if (!m.winner_id.empty()) take = (to_id == m.winner_id);
                } else if (cfg.update_gating == "target_only") {
                    take = (to_id == target_id);
                }
                if (!take) continue;
                const std::string k = key_for(from.getId(), to_id);
                float e = elig[k];
                delta[k] += cfg.lr * reward * e;
                if (usage_out) (*usage_out)[k] += e;
            }
        });

        return delta;
    }

    // Apply accumulated per-edge deltas to the current network (scaled by `scale`),
    // then apply weight decay.
    void applyDeltas(const std::unordered_map<std::string, float> &delta,
                     float scale,
                     const TrainingConfig &cfg) {
        auto key_for = [](const std::string &a, const std::string &b){ return a + "|" + b; };
        glia.forEachNeuron([&](Neuron &from){
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                const std::string &to_id = kv.first;
                const std::string k = key_for(from.getId(), to_id);
                float w = kv.second.first;
                auto it = delta.find(k);
                if (it != delta.end()) {
                    w += scale * it->second;
                }
                w -= cfg.weight_decay * w;
                if (cfg.weight_clip > 0.0f) {
                    float c = cfg.weight_clip;
                    if (w > c) w = c; else if (w < -c) w = -c;
                }
                from.setTransmitter(to_id, w);
            }
        });
    }

    // Train over a batch: accumulate per-episode deltas and apply once.
    void trainBatch(const std::vector<EpisodeData> &batch,
                    const TrainingConfig &cfg,
                    std::vector<EpisodeMetrics> *batch_metrics_out = nullptr) {
        std::unordered_map<std::string, float> sum_delta;
        std::unordered_map<std::string, float> sum_usage;
        if (batch_metrics_out) batch_metrics_out->clear();
        double sum_reward = 0.0;
        for (const auto &item : batch) {
            InputSequence seq = item.seq;
            EpisodeMetrics m;
            auto d = computeEpisodeDelta(seq, cfg, item.target_id, &m, &sum_usage);
            for (const auto &kv : d) sum_delta[kv.first] += kv.second;
            if (batch_metrics_out) batch_metrics_out->push_back(m);
            sum_reward += static_cast<double>(computeReward(m, cfg, item.target_id));
        }
        float scale = batch.empty() ? 1.0f : (1.0f / static_cast<float>(batch.size()));
        applyDeltas(sum_delta, scale, cfg);

        if (cfg.usage_boost_gain != 0.0f && !batch.empty()) {
            float avg_reward = static_cast<float>(sum_reward / static_cast<double>(batch.size()));
            glia.forEachNeuron([&](Neuron &from){
                const auto &conns = from.getConnections();
                for (const auto &kv : conns) {
                    const std::string k = from.getId() + std::string("|") + kv.first;
                    float usage = 0.0f;
                    auto it = sum_usage.find(k);
                    if (it != sum_usage.end()) usage = it->second / static_cast<float>(batch.size());
                    if (usage < 0.0f) usage = 0.0f;
                    if (usage > 1.0f) usage = 1.0f;
                    float w = kv.second.first;
                    w += cfg.usage_boost_gain * avg_reward * usage;
                    from.setTransmitter(kv.first, w);
                }
            });
        }

        // Prune/grow after batch; update prune counters and perform structural ops.
        std::vector<std::pair<std::string,std::string>> to_remove;
        auto key_for = [](const std::string &a, const std::string &b){ return a + "|" + b; };
        glia.forEachNeuron([&](Neuron &from){
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                const std::string &to_id = kv.first;
                const std::string k = key_for(from.getId(), to_id);
                float w = kv.second.first;
                if (std::fabs(w) < cfg.prune_epsilon) {
                    int c = prune_counter[k];
                    prune_counter[k] = c + 1;
                    if (prune_counter[k] >= cfg.prune_patience) to_remove.emplace_back(from.getId(), to_id);
                } else {
                    prune_counter[k] = 0;
                }
            }
        });
        for (auto &edge : to_remove) {
            Neuron *from = glia.getNeuronById(edge.first);
            if (from) from->removeConnection(edge.second);
        }

        if (cfg.grow_edges > 0) {
            std::vector<std::string> all_ids = collectAllIDs();
            std::uniform_int_distribution<size_t> dist_idx(0, all_ids.size() ? all_ids.size() - 1 : 0);
            std::uniform_real_distribution<float> dist_sign(-1.0f, 1.0f);
            int grown = 0;
            int attempts = 0;
            while (grown < cfg.grow_edges && attempts < cfg.grow_edges * 20) {
                attempts++;
                const std::string &from_id = all_ids[dist_idx(rng)];
                const std::string &to_id = all_ids[dist_idx(rng)];
                if (!cfg.topology.edgeAllowed(from_id, to_id)) continue;
                if (from_id == to_id) continue;
                Neuron *from = glia.getNeuronById(from_id);
                Neuron *to = glia.getNeuronById(to_id);
                if (!from || !to) continue;
                const auto &conns = from->getConnections();
                if (conns.find(to_id) != conns.end()) continue;
                float w = cfg.init_weight * (dist_sign(rng) >= 0 ? 1.0f : -1.0f);
                from->addConnection(w, *to);
                grown++;
            }
        }

        // Intrinsic plasticity after batch using EMA rates tracked during episodes.
        glia.forEachNeuron([&](Neuron &n){
            float r = neuron_rate[n.getId()];
            if (cfg.eta_theta != 0.0f) n.setThreshold(n.getThreshold() + cfg.eta_theta * (r - cfg.r_target));
            if (cfg.eta_leak != 0.0f) {
                float new_leak = n.getLeak() + cfg.eta_leak * (cfg.r_target - r);
                if (new_leak < 0.0f) new_leak = 0.0f;
                if (new_leak > 1.0f) new_leak = 1.0f;
                n.setLeak(new_leak);
            }
        });

        if (cfg.inactive_rate_threshold > 0.0f && cfg.inactive_rate_patience > 0 && cfg.prune_inactive_max > 0) {
            std::vector<std::pair<std::string,std::string>> to_remove_in;
            std::vector<std::pair<std::string,std::string>> to_remove_out;
            glia.forEachNeuron([&](Neuron &n){
                const std::string id = n.getId();
                float r = neuron_rate[id];
                int &ctr = inactive_counter[id];
                if (r < cfg.inactive_rate_threshold) ctr++; else ctr = 0;
                if (ctr >= cfg.inactive_rate_patience) {
                    if (cfg.prune_inactive_out) {
                        const auto &conns = n.getConnections();
                        std::vector<std::pair<std::string,float>> outs;
                        for (const auto &kv : conns) outs.emplace_back(kv.first, kv.second.first);
                        std::sort(outs.begin(), outs.end(), [](const std::pair<std::string,float> &a, const std::pair<std::string,float> &b){
                            return std::fabs(a.second) < std::fabs(b.second);
                        });
                        int c = 0;
                        for (auto &p : outs) { if (c++ >= cfg.prune_inactive_max) break; to_remove_out.emplace_back(id, p.first); }
                    }
                    if (cfg.prune_inactive_in) {
                        std::vector<std::pair<std::string,float>> ins;
                        glia.forEachNeuron([&](Neuron &from){
                            const auto &conns = from.getConnections();
                            auto it = conns.find(id);
                            if (it != conns.end()) ins.emplace_back(from.getId(), it->second.first);
                        });
                        std::sort(ins.begin(), ins.end(), [](const std::pair<std::string,float> &a, const std::pair<std::string,float> &b){
                            return std::fabs(a.second) < std::fabs(b.second);
                        });
                        int c = 0;
                        for (auto &p : ins) { if (c++ >= cfg.prune_inactive_max) break; to_remove_in.emplace_back(p.first, id); }
                    }
                    ctr = 0; // reset after pruning trigger
                }
            });
            for (auto &edge : to_remove_out) { Neuron *from = glia.getNeuronById(edge.first); if (from) from->removeConnection(edge.second); }
            for (auto &edge : to_remove_in) { Neuron *from = glia.getNeuronById(edge.first); if (from) from->removeConnection(edge.second); }
        }
    }

    // Train for E epochs over the dataset, with optional shuffle and batching per config.
    void trainEpoch(std::vector<EpisodeData> dataset, int epochs, const TrainingConfig &cfg) {
        if (dataset.empty() || epochs <= 0) return;
        if (cfg.weight_jitter_std > 0.0f) {
            std::normal_distribution<float> nd(0.0f, cfg.weight_jitter_std);
            glia.forEachNeuron([&](Neuron &from){
                const auto &conns = from.getConnections();
                for (const auto &kv : conns) {
                    float w = kv.second.first;
                    w += nd(rng);
                    from.setTransmitter(kv.first, w);
                }
            });
        }
        std::uniform_int_distribution<size_t> dist(0, dataset.size() - 1);
        for (int e = 0; e < epochs; ++e) {
            if (cfg.shuffle) {
                std::shuffle(dataset.begin(), dataset.end(), rng);
            }
            size_t epoch_total = 0;
            size_t epoch_correct = 0;
            double epoch_margin_sum = 0.0;
            for (size_t i = 0; i < dataset.size(); i += std::max(1, cfg.batch_size)) {
                size_t j = std::min(dataset.size(), i + static_cast<size_t>(std::max(1, cfg.batch_size)));
                std::vector<EpisodeData> batch(dataset.begin() + i, dataset.begin() + j);
                std::vector<EpisodeMetrics> bm;
                trainBatch(batch, cfg, &bm);
                if (cfg.verbose && cfg.log_every > 0 && ((e + 1) % cfg.log_every == 0)) {
                    int correct = 0;
                    double avg_margin = 0.0;
                    for (size_t k = 0; k < bm.size(); ++k) {
                        avg_margin += bm[k].margin;
                        if (k < batch.size() && bm[k].winner_id == batch[k].target_id) correct++;
                    }
                    if (!bm.empty()) avg_margin /= static_cast<double>(bm.size());
                    std::cout << "Epoch " << (e + 1) << "/" << epochs
                              << "  Batch " << (i / std::max(1, cfg.batch_size) + 1) << "/"
                              << ((dataset.size() + std::max(1, cfg.batch_size) - 1) / std::max(1, cfg.batch_size))
                              << "  Acc=" << (bm.empty() ? 0.0 : (static_cast<double>(correct) / bm.size()))
                              << "  AvgMargin=" << avg_margin
                              << std::endl;
                }
                for (size_t k = 0; k < bm.size() && k < batch.size(); ++k) {
                    epoch_total += 1;
                    if (bm[k].winner_id == batch[k].target_id) epoch_correct += 1;
                    epoch_margin_sum += static_cast<double>(bm[k].margin);
                }
            }
            double epoch_acc = (epoch_total == 0) ? 0.0 : (static_cast<double>(epoch_correct) / static_cast<double>(epoch_total));
            double epoch_margin = (epoch_total == 0) ? 0.0 : (epoch_margin_sum / static_cast<double>(epoch_total));
            epoch_acc_hist.push_back(epoch_acc);
            epoch_margin_hist.push_back(epoch_margin);
            if (cfg.checkpoints_enable) onEpochEndCapture(cfg);
            if (cfg.revert_enable) {
                if (cfg.revert_metric == "accuracy") {
                    int w = std::max(1, cfg.revert_window);
                    if ((int)epoch_acc_hist.size() > w) {
                        double prev = epoch_acc_hist[epoch_acc_hist.size() - 1 - w];
                        double curr = epoch_acc_hist.back();
                        if ((prev - curr) >= static_cast<double>(cfg.revert_drop)) {
                            revertOneCheckpoint();
                        }
                    }
                } else if (cfg.revert_metric == "margin") {
                    int w = std::max(1, cfg.revert_window);
                    if ((int)epoch_margin_hist.size() > w) {
                        double prev = epoch_margin_hist[epoch_margin_hist.size() - 1 - w];
                        double curr = epoch_margin_hist.back();
                        if ((prev - curr) >= static_cast<double>(cfg.revert_drop)) {
                            revertOneCheckpoint();
                        }
                    }
                }
            }
        }
    }

    EpisodeMetrics trainEpisode(InputSequence &seq, const TrainingConfig &cfg, const std::string &target_id) {
        std::vector<std::string> output_ids = collectOutputIDs();
        OutputDetectorOptions opts; opts.threshold = cfg.detector.threshold; opts.default_id = cfg.detector.default_id;
        EMAOutputDetector detector(cfg.detector.alpha, opts);
        detector.reset();
        seq.reset();

        std::unordered_map<std::string, float> elig;
        auto key_for = [](const std::string &a, const std::string &b){ return a + "|" + b; };

        const int U = cfg.warmup_ticks;
        const int W = cfg.decision_window;

        for (int t = 0; t < U + W; ++t) {
            injectFromSequence(seq);
            glia.step();

            std::unordered_map<std::string, bool> fired;
            glia.forEachNeuron([&](Neuron &n){
                bool f = n.didFire();
                fired[n.getId()] = f;
                float r = neuron_rate[n.getId()];
                neuron_rate[n.getId()] = (1.0f - cfg.rate_alpha) * r + cfg.rate_alpha * (f ? 1.0f : 0.0f);
            });

            glia.forEachNeuron([&](Neuron &from){
                const auto &conns = from.getConnections();
                for (const auto &kv : conns) {
                    const std::string &to_id = kv.first;
                    float &e = elig[key_for(from.getId(), to_id)];
                    float pre = fired[from.getId()] ? 1.0f : 0.0f;
                    float post = cfg.elig_post_use_rate ? neuron_rate[to_id] : (fired[to_id] ? 1.0f : 0.0f);
                    e = cfg.elig_lambda * e + pre * post;
                }
            });

            updateDetector(detector, output_ids);
            seq.advance();
        }

        EpisodeMetrics m;
        m.winner_id = detector.predict(output_ids);
        m.margin = detector.getMargin(output_ids);
        for (const auto &id : output_ids) m.rates[id] = detector.getRate(id);
        m.ticks_run = U + W;

        float reward_raw = computeReward(m, cfg, target_id);
        float reward = reward_raw;
        if (cfg.use_advantage_baseline) {
            float adv = reward_raw - reward_baseline;
            reward_baseline = (1.0f - cfg.baseline_beta) * reward_baseline + cfg.baseline_beta * reward_raw;
            reward = adv;
        }
        if (cfg.no_update_if_satisfied && m.winner_id == target_id && m.margin >= cfg.margin_delta) {
            reward = 0.0f;
        }

        std::vector<std::pair<std::string,std::string>> to_remove;
        glia.forEachNeuron([&](Neuron &from){
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                const std::string &to_id = kv.first;
                bool take = true;
                if (cfg.update_gating == "winner_only") {
                    if (!m.winner_id.empty()) take = (to_id == m.winner_id);
                } else if (cfg.update_gating == "target_only") {
                    take = (to_id == target_id);
                }
                if (!take) continue;
                std::string k = key_for(from.getId(), to_id);
                float w = kv.second.first;
                float e = elig[k];
                w += cfg.lr * reward * e;
                w -= cfg.weight_decay * w;
                if (cfg.weight_clip > 0.0f) {
                    float c = cfg.weight_clip;
                    if (w > c) w = c; else if (w < -c) w = -c;
                }
                from.setTransmitter(to_id, w);
                if (std::fabs(w) < cfg.prune_epsilon) {
                    int c = prune_counter[k];
                    prune_counter[k] = c + 1;
                    if (prune_counter[k] >= cfg.prune_patience) to_remove.emplace_back(from.getId(), to_id);
                } else {
                    prune_counter[k] = 0;
                }
            }
        });

        for (auto &edge : to_remove) {
            Neuron *from = glia.getNeuronById(edge.first);
            if (from) from->removeConnection(edge.second);
        }

        if (cfg.grow_edges > 0) {
            std::vector<std::string> all_ids = collectAllIDs();
            std::uniform_int_distribution<size_t> dist_idx(0, all_ids.size() ? all_ids.size() - 1 : 0);
            std::uniform_real_distribution<float> dist_sign(-1.0f, 1.0f);
            int grown = 0;
            int attempts = 0;
            while (grown < cfg.grow_edges && attempts < cfg.grow_edges * 20) {
                attempts++;
                const std::string &from_id = all_ids[dist_idx(rng)];
                const std::string &to_id = all_ids[dist_idx(rng)];
                if (!cfg.topology.edgeAllowed(from_id, to_id)) continue;
                if (from_id == to_id) continue;
                Neuron *from = glia.getNeuronById(from_id);
                Neuron *to = glia.getNeuronById(to_id);
                if (!from || !to) continue;
                const auto &conns = from->getConnections();
                if (conns.find(to_id) != conns.end()) continue;
                float w = cfg.init_weight * (dist_sign(rng) >= 0 ? 1.0f : -1.0f);
                from->addConnection(w, *to);
                grown++;
            }
        }

        glia.forEachNeuron([&](Neuron &n){
            float r = neuron_rate[n.getId()];
            if (cfg.eta_theta != 0.0f) n.setThreshold(n.getThreshold() + cfg.eta_theta * (r - cfg.r_target));
            if (cfg.eta_leak != 0.0f) {
                float new_leak = n.getLeak() + cfg.eta_leak * (cfg.r_target - r);
                if (new_leak < 0.0f) new_leak = 0.0f;
                if (new_leak > 1.0f) new_leak = 1.0f;
                n.setLeak(new_leak);
            }
        });

        return m;
    }

private:
    Glia &glia;
    std::unordered_map<std::string, float> neuron_rate;
    std::map<std::string, int> prune_counter;
    std::unordered_map<std::string, int> inactive_counter;
    std::mt19937 rng;
    std::vector<double> epoch_acc_hist;
    std::vector<double> epoch_margin_hist;
    float reward_baseline = 0.0f;

    struct EdgeRec { std::string from; std::string to; float w; };
    struct NeuronRec { std::string id; float thr; float leak; };
    struct Snapshot { std::vector<NeuronRec> neurons; std::vector<EdgeRec> edges; };
    std::vector<Snapshot> ckpt_l0; // most recent level
    std::vector<Snapshot> ckpt_l1; // mid level
    std::vector<Snapshot> ckpt_l2; // oldest level

    // Compute target-specific margin: rate[target] - max(rate[others])
    static inline float targetMargin(const std::map<std::string,float> &rates, const std::string &target_id) {
        auto itT = rates.find(target_id);
        float rT = (itT != rates.end()) ? itT->second : 0.0f;
        float rMaxOther = 0.0f;
        bool init = false;
        for (const auto &kv : rates) {
            if (kv.first == target_id) continue;
            if (!init) { rMaxOther = kv.second; init = true; }
            else if (kv.second > rMaxOther) rMaxOther = kv.second;
        }
        return rT - rMaxOther;
    }

    // Choose reward per config. Modes:
    //  - "binary": +reward_pos if (winner==target && margin>=delta) else reward_neg
    //  - "margin_linear": clamp(gain * target_margin, [reward_min, reward_max])
    //  - "softplus_margin": sigma(gain * (delta - target_margin)) in [0,1]
    static inline float computeReward(const EpisodeMetrics &m, const TrainingConfig &cfg, const std::string &target_id) {
        if (cfg.reward_mode == "margin_linear") {
            float tm = targetMargin(m.rates, target_id);
            float r = cfg.reward_gain * tm;
            if (r < cfg.reward_min) r = cfg.reward_min;
            if (r > cfg.reward_max) r = cfg.reward_max;
            return r;
        }
        if (cfg.reward_mode == "softplus_margin") {
            float tm = targetMargin(m.rates, target_id);
            float x = cfg.reward_gain * (cfg.margin_delta - tm);
            float r = 1.0f / (1.0f + std::exp(-x));
            if (cfg.reward_min < cfg.reward_max) {
                if (r < cfg.reward_min) r = cfg.reward_min;
                if (r > cfg.reward_max) r = cfg.reward_max;
            }
            return r;
        }
        if (m.winner_id == target_id && m.margin >= cfg.margin_delta) return cfg.reward_pos;
        return cfg.reward_neg;
    }

    std::vector<std::string> collectOutputIDs() {
        std::vector<std::string> ids;
        glia.forEachNeuron([&](Neuron &n){
            const std::string &id = n.getId();
            if (!id.empty() && id[0] == 'O') ids.push_back(id);
        });
        return ids;
    }

    std::vector<std::string> collectAllIDs() {
        std::vector<std::string> ids;
        glia.forEachNeuron([&](Neuron &n){ ids.push_back(n.getId()); });
        return ids;
    }

    Snapshot captureSnapshot() {
        Snapshot s;
        glia.forEachNeuron([&](Neuron &n){
            NeuronRec r{n.getId(), n.getThreshold(), n.getLeak()};
            s.neurons.push_back(r);
        });
        glia.forEachNeuron([&](Neuron &from){
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                s.edges.push_back(EdgeRec{from.getId(), kv.first, kv.second.first});
            }
        });
        return s;
    }

    void restoreSnapshot(const Snapshot &s) {
        // Remove edges not in snapshot
        std::unordered_map<std::string, std::unordered_map<std::string,float>> edge_set;
        for (const auto &e : s.edges) edge_set[e.from][e.to] = e.w;
        glia.forEachNeuron([&](Neuron &from){
            std::vector<std::string> to_remove;
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                auto itf = edge_set.find(from.getId());
                if (itf == edge_set.end() || itf->second.find(kv.first) == itf->second.end()) to_remove.push_back(kv.first);
            }
            for (const auto &tid : to_remove) from.removeConnection(tid);
        });
        // Restore weights and add missing edges
        for (const auto &e : s.edges) {
            Neuron *from = glia.getNeuronById(e.from);
            Neuron *to = glia.getNeuronById(e.to);
            if (!from || !to) continue;
            const auto &conns = from->getConnections();
            if (conns.find(e.to) == conns.end()) from->addConnection(e.w, *to);
            else from->setTransmitter(e.to, e.w);
        }
        // Restore neuron params
        for (const auto &r : s.neurons) {
            Neuron *n = glia.getNeuronById(r.id);
            if (!n) continue;
            n->setThreshold(r.thr);
            n->setLeak(r.leak);
        }
    }

    void onEpochEndCapture(const TrainingConfig &cfg) {
        Snapshot s = captureSnapshot();
        ckpt_l0.push_back(std::move(s));
        if ((int)ckpt_l0.size() > cfg.ckpt_l0) {
            // promote oldest to l1
            Snapshot m = std::move(ckpt_l0.front());
            ckpt_l0.erase(ckpt_l0.begin());
            ckpt_l1.push_back(std::move(m));
        }
        if ((int)ckpt_l1.size() > cfg.ckpt_l1) {
            Snapshot m = std::move(ckpt_l1.front());
            ckpt_l1.erase(ckpt_l1.begin());
            ckpt_l2.push_back(std::move(m));
        }
        if ((int)ckpt_l2.size() > cfg.ckpt_l2) {
            ckpt_l2.erase(ckpt_l2.begin());
        }
    }

    bool revertOneFrom(std::vector<Snapshot> &level) {
        if (level.empty()) return false;
        Snapshot s = std::move(level.back());
        level.pop_back();
        restoreSnapshot(s);
        return true;
    }

    bool revertOneCheckpoint() {
        if (revertOneFrom(ckpt_l0)) return true;
        if (revertOneFrom(ckpt_l1)) return true;
        if (revertOneFrom(ckpt_l2)) return true;
        return false;
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

#endif
