#include <iostream>
#include <vector>
#include <algorithm>
#include "../../arch/glia.h"
#include "../../arch/neuron.h"
#include "../../arch/output_detection.h"

/**
 * Run a test pattern through the network
 */
void runTest(Glia &network, int input0, int input1, int num_ticks, 
             EMAOutputDetector &detector, const std::vector<std::string> &output_neurons)
{
    std::cout << "\n=== Testing input: " << input0 << input1 << " ===" << std::endl;
    
    detector.reset();

    for (int t = 0; t < num_ticks; ++t)
    {
        // Inject sensory input
        // Sensory neurons need to be stimulated each tick
        // Use a value that will make them fire (above their threshold)
        if (input0 == 1)
        {
            network.injectSensory("S0", 200.0f);  // Strong input to ensure firing
        }
        if (input1 == 1)
        {
            network.injectSensory("S1", 200.0f);
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

    // Print results
    std::cout << "Firing rates after " << num_ticks << " ticks:" << std::endl;
    for (const auto &id : output_neurons) {
        std::cout << "  " << id << ": " << detector.getRate(id) << std::endl;
    }
    
    std::string winner = detector.predict(output_neurons);
    
    if (winner.empty())
    {
        std::cout << "Winner: None (both silent, no default configured)" << std::endl;
        std::cout << "XOR Result: UNDECIDED" << std::endl;
    }
    else
    {
        std::cout << "Winner (argmax): " << winner << std::endl;
        
        // Interpret XOR result
        if (winner == "O1")  // XOR true
        {
            std::cout << "XOR Result: TRUE (1)" << std::endl;
        }
        else if (winner == "O0")  // XOR false
        {
            std::cout << "XOR Result: FALSE (0)" << std::endl;
        }
        else
        {
            std::cout << "XOR Result: UNDECIDED" << std::endl;
        }
    }

    // Expected XOR results
    int expected = (input0 != input1) ? 1 : 0;
    std::string expected_str = expected ? "TRUE (1)" : "FALSE (0)";
    std::cout << "Expected: " << expected_str << std::endl;
}

int main()
{
    std::cout << "=== XOR Neural Network Test ===" << std::endl;
    std::cout << "Manually configured network (no training)" << std::endl;
    std::cout << std::endl;

    // Create empty network - all neurons defined in config file
    // S0, S1 = sensory inputs
    // N0 = A (AND detector)
    // O1 = XOR true output
    // O0 = XOR false output
    Glia network;

    // Load the manually configured XOR network
    std::cout << "Loading network configuration..." << std::endl;
    network.configureNetworkFromFile("xor_network.net");
    std::cout << std::endl;

    // Set up output detector (EMA) with default O0 when below threshold
    OutputDetectorOptions opts; opts.default_id = "O0"; opts.threshold = 0.01f;
    EMAOutputDetector detector(0.05f, opts);
    
    // Output neurons to monitor
    std::vector<std::string> output_neurons = {"O1", "O0"};
    
    // Number of ticks to run each test
    int num_ticks = 100;

    // Test all XOR input combinations
    runTest(network, 0, 0, num_ticks, detector, output_neurons);
    runTest(network, 0, 1, num_ticks, detector, output_neurons);
    runTest(network, 1, 0, num_ticks, detector, output_neurons);
    runTest(network, 1, 1, num_ticks, detector, output_neurons);

    std::cout << "\n=== Test Complete ===" << std::endl;

    return 0;
}
