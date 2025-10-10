#include <iostream>
#include <fstream>
#include <random>
#include <string>

// Generate test sequence files for 3-class network
void generate3ClassNoiseTest(int class_id, float noise_prob, int duration, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create file " << filename << std::endl;
        return;
    }
    
    // Write header
    file << "# 3-Class Network Test" << std::endl;
    file << "# Class " << class_id << " with " << (noise_prob * 100) << "% noise" << std::endl;
    file << "# Generated test sequence" << std::endl;
    file << std::endl;
    
    file << "DURATION " << duration << std::endl;
    file << "LOOP false" << std::endl;
    file << std::endl;
    
    // Generate events
    std::mt19937 rng(12345);  // Fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    std::string correct_input = "S" + std::to_string(class_id);
    
    for (int tick = 0; tick < duration; tick++) {
        // Always activate correct class
        file << tick << " " << correct_input << " 200.0" << std::endl;
        
        // Add noise with probability
        for (int other_class = 0; other_class < 3; other_class++) {
            if (other_class != class_id) {
                if (dist(rng) < noise_prob) {
                    std::string noise_input = "S" + std::to_string(other_class);
                    file << tick << " " << noise_input << " 200.0" << std::endl;
                }
            }
        }
    }
    
    file.close();
    std::cout << "Generated: " << filename << std::endl;
}

void generateXORTest(int ticks_per_pattern, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create file " << filename << std::endl;
        return;
    }
    
    // Write header
    file << "# XOR Network Test" << std::endl;
    file << "# All 4 input combinations" << std::endl;
    file << "# " << ticks_per_pattern << " ticks per pattern" << std::endl;
    file << std::endl;
    
    file << "DURATION " << (ticks_per_pattern * 4) << std::endl;
    file << "LOOP false" << std::endl;
    file << std::endl;
    
    // Pattern 00 (ticks 0 to ticks_per_pattern-1): both off (no events)
    
    // Pattern 01 (ticks ticks_per_pattern to 2*ticks_per_pattern-1): S1 only
    for (int t = ticks_per_pattern; t < ticks_per_pattern * 2; t++) {
        file << t << " S1 200.0" << std::endl;
    }
    
    // Pattern 10 (ticks 2*ticks_per_pattern to 3*ticks_per_pattern-1): S0 only
    for (int t = ticks_per_pattern * 2; t < ticks_per_pattern * 3; t++) {
        file << t << " S0 200.0" << std::endl;
    }
    
    // Pattern 11 (ticks 3*ticks_per_pattern to 4*ticks_per_pattern-1): both on
    for (int t = ticks_per_pattern * 3; t < ticks_per_pattern * 4; t++) {
        file << t << " S0 200.0" << std::endl;
        file << t << " S1 200.0" << std::endl;
    }
    
    file.close();
    std::cout << "Generated: " << filename << std::endl;
}

int main() {
    std::cout << "Generating test sequence files..." << std::endl << std::endl;
    
    // 3-class tests
    generate3ClassNoiseTest(0, 0.05f, 200, "test_class0_5pct.seq");
    generate3ClassNoiseTest(1, 0.10f, 200, "test_class1_10pct.seq");
    generate3ClassNoiseTest(2, 0.20f, 200, "test_class2_20pct.seq");
    
    // XOR test
    generateXORTest(100, "test_xor.seq");
    
    std::cout << std::endl << "Done! Generated 4 test files." << std::endl;
    std::cout << "Use with: vis.exe --network network.net --tests test1.seq test2.seq ..." << std::endl;
    
    return 0;
}
