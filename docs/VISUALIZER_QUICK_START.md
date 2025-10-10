# Visualizer Quick Start - Testing Framework

## How to Run a Test

### 1. Start the Visualizer
```cmd
cd build
debug\vis.exe --network ../src/testing/3class/3class_network.net --size 1500 1500
```

### 2. Load a Test Sequence
Press one of these function keys:

- **F1**: 3-Class test (class 0, 5% noise, 200 ticks)
- **F2**: 3-Class test (class 1, 10% noise, 200 ticks)
- **F3**: 3-Class test (class 2, 20% noise, 200 ticks)
- **F4**: XOR test (all 4 patterns, 100 ticks each)
- **F5**: Clear test (return to manual control)

### 3. Set Visualization Speed
**For slow-motion observation (recommended for first viewing)**:
- Press **`,`** (comma) several times to add delay
- Each press adds 0.1 seconds between ticks
- Typical: 0.5-2.0 seconds per tick for careful observation
- Console shows: "Tick delay: X.X seconds/tick"

**For fast-forward (when you understand the pattern)**:
- Press **`.`** (period) to reduce delay back to real-time
- Or use **`+`** to run multiple ticks per frame

### 4. Start Animation
- Press **`A`** to start
- Press **`X`** to stop
- Press **Space** for single step

### 5. Observe
- Watch neurons light up (blue → yellow/red when firing)
- Check console for tick counter
- See which output wins

### 6. Reset and Try Again
- Press **`R`** to reset to tick 0
- Press different F-key to load new test
- Adjust speed with **`,`** or **`.`**

## Complete Workflow Example

```
1. Launch visualizer
2. Press 'I' for inference mode
3. Press 'F1' to load class 0 test (5% noise)
4. Press ',' five times → "Tick delay: 0.5 seconds/tick"
5. Press 'A' to start animation
6. Watch: S0 fires continuously, S1/S2 fire occasionally
7. Observe: O0 maintains highest activation
8. Press 'X' to stop
9. Press 'F2' to load class 1 test (10% noise)
10. Press 'R' to reset counter
11. Press 'A' to start again
```

## Keyboard Controls Reference

### Test Loading
| Key | Action |
|-----|--------|
| **F1** | Load 3-class test (class 0, 5% noise) |
| **F2** | Load 3-class test (class 1, 10% noise) |
| **F3** | Load 3-class test (class 2, 20% noise) |
| **F4** | Load XOR test (4 patterns) |
| **F5** | Clear test sequence |

### Speed Control
| Key | Action |
|-----|--------|
| **`,`** (comma) | Slower (add 0.1s delay per tick) |
| **`.`** (period) | Faster (reduce delay by 0.1s) |
| **`+`** | Fast-forward (more ticks/frame) |
| **`-`** | Slow down fast-forward |

### Playback
| Key | Action |
|-----|--------|
| **A** | Start continuous animation |
| **X** | Stop animation |
| **Space** | Single step (one tick) |
| **R** | Reset to tick 0 |
| **C** | Toggle tick counter display |

### Modes
| Key | Action |
|-----|--------|
| **I** | Inference mode (recommended) |
| **T** | Training mode (physics) |

### Display
| Key | Action |
|-----|--------|
| **B** | Toggle bounding box |
| **[** / **]** | Adjust neuron size |

### Manual Input (when no test loaded)
| Key | Action |
|-----|--------|
| **1-9** | Toggle sensory neurons S0-S8 |
| **0** | Clear all inputs |

## Tips for Effective Visualization

### Understanding the Speed Settings

**Slow-Motion (0.5-2.0 seconds/tick)**:
- Use when first seeing a test
- Observe individual neuron firings
- See exact timing of events
- Good for: Understanding WTA dynamics

**Real-Time (0.0 seconds/tick)**:
- Default speed
- ~60 ticks/second (depends on frame rate)
- Good for: Seeing overall patterns

**Fast-Forward (10-50 ticks/frame)**:
- Use when sequence is long (>200 ticks)
- Skip to interesting parts
- Good for: Batch testing, finding steady state

### Recommended Speeds by Test Type

| Test | Recommended Speed | Why |
|------|------------------|-----|
| 3-class noise (first time) | 0.5-1.0 sec/tick | See noise spikes clearly |
| 3-class noise (analyzing) | 0.0 sec/tick | Natural rhythm |
| 3-class noise (verifying) | 10-20 ticks/frame | Quick confirmation |
| XOR patterns | 0.5-1.0 sec/tick | See pattern transitions |
| Long sequences (500+ ticks) | Start fast, slow at key moments | Efficient |

### What to Look For

#### 3-Class Noise Tests
1. **Correct sensory neuron** fires every tick (bright, steady)
2. **Noise sensory neurons** fire sporadically (brief flashes)
3. **Correct output** maintains highest activation (yellow/red)
4. **Competing outputs** briefly activate but get suppressed (flash then dim)
5. **Inhibitory pool (N0)** pulses when outputs compete (gray neuron in middle)

#### XOR Tests
1. **Pattern 00** (ticks 0-99): All quiet → O0 should win (default)
2. **Pattern 01** (ticks 100-199): S1 fires → O1 should win
3. **Pattern 10** (ticks 200-299): S0 fires → O1 should win
4. **Pattern 11** (ticks 300-399): Both fire → O0 should win

## Troubleshooting

### Test Not Running
**Problem**: Pressed F-key but nothing happens  
**Solution**: 
1. Make sure you're in inference mode (press `I`)
2. Check console for "Loaded test: ..." message
3. Press `A` to start animation

### Can't See Individual Ticks
**Problem**: Everything happens too fast  
**Solution**: 
1. Press `,` (comma) 5-10 times
2. Should see "Tick delay: X.X seconds/tick" increasing
3. Start at 0.5s/tick, adjust to your preference

### Animation Stutters
**Problem**: Jerky motion at slow speeds  
**Solution**: This is expected - each tick is a discrete event. At 1.0s/tick, you'll see 1-second pauses between updates. This is correct.

### Test Keeps Repeating
**Problem**: Sequence loops back to start  
**Solution**: Tests don't loop by default. If you see this, the test duration is shorter than you think. Check console for max tick or press `F5` to clear and start fresh.

### Can't Control Manually
**Problem**: Sliders/keys don't work  
**Solution**: A test sequence is loaded. Press `F5` to clear it and return to manual control.

## Console Output Example

```
Loaded test: 3-class (class 1, 10% noise, 200 ticks)
Tick delay: 0.5 seconds/tick
Tick delay: 0.6 seconds/tick
Tick delay: 0.7 seconds/tick
animation started, press 'X' to stop
[Network starts ticking...]
Tick counter: 50
Tick counter: 100
Tick counter: 150
Tick counter: 200
animation stopped, press 'A' to start
```

## Advanced: Creating Custom Tests

See `docs/VISUALIZER_TESTING_FRAMEWORK.md` for details on:
- Writing custom input sequences in code
- Modifying test parameters
- Creating new test shortcuts
- Loading sequences from files

---

**Start with F1 + slow-motion (`,,,,,`) for your first test!** 🎬
