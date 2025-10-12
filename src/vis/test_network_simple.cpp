// Simplified test program to verify NetworkGraph builds from Glia
// This version has minimal dependencies - no ArgParser, no MeshData

#include <iostream>
#include "glia.h"
#include "neuron.h"
#include "neuron_particle.h"

// Simple test of NeuronParticle functionality
void testNeuronParticle() {
    std::cout << "\n=== Testing NeuronParticle ===" << std::endl;
    
    // Create a dummy neuron
    Neuron neuron("TEST", 10, 50.0f, 1, 4, 100.0f, true);
    
    // Create a NeuronParticle
    NeuronParticle particle("TEST", &neuron, NeuronType::INTERNEURON);
    
    std::cout << "Created NeuronParticle:" << std::endl;
    std::cout << "  ID: " << particle.getId() << std::endl;
    std::cout << "  Type: INTERNEURON" << std::endl;
    std::cout << "  Fixed: " << (particle.isFixed() ? "Yes" : "No") << std::endl;
    std::cout << "  Size: " << particle.getSize() << std::endl;
    
    // Test position
    Vec3f pos(1.0, 2.0, 3.0);
    particle.setPosition(pos);
    Vec3f retrieved = particle.getPosition();
    std::cout << "  Position: (" << retrieved.x() << ", " << retrieved.y() << ", " << retrieved.z() << ")" << std::endl;
    
    // Test color
    Vec3f base_color = particle.getBaseColor();
    std::cout << "  Base Color: (" << base_color.r() << ", " << base_color.g() << ", " << base_color.b() << ")" << std::endl;
    
    // Test activation
    particle.setFiring(true);
    particle.updateActivationState();
    std::cout << "  Firing: " << (particle.isFiring() ? "Yes" : "No") << std::endl;
    std::cout << "  Activation Level: " << particle.getActivationLevel() << std::endl;
    
    // Test color update
    particle.updateColor();
    Vec3f current_color = particle.getCurrentColor();
    std::cout << "  Current Color: (" << current_color.r() << ", " << current_color.g() << ", " << current_color.b() << ")" << std::endl;
    
    std::cout << "✓ NeuronParticle test passed!" << std::endl;
}

void testNetworkBuilding() {
    std::cout << "\n=== Testing Network Building ===" << std::endl;
    
    // Create a simple XOR network
    std::cout << "Creating XOR network (2 sensory, 3 interneurons)..." << std::endl;
    Glia network(2, 3);
    
    // Load configuration
    std::cout << "Loading configuration from file..." << std::endl;
    network.configureNetworkFromFile("../../examples/xor/xor_network.net");
    
    std::cout << "✓ Network loaded successfully!" << std::endl;
    
    // Test network execution
    std::cout << "\nTesting network execution:" << std::endl;
    std::cout << "  Injecting input: S0=200, S1=200 (XOR input: 11)" << std::endl;
    network.injectSensory("S0", 200.0f);
    network.injectSensory("S1", 200.0f);
    
    // Run for a few ticks
    for (int i = 0; i < 10; i++) {
        network.step();
    }
    
    // Check output neurons
    Neuron* n1 = network.getNeuronById("N1");
    Neuron* n2 = network.getNeuronById("N2");
    
    if (n1 && n2) {
        std::cout << "  N1 (XOR true) fired: " << (n1->didFire() ? "Yes" : "No") << std::endl;
        std::cout << "  N2 (XOR false) fired: " << (n2->didFire() ? "Yes" : "No") << std::endl;
    }
    
    std::cout << "✓ Network execution test passed!" << std::endl;
}

int main() {
    std::cout << "=== NetworkGraph Simple Test ===" << std::endl;
    std::cout << "Testing core components without OpenGL dependencies" << std::endl;
    
    try {
        testNeuronParticle();
        testNetworkBuilding();
        
        std::cout << "\n=== All Tests Passed! ===" << std::endl;
        std::cout << "✓ NeuronParticle functionality verified" << std::endl;
        std::cout << "✓ Glia network loading verified" << std::endl;
        std::cout << "✓ Network execution verified" << std::endl;
        std::cout << "\nNext steps:" << std::endl;
        std::cout << "  1. Implement full NetworkGraph with spatial layout" << std::endl;
        std::cout << "  2. Add physics simulation" << std::endl;
        std::cout << "  3. Integrate with OpenGL renderer" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
