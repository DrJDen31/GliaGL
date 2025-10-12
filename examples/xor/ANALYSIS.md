# XOR Network Test - Analysis and Debugging Summary

## Final Results ✓

**All 4 XOR cases working correctly:**

| Input | N1 Rate | N2 Rate | Output | Expected | Status |
|-------|---------|---------|--------|----------|--------|
| 00    | 0.000   | 0.000   | FALSE  | FALSE    | ✓ PASS |
| 01    | 0.993   | 0.000   | TRUE   | TRUE     | ✓ PASS |
| 10    | 0.994   | 0.000   | TRUE   | TRUE     | ✓ PASS |
| 11    | 0.001   | 0.993   | FALSE  | FALSE    | ✓ PASS |

---

## Bugs Found and Fixed

### Bug 1: Double Application of Synaptic Input ⚠️ CRITICAL

**Location:** `neuron.cpp::receive()`

**Problem:**
```cpp
void Neuron::receive(float transmission)
{
    if (this->using_tick)
    {
        this->on_deck += transmission;  // Staged for next tick
    }
    
    this->value += transmission;  // ❌ ALSO applied immediately!
    // ... then checked threshold and fired immediately
}
```

When `using_tick=true`, the code was:
1. Staging input for next tick (correct)
2. **AND** applying it immediately to voltage (wrong!)
3. This caused immediate firing, bypassing 1-tick synaptic delay
4. Double-counted inputs (once immediate, once delayed)

**Fix:**
```cpp
void Neuron::receive(float transmission)
{
    if (this->using_tick)
    {
        this->on_deck += transmission;
        return;  // ✓ Exit early - tick() handles the update
    }
    // Only apply immediately in non-tick mode
    this->value += transmission;
}
```

**Impact:** Without this fix, timing was completely broken and neurons fired unpredictably.

---

### Bug 2: Voltage Transient from Configuration ⚠️

**Location:** `glia.cpp::Glia()` constructor + `neuron.cpp::setResting()`

**Problem:**
Neurons initialized with `resting=70.0` in constructor, then `configureNetworkFromFile()` changed `resting` to `0.0` but didn't reset current voltage. This left neurons with `value=70.0` at start of simulation.

**Fix:**
```cpp
void Neuron::setResting(float new_resting)
{
    this->resting = new_resting;
    this->value = new_resting;  // ✓ Reset current voltage
}
```

**Impact:** Caused spurious firing in first few ticks before voltage decayed.

---

### Bug 3: Incorrect Leak/Membrane Dynamics ⚠️ CRITICAL

**Location:** `neuron.cpp::tick()`

**Problem:**
The toy example spec says: `V = max(0, V*leak + incoming)`

But the code was using exponential decay toward resting:
```cpp
float lambda = this->balancer;
this->value += -lambda * (this->value - this->resting);  // Wrong model
```

This is **backwards**:
- spec: `leak=1.0` means no decay (voltage persists)
- code: `leak=1.0` meant full decay (exponential)

For coincidence detectors (leak=0), this completely broke them:
- spec: `leak=0` → voltage resets to 0 each tick, only fires on simultaneous inputs
- code: `leak=0` → no decay at all!

**Fix:**
```cpp
// Apply membrane update: V = max(0, V*leak + incoming)
float leak_factor = this->balancer;
this->value = leak_factor * this->value + incoming;
if (this->value < 0) this->value = 0;
```

**Impact:** 
- AND neuron (N0) now correctly requires coincident inputs
- Output neurons properly decay/persist based on leak parameter
- Inhibition from AND to XOR_true works correctly

---

### Bug 4: Tied Output Handling 🔧

**Location:** `main.cpp::runTest()`

**Problem:**
For input `00`, both outputs are silent (0.000 rate). The `argmax()` function picked N1 by default, declaring TRUE when it should be FALSE.

**Fix:**
```cpp
float max_rate = std::max(tracker.getRate("N1"), tracker.getRate("N2"));

if (max_rate < 0.01f)  // Both silent
{
    std::cout << "XOR Result: FALSE (0) - default for no activity" << std::endl;
}
else
{
    // Normal argmax classification
}
```

**Impact:** Correctly handles the edge case where no output neuron fires.

---

## Key Implementation Details

### Network Architecture

```
      S0 ──┬──[+60]──> N1 (O1, XOR true)  TH=50
           │
           └──[+60]──> N0 (A, AND)       TH=90, leak=0 (coincidence)
                       │
                       ├──[-120]──> N1 (inhibit when both inputs)
                       │
      S1 ──┬──[+60]──> N1
           │
           └──[+60]──> N0
                       │
                       └──[+120]──> N2 (O0, XOR false) TH=60
```

### Behavior by Case

**00 (both inputs off):**
- No sensory firing → no activity → both outputs silent
- Correctly outputs FALSE (default)

**01 or 10 (single input):**
- Sensory neuron fires continuously
- N1 receives +60 each tick, crosses TH=50 ✓
- N0 receives only +60, doesn't cross TH=90 (needs 90 for coincidence) ✗
- N2 receives nothing (only gets input from N0)
- **Result: N1 fires ~99.3%, N2 silent → TRUE** ✓

**11 (both inputs on):**
- Both sensory neurons fire continuously
- N1 receives +120 from sensory inputs at t+1
- N0 receives +120, crosses TH=90 (coincidence detected) ✓
- At t+2: N0 fires
  - Sends -120 to N1 (inhibits it)
  - Sends +120 to N2 (excites it)
- N1 is suppressed to ~0.1% firing
- N2 fires at ~99.3%
- **Result: N2 wins → FALSE** ✓

### Membrane Dynamics (Corrected)

```
V(t+1) = leak * V(t) + incoming(t+1)
V = max(0, V)
```

Where:
- `leak=1.0`: Voltage persists (integrator neuron)
- `leak=0.0`: Voltage resets each tick (coincidence detector)
- `leak=0.8`: Partial decay (used in some toy examples for inhibitory pools)

### Firing Rate Tracking

**Exponential Moving Average (EMA):**
```
rate_k ← (1−α) × rate_k + α × [neuron fired this tick]
α = 0.05 (approximately 1/20 from spec)
```

Smooths firing patterns over ~20 ticks.

### Argmax Classification

Winner = neuron with highest firing rate after 100 ticks
- Provides stable classification
- Handles noisy/transient firing
- Can be extended with margin/confidence thresholds

---

## Performance Analysis

### Expected vs Actual Firing Rates

From `docs/TOY_EXAMPLES.md`:
> **Measured steady rates (100 ticks, no refractory, leak=1)**
> - 10/01 → **`O1≈0.50`**, `O0≈0.49`, `A≈0.49`
> - 11 → `O1≈0.01`, **`O0≈0.98`**, `A≈0.99`
>
> **Tip:** make `A` a true coincidence detector by setting `leak[A]=0`. Then
> 10/01 → `O1≈1.00`, `O0≈0.00`, and 11 → `O0≈1.00`, `O1≈0.00` (clean margin).

**Our Results (with leak[A]=0):**
- 01/10: N1≈0.99, N2≈0.00 ✓ (clean separation!)
- 11: N1≈0.00, N2≈0.99 ✓ (clean separation!)

The coincidence detector (leak=0) gives **much cleaner margins** than the spec's leak=1 version.

---

## Lessons Learned

### 1. **Synchronous vs Asynchronous Updates**

The `using_tick` flag gates whether neurons update synchronously (all at once) or asynchronously (immediate propagation). For spiking networks with precise timing, synchronous is essential.

**Key insight:** `receive()` must stage inputs without applying them when in tick mode.

### 2. **Leak Parameter Semantics**

Always verify whether "leak" means:
- Multiplicative persistence (voltage retained)
- Decay rate (voltage lost)

Our spec uses multiplicative: `V ← leak*V + input`

### 3. **Configuration vs Initialization**

When loading configs that change neuron parameters (threshold, leak, resting), ensure:
1. Current voltage resets to new resting
2. Delta/on_deck buffers are cleared
3. No transient states from old parameters

### 4. **Edge Case Handling**

Classification systems need to handle:
- Tied outputs (multiple neurons with same rate)
- Silent outputs (no neuron fires)
- Ambiguous cases (rates too close)

Options: default class, abstention, margin thresholds

### 5. **Debugging Spiking Networks**

Tools that helped:
1. **Print network structure** after loading
2. **Track individual neuron voltages** over time
3. **Compare firing rates** against theoretical predictions
4. **Test simple cases first** (00, 01, 10, 11 in order)

---

## Next Steps

### Immediate
- ✅ XOR working with manual configuration
- ⬜ Implement other toy examples (3-class, temporal AB/BA)
- ⬜ Add visualization of firing rates over time

### Training
- ⬜ Implement learning rules (STDP, reward-modulated, etc.)
- ⬜ Compare trained weights vs manual configuration
- ⬜ Test generalization

### Architecture Improvements
- ⬜ Move output detection into Glia class (see OUTPUT_DETECTION_OPTIONS.md)
- ⬜ Add margin-based classification
- ⬜ Support refractory periods (currently commented out)
- ⬜ Implement neuron groups/populations

### Performance
- ⬜ Profile network tick speed
- ⬜ Optimize for larger networks (100+ neurons)
- ⬜ Parallel processing of independent neurons

---

## References

- `docs/TOY_EXAMPLES.md` - XOR specification
- `docs/OUTPUT_DETECTION_OPTIONS.md` - Classification strategies
- `src/arch/XOR_TEST_README.md` - Compilation and usage guide
