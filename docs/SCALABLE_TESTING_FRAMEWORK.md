# Scalable Testing Framework

## Overview

The visualizer now supports a **fully scalable testing system** with:
- **Dynamic test loading** from files
- **Unlimited test sequences** (not hardcoded)
- **Both slow-motion AND fast-forward** modes
- **Alt+1-9 for custom tests**, F1-F9 for built-in tests

## Architecture

### Test File Format (.seq)

Simple text format for defining input sequences:

```
# Comments start with #
DURATION 200          # Total duration (informational)
LOOP false            # Whether to loop (true/false)

# Events: TICK NEURON_ID VALUE
0 S0 200.0           # At tick 0, inject 200.0 to S0
0 S1 200.0           # Multiple events can occur at same tick
5 S2 150.0           # At tick 5, inject 150.0 to S2
10 S0 0.0            # At tick 10, turn off S0
```

### Command Line Usage

```cmd
# Load network with custom test files
vis.exe --network network.net --tests test1.seq test2.seq test3.seq

# Tests are mapped to Alt+1, Alt+2, Alt+3, etc.
# Up to 9 tests supported (Alt+1 through Alt+9)
```

### Keyboard Controls

| Keys | Purpose | Range |
|------|---------|-------|
| **`,`** / **`.`** | Slow-motion (delay per tick) | 0-5 seconds/tick |
| **`+`** / **`-`** | Fast-forward (ticks per frame) | 1-100 ticks/frame |
| **Alt+1-9** | Load custom test (from --tests) | User-defined |
| **F1-F9** | Load built-in test | Hardcoded |
| **F5** | Clear test (manual control) | - |

## Speed Control Philosophy

### Slow-Motion Mode (`,` / `.`)
- **Purpose**: Careful observation of individual events
- **Range**: 0.0 to 5.0 seconds per tick
- **Use case**: Understanding dynamics, debugging, teaching
- **Example**: Set 1.0s/tick to watch WTA suppression in detail

### Fast-Forward Mode (`+` / `-`)
- **Purpose**: Quick testing of long sequences
- **Range**: 1 to 100 ticks per frame
- **Use case**: Batch testing, finding steady-state, skipping boring parts
- **Example**: Set 50 ticks/frame to run through 500-tick test quickly

### Combined Usage
**You can use BOTH simultaneously!**
- Set slow-motion to 0.5s/tick (careful observation)
- Set fast-forward to 1 tick/frame (default, no skipping)
- **OR** set fast-forward to 10 ticks/frame + 0.0s/tick (fast batch mode)

## Creating Test Files

### Option 1: Manual Creation

```
# test_my_experiment.seq
DURATION 100
LOOP false

# Activate S0 for first 50 ticks
0 S0 200.0
50 S0 0.0

# Activate S1 for next 50 ticks
50 S1 200.0
100 S1 0.0
```

### Option 2: Programmatic Generation

Use the provided generator:

```bash
cd examples/3class
make -f Makefile_gentests
./generate_tests
```

**Output**:
- `test_class0_5pct.seq` - Class 0, 5% noise, 200 ticks
- `test_class1_10pct.seq` - Class 1, 10% noise, 200 ticks
- `test_class2_20pct.seq` - Class 2, 20% noise, 200 ticks
- `test_xor.seq` - XOR patterns, 100 ticks each

### Option 3: Custom Generator

```cpp
#include <fstream>

void generateCustomTest(const std::string& filename) {
    std::ofstream file(filename);
    file << "# My Custom Test\n";
    file << "DURATION 500\n";
    file << "LOOP false\n\n";
    
    for (int t = 0; t < 500; t++) {
        if (t % 10 == 0) {
            file << t << " S0 200.0\n";
        }
        // ... custom logic ...
    }
    
    file.close();
}
```

## Complete Workflow Example

### Step 1: Generate Test Files

```bash
cd examples/3class
make clean && make
./3class_test
./generate_tests
```

### Step 2: Launch with Tests

```cmd
cd build
cmake --build . --clean-first
debug\vis.exe --network ../examples/3class/3class_network.net --size 1500 1500
            --tests ../examples/3class/test_class0_5pct.seq ^
                      ../examples/3class/test_class1_10pct.seq ^
                      ../examples/3class/test_class2_20pct.seq ^
              --size 1500 1500
```

**Console output**:
```
Loading 3 test sequence file(s)...
  [1] Loaded: ../examples/3class/test_class0_5pct.seq (200 ticks, one-shot)
  [2] Loaded: ../examples/3class/test_class1_10pct.seq (200 ticks, one-shot)
  [3] Loaded: ../examples/3class/test_class2_20pct.seq (200 ticks, one-shot)

Press Alt+[1-9] to load a test, or F1-F9 for built-in tests
```

### Step 3: Run Tests

```
1. Press 'I' for inference mode
2. Press Alt+1 to load first test
3. Press ',' five times → 0.5s/tick (slow-motion)
4. Press 'A' to start
5. Watch test run slowly
6. Press 'X' to stop
7. Press Alt+2 to load second test
8. Press 'R' to reset tick counter
9. Press '.' five times → 0.0s/tick (real-time)
10. Press 'A' to start
```

## Scalability Features

### Unlimited Tests
- Not limited to F1-F12 (only 12 tests)
- Load as many .seq files as needed
- First 9 accessible via Alt+1-9
- Others can be loaded by command-line restart or future file picker

### Network-Agnostic
- Test files reference neuron IDs (S0, S1, etc.)
- Works with any network architecture
- Same test file format for 3-class, XOR, or custom networks

### Flexible Speed Control
- **Slow networks** (few neurons): Use slow-motion (0.5-2.0s/tick)
- **Fast networks** (many neurons): Use fast-forward (10-50 ticks/frame)
- **Long tests** (1000+ ticks): Use extreme fast-forward (100 ticks/frame)

### Easy Test Creation
- Simple text format (human-readable)
- Programmatic generation (for complex patterns)
- Copy/modify existing tests

## Advanced Usage

### Multiple Test Suites

```cmd
# Suite 1: Noise robustness
vis.exe --network 3class.net --tests noise_5pct.seq noise_10pct.seq noise_20pct.seq

# Suite 2: Temporal patterns
vis.exe --network 3class.net --tests burst.seq ramp.seq oscillation.seq

# Suite 3: Edge cases
vis.exe --network 3class.net --tests all_on.seq all_off.seq rapid_switch.seq
```

### Batch Testing Script

```bash
# test_all.sh
for noise in 5 10 15 20 25; do
    echo "Testing ${noise}% noise..."
    ./vis.exe --network network.net --tests test_noise_${noise}pct.seq
    # Wait for user to observe...
done
```

### Test File Library

Organize tests by purpose:

```
tests/
  3class/
    baseline/
      test_class0_clean.seq
      test_class1_clean.seq
      test_class2_clean.seq
    noise/
      test_class0_5pct.seq
      test_class0_10pct.seq
      ...
    temporal/
      test_burst.seq
      test_ramp.seq
      ...
  xor/
    test_all_patterns.seq
    test_pattern_00.seq
    test_pattern_01.seq
    ...
  custom/
    ...
```

### Automated Test Generation

Create generators for different test categories:

```cpp
// generate_noise_tests.cpp
void generateNoiseSweep() {
    for (float noise = 0.0f; noise <= 0.50f; noise += 0.05f) {
        std::string filename = "test_noise_" + std::to_string(int(noise*100)) + "pct.seq";
        generate3ClassNoiseTest(0, noise, 200, filename);
    }
}

// generate_temporal_tests.cpp
void generateBurstTest() { /* ... */ }
void generateRampTest() { /* ... */ }
void generateOscillationTest() { /* ... */ }
```

## Future Enhancements

### Short Term
- [x] Test file format (.seq)
- [x] Command-line test loading (--tests)
- [x] Alt+1-9 key bindings
- [ ] File picker dialog (native UI)
- [ ] Test library browser (in-app menu)

### Medium Term
- [ ] Test recording (capture manual inputs → .seq file)
- [ ] Test editing UI (modify sequences in real-time)
- [ ] Test validation (check neuron IDs exist)
- [ ] Test metadata (author, description, expected outcome)

### Long Term
- [ ] Test result logging (CSV export)
- [ ] Automated pass/fail checking
- [ ] Test suite management (JSON manifests)
- [ ] Cloud test library (share tests online)

## Comparison: Old vs New

| Feature | Old (F1-F12) | New (--tests) |
|---------|--------------|---------------|
| **Max tests** | 12 | Unlimited (9 per session) |
| **Test creation** | Edit C++ code | Create .seq file |
| **Recompile needed?** | Yes | No |
| **Shareable?** | No | Yes (.seq files) |
| **Network-specific?** | Yes | No |
| **Version control** | Hard (C++ code) | Easy (text files) |
| **Test organization** | Flat | Hierarchical (folders) |

## File Format Specification

### Grammar

```
test_file      ::= (comment | command | event)*
comment        ::= '#' text '\n'
command        ::= 'DURATION' integer '\n'
                 | 'LOOP' boolean '\n'
event          ::= integer neuron_id float '\n'
                 | 'EVENT' integer neuron_id float '\n'
neuron_id      ::= 'S' integer | 'N' integer | 'O' integer | string
boolean        ::= 'true' | 'false' | '1' | '0'
```

### Examples

**Minimal test**:
```
0 S0 200.0
```

**Complete test**:
```
# My test
DURATION 100
LOOP false

0 S0 200.0
50 S0 0.0
50 S1 200.0
```

**Complex test**:
```
# Burst pattern
DURATION 500
LOOP true

# Burst 1
0 S0 250.0
5 S0 0.0

# Burst 2
100 S1 250.0
105 S1 0.0

# Burst 3
200 S2 250.0
205 S2 0.0
```

## Tips & Best Practices

### Test Design
1. **Start small** - 100-200 ticks for initial tests
2. **Use comments** - Document what each section does
3. **Test one thing** - Each file should test one hypothesis
4. **Name clearly** - `test_class0_noise5pct.seq` not `test1.seq`

### Speed Selection
- **0.5-1.0s/tick**: First time viewing, debugging
- **0.0s/tick + 1 tick/frame**: Normal observation
- **0.0s/tick + 10-20 ticks/frame**: Quick verification
- **0.0s/tick + 50-100 ticks/frame**: Batch testing, long sequences

### Key Bindings
- **Alt+1-9**: Your custom tests (--tests flag)
- **F1-F9**: Built-in tests (hardcoded, for backwards compatibility)
- **F5**: Clear test (return to manual control)

### File Organization
```
project/
  tests/
    network1/
      suite1/
        test1.seq
        test2.seq
      suite2/
        ...
    network2/
      ...
```

---

**The testing framework is now fully scalable!** 🚀

Add as many tests as you need, no recompilation required.
