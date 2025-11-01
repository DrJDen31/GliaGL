# Winner-Takes-All Analysis and Fix

## Problem
When multiple sensory inputs are activated sequentially or simultaneously:
- Outputs toggle rapidly between winners
- Sometimes a 4th neuron is selected (impossible - only 3 outputs exist)
- No stable winner emerges

## Root Cause Analysis

### Current Parameters (from 3class_network.net)
- **Output threshold**: 50
- **Pool threshold**: 40
- **Pool leak**: 0.8
- **Feedforward weights**: S→O = +60
- **Pool feedback**: O→Pool = +35, Pool→O = -45

### Timing Analysis

With 1-tick synaptic delay:
```
t=0: S0 fires (input injected)
t=1: O0 receives +60, crosses threshold (50), fires
     Pool receives nothing yet
t=2: Pool receives +35 from O0
     Pool does NOT fire yet (V=35 < 40)
     O0 still receives +60, fires again
t=3: Pool receives another +35 (now V=35*0.8 + 35 = 63)
     Pool fires! Sends -45 to all outputs
     O0 receives +60-45 = +15, does NOT cross threshold
t=4: O0 suppressed, Pool decays (V=63*0.8 = 50.4)
     Pool fires again (above threshold 40)
```

### Problem 1: Pool Takes Too Long to Activate
Pool threshold (40) requires 2 ticks of accumulation before firing. During this time, the "correct" output fires freely while competitors can also start firing.

### Problem 2: Insufficient Inhibition
When inputs switch, the new output gets +60 while the old one gets +60-45 = +15. The new output immediately dominates, causing rapid switching.

### Problem 3: Leak Allows Recovery
Pool leak (0.8) means it decays slowly, but outputs have leak=1.0 (no decay). This can cause oscillations.

## Solutions

### Option A: Faster Pool Activation (Recommended)
Lower pool threshold so it fires faster:
```
NEURON N0 30.0 0.8 0.0  # Was 40.0
```

Now pool fires after 1 tick (V=35 > 30), providing immediate lateral inhibition.

### Option B: Stronger Inhibition
Increase inhibitory weight:
```
CONNECTION N0 O0 -60.0  # Was -45.0
CONNECTION N0 O1 -60.0
CONNECTION N0 O2 -60.0
```

Now outputs receive +60-60 = 0 when pool is active, completely suppressing competitors.

### Option C: Combined Approach (Best)
Both faster activation AND stronger inhibition:
```
NEURON N0 30.0 0.8 0.0      # Lower threshold
CONNECTION N0 O0 -55.0      # Stronger inhibition
CONNECTION N0 O1 -55.0
CONNECTION N0 O2 -55.0
```

This creates strong winner-takes-all with fast response.

### Option D: Output Leak
Add leak to outputs to prevent lingering activation:
```
NEURON O0 50.0 0.9 0.0  # Was 1.0
NEURON O1 50.0 0.9 0.0
NEURON O2 50.0 0.9 0.0
```

## Recommended Fix

Use **Option C** for robust WTA behavior:

```
# Inhibitory pool with LOWER threshold for faster activation
NEURON N0 30.0 0.8 0.0

# Output neurons (unchanged)
NEURON O0 50.0 1.0 0.0
NEURON O1 50.0 1.0 0.0
NEURON O2 50.0 1.0 0.0

# Feedforward (unchanged)
CONNECTION S0 O0 60.0
CONNECTION S1 O1 60.0
CONNECTION S2 O2 60.0

# Pool feedback (STRONGER inhibition)
CONNECTION O0 N0 35.0       # Unchanged
CONNECTION O1 N0 35.0
CONNECTION O2 N0 35.0
CONNECTION N0 O0 -55.0      # Increased from -45
CONNECTION N0 O1 -55.0
CONNECTION N0 O2 -55.0
```

### Expected Behavior After Fix

1. **Single Input (e.g., S0)**:
   - t=0: S0 fires
   - t=1: O0 fires (V=60 > 50)
   - t=2: Pool fires (V=35 > 30), sends -55 to all outputs
   - t=3+: O0 receives +60-55 = +5 each tick, stays near threshold but doesn't fire
   - O1, O2 receive only -55, stay at 0
   - **Stable winner: O0**

2. **Multiple Inputs (e.g., S0 + S1)**:
   - Both O0 and O1 try to fire
   - Whichever fires first activates pool
   - Pool suppresses both
   - Small differences in initial conditions determine winner
   - **One stable winner emerges**

3. **Sequential Switching (S0 → S1)**:
   - O0 fires first, activates pool
   - Pool suppresses O0 and O1
   - When input switches, O1 gets +60-55 = +5
   - Pool may decay slightly (0.8 leak)
   - If pool drops below threshold, O1 can take over
   - **Graceful handoff to new winner**

## Testing the Fix

Recompile and test:
```cmd
cd C:\Users\jaden\OneDrive\Documents\AI\GliaGL\src\testing\3class
make clean && make
./3class_test
```

Look for:
- Stable winners (no toggling)
- Clean classification (correct output dominates)
- Margins > 0.3 (indicating separation)
