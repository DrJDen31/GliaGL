#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <random>
#include <cmath>
#include <algorithm>

#include "../hebbian/trainer.h" // for EpisodeMetrics, EpisodeData, and TrainingConfig via include chain
#include "../../arch/glia.h"
#include "../../arch/neuron.h"
#include "../../arch/output_detection.h"
#include "../../arch/input_sequence.h"

class RateGDTrainer {
public:
    explicit RateGDTrainer(Glia &net) : glia(net), rng(123456u) {}
    void reseed(unsigned int s) { rng.seed(s); }
    std::vector<double> getEpochAccHistory() const { return epoch_acc_hist; }
    std::vector<double> getEpochMarginHistory() const { return epoch_margin_hist; }

    EpisodeMetrics evaluate(InputSequence &seq, const TrainingConfig &cfg) {
        std::vector<std::string> output_ids = collectOutputIDs();
        neuron_rate.clear();
        seq.reset();
        const int U = cfg.warmup_ticks;
        const int W = cfg.decision_window;
        for (int t = 0; t < U + W; ++t) {
            injectFromSequence(seq);
            glia.step();
            glia.forEachNeuron([&](Neuron &n){ bool f = n.didFire(); std::string nid = n.getId(); float r = 0.0f; auto itnr = neuron_rate.find(nid); if (itnr != neuron_rate.end()) r = itnr->second; float newr = (1.0f - cfg.rate_alpha) * r + cfg.rate_alpha * (f ? 1.0f : 0.0f); if (itnr != neuron_rate.end()) itnr->second = newr; else neuron_rate.emplace(nid, newr); });
            seq.advance();
        }
        EpisodeMetrics m;
        float top1 = -1e9f, top2 = -1e9f; std::string win;
        for (const auto &id : output_ids) {
            float r = 0.0f; auto it = neuron_rate.find(id); if (it != neuron_rate.end()) r = it->second;
            m.rates[id] = r;
            if (r > top1) { top2 = top1; top1 = r; win = id; }
            else if (r > top2) { top2 = r; }
        }
        m.winner_id = win;
        m.margin = (top1 > -1e8f && top2 > -1e8f) ? (top1 - top2) : 0.0f;
        m.ticks_run = U + W; return m;
    }

    void trainBatch(const std::vector<Trainer::EpisodeData> &batch,
                    const TrainingConfig &cfg,
                    std::vector<EpisodeMetrics> *batch_metrics_out = nullptr) {
        std::unordered_map<std::string, float> sum_grad;
        if (batch_metrics_out) batch_metrics_out->clear();
        for (const auto &item : batch) {
            InputSequence seq = item.seq;
            EpisodeMetrics m;
            auto g = computeEpisodeGrad(seq, cfg, item.target_id, &m);
            for (const auto &kv : g) sum_grad[kv.first] += kv.second;
            if (batch_metrics_out) batch_metrics_out->push_back(m);
        }
        float scale = batch.empty() ? 1.0f : (1.0f / static_cast<float>(batch.size()));
        applyGradients(sum_grad, scale, cfg);
        postBatchPlasticity(cfg);
    }

    void trainEpoch(std::vector<Trainer::EpisodeData> dataset, int epochs, const TrainingConfig &cfg) {
        if (dataset.empty() || epochs <= 0) return;
        if (cfg.weight_jitter_std > 0.0f) {
            std::normal_distribution<float> nd(0.0f, cfg.weight_jitter_std);
            glia.forEachNeuron([&](Neuron &from){
                const auto &conns = from.getConnections();
                for (const auto &kv : conns) { float w = kv.second.first; w += nd(rng); from.setTransmitter(kv.first, w); }
            });
        }
        for (int e = 0; e < epochs; ++e) {
            if (cfg.shuffle) std::shuffle(dataset.begin(), dataset.end(), rng);
            size_t epoch_total = 0, epoch_correct = 0; double epoch_margin_sum = 0.0;
            for (size_t i = 0; i < dataset.size(); i += std::max(1, cfg.batch_size)) {
                size_t j = std::min(dataset.size(), i + static_cast<size_t>(std::max(1, cfg.batch_size)));
                std::vector<Trainer::EpisodeData> batch(dataset.begin() + i, dataset.begin() + j);
                std::vector<EpisodeMetrics> bm; trainBatch(batch, cfg, &bm);
                if (cfg.verbose && cfg.log_every > 0 && ((e + 1) % cfg.log_every == 0)) {
                    int correct = 0; double avg_margin = 0.0;
                    for (size_t k = 0; k < bm.size(); ++k) { avg_margin += bm[k].margin; if (k < batch.size() && bm[k].winner_id == batch[k].target_id) correct++; }
                    if (!bm.empty()) avg_margin /= static_cast<double>(bm.size());
                    std::cout << "Epoch " << (e + 1) << "/" << epochs
                              << "  Batch " << (i / std::max(1, cfg.batch_size) + 1) << "/"
                              << ((dataset.size() + std::max(1, cfg.batch_size) - 1) / std::max(1, cfg.batch_size))
                              << "  Acc=" << (bm.empty() ? 0.0 : (static_cast<double>(correct) / bm.size()))
                              << "  AvgMargin=" << avg_margin << std::endl;
                }
                for (size_t k = 0; k < bm.size() && k < batch.size(); ++k) {
                    epoch_total += 1; if (bm[k].winner_id == batch[k].target_id) epoch_correct += 1; epoch_margin_sum += static_cast<double>(bm[k].margin);
                }
            }
            double epoch_acc = (epoch_total == 0) ? 0.0 : (static_cast<double>(epoch_correct) / static_cast<double>(epoch_total));
            double epoch_margin = (epoch_total == 0) ? 0.0 : (epoch_margin_sum / static_cast<double>(epoch_total));
            epoch_acc_hist.push_back(epoch_acc); epoch_margin_hist.push_back(epoch_margin);
        }
    }

private:
    Glia &glia;
    std::unordered_map<std::string, float> neuron_rate;
    std::mt19937 rng;
    std::vector<double> epoch_acc_hist;
    std::vector<double> epoch_margin_hist;
    // Adam optimizer state
    std::unordered_map<std::string, float> adam_m;
    std::unordered_map<std::string, float> adam_v;
    int adam_step = 0;

    static inline std::string edge_key(const std::string &a, const std::string &b) { return a + "|" + b; }

    std::vector<std::string> collectOutputIDs() {
        std::vector<std::string> ids; glia.forEachNeuron([&](Neuron &n){ const std::string &id = n.getId(); if (!id.empty() && id[0] == 'O') ids.push_back(id); }); return ids;
    }
    std::vector<std::string> collectAllIDs() {
        std::vector<std::string> ids; glia.forEachNeuron([&](Neuron &n){ ids.push_back(n.getId()); }); return ids;
    }
    void injectFromSequence(InputSequence &seq) {
        auto inputs = seq.getCurrentInputs(); for (const auto &kv : inputs) glia.injectSensory(kv.first, kv.second);
    }
    void updateDetector(IOutputDetector &detector, const std::vector<std::string> &output_ids) {
        for (const auto &id : output_ids) { auto n = glia.getNeuronById(id); if (n) detector.update(id, n->didFire()); }
    }

    std::unordered_map<std::string, float> computeEpisodeGrad(InputSequence &seq,
                                                              const TrainingConfig &cfg,
                                                              const std::string &target_id,
                                                              EpisodeMetrics *out) {
        std::vector<std::string> output_ids = collectOutputIDs();
        neuron_rate.clear(); seq.reset();
        std::unordered_map<std::string, float> elig;
        const int U = cfg.warmup_ticks; const int W = cfg.decision_window;
        for (int t = 0; t < U + W; ++t) {
            injectFromSequence(seq); glia.step();
            std::unordered_map<std::string, bool> fired;
            glia.forEachNeuron([&](Neuron &n){ bool f = n.didFire(); std::string nid = n.getId(); fired.emplace(nid, f); float r = 0.0f; auto itnr = neuron_rate.find(nid); if (itnr != neuron_rate.end()) r = itnr->second; float newr = (1.0f - cfg.rate_alpha) * r + cfg.rate_alpha * (f ? 1.0f : 0.0f); if (itnr != neuron_rate.end()) itnr->second = newr; else neuron_rate.emplace(nid, newr); });
            glia.forEachNeuron([&](Neuron &from){ std::string from_id = from.getId(); const auto &conns = from.getConnections(); for (const auto &kv : conns) { std::string to_id = kv.first; std::string k = edge_key(from_id, to_id); float e_prev = 0.0f; auto itE = elig.find(k); if (itE != elig.end()) e_prev = itE->second; float pre = 0.0f; auto itPre = neuron_rate.find(from_id); if (itPre != neuron_rate.end()) pre = itPre->second; float e_new = cfg.elig_lambda * e_prev + pre; if (itE != elig.end()) itE->second = e_new; else elig.emplace(std::move(k), e_new); }});
            seq.advance();
        }
        EpisodeMetrics m;
        float top1 = -1e9f, top2 = -1e9f; std::string win;
        for (const auto &id : output_ids) {
            float r = 0.0f; auto it = neuron_rate.find(id); if (it != neuron_rate.end()) r = it->second; m.rates[id] = r; if (r > top1) { top2 = top1; top1 = r; win = id; } else if (r > top2) { top2 = r; }
        }
        m.winner_id = win; m.margin = (top1 > -1e8f && top2 > -1e8f) ? (top1 - top2) : 0.0f;
        m.ticks_run = U + W;
        if (out) *out = m;
        std::unordered_map<std::string, float> grad;
        if (!output_ids.empty()) {
            std::vector<float> logits; logits.reserve(output_ids.size());
            float T = (cfg.grad.temperature > 0.0f ? cfg.grad.temperature : 1.0f);
            for (const auto &id : output_ids) {
                float r = 0.0f; auto itnr = neuron_rate.find(id); if (itnr != neuron_rate.end()) r = itnr->second;
                logits.push_back(r / T);
            }
            float max_logit = *std::max_element(logits.begin(), logits.end());
            std::vector<float> exps(logits.size()); float sum_exp = 0.0f;
            for (size_t i = 0; i < logits.size(); ++i) { exps[i] = std::exp(logits[i] - max_logit); sum_exp += exps[i]; }
            std::vector<float> p(logits.size()); for (size_t i = 0; i < logits.size(); ++i) p[i] = exps[i] / (sum_exp > 0.0f ? sum_exp : 1.0f);
            std::unordered_map<std::string, float> g_rate;
            for (size_t i = 0; i < output_ids.size(); ++i) { g_rate[output_ids[i]] = p[i]; }
            for (size_t i = 0; i < output_ids.size(); ++i) { if (output_ids[i] == target_id) { g_rate[output_ids[i]] -= 1.0f; break; } }
            for (auto &kv : g_rate) kv.second *= (1.0f / T);

            std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> outgoing;
            std::unordered_map<std::string, std::vector<std::string>> inbound;
            glia.forEachNeuron([&](Neuron &from){ std::string from_id = from.getId(); const auto &conns = from.getConnections(); for (const auto &kv : conns) { std::string to_id = kv.first; float w = kv.second.first; auto it_out = outgoing.find(from_id); if (it_out == outgoing.end()) it_out = outgoing.emplace(from_id, std::vector<std::pair<std::string,float>>{}).first; it_out->second.emplace_back(to_id, w); auto it_in = inbound.find(to_id); if (it_in == inbound.end()) it_in = inbound.emplace(to_id, std::vector<std::string>{}).first; it_in->second.emplace_back(from_id); }});

            std::unordered_map<std::string, float> phi_prime;
            glia.forEachNeuron([&](Neuron &n){ std::string nid = n.getId(); float rloc = 0.0f; auto itn = neuron_rate.find(nid); if (itn != neuron_rate.end()) rloc = itn->second; if (rloc < 0.0f) rloc = 0.0f; if (rloc > 1.0f) rloc = 1.0f; float eps = 0.05f; if (rloc < eps) rloc = eps; if (rloc > 1.0f - eps) rloc = 1.0f - eps; phi_prime.emplace(nid, rloc * (1.0f - rloc)); });

            std::map<std::string, int> dist;
            std::vector<std::string> q; q.reserve(outgoing.size()); size_t qi = 0;
            for (const auto &id : output_ids) { dist.emplace(id, 0); q.push_back(id); }
            while (qi < q.size()) {
                const std::string &u = q[qi++];
                auto it_in = inbound.find(u);
                if (it_in != inbound.end()) {
                    int du = 0; auto itu = dist.find(u); if (itu != dist.end()) du = itu->second;
                    for (const auto &pred : it_in->second) {
                        if (dist.find(pred) == dist.end()) { dist.emplace(pred, du + 1); q.push_back(pred); }
                    }
                }
            }
            std::vector<std::pair<std::string,int>> order; order.reserve(dist.size());
            for (const auto &kv : dist) { if (kv.second > 0) order.push_back(kv); }
            std::sort(order.begin(), order.end(), [](const std::pair<std::string,int> &a, const std::pair<std::string,int> &b){ return a.second < b.second; });
            for (const auto &pr : order) {
                const std::string &j = pr.first; int dj = pr.second;
                float acc = 0.0f;
                auto it_out = outgoing.find(j);
                if (it_out != outgoing.end()) {
                    for (const auto &edge : it_out->second) {
                        const std::string &k = edge.first; float w = edge.second;
                        auto itdk = dist.find(k); if (itdk == dist.end()) continue;
                        if (itdk->second < dj) {
                            float gk = 0.0f; auto itgk = g_rate.find(k); if (itgk != g_rate.end()) gk = itgk->second;
                            float phip = 0.0f; auto itpp = phi_prime.find(k); if (itpp != phi_prime.end()) phip = itpp->second;
                            acc += w * phip * gk;
                        }
                    }
                }
                auto itgj = g_rate.find(j); if (itgj != g_rate.end()) itgj->second += acc; else g_rate.emplace(j, acc);
            }

            glia.forEachNeuron([&](Neuron &from){ const auto &conns = from.getConnections(); for (const auto &kv : conns) { const std::string &to_id = kv.first; const std::string k = edge_key(from.getId(), to_id); float e = 0.0f; auto itE = elig.find(k); if (itE != elig.end()) e = itE->second; auto itg = g_rate.find(to_id); if (itg != g_rate.end()) { float phip = 0.0f; auto itpp = phi_prime.find(to_id); if (itpp != phi_prime.end()) phip = itpp->second; float delta = itg->second * phip * e; auto itgr = grad.find(k); if (itgr != grad.end()) itgr->second += delta; else grad.emplace(k, delta); } }});
        }
        return grad;
    }

    void applyGradients(const std::unordered_map<std::string, float> &grad,
                        float scale,
                        const TrainingConfig &cfg) {
        // Optional gradient norm clipping (global L2 over provided grad map)
        float clip_scale = 1.0f;
        if (cfg.grad.clip_grad_norm > 0.0f) {
            double sumsq = 0.0; for (const auto &kv : grad) { double g = (double)kv.second * (double)scale; sumsq += g * g; }
            double norm = std::sqrt(std::max(1e-30, sumsq));
            if (norm > (double)cfg.grad.clip_grad_norm) {
                clip_scale = (float)((double)cfg.grad.clip_grad_norm / norm);
            }
        }
        bool use_adam = (cfg.grad.optimizer == "adam");
        bool use_adamw = (cfg.grad.optimizer == "adamw");
        if (use_adam || use_adamw) adam_step = std::max(1, adam_step + 1);
        glia.forEachNeuron([&](Neuron &from){
            const auto &conns = from.getConnections();
            for (const auto &kv : conns) {
                const std::string &to_id = kv.first;
                const std::string k = edge_key(from.getId(), to_id);
                float w = kv.second.first;
                auto itg = grad.find(k);
                float g = (itg != grad.end()) ? (itg->second * scale * clip_scale) : 0.0f;
                if (use_adam || use_adamw) {
                    float b1 = cfg.grad.adam_beta1;
                    float b2 = cfg.grad.adam_beta2;
                    float eps = cfg.grad.adam_eps > 0.0f ? cfg.grad.adam_eps : 1e-8f;
                    float m = 0.0f, v = 0.0f;
                    auto itm = adam_m.find(k); if (itm != adam_m.end()) m = itm->second;
                    auto itv = adam_v.find(k); if (itv != adam_v.end()) v = itv->second;
                    m = b1 * m + (1.0f - b1) * g;
                    v = b2 * v + (1.0f - b2) * (g * g);
                    adam_m[k] = m; adam_v[k] = v;
                    double bias1 = 1.0 - std::pow((double)b1, (double)adam_step);
                    double bias2 = 1.0 - std::pow((double)b2, (double)adam_step);
                    double mhat = (double)m / (bias1 > 1e-20 ? bias1 : 1.0);
                    double vhat = (double)v / (bias2 > 1e-20 ? bias2 : 1.0);
                    // AdamW: decoupled weight decay as separate step scaled by lr
                    if (use_adamw && cfg.weight_decay > 0.0f) {
                        w -= cfg.lr * cfg.weight_decay * w;
                    }
                    // Parameter update
                    w -= cfg.lr * (float)(mhat / (std::sqrt(vhat) + (double)eps));
                } else {
                    w -= cfg.lr * g;
                }
                // Coupled L2 decay for SGD/Adam only (AdamW handled above)
                if (!use_adamw && cfg.weight_decay > 0.0f) {
                    w -= cfg.weight_decay * w;
                }
                if (cfg.weight_clip > 0.0f) {
                    float c = cfg.weight_clip; if (w > c) w = c; else if (w < -c) w = -c;
                }
                from.setTransmitter(to_id, w);
            }
        });
    }

    void postBatchPlasticity(const TrainingConfig &cfg) {
        std::vector<std::pair<std::string,std::string>> to_remove; glia.forEachNeuron([&](Neuron &from){ const auto &conns = from.getConnections(); for (const auto &kv : conns) { const std::string &to_id = kv.first; const std::string k = edge_key(from.getId(), to_id); float w = kv.second.first; if (std::fabs(w) < cfg.prune_epsilon) to_remove.emplace_back(from.getId(), to_id); }});
        for (auto &edge : to_remove) { auto from = glia.getNeuronById(edge.first); if (from) from->removeConnection(edge.second); }
        if (cfg.grow_edges > 0) {
            std::vector<std::string> all_ids = collectAllIDs(); std::uniform_int_distribution<size_t> dist_idx(0, all_ids.size() ? all_ids.size() - 1 : 0); std::uniform_real_distribution<float> dist_sign(-1.0f, 1.0f);
            int grown = 0; int attempts = 0;
            while (grown < cfg.grow_edges && attempts < cfg.grow_edges * 20) {
                attempts++;
                const std::string &from_id = all_ids[dist_idx(rng)];
                const std::string &to_id = all_ids[dist_idx(rng)];
                if (!cfg.topology.edgeAllowed(from_id, to_id)) continue;
                if (from_id == to_id) continue;
                auto from = glia.getNeuronById(from_id);
                auto to = glia.getNeuronById(to_id);
                if (!from || !to) continue;
                const auto &conns = from->getConnections();
                if (conns.find(to_id) != conns.end()) continue;
                float w = cfg.init_weight * (dist_sign(rng) >= 0 ? 1.0f : -1.0f);
                from->addConnection(w, to);
                grown++;
            }
        }
        glia.forEachNeuron([&](Neuron &n){ float r = neuron_rate[n.getId()]; if (cfg.eta_theta != 0.0f) n.setThreshold(n.getThreshold() + cfg.eta_theta * (r - cfg.r_target)); if (cfg.eta_leak != 0.0f) { float new_leak = n.getLeak() + cfg.eta_leak * (cfg.r_target - r); if (new_leak < 0.0f) new_leak = 0.0f; if (new_leak > 1.0f) new_leak = 1.0f; n.setLeak(new_leak); }});
    }
};
