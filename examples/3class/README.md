# 3-Class One-Hot Classification Test with Noise + Inhibitory Pool

This directory contains a manually configured spiking neural network that implements 3-class one-hot classification with noise robustness as described in `docs/TOY_EXAMPLES.md`.

**Location:** `examples/3class/`

This is part of a series of toy examples. Each example is organized in its own subdirectory under `examples/`.

## Overview

The 3-class network demonstrates noise-robust classification using an inhibitory pool that suppresses competing output neurons. The network can correctly classify inputs even with 5-20% noisy sensory activations.

### Network Architecture

- **Sensory Neurons**: `S0`, `S1`, `S2` (one-hot class inputs)
- **Interneurons**:
  - `I` = Inhibitory pool (leak=0.8)
  - `N0` = `O0` (class 0 output)
  - `N1` = `O1` (class 1 output)
  - `N2` = `O2` (class 2 output)

### Configuration

The network is configured via `3class_network.net` with:
- **Thresholds**: I=40, O0=O1=O2=50
- **Weights**: 
  - Feedforward: S→O: +60
  - Pool feedback: O→I: +35, I→O: -45
- **Leak**: I uses leak=0.8 for gradual decay; outputs use leak=1.0 (no decay)

## Files

- **3class_test.cpp**: Test harness that runs classification tests with varying noise levels
- **3class_network.net**: Network configuration file
- **Makefile**: Build configuration (references arch/ for core classes)
- **../../arch/output_detection.h**: Firing rate tracker for classification
- **../../arch/glia.cpp/h**: Network management class
- **../../arch/neuron.cpp/h**: Individual spiking neuron implementation

## Compilation

### Using Make (Linux/Mac/WSL)

```bash
make
```

Or manually build:

```bash
make clean
make
```

### Using g++ directly (Windows/Linux/Mac)

```bash
g++ -std=c++11 -Wall -O2 -o 3class_test 3class_test.cpp ../../arch/glia.cpp ../../arch/neuron.cpp
```

### Using MSVC (Windows)

```cmd
cl /EHsc /std:c++17 /Fe:3class_test.exe 3class_test.cpp ..\..\arch\glia.cpp ..\..\arch\neuron.cpp
```

## Running the Test

### Unix-like systems:
```bash
./3class_test
```

### Windows:
```cmd
3class_test.exe
```

**Important**: The executable must be run from the `examples/3class/` directory (where `3class_network.net` is located).

## Test Structure

The test runs four sets of experiments:

1. **No noise** (0%): Sanity check - should achieve 100% accuracy
2. **Low noise** (5%): Should maintain correct classification
3. **Medium noise** (10%): Should maintain correct classification
4. **High noise** (20%): Should maintain correct classification with reduced margin

For each noise level, all three classes (0, 1, 2) are tested over 100 simulation ticks.

## Expected Output

The program will test each class with varying noise levels and display:

1. **Configuration loading**: Shows neurons and connections being set up
2. **For each test**:
   - True class label
   - Noise percentage
   - Actual noise activations (random)
   - Firing rates for all output neurons (N0, N1, N2)
   - Winner via argmax classification
   - Predicted class
   - Correctness indicator (✓/✗)
   - Margin (confidence metric)

### Example Output Snippet

```
=== Testing class 1 with 10% noise ===
Noise activations: S0=8 S2=12
Firing rates after 100 ticks:
  N0: 0.123
  N1: 0.487
  N2: 0.105
Winner (argmax): N1
Predicted class: 1
Result: ✓ CORRECT
Expected class: 1
Margin (confidence): 0.364
```

### Expected Behavior

- **No noise**: Perfect classification with high margins (~0.5)
- **5-10% noise**: Correct classification maintained, margins reduced slightly
- **20% noise**: Correct classification maintained, margins may be lower
- **Inhibitory pool**: Suppresses competing outputs, maintains winner-take-all dynamics

## Implementation Details

### Firing Rate Tracking

The test uses an **Exponential Moving Average (EMA)** to track firing rates:

```
rate_k ← (1−α) × rate_k + α × [fired_k]
```

- **α = 0.05** (approximately 1/20 as suggested in TOY_EXAMPLES.md)
- Updated every tick for each output neuron

### Noise Injection

On each tick:
1. The true class sensory neuron always receives input (200.0)
2. Each other sensory neuron receives input with probability `noise_prob`

```cpp
// Always activate true class
network.injectSensory("S" + std::to_string(true_class), 200.0f);

// Noisy activations
for (int c = 0; c < 3; ++c) {
    if (c != true_class && random() < noise_prob) {
        network.injectSensory("S" + std::to_string(c), 200.0f);
    }
}
```

### Inhibitory Pool Mechanism

The inhibitory pool (`I`) acts as a winner-take-all circuit:
1. All output neurons excite `I` (+35 weight)
2. `I` inhibits all output neurons (-45 weight)
3. `I` has leak=0.8, so it gradually decays between activations
4. The strongest output dominates and suppresses competitors

### Margin Computation

The margin measures classification confidence:

```
margin = max_rate - second_max_rate
```

Higher margins indicate stronger separation between classes.

## Troubleshooting

### File not found error
- Ensure `3class_network.net` is in the same directory as the executable
- Or modify `3class_test.cpp` to use an absolute path

### Compilation errors
- Ensure you have C++11 or later support (C++17 recommended for MSVC)
- Verify include paths point to `../../arch/` correctly
- Check that `output_detection.h` is in `../../arch/` directory

### Unexpected results
- Check that neuron parameters match the toy example specification
- Verify leak parameter for pool neuron is set to 0.8
- Check that all weights are correctly configured (especially inhibitory connections)
- Increase number of ticks if rates haven't stabilized (default is 100)

### Low accuracy with noise
- Verify inhibitory pool connections are correct
- Check that pool threshold is appropriate (40 should work)
- Ensure leak parameter for pool is set correctly (0.8)

## Next Steps

After verifying the 3-class network works:

1. **Experiment with noise levels**: Try different noise probabilities (30%, 40%, etc.)
2. **Tune inhibitory pool**: Adjust pool threshold, leak, or connection weights
3. **Add visualization**: Plot firing rates over time for each neuron
4. **Implement training**: Use this as a baseline to compare against learned weights
5. **Try temporal examples**: Implement the AB/BA temporal order detection example
6. **Add abstention**: Use margin thresholds to abstain from low-confidence predictions

## Network Behavior

### Without Inhibitory Pool
If you remove the inhibitory pool, multiple outputs may fire with similar rates when noise is present, leading to ambiguous classifications.

### With Inhibitory Pool
The pool implements lateral inhibition:
- The strongest output activates first
- It drives the pool, which suppresses other outputs
- This creates a winner-take-all dynamic
- Result: Clean separation even with noise

## File Format: .net Configuration Files

The `.net` format is simple and human-readable:

```
# Comments start with #
NEURON <id> <threshold> <leak> <resting>
CONNECTION <from_id> <to_id> <weight>
```

You can:
- Save networks: `network.saveNetworkToFile("my_network.net")`
- Load networks: `network.configureNetworkFromFile("my_network.net")`

This allows you to snapshot trained networks or manually design architectures.
