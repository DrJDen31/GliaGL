# Testing Framework - Implementation Summary

## What Was Implemented

### ✅ Scalable Test Loading
- **Command line**: `--tests test1.seq test2.seq test3.seq`
- **Keys 1-9**: Load tests directly (no Alt+ modifier needed)
- **Key 0**: Clear test (return to manual control)
- **Unlimited tests**: Up to 9 per session, restart for more

### ✅ Unified Logarithmic Speed Control
- **Single time controller**: 5.0 s/tick to 0.001 s/tick
- **Logarithmic scaling**: Press `,` or `.` to multiply/divide by 2
- **Range**: 0.2 ticks/s (very slow) to 1000 ticks/s (very fast)
- **Default**: 1.0 s/tick (1 tick per second)

### ✅ Test File Format (.seq)
```
# Comments
DURATION 200
LOOP false

# Events: TICK NEURON_ID VALUE
0 S0 200.0
5 S1 150.0
```

### ✅ Test Generators
- **Per-network generators**: Each network type has its own `generate_tests.cpp`
- **Makefile**: `Makefile_gentests` for building
- **Example**: `examples/3class/generate_tests.cpp`

### ✅ Clean Architecture
- Removed F-key tests (no backwards compatibility baggage)
- Removed Alt+ modifier (unnecessary complexity)
- Removed dual speed controls (unified into one logarithmic scale)
- Removed sensory neuron toggles on 1-9 (not scalable)

## Key Design Decisions

### 1. One Generator Per Network
**Why**: Each network has different architecture and test needs.
- 3-class: Noise robustness tests
- XOR: Pattern sequence tests
- Custom: Your specific tests

**Benefit**: Simple, focused, easy to modify.

### 2. Logarithmic Speed Control
**Why**: Need to cover 5000x range (0.2 to 1000 ticks/s).

**Math**:
- Press `,` → multiply by 2 (slower)
- Press `.` → divide by 2 (faster)
- 10 presses = 1024x change

**Benefit**: Quick to reach any speed, predictable steps.

### 3. Keys 1-9 for Test Loading
**Why**: Removed sensory neuron toggles for scalability.

**Rationale**:
- Networks can have 100+ sensory neurons
- Can't map all to keyboard
- Mouse sliders already provide manual control
- Test sequences are the scalable solution

**Benefit**: Clean mapping, no modifier keys needed.

### 4. File-Based Tests (Not Hardcoded)
**Why**: Avoid recompilation for new tests.

**Benefit**:
- Share tests easily
- Version control friendly
- Network-agnostic format
- Quick iteration

## Usage Flow

```
┌─────────────────────────────────────────────────┐
│ 1. Generate Tests (per network)                │
│    cd examples/3class                          │
│    make -f Makefile_gentests && ./generate_tests│
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│ 2. Launch Visualizer with Tests                 │
│    vis.exe --network network.net \              │
│            --tests test1.seq test2.seq          │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│ 3. Load & Run Test                              │
│    Press '1' to load test                       │
│    Press ',' or '.' to adjust speed             │
│    Press 'A' to animate                         │
└─────────────────────────────────────────────────┘
```

## Speed Scale Reference

| Presses | Speed (s/tick) | Speed (ticks/s) | Use Case |
|---------|----------------|-----------------|----------|
| `,,,,,` | 5.0 | 0.2 | Very slow observation |
| `,,,` | 2.0 | 0.5 | Slow observation |
| `,` | 1.0 | 1.0 | **Default** |
| `.` | 0.5 | 2.0 | Slightly faster |
| `...` | 0.125 | 8.0 | Fast |
| `.....` | 0.03125 | 32.0 | Very fast |
| `........` | 0.004 | 250.0 | Ultra fast |
| `..........` | 0.001 | 1000.0 | Maximum |

## File Locations

```
GliaGL/
├── src/
│   ├── arch/              # Core network code
│   │   ├── glia.cpp
│   │   ├── glia.h
│   │   └── output_detection.h
│   │
│   ├── vis/               # Visualizer code
│   │   ├── input_sequence.h      # NEW: Test sequence format
│   │   ├── argparser.cpp          # MODIFIED: --tests flag
│   │   ├── argparser.h            # MODIFIED: loaded_tests
│   │   ├── meshdata.cpp           # MODIFIED: Unified speed control
│   │   ├── meshdata.h             # MODIFIED: seconds_per_tick
│   │   └── OpenGLCanvas.cpp       # MODIFIED: Key bindings
│   │
│   └── examples/
│       ├── 3class/
│       │   ├── generate_tests.cpp       # NEW: Test generator
│       │   ├── Makefile_gentests        # NEW: Build generator
│       │   ├── 3class_network.net
│       │   ├── 3class_test.cpp
│       │   └── test_*.seq               # Generated tests
│       │
│       ├── xor/
│       │   └── (similar structure)
│       │
│       └── TEST_GENERATORS_README.md    # NEW: Documentation
│
└── docs/
    ├── TESTING_QUICK_REFERENCE.md       # NEW: Quick guide
    ├── SCALABLE_TESTING_FRAMEWORK.md    # NEW: Detailed docs
    └── TESTING_FRAMEWORK_SUMMARY.md     # NEW: This file
```

## Keyboard Reference Card

### Test Control
- **1-9**: Load test 1-9
- **0**: Clear test (manual mode)
- **R**: Reset to tick 0

### Speed Control
- **`,`**: Slower (×2)
- **`.`**: Faster (÷2)

### Playback
- **A**: Animate
- **X**: Stop
- **Space**: Single step

### Display
- **I**: Inference mode
- **T**: Training mode
- **C**: Toggle tick counter
- **[** / **]**: Adjust neuron size

## Example Session

```cmd
# Terminal 1: Generate tests
cd examples/3class
make -f Makefile_gentests
./generate_tests
# Output:
#   Generated: test_class0_5pct.seq
#   Generated: test_class1_10pct.seq
#   Generated: test_class2_20pct.seq
#   Generated: test_xor.seq

# Terminal 2: Launch visualizer
cd ../../../build
debug\vis.exe --network ../examples/3class/3class_network.net ^
              --tests ../examples/3class/test_class0_5pct.seq ^
                      ../examples/3class/test_class1_10pct.seq ^
                      ../examples/3class/test_class2_20pct.seq

# Console output:
#   Loading 3 test sequence file(s)...
#     [1] Loaded: test_class0_5pct.seq (200 ticks, one-shot)
#     [2] Loaded: test_class1_10pct.seq (200 ticks, one-shot)
#     [3] Loaded: test_class2_20pct.seq (200 ticks, one-shot)
#   
#   Press [1-9] to load a test, [0] to clear

# In visualizer:
Press 'I'         # Inference mode
Press '1'         # Load test 1
Press ','         # Slow down to 2.0 s/tick
Press ','         # Slow down to 4.0 s/tick
Press 'A'         # Start animation
# Watch test run slowly...
Press 'X'         # Stop
Press '2'         # Load test 2
Press '.'         # Speed up to 2.0 s/tick
Press '.'         # Speed up to 1.0 s/tick (back to default)
Press 'A'         # Start again
```

## Next Steps

### For Users
1. Generate tests: `cd examples/3class && make -f Makefile_gentests && ./generate_tests`
2. Launch visualizer with `--tests` flag
3. Press 1-9 to load tests, adjust speed with `,` and `.`

### For Developers (New Networks)
1. Create `examples/your_network/` directory
2. Copy `generate_tests.cpp` template from `3class/`
3. Customize generator for your network's test scenarios
4. Build and run generator to create .seq files
5. Launch visualizer with your network and tests

### For Advanced Testing
1. Create test suites (multiple .seq files per scenario)
2. Use generator to create parametric sweeps
3. Organize tests in subdirectories (baseline/, noise/, temporal/)
4. Document expected outcomes in .seq comments

## Summary of Changes

| Component | Change | Benefit |
|-----------|--------|---------|
| Speed control | Unified logarithmic | 5000x range, simple controls |
| Test loading | File-based (--tests) | No recompilation, shareable |
| Key bindings | 1-9 load tests | Clean, no modifiers |
| Test format | .seq text files | Human-readable, version-controllable |
| Generators | Per-network | Simple, focused, maintainable |
| F-keys | Removed | No legacy baggage |
| Alt+ | Removed | Unnecessary complexity |
| Dual controls | Unified | One scale, not two |

---

**The testing framework is now fully scalable and production-ready!** 🚀

**Default**: 1 tick/second  
**Adjust**: `,` (slower) or `.` (faster)  
**Load tests**: Keys 1-9  
**Generate tests**: One generator per network type
