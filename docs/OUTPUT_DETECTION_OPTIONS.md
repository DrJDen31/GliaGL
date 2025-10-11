# Output Detection Implementation Options

This document analyzes different approaches for implementing output detection in spiking neural networks, specifically focusing on argmax classification based on firing rates.

## Background

The network produces output through specialized output neurons whose firing rates encode the classification decision. The detection mechanism must:
1. Track firing rates over time
2. Compare rates across output neurons
3. Determine the "winner" (argmax)
4. Optionally apply confidence thresholds

---

## Option 1: External Detector Class (Current Implementation)

**Location:** Header-only in `src/arch/output_detection.h` (`EMAOutputDetector` via `IOutputDetector`)

### Pros
- **Separation of concerns**: Network simulation and output analysis are decoupled
- **Flexibility**: Swap detection strategies without modifying core network
- **Multiple detectors**: Can run multiple strategies side-by-side
- **No network modification**: Glia/Neuron remain focused on simulation
- **Easy testing**: Tune EMA alpha, threshold, defaults, margin, etc.

### Cons
- **Manual neuron querying**: Must explicitly track which neurons to monitor
- **Extra bookkeeping**: Application code must manage tracker state
- **Not encapsulated**: Detection logic lives outside the network object

### Implementation Example
```cpp
#include "src/arch/output_detection.h"

OutputDetectorOptions opts; 
opts.threshold = 0.01f;     // abstain below this
opts.default_id = "O0";    // optional default when abstaining
EMAOutputDetector det(0.05f, opts);

// During sim
det.update("O1", n1->didFire());
det.update("O0", n0->didFire());

// Classification
std::string winner = det.predict({"O1","O0"});
float margin = det.getMargin({"O1","O0"});
```

### Best For
- Research/experimentation with different detection strategies
- Small networks with clearly defined output neurons
- Applications that need multiple simultaneous readout methods
- **Current XOR toy example** ✓

---

## Option 2: Glia-Level Detection

**Location:** Methods added to the `Glia` class

### Pros
- **Centralized**: Network manages its own output analysis
- **Convenient API**: Single method call to get classification
- **Network metadata**: Glia knows which neurons are outputs vs interneurons
- **Automatic tracking**: Can track all neurons without external specification
- **Training integration**: Output rates can be used directly in training algorithms

### Cons
- **Mixed responsibilities**: Glia handles both simulation and analysis
- **Less flexible**: Harder to swap detection strategies
- **Configuration burden**: Need to specify which neurons are "outputs" during network setup

### Implementation Sketch
```cpp
// In glia.h
class Glia {
public:
    void designateOutputNeurons(std::vector<std::string> ids);
    std::string getClassification();  // Returns argmax winner
    std::map<std::string, float> getOutputRates();
    
private:
    std::vector<std::string> output_neuron_ids;
    std::map<std::string, float> firing_rates;
    float alpha = 0.05f;  // EMA parameter
};

// Usage
network.designateOutputNeurons({"N1", "N2"});
network.step();
std::string result = network.getClassification();
```

### Best For
- Production systems with fixed network architectures
- Applications that need clean API boundaries
- Networks where output neurons are known at configuration time
- Integration with training loops

---

## Option 3: Specialized Output Neuron Subclass

**Location:** `OutputNeuron` class inheriting from `Neuron`

### Pros
- **Object-oriented**: Each output neuron tracks its own rate
- **Encapsulation**: Firing rate tracking is internal to the neuron
- **Type safety**: Output neurons are distinguishable by type
- **Local state**: Each neuron maintains its own EMA independently
- **Minimal changes**: Glia just queries OutputNeuron instances

### Cons
- **Class hierarchy**: Adds complexity with inheritance
- **Comparison logic**: Still need external argmax (outputs don't compare themselves)
- **Refactoring required**: Need to modify Glia constructor to create OutputNeuron instances
- **Less general**: What about neurons that are outputs in one task but hidden in another?

### Implementation Sketch
```cpp
// In neuron.h
class OutputNeuron : public Neuron {
public:
    OutputNeuron(...) : Neuron(...), firing_rate(0.0f), alpha(0.05f) {}
    
    void tick() override {
        Neuron::tick();
        // Update firing rate after tick
        firing_rate = (1.0f - alpha) * firing_rate + alpha * (didFire() ? 1.0f : 0.0f);
    }
    
    float getFiringRate() const { return firing_rate; }
    
private:
    float firing_rate;
    float alpha;
};

// In glia.cpp
OutputNeuron* out1 = new OutputNeuron("O1", ...);

// Usage
float rate = dynamic_cast<OutputNeuron*>(neuron)->getFiringRate();
```

### Best For
- Architectures with fixed, known output layers
- Networks where outputs have special properties beyond just tracking
- Systems that benefit from strong type distinctions

---

## Option 4: Separate OutputAnalyzer Class (In Glia)

**Location:** Helper class used internally by `Glia`

### Pros
- **Modular**: Analysis logic is isolated but still part of the network
- **Reusable**: Can use same analyzer across different Glia instances
- **Testable**: OutputAnalyzer can be unit tested independently
- **Strategy pattern**: Can swap analyzers (e.g., MarginAnalyzer, SoftmaxAnalyzer)

### Cons
- **Extra indirection**: Adds another layer of abstraction
- **Still need configuration**: Must tell analyzer which neurons to monitor

### Implementation Sketch
```cpp
class OutputAnalyzer {
public:
    void setOutputNeurons(std::vector<Neuron*> outputs);
    void update();  // Query neurons and update rates
    std::string getWinner();
    float getMargin();  // r_best - r_second
    
private:
    std::vector<Neuron*> output_neurons;
    std::map<std::string, float> rates;
};

// In Glia
class Glia {
private:
    OutputAnalyzer analyzer;
};
```

### Best For
- Medium-to-large networks with varying output configurations
- Systems that need multiple analysis metrics (margin, confidence, etc.)
- Codebases that emphasize testability and modularity

---

## Comparison Matrix

| Feature | External Tracker | Glia-Level | OutputNeuron Subclass | Separate Analyzer |
|---------|-----------------|------------|----------------------|-------------------|
| Decoupling | ✓✓✓ | ✓ | ✓✓ | ✓✓ |
| Flexibility | ✓✓✓ | ✓ | ✓ | ✓✓✓ |
| Ease of Use | ✓✓ | ✓✓✓ | ✓✓ | ✓✓ |
| Encapsulation | ✓ | ✓✓ | ✓✓✓ | ✓✓ |
| Testability | ✓✓✓ | ✓✓ | ✓✓ | ✓✓✓ |
| Implementation Complexity | ✓✓✓ (simple) | ✓✓ | ✓ (requires refactoring) | ✓✓ |

---

## Recommendation for This Project

**For the XOR and 3-class toy examples and initial development**: **Option 1 (External Detector)** is ideal.

**Reasons:**
1. **Experimentation-friendly**: We're still testing different network configurations
2. **Minimal changes**: Doesn't require modifying Glia or Neuron
3. **Clear separation**: Makes it obvious what is "network simulation" vs "output analysis"
4. **Easy to extend**: Can add margin analysis, confidence thresholds, etc. without touching core code

**For production/scaling**: Consider **Option 2 (Glia-Level)** or **Option 4 (Separate Analyzer)**.

**Reasons:**
1. **Convenience**: Single API call to get results
2. **Integration**: Better for training loops that need gradient/reward signals
3. **Configuration**: Can specify output neurons in the .net file
4. **Encapsulation**: Network "owns" its output analysis

---

## Advanced Features to Consider

### Margin-Based Classification
```cpp
// Don't just return argmax, also check margin
float getMargin() {
    // r_best - r_second_best
    // Reject if margin < threshold (abstention)
}
```

### Confidence Thresholds
```cpp
// Only classify if winner rate > min_threshold
if (max_rate < 0.3f) return "UNDECIDED";
```

### Time-Windowed Analysis
```cpp
// Instead of EMA, use sliding window
// Useful for detecting transient vs sustained responses
std::deque<bool> recent_spikes;  // Last N ticks
float rate = std::count(recent_spikes.begin(), recent_spikes.end(), true) / N;
```

### Softmax Normalization
```cpp
// Convert rates to probabilities
std::vector<float> probs = softmax(rates);
```

---

## Implementation Notes

### Current Setup
- Uses **Option 1** with `EMAOutputDetector`
- Monitors O* neurons (e.g., XOR: `O1` true, `O0` false)
- EMA with α ≈ 0.05 and configurable threshold/default
- Argmax + optional margin reporting

### Future Extensions
1. Add margin checking to detect ambiguous cases
2. Support multi-class classification (already structured for this)
3. Add confidence reporting
4. Integrate with visualization (track rates over time)
