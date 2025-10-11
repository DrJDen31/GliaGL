# XOR Neural Network Test

This directory contains a manually configured spiking neural network that implements the XOR function as described in `docs/TOY_EXAMPLES.md`.

**Location:** `src/testing/xor/`

This is part of a series of toy examples. Each example is organized in its own subdirectory under `src/testing/`.

## Overview

The XOR network demonstrates that the manually configured network (before training) can correctly perform XOR classification based on firing rate analysis.

### Network Architecture

- **Sensory Neurons**: `S0`, `S1` (inputs)
- **Interneurons**:
  - `N0` = `A` (AND detector with coincidence detection, leak=0)
  - `O1` (XOR true output)
  - `O0` (XOR false output)

### Configuration

The network is configured via `xor_network.net` with:
- **Thresholds**: A=90, O1=50, O0=60
- **Weights**: As specified in the toy example (S→O1: +60, S→A: +60, A→O1: -120, A→O0: +120)
- **Leak**: A uses leak=0 for clean coincidence detection

## Files

- **xor_test.cpp**: Test harness that runs all XOR input combinations
- **xor_network.net**: Network configuration file
- **Makefile**: Build configuration (references arch/ for core classes)
- **../../arch/output_detection.h**: Output detector (EMA) for classification
- **../../arch/glia.cpp/h**: Network management class (creates/manages neurons)
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
g++ -std=c++11 -Wall -O2 -o xor_test xor_test.cpp ../../arch/glia.cpp ../../arch/neuron.cpp
```

### Using MSVC (Windows)

```cmd
cl /EHsc /std:c++17 /Fe:xor_test.exe xor_test.cpp ..\..\arch\glia.cpp ..\..\arch\neuron.cpp
```

## Running the Test

### Unix-like systems:
```bash
./xor_test
```

### Windows:
```cmd
xor_test.exe
```

**Important**: The executable must be run from the `src/testing/xor/` directory (where `xor_network.net` is located).

## Expected Output

The program will test all four XOR input combinations (00, 01, 10, 11) and display:

1. **Configuration loading**: Shows neurons and connections being set up
2. **For each input**:
   - Input pattern (e.g., "01")
   - Firing rates for output neurons O1 (true) and O0 (false)
   - Winner via argmax classification (EMA)
   - XOR result interpretation
   - Expected result for comparison

### Example Output Snippet

```
=== Testing input: 01 ===
Firing rates after 100 ticks:
  O1: 0.500
  O0: 0.490
Winner (argmax): O1
XOR Result: TRUE (1)
Expected: TRUE (1)
```

### Expected Results

- **00** → FALSE (O0 wins, both rates ≈0)
- **01** → TRUE (O1 wins, rate ≈0.50)
- **10** → TRUE (O1 wins, rate ≈0.50)
- **11** → FALSE (O0 wins, rate ≈0.98)

## Implementation Details

### Output Detection (EMA)

The test uses an **Exponential Moving Average (EMA)**-based detector to track rates and select the winning output.
Alpha defaults to 0.05 (≈1/20). See `../../arch/output_detection.h` for API.

### Sensory Injection

Sensory neurons receive a strong input (200.0) when their corresponding bit is 1:

```cpp
if (input0 == 1) network.injectSensory("S0", 200.0f);
if (input1 == 1) network.injectSensory("S1", 200.0f);
```

## Output Detection Design

The current implementation uses **Option 1: External Tracker Class** as described in `docs/OUTPUT_DETECTION_OPTIONS.md`.

The `FiringRateTracker` class (in `../../arch/output_detection.h`) is part of the core architecture and provides:
- Exponential Moving Average (EMA) tracking
- Argmax classification
- Margin computation for confidence metrics
- Clean, reusable API

This approach:
- ✓ Decouples detection from network simulation
- ✓ Allows easy experimentation with different tracking strategies
- ✓ Maintains clean separation of concerns
- ✓ Part of core architecture, usable in both testing and production

## Troubleshooting

### File not found error
- Ensure `xor_network.net` is in the same directory as the executable
- Or modify `xor_test.cpp` to use an absolute path

### Compilation errors
- Ensure you have C++11 or later support
- Verify include paths point to `../../arch/` correctly
- Check that `output_detection.h` is in `../../arch/` directory

### Unexpected results
- Check that neuron parameters match the toy example specification
- Verify leak parameter for AND neuron is set to 0.0 (for clean coincidence detection)
- Increase number of ticks if rates haven't stabilized (default is 100)

## Next Steps

After verifying the XOR network works:

1. **Experiment with parameters**: Try different leak values, thresholds, or weights
2. **Add visualization**: Track firing rates over time (plot)
3. **Implement training**: Use this as a baseline to compare against learned weights
4. **Try other toy examples**: Implement the 3-class or temporal AB/BA examples
5. **Add confidence metrics**: Implement margin-based classification (see OUTPUT_DETECTION_OPTIONS.md)

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
