#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include "../../arch/glia.h"
#include "../../arch/neuron.h"
#include "../../arch/output_detection.h"

/**
 * Run a test with a specific class label and noise level
 * @param network The neural network
 * @param true_class The correct class (0, 1, or 2)
 * @param noise_prob Probability of activating a wrong sensory neuron (0.0 to 1.0)
 * @param num_ticks Number of simulation ticks
 * @param tracker Firing rate tracker for output neurons
 * @param output_neurons List of output neuron IDs
 * @param rng Random number generator
 */
void runTest(Glia &network, int true_class, float noise_prob, int num_ticks,
             EMAOutputDetector &detector, const std::vector<std::string> &output_neurons,
             std::mt19937 &rng)
{
    std::cout << "\n=== Testing class " << true_class << " with " 
              << (noise_prob * 100.0f) << "% noise ===" << std::endl;
    
    detector.reset();
    
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Track noise statistics
    int noise_activations[3] = {0, 0, 0};

    for (int t = 0; t < num_ticks; ++t)
    {
        // Always activate the true class sensory neuron
        std::string true_sensory = "S" + std::to_string(true_class);
        network.injectSensory(true_sensory, 200.0f);
        
        // With probability noise_prob, activate each of the other sensory neurons
        for (int c = 0; c < 3; ++c)
        {
            if (c != true_class && dist(rng) < noise_prob)
            {
                std::string noisy_sensory = "S" + std::to_string(c);
                network.injectSensory(noisy_sensory, 200.0f);
                noise_activations[c]++;
            }
        }

        // Step the network forward
        network.step();

        // Track firing rates for output neurons
        for (const auto &id : output_neurons)
        {
            Neuron *n = network.getNeuronById(id);
            if (n)
            {
                detector.update(id, n->didFire());
            }
        }
    }

    // Print noise statistics
    std::cout << "Noise activations: ";
    for (int c = 0; c < 3; ++c)
    {
        if (c != true_class)
        {
            std::cout << "S" << c << "=" << noise_activations[c] << " ";
        }
    }
    std::cout << std::endl;

    // Print results
    std::cout << "Firing rates after " << num_ticks << " ticks:" << std::endl;
    for (const auto &id : output_neurons) {
        std::cout << "  " << id << ": " << detector.getRate(id) << std::endl;
    }
    
    // Get winner via argmax
    std::string winner = detector.predict(output_neurons);
    
    if (winner.empty())
    {
        std::cout << "Winner: None (all outputs silent)" << std::endl;
        std::cout << "Classification: UNDECIDED" << std::endl;
    }
    else
    {
        std::cout << "Winner (argmax): " << winner << std::endl;
        
        // Extract class from winner (O0 -> 0, O1 -> 1, O2 -> 2)
        int predicted_class = -1;
        if (winner == "O0") predicted_class = 0;
        else if (winner == "O1") predicted_class = 1;
        else if (winner == "O2") predicted_class = 2;
        
        std::cout << "Predicted class: " << predicted_class << std::endl;
        
        // Check if correct
        if (predicted_class == true_class)
        {
            std::cout << "Result: ✓ CORRECT" << std::endl;
        }
        else
        {
            std::cout << "Result: ✗ INCORRECT" << std::endl;
        }
    }
    
    std::cout << "Expected class: " << true_class << std::endl;
    
    // Calculate margin (confidence)
    float margin = detector.getMargin(output_neurons);
    std::cout << "Margin (confidence): " << margin << std::endl;
}

int main()
{
    std::cout << "=== 3-Class One-Hot Classification Test ===" << std::endl;
    std::cout << "Network: 3 sensory inputs + inhibitory pool + 3 output neurons" << std::endl;
    std::cout << "Testing robustness to noisy sensory input" << std::endl;
    std::cout << std::endl;

    // Create empty network - all neurons created from config file
    Glia network;

    // Load the manually configured 3-class network
    std::cout << "Loading network configuration..." << std::endl;
    network.configureNetworkFromFile("3class_network.net");
    std::cout << std::endl;

    // Set up output detector (EMA)
    EMAOutputDetector detector(0.05f);
    
    // Output neurons to monitor
    std::vector<std::string> output_neurons = {"O0", "O1", "O2"};
    
    // Number of ticks to run each test
    int num_ticks = 100;
    
    // Random number generator for noise
    std::random_device rd;
    std::mt19937 rng(rd());

    // Test 1: No noise (sanity check)
    std::cout << "\n========== Test Set 1: No Noise ==========" << std::endl;
    for (int c = 0; c < 3; ++c)
    {
        runTest(network, c, 0.0f, num_ticks, detector, output_neurons, rng);
    }

    // Test 2: Low noise (5%)
    std::cout << "\n========== Test Set 2: 5% Noise ==========" << std::endl;
    for (int c = 0; c < 3; ++c)
    {
        runTest(network, c, 0.05f, num_ticks, detector, output_neurons, rng);
    }

    // Test 3: Medium noise (10%)
    std::cout << "\n========== Test Set 3: 10% Noise ==========" << std::endl;
    for (int c = 0; c < 3; ++c)
    {
        runTest(network, c, 0.10f, num_ticks, detector, output_neurons, rng);
    }

    // Test 4: High noise (20%)
    std::cout << "\n========== Test Set 4: 20% Noise ==========" << std::endl;
    for (int c = 0; c < 3; ++c)
    {
        runTest(network, c, 0.20f, num_ticks, detector, output_neurons, rng);
    }

    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "\nExpected behavior:" << std::endl;
    std::cout << "- With no noise: Perfect classification (100% accuracy)" << std::endl;
    std::cout << "- With 5-20% noise: Correct class should maintain highest firing rate" << std::endl;
    std::cout << "- Inhibitory pool suppresses competitors" << std::endl;
    std::cout << "- Margin indicates classification confidence" << std::endl;

    return 0;
}
