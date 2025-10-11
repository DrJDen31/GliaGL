#ifndef __output_detection_h__
#define __output_detection_h__

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <algorithm>

// =====================================================================================
// Pluggable Output Detector Interface + EMA Implementation
// =====================================================================================

struct OutputDetectorOptions
{
    // Optional default: if all outputs are below threshold, return this ID
    // Empty string = abstain
    std::string default_id;
    // Minimum firing metric required to consider an output active
    float threshold = 0.01f;
};

class IOutputDetector
{
public:
    virtual ~IOutputDetector() {}
    // Reset internal state (rates, windows, etc.)
    virtual void reset() = 0;
    // Update per-tick with whether a neuron fired
    virtual void update(const std::string &neuron_id, bool fired) = 0;
    // Predict winning output among provided IDs according to detector policy
    virtual std::string predict(const std::vector<std::string> &output_ids) const = 0;
    // Optional metrics
    virtual float getRate(const std::string &neuron_id) const { (void)neuron_id; return 0.0f; }
    virtual float getMargin(const std::vector<std::string> &output_ids) const { (void)output_ids; return 0.0f; }
};

// EMA-based detector that uses FiringRateTracker under the hood
class EMAOutputDetector : public IOutputDetector
{
public:
    EMAOutputDetector(float alpha = 0.05f, OutputDetectorOptions opts = {})
        : alpha(alpha), options(opts) {}

    void reset() override { rates.clear(); }

    void update(const std::string &neuron_id, bool fired) override {
        if (rates.find(neuron_id) == rates.end()) rates[neuron_id] = 0.0f;
        rates[neuron_id] = (1.0f - alpha) * rates[neuron_id] + alpha * (fired ? 1.0f : 0.0f);
    }

    std::string predict(const std::vector<std::string> &output_ids) const override {
        std::string max_id;
        float max_rate = -1.0f;
        for (const auto &id : output_ids) {
            float r = getRate(id);
            if (r > max_rate) { max_rate = r; max_id = id; }
        }
        if (max_rate < options.threshold) {
            return options.default_id; // empty => abstain
        }
        return max_id;
    }

    float getRate(const std::string &neuron_id) const override {
        auto it = rates.find(neuron_id);
        return (it != rates.end()) ? it->second : 0.0f;
    }

    float getMargin(const std::vector<std::string> &output_ids) const override {
        if (output_ids.size() < 2) return 0.0f;
        std::vector<float> vals; vals.reserve(output_ids.size());
        for (const auto &id : output_ids) vals.push_back(getRate(id));
        std::sort(vals.rbegin(), vals.rend());
        return vals[0] - vals[1];
    }

private:
    float alpha;
    std::map<std::string, float> rates;
    OutputDetectorOptions options;
};

#endif // __output_detection_h__
