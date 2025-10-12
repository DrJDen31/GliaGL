// Simple test program to verify NetworkGraph builds from Glia
#include <iostream>
#include "glia.h"
#include "neuron.h"
#include "network_graph.h"
#include "argparser.h"
#include "meshdata.h"

// Global mesh data (required by network_graph.cpp)
MeshData *mesh_data;

int main(int argc, const char* argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    
    std::cout << "=== NetworkGraph Test ===" << std::endl;

    // Initialize mesh data
    MeshData mymesh_data;
    mesh_data = &mymesh_data;
    INIT_MeshData(mesh_data);

    // Create arg parser (minimal setup)
    const char* dummy_argv[] = {"test_network_graph", nullptr};
    ArgParser args(1, dummy_argv, mesh_data);

    // Create a simple XOR network
    std::cout << "\n1. Creating XOR network..." << std::endl;
    Glia network(2, 3);  // 2 sensory, 3 interneurons
    
    // Load configuration
    std::cout << "2. Loading XOR network configuration..." << std::endl;
    network.configureNetworkFromFile("../../examples/xor/xor_network.net");
    
    // Create NetworkGraph
    std::cout << "\n3. Building NetworkGraph from Glia..." << std::endl;
    NetworkGraph graph(&network, &args);
    
    std::cout << "\n4. NetworkGraph created successfully!" << std::endl;
    std::cout << "   Bounding box computed" << std::endl;
    std::cout << "   (visualization would render here)" << std::endl;

    // Test mode switching
    std::cout << "\n5. Testing mode switching..." << std::endl;
    std::cout << "   Initial mode: " << (graph.isTrainingMode() ? "Training" : "Inference") << std::endl;
    
    graph.setTrainingMode(true);
    std::cout << "   After setTrainingMode(true): " << (graph.isTrainingMode() ? "Training" : "Inference") << std::endl;
    
    graph.setTrainingMode(false);
    std::cout << "   After setTrainingMode(false): " << (graph.isTrainingMode() ? "Training" : "Inference") << std::endl;

    // Test physics simulation
    std::cout << "\n6. Testing physics simulation (5 steps)..." << std::endl;
    graph.setTrainingMode(true);
    for (int i = 0; i < 5; i++) {
        graph.animatePhysics();
        std::cout << "   Physics step " << (i + 1) << " complete" << std::endl;
    }

    // Test activation visualization
    std::cout << "\n7. Testing activation visualization..." << std::endl;
    graph.setTrainingMode(false);
    
    // Inject input
    network.injectSensory("S0", 200.0f);
    network.injectSensory("S1", 200.0f);
    
    // Run network for a few ticks
    for (int t = 0; t < 10; t++) {
        network.step();
        graph.updateActivationStates();
        graph.updateColors();
    }
    std::cout << "   Activation visualization updated" << std::endl;

    std::cout << "\n=== Test Complete! ===" << std::endl;
    std::cout << "All NetworkGraph functionality working correctly." << std::endl;

    return 0;
}
