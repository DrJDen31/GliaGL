# Test Generators - Pattern & Guidelines

## Philosophy

**Each network type has its own test generator.**

This ensures:
- Tests are tailored to the specific network architecture
- Easy to maintain and modify per network
- Clear separation of concerns
- Generators can evolve independently

## Directory Structure

```
examples/
  3class/                      # 3-class classification network
    3class_network.net
    3class_test.cpp            # Standalone test harness
    generate_tests.cpp         # Test sequence generator
    Makefile
    Makefile_gentests
    test_*.seq                 # Generated test files
    
  xor/                         # XOR network
    xor_network.net
    xor_test.cpp
    generate_tests.cpp         # XOR-specific generator
    Makefile
    Makefile_gentests
    test_*.seq
    
  your_network/                # Your custom network
    your_network.net
    your_test.cpp
    generate_tests.cpp         # Your custom generator
    Makefile
    Makefile_gentests
    test_*.seq
```

## Creating a Generator for Your Network

### 1. Copy Template

```bash
cd examples/your_network
cp ../3class/generate_tests.cpp .
cp ../3class/Makefile_gentests .
```

### 2. Customize Generator

Edit `generate_tests.cpp`:

```cpp
#include <iostream>
#include <fstream>
#include <random>
#include <string>

// Example: Generate a test for your network
void generateYourNetworkTest(/* params */, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create file " << filename << std::endl;
        return;
    }
    
    // Header
    file << "# Your Network Test\n";
    file << "# Description of what this tests\n\n";
    file << "DURATION 300\n";
    file << "LOOP false\n\n";
    
    // Generate events based on your network's inputs
    for (int tick = 0; tick < 300; tick++) {
        // Your test logic here
        // Example: Activate different sensory neurons
        if (tick < 100) {
            file << tick << " S0 200.0\n";
        } else if (tick < 200) {
            file << tick << " S1 200.0\n";
        } else {
            file << tick << " S0 200.0\n";
            file << tick << " S1 200.0\n";
        }
    }
    
    file.close();
    std::cout << "Generated: " << filename << std::endl;
}

int main() {
    std::cout << "Generating test files for YourNetwork...\n\n";
    
    // Generate your tests
    generateYourNetworkTest(/* params */, "test_scenario1.seq");
    generateYourNetworkTest(/* params */, "test_scenario2.seq");
    generateYourNetworkTest(/* params */, "test_scenario3.seq");
    
    std::cout << "\nDone!\n";
    return 0;
}
```

### 3. Build and Run

```bash
make -f Makefile_gentests
./generate_tests
```

### 4. Use with Visualizer

```cmd
cd ../../../build
debug\vis.exe --network ../examples/your_network/your_network.net ^
              --tests ../examples/your_network/test_scenario1.seq ^
                      ../examples/your_network/test_scenario2.seq ^
                      ../examples/your_network/test_scenario3.seq
```

## Example Generators

### 3-Class Network (Noise Robustness)

```cpp
void generate3ClassNoiseTest(int class_id, float noise_prob, int duration, 
                             const std::string& filename) {
    std::ofstream file(filename);
    // ... header ...
    
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    std::string correct = "S" + std::to_string(class_id);
    
    for (int t = 0; t < duration; t++) {
        // Always activate correct class
        file << t << " " << correct << " 200.0\n";
        
        // Add noise
        for (int i = 0; i < 3; i++) {
            if (i != class_id && dist(rng) < noise_prob) {
                file << t << " S" << i << " 200.0\n";
            }
        }
    }
    
    file.close();
}
```

### XOR Network (Pattern Sequences)

```cpp
void generateXORTest(int ticks_per_pattern, const std::string& filename) {
    std::ofstream file(filename);
    // ... header ...
    
    // Pattern 00: nothing
    // Pattern 01: S1 only
    for (int t = ticks_per_pattern; t < ticks_per_pattern * 2; t++) {
        file << t << " S1 200.0\n";
    }
    
    // Pattern 10: S0 only
    for (int t = ticks_per_pattern * 2; t < ticks_per_pattern * 3; t++) {
        file << t << " S0 200.0\n";
    }
    
    // Pattern 11: both
    for (int t = ticks_per_pattern * 3; t < ticks_per_pattern * 4; t++) {
        file << t << " S0 200.0\n";
        file << t << " S1 200.0\n";
    }
    
    file.close();
}
```

### Temporal Pattern Network (Bursts, Ramps)

```cpp
void generateBurstTest(int burst_duration, int interval, 
                       const std::string& filename) {
    std::ofstream file(filename);
    // ... header ...
    
    for (int burst = 0; burst < 10; burst++) {
        int start = burst * (burst_duration + interval);
        for (int t = start; t < start + burst_duration; t++) {
            file << t << " S0 250.0\n";  // High-intensity burst
        }
    }
    
    file.close();
}

void generateRampTest(int duration, const std::string& filename) {
    std::ofstream file(filename);
    // ... header ...
    
    for (int t = 0; t < duration; t++) {
        float intensity = 50.0f + (150.0f * t / duration);  // Ramp 50→200
        file << t << " S0 " << intensity << "\n";
    }
    
    file.close();
}
```

## Common Patterns

### Noise Injection

```cpp
std::mt19937 rng(seed);
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

if (dist(rng) < noise_probability) {
    file << tick << " " << noise_neuron << " " << noise_value << "\n";
}
```

### Timed Sequences

```cpp
struct Pattern {
    int start_tick;
    int duration;
    std::vector<std::string> active_neurons;
};

for (const auto& pattern : patterns) {
    for (int t = pattern.start_tick; t < pattern.start_tick + pattern.duration; t++) {
        for (const auto& neuron : pattern.active_neurons) {
            file << t << " " << neuron << " 200.0\n";
        }
    }
}
```

### Parametric Sweeps

```cpp
// Generate multiple tests with varying parameters
for (float param = 0.0f; param <= 1.0f; param += 0.1f) {
    std::string filename = "test_param_" + 
                          std::to_string(int(param * 100)) + ".seq";
    generateParametricTest(param, filename);
}
```

## Best Practices

### 1. Use Fixed Seeds for Reproducibility

```cpp
std::mt19937 rng(12345);  // Fixed seed
// Tests will generate identically every time
```

### 2. Descriptive Filenames

```
✓ test_class0_noise5pct_200ticks.seq
✓ test_xor_all_patterns.seq
✓ test_burst_10ms_5hz.seq

✗ test1.seq
✗ test_a.seq
✗ mytest.seq
```

### 3. Document in Comments

```cpp
file << "# Test: 3-Class Classification\n";
file << "# Scenario: Class 0 with 5% noise\n";
file << "# Expected: O0 should win (>90% of ticks)\n";
file << "# Duration: 200 ticks\n";
file << "#\n";
file << "# Neuron mapping:\n";
file << "#   S0, S1, S2 = sensory inputs\n";
file << "#   O0, O1, O2 = output neurons\n";
file << "#\n";
```

### 4. Parameterize Generator

```cpp
// Good: flexible
void generateTest(int class_id, float noise, int duration, 
                 const std::string& filename);

// Less flexible: hardcoded
void generateClass0Test5Percent();
```

### 5. Main() as Test Suite Configuration

```cpp
int main() {
    // Baseline tests
    generateCleanTest(0, "test_class0_clean.seq");
    generateCleanTest(1, "test_class1_clean.seq");
    
    // Noise robustness suite
    for (float noise = 0.05f; noise <= 0.25f; noise += 0.05f) {
        std::string name = "test_noise_" + 
                          std::to_string(int(noise * 100)) + "pct.seq";
        generateNoiseTest(noise, name);
    }
    
    // Edge cases
    generateAllOnTest("test_all_on.seq");
    generateRapidSwitchTest("test_rapid_switch.seq");
    
    return 0;
}
```

## Generator Checklist

When creating a generator for a new network:

- [ ] Copy template from existing generator
- [ ] Update header comments (network name, purpose)
- [ ] Identify network's sensory neurons (S0, S1, ...)
- [ ] Define test scenarios (what are you testing?)
- [ ] Implement generation functions
- [ ] Configure main() with test suite
- [ ] Use fixed random seeds
- [ ] Add descriptive filenames
- [ ] Document expected outcomes in .seq files
- [ ] Test: build, run, verify .seq files
- [ ] Test: load in visualizer, confirm behavior
- [ ] Commit generator + .seq files to version control

## Why Not a Single Universal Generator?

**Question**: Why not one generator for all networks?

**Answer**: 
- Each network has different architectures (different sensory/output neurons)
- Test scenarios are network-specific (noise for 3-class, patterns for XOR)
- Parameters differ (noise % vs. pattern duration)
- Keeps generators simple and focused
- Easy to understand and modify
- No complex configuration files needed

**Principle**: Duplication is better than wrong abstraction.

---

**One generator per network type = clarity and maintainability** ✨
