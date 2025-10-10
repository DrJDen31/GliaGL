#ifndef __output_detection_h__
#define __output_detection_h__

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <algorithm>

/**
 * Firing rate tracker using Exponential Moving Average (EMA)
 * 
 * This class tracks the firing rates of output neurons over time and provides
 * argmax classification for winner-take-all decision making.
 * 
 * Usage:
 *   FiringRateTracker tracker(0.05f);  // alpha = 1/20
 *   tracker.update("N1", neuron->didFire());
 *   std::string winner = tracker.argmax({"N1", "N2", "N3"});
 */
class FiringRateTracker
{
public:
    /**
     * Constructor
     * @param alpha EMA smoothing factor (typical: 0.05 for 1/20 smoothing)
     *              Higher alpha = faster response, lower alpha = more smoothing
     */
    FiringRateTracker(float alpha = 0.05f) : alpha(alpha) {}

    /**
     * Reset all tracked firing rates to zero
     */
    void reset()
    {
        rates.clear();
    }

    /**
     * Update the firing rate for a specific neuron
     * @param neuron_id Unique identifier for the neuron
     * @param fired Whether the neuron fired this timestep (true/false)
     */
    void update(const std::string &neuron_id, bool fired)
    {
        if (rates.find(neuron_id) == rates.end())
        {
            rates[neuron_id] = 0.0f;
        }
        // EMA update: r_k ← (1−α)r_k + α·[fired_k]
        rates[neuron_id] = (1.0f - alpha) * rates[neuron_id] + alpha * (fired ? 1.0f : 0.0f);
    }

    /**
     * Get the current firing rate for a specific neuron
     * @param neuron_id Unique identifier for the neuron
     * @return Firing rate in range [0.0, 1.0], or 0.0 if neuron not tracked
     */
    float getRate(const std::string &neuron_id) const
    {
        auto it = rates.find(neuron_id);
        return (it != rates.end()) ? it->second : 0.0f;
    }

    /**
     * Find the neuron with the highest firing rate (argmax classification)
     * @param neuron_ids List of neuron IDs to compare
     * @param default_id Default neuron ID to return when all rates are below threshold (empty string = no default)
     * @param threshold Minimum rate to consider active (default 0.01)
     * @return ID of the neuron with highest rate, default_id if all below threshold, or empty if no default
     */
    std::string argmax(const std::vector<std::string> &neuron_ids, 
                      const std::string &default_id = "", 
                      float threshold = 0.01f) const
    {
        std::string max_id;
        float max_rate = -1.0f;

        for (const auto &id : neuron_ids)
        {
            float rate = getRate(id);
            if (rate > max_rate)
            {
                max_rate = rate;
                max_id = id;
            }
        }
        
        // If maximum rate is below threshold, return configured default
        if (max_rate < threshold)
        {
            return default_id;  // Empty string if no default configured
        }

        return max_id;
    }

    /**
     * Get the margin between the top two neurons (confidence metric)
     * @param neuron_ids List of neuron IDs to compare
     * @return Difference between highest and second-highest rate
     */
    float getMargin(const std::vector<std::string> &neuron_ids) const
    {
        if (neuron_ids.size() < 2)
            return 0.0f;

        std::vector<float> sorted_rates;
        for (const auto &id : neuron_ids)
        {
            sorted_rates.push_back(getRate(id));
        }

        std::sort(sorted_rates.rbegin(), sorted_rates.rend()); // Descending order
        return sorted_rates[0] - sorted_rates[1];
    }

    /**
     * Print firing rates for a list of neurons
     * @param neuron_ids List of neuron IDs to display
     */
    void printRates(const std::vector<std::string> &neuron_ids) const
    {
        for (const auto &id : neuron_ids)
        {
            std::cout << "  " << id << ": " << std::fixed << std::setprecision(3) 
                      << getRate(id) << std::endl;
        }
    }

    /**
     * Get all tracked rates as a map
     * @return Map of neuron_id -> firing_rate
     */
    const std::map<std::string, float>& getAllRates() const
    {
        return rates;
    }

private:
    float alpha;                              // EMA smoothing factor
    std::map<std::string, float> rates;      // neuron_id -> firing_rate
};

#endif // __output_detection_h__
