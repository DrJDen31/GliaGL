# [ARCHIVED] Visualizer Fixes - Firing Rate Display
This legacy doc referenced FiringRateTracker. The codebase now uses `EMAOutputDetector` via `src/arch/output_detection.h` with configurable `default_id` and `threshold`.
See updated overview in `docs/OUTPUT_DETECTION_OPTIONS.md` and example usage in `src/testing/xor/xor_test.cpp`.

## Original: Visualizer Fixes - Firing Rate Display

## Problem

The visualizer was showing **instantaneous firing state** (did neuron fire this tick?), causing flickering at slow speeds. The network's **output detection** uses **firing rate tracking** over time to determine the winner.

**Mismatch**: 
- Network logic: Uses `FiringRateTracker.argmax()` → stable winner
- Visualizer: Uses `neuron->didFire()` → flickers every tick

## Solution

Updated visualizer to use **firing rate tracking** (same as network logic):

1. **Added FiringRateTracker to NetworkGraph**
2. **Track output firing rates** every tick
3. **Determine winner via argmax** (same algorithm as test harness)
4. **Highlight winning output** consistently (not just when firing)

## Changes Made

### network_graph.h
```cpp
#include "../arch/output_detection.h"

class NetworkGraph {
    // ...
    FiringRateTracker output_tracker;    // Tracks firing rates
    std::string current_winner;           // Current winning output
    
    const std::string& getCurrentWinner() const { return current_winner; }
};
```

### network_graph.cpp
```cpp
void NetworkGraph::updateActivationStates() {
    // Update all particles
    for (auto* p : particles) {
        p->updateActivationState();
    }
    
    // Track output neuron firing rates
    for (const auto& output_id : output_neuron_ids) {
        Neuron* n = glia->getNeuronById(output_id);
        if (n) {
            output_tracker.update(output_id, n->didFire());
        }
    }
    
    // Determine winner using firing rate (same as test harness)
    std::string default_output = (default_output_index >= 0) 
        ? output_neuron_ids[default_output_index] 
        : "";
    current_winner = output_tracker.argmax(output_neuron_ids, default_output, 0.01f);
}

void NetworkGraph::updateColors() {
    // Update winner flags for output neurons
    for (const auto& output_id : output_neuron_ids) {
        NeuronParticle* p = getParticleById(output_id);
        if (p) {
            p->setWinner(output_id == current_winner);
        }
    }
    
    // Update all particle colors
    for (auto* p : particles) {
        p->updateColor();
    }
}
```

### neuron_particle.h
```cpp
class NeuronParticle {
public:
    void setWinner(bool is_winner) { is_winner_output = is_winner; }
    
private:
    bool is_winner_output;  // True if winning output (from firing rate)
};
```

### neuron_particle.cpp
```cpp
void NeuronParticle::updateColor(float alpha) {
    // For output neurons, show as active if they're the winner
    float t = activation_level;
    if (type == NeuronType::OUTPUT && is_winner_output) {
        t = 1.0f;  // Always bright if winning
    }
    
    // Interpolate between base and active colors
    current_color = base_color * (1.0f - t) + active_color * t;
    // ...
}
```

### meshdata.cpp - Tick Counter Display
```cpp
// Step the network
GLOBAL_args->glia->step();
GLOBAL_args->mesh_data->current_tick++;

// Print tick counter if enabled (every 10 ticks)
if (GLOBAL_args->mesh_data->show_tick_counter && 
    GLOBAL_args->mesh_data->current_tick % 10 == 0) {
    std::cout << "Tick: " << GLOBAL_args->mesh_data->current_tick << std::endl;
}

// Update visualization
GLOBAL_args->network_graph->updateActivationStates();
GLOBAL_args->network_graph->updateColors();
GLOBAL_args->network_graph->packMesh();

// Print current winner
if (GLOBAL_args->mesh_data->show_tick_counter && 
    GLOBAL_args->mesh_data->current_tick % 10 == 0) {
    std::string winner = GLOBAL_args->network_graph->getCurrentWinner();
    if (!winner.empty()) {
        std::cout << "  Winner: " << winner << std::endl;
    }
}
```

## Behavior Now

### Before (Instantaneous)
```
Tick 1: S1 fires → O1 fires (bright) → O0 dim
Tick 2: S1 fires → O1 silent (dim) → O0 dim
Tick 3: S1 fires → O1 fires (bright) → O0 dim
Tick 4: S1 fires → O1 silent (dim) → O0 dim
```
**Result**: O1 flickers on/off every other tick

### After (Firing Rate)
```
Tick 1: S1 fires → O1 fires → Tracker: O1=1, O0=0 → Winner: O1 (bright)
Tick 2: S1 fires → O1 silent → Tracker: O1=1, O0=0 → Winner: O1 (bright)
Tick 3: S1 fires → O1 fires → Tracker: O1=2, O0=0 → Winner: O1 (bright)
Tick 4: S1 fires → O1 silent → Tracker: O1=2, O0=0 → Winner: O1 (bright)
```
**Result**: O1 stays consistently bright (winning based on rate, not instant)

## Tick Counter Output

```
Tick: 10
  Winner: O1
Tick: 20
  Winner: O1
Tick: 30
  Winner: O1
```

Press `C` to toggle tick counter on/off.

## Why This Matters

**Spiking Neural Networks**:
- Neurons fire in **bursts** (spike → silence → spike → silence)
- Information encoded in **firing rate** (spikes per second)
- NOT encoded in instantaneous state

**Old visualizer**: Showed instantaneous spikes (misleading at slow speeds)
**New visualizer**: Shows firing rate winner (matches network's actual decision)

## Testing

```cmd
cd build
cmake --build .
debug\vis.exe --network ../src/testing/3class/3class_network.net --size 1500 1500

# In visualizer:
1. Press 'I' for inference mode
2. Press '1' to activate S0 (or use sliders)
3. Press 'A' to animate
4. Observe: O0 stays bright (not flickering)
5. Console shows: "Tick: 10 / Winner: O0" every 10 ticks
```

---

**The visualizer now correctly shows the winner-take-all decision based on firing rates!** ✨
