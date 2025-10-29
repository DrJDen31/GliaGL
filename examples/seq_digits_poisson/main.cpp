// Digits (.seq) trainer and evaluator
// Loads train/test from labels.csv, trains with gradient (default) or Hebbian, prints progress, saves metrics and net

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <cstdio>
#include <cmath>
#include <set>

#include "../../src/arch/glia.h"
#include "../../src/arch/neuron.h"
#include "../../src/arch/input_sequence.h"
#include "../../src/train/trainer.h"              // wrapper -> hebbian/trainer.h
#include "../../src/train/training_config.h"      // wrapper -> hebbian/training_config.h
#include "../../src/train/gradient/rate_gd_trainer.h"

struct Args {
    std::string net_path;
    std::string data_root; // expects data_root/train/{labels.csv,*.seq}, data_root/test/{labels.csv,*.seq}
    int epochs = 10;
    int batch = 16;
    int seed = 123456;
    bool hebbian = false;     // default to gradient
    // Episode/detector
    int warmup = 20;
    int window = 80;
    float alpha = 0.05f;
    float threshold = 0.01f;
    std::string default_id = "O0";
    // Optim
    float lr = 0.01f;
    float lambda_ = 0.95f;
    float weight_decay = 1e-4f;
    float gd_temperature = 1.0f;
    // Output paths
    std::string save_net = "digits_trained.net";
    std::string train_metrics_json = "digits_train_metrics.json";
    std::string train_metrics_csv;
    std::string train_plot_html;
    std::string predictions_csv_test;
};

static bool parse_args(int argc, char** argv, Args &a) {
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&](std::string &out){ if (i+1>=argc) return false; out = argv[++i]; return true; };
        if (k == "--net") { if (!next(a.net_path)) return false; }
        else if (k == "--root") { if (!next(a.data_root)) return false; }
        else if (k == "--epochs") { std::string v; if (!next(v)) return false; a.epochs = std::atoi(v.c_str()); }
        else if (k == "--batch") { std::string v; if (!next(v)) return false; a.batch = std::atoi(v.c_str()); }
        else if (k == "--seed") { std::string v; if (!next(v)) return false; a.seed = std::atoi(v.c_str()); }
        else if (k == "--hebbian") { a.hebbian = true; }
        else if (k == "--warmup") { std::string v; if (!next(v)) return false; a.warmup = std::atoi(v.c_str()); }
        else if (k == "--window") { std::string v; if (!next(v)) return false; a.window = std::atoi(v.c_str()); }
        else if (k == "--alpha") { std::string v; if (!next(v)) return false; a.alpha = std::atof(v.c_str()); }
        else if (k == "--threshold") { std::string v; if (!next(v)) return false; a.threshold = std::atof(v.c_str()); }
        else if (k == "--default") { if (!next(a.default_id)) return false; }
        else if (k == "--lr") { std::string v; if (!next(v)) return false; a.lr = std::atof(v.c_str()); }
        else if (k == "--lambda") { std::string v; if (!next(v)) return false; a.lambda_ = std::atof(v.c_str()); }
        else if (k == "--weight_decay") { std::string v; if (!next(v)) return false; a.weight_decay = std::atof(v.c_str()); }
        else if (k == "--gd_temperature") { std::string v; if (!next(v)) return false; a.gd_temperature = std::atof(v.c_str()); }
        else if (k == "--save_net") { if (!next(a.save_net)) return false; }
        else if (k == "--train_metrics_json") { if (!next(a.train_metrics_json)) return false; }
        else if (k == "--train_metrics_csv") { if (!next(a.train_metrics_csv)) return false; }
        else if (k == "--train_plot_html") { if (!next(a.train_plot_html)) return false; }
        else if (k == "--predictions_csv_test") { if (!next(a.predictions_csv_test)) return false; }
        else { std::cerr << "Unknown arg: " << k << "\n"; return false; }
    }
    // Only data_root is required; net is optional (a default net will be created if omitted)
    return !a.data_root.empty();
}

static inline std::string join_path(const std::string &a, const std::string &b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    char sep = '/';
    if (a.back() == '/' || a.back() == '\\') return a + b;
    return a + sep + b;
}

static bool load_labels_csv(const std::string &dir, std::vector<Trainer::EpisodeData> &out) {
    std::string path = join_path(dir, "labels.csv");
    std::ifstream f(path.c_str());
    if (!f.is_open()) { std::cerr << "Could not open labels: " << path << "\n"; return false; }
    std::string line; bool header = true;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (header) { header = false; continue; }
        // expect CSV: filename,label
        std::istringstream iss(line);
        std::string fname, label_str;
        if (!std::getline(iss, fname, ',')) continue;
        if (!std::getline(iss, label_str)) continue;
        // Trim quotes/spaces
        auto trim = [](std::string &s){
            while (!s.empty() && (s.front()=='"' || s.front()==' ')) s.erase(s.begin());
            while (!s.empty() && (s.back()=='"' || s.back()==' ' || s.back()=='\r')) s.pop_back();
        };
        trim(fname); trim(label_str);
        int label = std::atoi(label_str.c_str());
        std::string seq_path = join_path(dir, fname);
        InputSequence seq;
        if (!seq.loadFromFile(seq_path)) {
            std::cerr << "Failed to load seq: " << seq_path << "\n"; continue;
        }
        Trainer::EpisodeData ex; ex.seq = seq; ex.target_id = std::string("O") + std::to_string(label);
        out.push_back(ex);
    }
    return true;
}

static bool read_labels_list(const std::string &dir, std::vector<std::pair<std::string,int>> &out) {
    std::string path = join_path(dir, "labels.csv");
    std::ifstream f(path.c_str());
    if (!f.is_open()) return false;
    std::string line; bool header = true;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (header) { header = false; continue; }
        std::istringstream iss(line);
        std::string fname, label_str;
        if (!std::getline(iss, fname, ',')) continue;
        if (!std::getline(iss, label_str)) continue;
        auto trim = [](std::string &s){ while (!s.empty() && (s.front()=='"' || s.front()==' ')) s.erase(s.begin()); while (!s.empty() && (s.back()=='"' || s.back()==' ' || s.back()=='\r')) s.pop_back(); };
        trim(fname); trim(label_str);
        int label = std::atoi(label_str.c_str());
        out.emplace_back(fname, label);
    }
    return true;
}

static void print_progress_bar(size_t done, size_t total, double acc, double loss) {
    const int width = 30;
    double frac = (total==0) ? 1.0 : static_cast<double>(done)/static_cast<double>(total);
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;
    int filled = static_cast<int>(frac * width + 0.5);
    std::ostringstream bar;
    bar << "[";
    for (int i=0;i<width;i++) bar << (i<filled ? '#' : '-');
    bar << "] ";
    int pct = static_cast<int>(frac*100.0 + 0.5);
    std::cout << "\r" << bar.str() << pct << "%  Acc=" << acc << "  Loss=" << loss << std::flush;
}

static void write_train_metrics_json(const std::string &path,
                                     const std::vector<double> &loss,
                                     const std::vector<double> &acc,
                                     const std::vector<double> &margin)
{
    std::ofstream jf(path.c_str(), std::ios::out | std::ios::trunc);
    if (!jf.is_open()) return;
    int epochs = (int)std::max(loss.size(), std::max(acc.size(), margin.size()));
    jf << "{\n";
    jf << "  \"epochs\": " << epochs << ",\n";
    jf << "  \"loss\": ["; for (size_t i=0;i<loss.size();++i){ jf<<loss[i]; if(i+1<loss.size()) jf<<","; } jf << "],\n";
    jf << "  \"accuracy\": ["; for (size_t i=0;i<acc.size();++i){ jf<<acc[i]; if(i+1<acc.size()) jf<<","; } jf << "],\n";
    jf << "  \"margin\": ["; for (size_t i=0;i<margin.size();++i){ jf<<margin[i]; if(i+1<margin.size()) jf<<","; } jf << "]\n";
    jf << "}\n";
}

static void write_train_metrics_csv(const std::string &path,
                                    const std::vector<double> &loss,
                                    const std::vector<double> &acc)
{
    std::ofstream f(path.c_str(), std::ios::out | std::ios::trunc);
    if (!f.is_open()) return;
    f << "epoch,loss,accuracy\n";
    size_t n = std::max(loss.size(), acc.size());
    for (size_t i = 0; i < n; ++i) {
        double l = (i < loss.size()) ? loss[i] : 0.0;
        double a = (i < acc.size()) ? acc[i] : 0.0;
        f << (i + 1) << "," << l << "," << a << "\n";
    }
}

static void write_train_plot_html(const std::string &path,
                                  const std::vector<double> &loss,
                                  const std::vector<double> &acc)
{
    std::ofstream f(path.c_str(), std::ios::out | std::ios::trunc);
    if (!f.is_open()) return;
    f << "<!doctype html><html><head><meta charset=\"utf-8\"><title>Training Metrics</title>";
    f << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>";
    f << "<style>body{font-family:sans-serif;margin:20px;}canvas{max-width:900px;margin:12px auto;display:block}</style>";
    f << "</head><body><h2>Loss</h2><canvas id=\"loss\" height=\"140\"></canvas><h2>Accuracy</h2><canvas id=\"acc\" height=\"140\"></canvas><script>";
    f << "const labels=[";
    for (size_t i = 0; i < loss.size(); ++i) { f << (i + 1); if (i + 1 < loss.size()) f << ","; }
    f << "]\n";
    f << "const lossData=[";
    for (size_t i = 0; i < loss.size(); ++i) { f << loss[i]; if (i + 1 < loss.size()) f << ","; }
    f << "]\n";
    f << "const accData=[";
    for (size_t i = 0; i < acc.size(); ++i) { f << acc[i]; if (i + 1 < acc.size()) f << ","; }
    f << "]\n";
    f << "new Chart(document.getElementById('loss').getContext('2d'),{type:'line',data:{labels:labels,datasets:[{label:'Loss',data:lossData,borderColor:'red',tension:0.25,fill:false}]},options:{responsive:true,plugins:{legend:{display:true}},scales:{x:{title:{display:true,text:'Epoch'}},y:{title:{display:true,text:'Loss'}}}}});";
    f << "new Chart(document.getElementById('acc').getContext('2d'),{type:'line',data:{labels:labels,datasets:[{label:'Accuracy',data:accData,borderColor:'blue',tension:0.25,fill:false}]},options:{responsive:true,plugins:{legend:{display:true}},scales:{x:{title:{display:true,text:'Epoch'}},y:{title:{display:true,text:'Accuracy'},min:0,max:1}}}});";
    f << "</script></body></html>";
}

static bool file_exists(const std::string &path) {
    std::ifstream f(path.c_str());
    return f.is_open();
}

static bool write_default_digits_net(const std::string &path) {
    std::ofstream f(path.c_str(), std::ios::out | std::ios::trunc);
    if (!f.is_open()) return false;
    f << "# Default digits network (8x8 inputs -> 10 outputs)\n";
    f << "NEWNET S=64 H=128 O=10 WTA=1\n";
    f << "DENSITY S->H 0.6\n";
    f << "DENSITY H->H 0.05\n";
    f << "DENSITY H->O 0.6\n";
    f << "DENSITY S->O 0.2\n";
    f << "INIT he\n";
    f << "EXCIT_RATIO 0.7\n";
    f << "W_SCALE 1.0\n";
    f << "THRESHOLDS S 100 H 45 O 55\n";
    f << "LEAK S 1.0 H 0.90 O 1.0\n";
    return true;
}

static double xent_from_rates(const std::map<std::string,float> &rates,
                              const std::string &target_id,
                              float temperature)
{
    // softmax over rates/temperature
    if (rates.empty()) return 0.0;
    std::vector<double> vals; vals.reserve(rates.size());
    std::vector<std::string> ids; ids.reserve(rates.size());
    for (const auto &kv : rates) { ids.push_back(kv.first); vals.push_back((double)kv.second / (temperature>0.0f?temperature:1.0f)); }
    double mx = *std::max_element(vals.begin(), vals.end());
    double sum_exp = 0.0; for (size_t i=0;i<vals.size();++i){ vals[i] = std::exp(vals[i]-mx); sum_exp += vals[i]; }
    double p_t = 1e-12;
    for (size_t i=0;i<ids.size(); ++i) if (ids[i] == target_id) { p_t = vals[i] / (sum_exp>0.0?sum_exp:1.0); break; }
    if (p_t < 1e-12) p_t = 1e-12;
    return -std::log(p_t);
}

int main(int argc, char** argv) {
    Args args;
    if (!parse_args(argc, argv, args)) {
        std::cout << "Usage: " << argv[0] << " --root <data_root> [--net <net_path> --epochs E --batch B --seed S --hebbian --gd_temperature T --lr L --lambda B --weight_decay D --warmup U --window W --alpha A --threshold T --default OX --save_net PATH --train_metrics_json PATH --train_metrics_csv PATH --train_plot_html PATH --predictions_csv_test PATH]\n";
        return 1;
    }

    if (args.net_path.empty()) {
        std::string def_net = join_path(args.data_root, "digits_default.net");
        if (!file_exists(def_net)) {
            if (!write_default_digits_net(def_net)) { std::cerr << "Failed to create default net at " << def_net << "\n"; return 1; }
            std::cout << "Created default digits net -> " << def_net << "\n";
        }
        args.net_path = def_net;
    }

    // Load datasets
    std::vector<Trainer::EpisodeData> train_set, test_set;
    if (!load_labels_csv(join_path(args.data_root, "train"), train_set)) return 2;
    if (!load_labels_csv(join_path(args.data_root, "test"), test_set)) return 3;
    std::cout << "Digits .seq dataset: train=" << train_set.size() << "  test=" << test_set.size() << "\n";

    // Network
    Glia net; net.configureNetworkFromFile(args.net_path);

    // Config
    TrainingConfig cfg;
    cfg.warmup_ticks = args.warmup;
    cfg.decision_window = args.window;
    cfg.detector.alpha = args.alpha;
    cfg.detector.threshold = args.threshold;
    cfg.detector.default_id = args.default_id;
    cfg.lr = args.lr;
    cfg.elig_lambda = args.lambda_;
    cfg.weight_decay = args.weight_decay;
    cfg.batch_size = std::max(1, args.batch);
    cfg.shuffle = true;
    cfg.verbose = true;
    cfg.log_every = 1;
    cfg.seed = args.seed;
    cfg.grad.temperature = args.gd_temperature;

    // Trainers
    Trainer hebb_trainer(net);
    RateGDTrainer gd_trainer(net);
    hebb_trainer.reseed(cfg.seed);
    gd_trainer.reseed(cfg.seed);
    bool use_hebbian = args.hebbian;

    // Training loop (custom to collect loss)
    std::mt19937 rng(cfg.seed);
    std::vector<double> epoch_loss, epoch_acc, epoch_margin;
    for (int e = 0; e < std::max(1, args.epochs); ++e) {
        if (cfg.shuffle) std::shuffle(train_set.begin(), train_set.end(), rng);
        size_t epoch_total = 0, epoch_correct = 0; double epoch_margin_sum = 0.0, epoch_loss_sum = 0.0;
        size_t batches_total = (train_set.size() + static_cast<size_t>(std::max(1, cfg.batch_size)) - 1) / static_cast<size_t>(std::max(1, cfg.batch_size));
        size_t batches_done = 0;
        for (size_t i = 0; i < train_set.size(); i += std::max(1, cfg.batch_size)) {
            size_t j = std::min(train_set.size(), i + static_cast<size_t>(std::max(1, cfg.batch_size)));
            std::vector<Trainer::EpisodeData> batch(train_set.begin() + i, train_set.begin() + j);
            std::vector<EpisodeMetrics> bm;
            if (use_hebbian) hebb_trainer.trainBatch(batch, cfg, &bm);
            else gd_trainer.trainBatch(batch, cfg, &bm);
            // compute batch acc, loss, margin
            int correct = 0; double avg_margin = 0.0; double avg_loss = 0.0;
            for (size_t k = 0; k < bm.size() && k < batch.size(); ++k) {
                const auto &m = bm[k];
                const auto &ex = batch[k];
                if (m.winner_id == ex.target_id) correct++;
                avg_margin += m.margin;
                avg_loss += xent_from_rates(m.rates, ex.target_id, cfg.grad.temperature);
            }
            size_t denom = bm.size();
            if (denom > 0) { avg_margin /= static_cast<double>(denom); avg_loss /= static_cast<double>(denom); }
            epoch_total += denom; epoch_correct += correct; epoch_margin_sum += avg_margin * denom; epoch_loss_sum += avg_loss * denom;
            batches_done++;
            if (cfg.verbose) {
                double running_acc = (epoch_total==0)?0.0:(static_cast<double>(epoch_correct)/static_cast<double>(epoch_total));
                double running_loss = (epoch_total==0)?0.0:(epoch_loss_sum/static_cast<double>(epoch_total));
                print_progress_bar(batches_done, batches_total, running_acc, running_loss);
            }
        }
        if (cfg.verbose) std::cout << std::endl;
        double eacc = (epoch_total == 0) ? 0.0 : (static_cast<double>(epoch_correct) / static_cast<double>(epoch_total));
        double emag = (epoch_total == 0) ? 0.0 : (epoch_margin_sum / static_cast<double>(epoch_total));
        double eloss = (epoch_total == 0) ? 0.0 : (epoch_loss_sum / static_cast<double>(epoch_total));
        epoch_acc.push_back(eacc); epoch_margin.push_back(emag); epoch_loss.push_back(eloss);
        if (cfg.verbose) {
            std::cout << "Epoch " << (e + 1) << "/" << std::max(1, args.epochs)
                      << "  Acc=" << eacc
                      << "  Loss=" << eloss
                      << "  Margin=" << emag
                      << std::endl;
        }
    }

    // Save trained net
    if (!args.save_net.empty()) {
        net.saveNetworkToFile(args.save_net);
        std::cout << "Saved trained net -> " << args.save_net << "\n";
    }

    // Save training metrics for plotting
    if (!args.train_metrics_json.empty()) {
        write_train_metrics_json(args.train_metrics_json, epoch_loss, epoch_acc, epoch_margin);
        std::cout << "Wrote training metrics -> " << args.train_metrics_json << "\n";
    }
    if (!args.train_metrics_csv.empty()) {
        write_train_metrics_csv(args.train_metrics_csv, epoch_loss, epoch_acc);
        std::cout << "Wrote training metrics CSV -> " << args.train_metrics_csv << "\n";
    }
    if (!args.train_plot_html.empty()) {
        write_train_plot_html(args.train_plot_html, epoch_loss, epoch_acc);
        std::cout << "Wrote training plot HTML -> " << args.train_plot_html << "\n";
    }

    // Validate on test set
    size_t total = 0, correct = 0; double sum_margin = 0.0;
    std::vector<std::pair<std::string,int>> test_names;
    read_labels_list(join_path(args.data_root, "test"), test_names);
    std::vector<EpisodeMetrics> test_metrics;
    test_metrics.reserve(test_set.size());
    std::set<std::string> rate_keys_set;
    for (size_t idx = 0; idx < test_set.size(); ++idx) {
        const auto &ex = test_set[idx];
        InputSequence seq = ex.seq;
        EpisodeMetrics m = (use_hebbian ? hebb_trainer.evaluate(seq, cfg) : gd_trainer.evaluate(seq, cfg));
        total += 1; if (m.winner_id == ex.target_id) correct += 1; sum_margin += m.margin;
        test_metrics.push_back(m);
        for (const auto &kv : m.rates) rate_keys_set.insert(kv.first);
    }
    double acc = (total==0) ? 0.0 : (static_cast<double>(correct)/static_cast<double>(total));
    double avg_margin = (total==0) ? 0.0 : (sum_margin/static_cast<double>(total));
    std::cout << "Test: samples=" << total << "  accuracy=" << (acc*100.0) << "%  avg_margin=" << avg_margin << "\n";

    if (!args.predictions_csv_test.empty()) {
        std::vector<std::string> rate_keys(rate_keys_set.begin(), rate_keys_set.end());
        std::ofstream pf(args.predictions_csv_test.c_str(), std::ios::out | std::ios::trunc);
        if (pf.is_open()) {
            pf << "index,filename,true,pred,correct,margin";
            for (const auto &k : rate_keys) pf << "," << k;
            pf << "\n";
            for (size_t i = 0; i < test_metrics.size(); ++i) {
                std::string fname = (i < test_names.size() ? test_names[i].first : std::to_string(i));
                int tlabel = (i < test_names.size() ? test_names[i].second : -1);
                std::string true_id = std::string("O") + (tlabel>=0?std::to_string(tlabel):std::string("?"));
                const auto &m = test_metrics[i];
                bool ok = (m.winner_id == true_id);
                pf << i << "," << fname << "," << true_id << "," << m.winner_id << "," << (ok?1:0) << "," << m.margin;
                for (const auto &k : rate_keys) {
                    auto it = m.rates.find(k);
                    double v = (it==m.rates.end()?0.0:it->second);
                    pf << "," << v;
                }
                pf << "\n";
            }
        }
        std::cout << "Wrote predictions CSV (test) -> " << args.predictions_csv_test << "\n";
    }

    return 0;
}
