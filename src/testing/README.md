# Testing Directory


## Structure

testing/
├── README.md                 
│
└── xor/                       # XOR example (self-contained)
    ├── xor_test.cpp
    ├── xor_network.net
    ├── Makefile
    ├── README.md
    └── ANALYSIS.md
│
{{ ... }}
    ├── 3class/               # 3-Class One-Hot (planned)
    ├── temporal/             # Temporal AB vs BA (planned)
    └── ...
```

## Current Examples

### XOR (2 inputs → 2 outputs)

**Location:** `xor/`  
**Status:** ✅ Complete and working

Tests the XOR function using:
- 2 sensory neurons (S0, S1)
- AND coincidence detector (N0) with leak=0
- 2 output neurons (N1=XOR_true, N2=XOR_false)
- Inhibitory/excitatory connections for logic

**Build:** `cd xor && make`  
**Run:** `./xor_test`

---

## Shared Components (from arch/)

All test examples use the core architecture components:

### output_detection.h (from `../arch/`)

Provides `FiringRateTracker` class for monitoring and classifying neuron firing rates:

**Features:**
- **EMA tracking**: Exponential Moving Average smoothing
- **Argmax classification**: Winner-take-all decision
- **Margin computation**: Confidence metrics
- **Rate queries**: Access to raw firing rates

**Usage Example:**
```cpp
#include "../../arch/output_detection.h"

FiringRateTracker tracker(0.05f);  // alpha = 1/20

// During simulation
for (int t = 0; t < num_ticks; ++t)
{
    network.step();
    tracker.update("N1", neuron1->didFire());
    tracker.update("N2", neuron2->didFire());
}

// Classification
std::string winner = tracker.argmax({"N1", "N2", "N3"});
float margin = tracker.getMargin({"N1", "N2", "N3"});
```

---

## Adding New Examples

To add a new toy example:

1. **Create subdirectory** under `testing/`
   ```bash
   mkdir testing/my_example
   ```

2. **Create test file** (e.g., `my_example_test.cpp`)
   ```cpp
   #include "../../arch/glia.h"
   #include "../../arch/neuron.h"
   #include "../output_detection.h"
   
   int main() {
       Glia network(...);
       FiringRateTracker tracker(0.05f);
       // ... your test logic
   }
   ```

3. **Create network config** (e.g., `my_example.net`)
   ```
   NEURON <id> <threshold> <leak> <resting>
   CONNECTION <from> <to> <weight>
   ```

4. **Copy and adapt Makefile** from `xor/Makefile`
   - Update `TARGET` name
   - Update source file names
   - Keep paths to `../../arch/` intact

5. **Add README.md** documenting:
   - Network architecture
   - Expected behavior
   - Build/run instructions
   - Analysis of results

---

## Design Philosophy

### Why separate testing/ from arch/?

- **arch/**: Core simulation engine (Glia, Neuron) - reusable, general-purpose
- **testing/**: Specific test scenarios and configurations - experimental, example-driven

This separation allows:
- Multiple test scenarios to share the same core code
- Easy addition of new examples without cluttering arch/
- Clear distinction between "engine" and "experiments"

### Why shared output_detection.h?

All toy examples need output classification. By extracting this into a shared header:
- ✓ No code duplication across examples
- ✓ Consistent classification methodology
- ✓ Easy to enhance (add margin, confidence, etc.) in one place
- ✓ Simple to swap strategies (e.g., softmax instead of argmax)

---

## Future Enhancements

### Planned Examples
- [ ] **3-Class One-Hot** - Multi-class classification with inhibitory pool
- [ ] **Temporal AB/BA** - Sequence detection with memory neurons
- [ ] **Custom patterns** - User-defined test cases

### Planned Features
- [ ] **Visualization**: Plot firing rates over time
- [ ] **Batch testing**: Run multiple configs automatically
- [ ] **Performance benchmarks**: Measure tick speed, scaling
- [ ] **Training integration**: Compare manual vs learned weights

---

## References

- `docs/TOY_EXAMPLES.md` - Full specifications for each example
- `docs/OUTPUT_DETECTION_OPTIONS.md` - Analysis of classification strategies
- `arch/README.md` - Core architecture documentation
