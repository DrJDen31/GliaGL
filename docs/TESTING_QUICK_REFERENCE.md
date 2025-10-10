# Testing Framework - Quick Reference

## Quick Start

```cmd
# 1. Generate test files (each network type has its own generator)
cd src/testing/3class
make -f Makefile_gentests && ./generate_tests

# 2. Launch with tests
cd ../../../build
debug\vis.exe --network ../src/testing/3class/3class_network.net ^
              --tests ../src/testing/3class/test_class0_5pct.seq ^
                      ../src/testing/3class/test_class1_10pct.seq ^
                      ../src/testing/3class/test_class2_20pct.seq

# 3. Run test
Press 'I' for inference mode
Press '1' to load first test
Press ',' or '.' to adjust speed
Press 'A' to animate
```

## Keyboard Controls

| Key | Action | Details |
|-----|--------|---------|
| **1-9** | Load test | Load test sequence from --tests flag |
| **0** | Clear test | Return to manual control mode |
| **`,`** | Slower | Multiply seconds/tick by 2 (logarithmic) |
| **`.`** | Faster | Divide seconds/tick by 2 (logarithmic) |
| **A** | Animate | Start continuous animation |
| **X** | Stop | Stop animation |
| **Space** | Step | Single network step |
| **R** | Reset | Reset tick counter to 0 |
| **C** | Toggle counter | Show/hide tick counter |
| **I** | Inference mode | Run network (default) |
| **T** | Training mode | Physics-based layout |

## Speed Control

**Logarithmic scale**: 5.0 s/tick ↔ 0.001 s/tick

| Speed | s/tick | ticks/s | Use Case |
|-------|--------|---------|----------|
| Very Slow | 5.0 | 0.2 | Detailed observation |
| Slow | 2.5 | 0.4 | Careful analysis |
| | 1.25 | 0.8 | |
| **Default** | **1.0** | **1.0** | **Natural pace** |
| | 0.5 | 2.0 | |
| Fast | 0.25 | 4.0 | |
| | 0.125 | 8.0 | Quick testing |
| Very Fast | 0.0625 | 16.0 | |
| | 0.03125 | 32.0 | Batch testing |
| Ultra Fast | 0.016 | ~60 | ~1 tick/frame |
| Maximum | 0.001 | 1000 | Fast-forward |

**Press `,` to slow down (×2), `.` to speed up (÷2)**

## Test File Format (.seq)

```
# Comments start with #
DURATION 200          # Informational
LOOP false            # Loop test? true/false

# Events: TICK NEURON_ID VALUE
0 S0 200.0           # Tick 0: activate S0 with value 200.0
5 S1 150.0           # Tick 5: activate S1 with value 150.0
10 S0 0.0            # Tick 10: deactivate S0
```

## Creating Test Files

### Each Network Type Has Its Own Generator

```
src/testing/
  3class/
    generate_tests.cpp       # 3-class network generator
    Makefile_gentests
  xor/
    generate_tests.cpp       # XOR network generator (to be created)
    Makefile_gentests
  your_network/
    generate_tests.cpp       # Your network generator
    Makefile_gentests
```

### Generate Tests

```bash
cd src/testing/3class
make -f Makefile_gentests
./generate_tests

# Creates:
# - test_class0_5pct.seq
# - test_class1_10pct.seq  
# - test_class2_20pct.seq
# - test_xor.seq
```

### Modify Generator for Your Needs

Edit `generate_tests.cpp`:

```cpp
int main() {
    // Customize parameters
    generate3ClassNoiseTest(0, 0.05f, 200, "test_class0_5pct.seq");
    generate3ClassNoiseTest(0, 0.10f, 200, "test_class0_10pct.seq");
    generate3ClassNoiseTest(0, 0.15f, 200, "test_class0_15pct.seq");
    
    // Add your custom tests
    generateCustomTest(params, "test_custom.seq");
    
    return 0;
}
```

## Example Workflow

### 1. Slow-Motion Observation

```
Start: 1.0 s/tick (default)
Press ',' five times:
  1.0 → 2.0 → 4.0 → 5.0 (hit max) → 5.0
Result: 5 seconds per tick
Use: Watch individual neuron firings
```

### 2. Fast-Forward Testing

```
Start: 1.0 s/tick (default)
Press '.' ten times:
  1.0 → 0.5 → 0.25 → 0.125 → 0.0625 → 0.03125 → 0.016 → 0.008 → 0.004 → 0.002 → 0.001
Result: 0.001 seconds per tick = 1000 ticks/sec
Use: Run through long test sequences quickly
```

### 3. Find Optimal Speed

```
1. Load test (press '1')
2. Start too slow (default 1.0 s/tick might be slow)
3. Press '.' a few times until comfortable
4. If too fast, press ',' to slow down
5. Find sweet spot for your observation needs
```

## Command Line Examples

### Single Test

```cmd
vis.exe --network network.net --tests test1.seq
# Press '1' to load
```

### Multiple Tests

```cmd
vis.exe --network network.net --tests test1.seq test2.seq test3.seq
# Press '1', '2', or '3' to load
```

### Full Suite

```cmd
vis.exe --network ../src/testing/3class/3class_network.net ^
        --tests ../src/testing/3class/test_class0_5pct.seq ^
                ../src/testing/3class/test_class1_10pct.seq ^
                ../src/testing/3class/test_class2_20pct.seq ^
                ../src/testing/3class/test_class0_10pct.seq ^
                ../src/testing/3class/test_class1_20pct.seq ^
                ../src/testing/3class/test_class2_30pct.seq ^
        --size 1500 1500
# Tests mapped to keys 1-6
```

## Tips

### Speed Selection
- **First viewing**: Start at 1-2 s/tick
- **Analysis**: 0.5-1.0 s/tick
- **Verification**: 0.1-0.5 s/tick
- **Batch testing**: 0.001-0.01 s/tick

### Test Organization
```
tests/
  baseline/
    test_class0_clean.seq
    test_class1_clean.seq
  noise_sweep/
    test_noise_5pct.seq
    test_noise_10pct.seq
    test_noise_15pct.seq
  temporal/
    test_burst.seq
    test_oscillation.seq
```

### Reload Tests
To load different test files, restart visualizer with new `--tests` arguments.

### Manual Control
Press `0` to clear any loaded test and return to manual control (sliders/keyboard).

---

**Default speed: 1 tick/second**  
**Adjust with `,` (slower) and `.` (faster)**  
**Range: 0.2 to 1000 ticks/second**
