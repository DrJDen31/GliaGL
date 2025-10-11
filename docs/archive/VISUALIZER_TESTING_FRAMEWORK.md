# Visualizer Testing Framework

## Overview

The visualizer now supports **pre-programmed input sequences** for complex temporal testing, with adjustable tick speed and tick counting for precise experimental control.

## New Features

### 1. **Tick Counter**
- Tracks simulation ticks independently of rendering frames
- Displayed in console (UI text rendering TODO)
- Can be reset with `R` key

### 2. **Adjustable Simulation Speed**
- Control how many ticks run per rendering frame
- Range: 1-100 ticks/frame
- Allows slow-motion (1 tick/frame) or fast-forward (100 ticks/frame)

### 3. **Input Sequences**
- Pre-programmed temporal input patterns
- Defined in code or loaded from files
- Automatically advances each tick
- Supports looping for repeated tests

## Keyboard Controls

### Simulation Control
- **`,`** (comma): Decrease speed (fewer ticks/frame)
- **`.`** (period): Increase speed (more ticks/frame)
- **`C`**: Toggle tick counter display
- **`R`**: Reset tick counter and sequence to beginning

### Existing Controls
- **`A`**: Start continuous animation
- **`X`**: Stop animation
- **`N`**: Single network step
- **`I`**: Inference mode
- **`T`**: Training mode
- **`1-9`**: Toggle sensory inputs S0-S8
- **`0`**: Clear all inputs

## Input Priority System

Inputs are applied in this priority order:
1. **Input Sequence** (highest) - overrides all manual controls
2. **Keyboard toggles** (keys 1-9, 0)
3. **Mouse sliders** (lowest)

When an input sequence is active, manual controls are ignored.

## Creating Input Sequences

### In Code

```cpp
#include "input_sequence.h"

// Create a sequence
InputSequence* seq = new InputSequence();

// Add events: tick number, neuron ID, input value
seq->addEvent(0, "S0", 200.0f);      // Tick 0: activate S0
seq->addEvent(100, "S0", 0.0f);      // Tick 100: deactivate S0
seq->addEvent(100, "S1", 200.0f);    // Tick 100: activate S1
seq->addEvent(200, "S1", 0.0f);      // Tick 200: deactivate S1

// Optional: enable looping
seq->setLoop(true);

// Assign to visualizer
GLOBAL_args->input_sequence = seq;
```

### Predefined Test Sequences

#### 3-Class Noise Test
```cpp
// Test class 0 with 5% noise for 200 ticks
InputSequence* seq = create3ClassNoiseTest(
    0,        // class_id (0, 1, or 2)
    0.05f,    // noise_prob (5%)
    200       // duration in ticks
);
GLOBAL_args->input_sequence = seq;
```

**Behavior**:
- Activates correct sensory neuron every tick (e.g., S0)
- Randomly activates other sensory neurons with 5% probability
- Visualizes signal-vs-noise dynamics in real-time

#### XOR Test
```cpp
// Test all 4 XOR patterns, 100 ticks each
InputSequence* seq = createXORTest(100);  // ticks_per_pattern
GLOBAL_args->input_sequence = seq;
```

**Pattern sequence**:
- Ticks 0-99: 00 (both off)
- Ticks 100-199: 01 (S1 only)
- Ticks 200-299: 10 (S0 only)
- Ticks 300-399: 11 (both on)

## Usage Example

### Testing 3-Class Network with Noise

```cpp
// In main() or after network is loaded:
#include "input_sequence.h"

// Create noise test for class 2 (S2)
InputSequence* test_seq = create3ClassNoiseTest(2, 0.10f, 300);
GLOBAL_args->input_sequence = test_seq;

// Start visualizer
// Press 'A' to animate
// Press '.' to speed up (watch at 10-20 ticks/frame)
// Press 'C' to show tick counter
// Press 'R' to restart sequence
```

**What to observe**:
1. S2 activates continuously (strong signal)
2. S0, S1 activate sporadically (10% noise)
3. O2 should maintain highest firing rate
4. Pool (N0) pulses as outputs compete
5. Winner-takes-all dynamics visible

### Slow-Motion Analysis

```bash
# Start with slow speed for detailed observation
1. Press 'A' to start animation
2. Keep at 1 tick/frame (default)
3. Watch each neuron's activation frame-by-frame
4. Press '.' to gradually increase speed
5. Find optimal visualization speed
```

### Fast-Forward Through Long Tests

```bash
# Speed through long sequences
1. Press '.' multiple times to reach 50-100 ticks/frame
2. Observe aggregate behavior
3. Press ',' to slow down at interesting moments
4. Press 'R' to restart
```

## Implementation Details

### Tick Management

```cpp
// In Step() function:
for (int t = 0; t < mesh_data->ticks_per_frame; t++) {
    // Apply inputs (sequence or manual)
    // Step network
    mesh_data->current_tick++;
}
// Update visualization once per frame
```

**Benefits**:
- Decouples simulation from rendering
- Allows fast simulation without dropping frames
- Precise tick counting for reproducibility

### Input Sequence Structure

```cpp
struct InputEvent {
    int tick;                              // When to apply
    std::map<std::string, float> inputs;   // neuron_id -> value
};

class InputSequence {
    std::vector<InputEvent> events;
    int current_tick;
    bool loop;
    
    std::map<std::string, float> getCurrentInputs();
    void advance();
    void reset();
};
```

## Integration with Test Harness

The visualizer can now **visualize test harness experiments**:

### Convert Test to Sequence

```cpp
// From 3class_test.cpp logic:
void create3ClassNoiseVisualization() {
    InputSequence* seq = new InputSequence();
    
    // Test class 0, 5% noise, 100 ticks
    for (int t = 0; t < 100; t++) {
        seq->addEvent(t, "S0", 200.0f);  // Correct class always on
        
        // 5% noise on other classes
        if (rand() / (float)RAND_MAX < 0.05f) {
            seq->addEvent(t, "S1", 200.0f);
        }
        if (rand() / (float)RAND_MAX < 0.05f) {
            seq->addEvent(t, "S2", 200.0f);
        }
    }
    
    GLOBAL_args->input_sequence = seq;
}
```

## Future Enhancements

### Short Term
- [ ] **Text rendering** for tick counter overlay
- [ ] **Sequence progress bar** showing position in sequence
- [ ] **Load sequences from files** (.seq format)
- [ ] **Record sequences from manual input** (capture user interactions)

### Medium Term
- [ ] **Sequence editor UI** (create sequences interactively)
- [ ] **Multiple sequence library** (quick load from menu)
- [ ] **Playback controls** (pause, step, rewind)
- [ ] **Sequence timeline visualization** (show upcoming events)

### Long Term
- [ ] **Firing rate graphs** overlaid on visualization
- [ ] **Real-time metrics** (margin, accuracy, etc.)
- [ ] **Export test results** to CSV
- [ ] **Synchronized multi-network visualization**

## Troubleshooting

### Sequence Not Running
- Check if `input_sequence` is set: `if (GLOBAL_args->input_sequence == nullptr)`
- Verify sequence has events: `if (seq->isEmpty())`
- Ensure animation is started: Press `A`

### Manual Inputs Not Working
- Sequences override manual controls
- Clear sequence: `GLOBAL_args->input_sequence = nullptr;`
- Or reset: `GLOBAL_args->input_sequence->clear();`

### Simulation Too Fast/Slow
- Adjust with `,` (slower) and `.` (faster)
- Current speed shown in console
- Optimal range: 1-10 ticks/frame for visualization
- Use 50-100 for fast batch testing

### Tick Counter Not Visible
- Currently console-only (check terminal output)
- Press `C` to toggle (affects future UI rendering)
- Text rendering implementation pending

## Files Modified/Created

| File | Purpose |
|------|---------|
| `input_sequence.h` | **NEW**: Sequence data structures |
| `meshdata.h` | Added tick counter and speed control |
| `meshdata.cpp` | Updated Step() for multi-tick execution |
| `argparser.h` | Added input_sequence pointer |
| `argparser.cpp` | Initialize input_sequence |
| `OpenGLCanvas.cpp` | Added keyboard controls (`,` `.` `C` `R`) |

## Example: Visualizing Noise Robustness

```cpp
// In your test setup code:
#include "input_sequence.h"

// Test all three classes with increasing noise
void visualizeNoiseRobustness() {
    // Class 0, 5% noise
    auto seq0 = create3ClassNoiseTest(0, 0.05f, 200);
    GLOBAL_args->input_sequence = seq0;
    // Run, observe, reset
    
    // Class 1, 10% noise
    auto seq1 = create3ClassNoiseTest(1, 0.10f, 200);
    GLOBAL_args->input_sequence = seq1;
    // Run, observe, reset
    
    // Class 2, 20% noise
    auto seq2 = create3ClassNoiseTest(2, 0.20f, 200);
    GLOBAL_args->input_sequence = seq2;
    // Run, observe
}
```

**Press `R` between tests to reset the tick counter and switch sequences.**

---

**Complex temporal experiments are now easy to visualize!** 🎬🧪
