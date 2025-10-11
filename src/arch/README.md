# Glia Architecture - Core Components

## Overview

Glia is a spiking neural network architecture that simulates the behavior of biological neurons. It uses discrete-time tick-based simulation with configurable synaptic delays, leak dynamics, and spike-based communication.

This directory contains the **core simulation engine** - reusable, general-purpose classes for building and running spiking neural networks.

## Components

### Neuron (`neuron.h` / `neuron.cpp`)

Models a single spiking neuron with:
- **Membrane dynamics**: `V = leak * V + incoming` (configurable leak coefficient)
- **Threshold-based firing**: Spike when voltage exceeds threshold
- **Synaptic connections**: Weighted connections to other neurons
- **Delayed communication**: 1-tick synaptic delay (configurable via `using_tick`)
- **Refractory periods**: Optional post-spike inactivity (currently disabled)

**Key Methods:**
- `tick()` - Advance neuron by one timestep
- `receive(float)` - Receive synaptic input from another neuron
- `fire()` - Emit spike to all connected neurons
- `addConnection(weight, target)` - Create synaptic connection

**Configuration:**
- `setThreshold(float)` - Firing threshold
- `setLeak(float)` - Membrane leak coefficient (1.0 = no decay, 0.0 = coincidence detector)
- `setResting(float)` - Resting voltage

### Glia (`glia.h` / `glia.cpp`)

Manages networks of neurons:
- **Network construction**: Creates and organizes sensory neurons and interneurons
- **Simulation control**: Synchronous tick-based updates
- **Configuration I/O**: Load/save networks from `.net` files
- **Connectivity management**: Maps neuron IDs to objects and tracks connections

**Key Methods:**
- `step()` - Advance entire network by one tick
- `configureNetworkFromFile(path)` - Load network from config file
- `saveNetworkToFile(path)` - Export network to config file
- `injectSensory(id, value)` - Stimulate sensory neurons
- `getNeuronById(id)` - Access neurons for monitoring/training

### Output Detection (`output_detection.h`)

Provides a pluggable interface and default EMA-based output detector:
- **IOutputDetector**: Abstract interface for output selection
- **EMAOutputDetector**: Exponential Moving Average tracking + argmax selection

**Key Features:**
- Header-only implementation (no .cpp file needed)
- Configurable smoothing factor (alpha) and activity threshold
- Margin and per-neuron rate queries

**Key Methods (EMAOutputDetector):**
- `update(id, fired)` - Update firing rate for a neuron
- `getRate(id)` - Get current firing rate
- `predict(ids)` - Winner via argmax with threshold abstention
- `getMargin(ids)` - Confidence margin
- `reset()` - Clear tracked rates

**Network File Format (.net):**
```
# Comments start with #
NEURON <id> <threshold> <leak> <resting>
CONNECTION <from_id> <to_id> <weight>
```

## Architecture Principles

### Synchronous Tick-Based Simulation

All neurons update simultaneously each tick:
1. **Receive phase**: Neurons accumulate incoming spikes into buffers
2. **Update phase**: Apply membrane dynamics and check for firing
3. **Send phase**: Firing neurons transmit to connected targets

This ensures deterministic, reproducible behavior.

### Leak Dynamics

The membrane voltage update follows: `V(t+1) = leak * V(t) + incoming(t+1)`

- **leak = 1.0**: Voltage persists (integrator neuron)
- **leak = 0.0**: Voltage resets each tick (coincidence detector)
- **leak = 0.8**: Partial decay (typical for inhibitory pools)

### Spike-Based Communication

Neurons communicate via discrete spikes, not continuous values:
- Spike → sends `weight` to connected neurons
- Positive weights = excitation
- Negative weights = inhibition
- 1-tick synaptic delay enforced when `using_tick=true`

## Usage Example

```cpp
#include "glia.h"
#include "neuron.h"

// Create network with 2 sensory neurons, 3 interneurons
Glia network(2, 3);

// Load configuration
network.configureNetworkFromFile("my_network.net");

// Simulation loop
for (int t = 0; t < 100; ++t)
{
    // Inject sensory input
    network.injectSensory("S0", 100.0f);
    
    // Step forward
    network.step();
    
    // Monitor output
    Neuron* output = network.getNeuronById("O0");
    if (output->didFire())
    {
        std::cout << "Output fired at tick " << t << std::endl;
    }
}
```

## Files

- **neuron.h / neuron.cpp** - Individual neuron simulation
- **glia.h / glia.cpp** - Network management
- **output_detection.h** - Firing rate tracking and classification (header-only)
- **README.md** - This file

## Related Directories

- **`../testing/`** - Test implementations and toy examples
- **`../vis/`** - Visualization tools (future)
- **`../../docs/`** - Documentation and specifications

## Design Status

✅ **Working:**
- Basic neuron dynamics
- Synchronous tick updates
- Network file I/O
- Sensory injection
- Connection management
- Output detection and classification

🚧 **In Progress:**
- Refractory periods (implemented but disabled)
- Training/plasticity rules
- Performance optimization

📋 **Planned:**
- Neuron populations/groups
- Heterogeneous neuron types
- STDP and reward-modulated learning
- GPU acceleration

## Testing

The core architecture is tested via toy examples in `../testing/`:
- **XOR** - Logic gates with coincidence detection
- **3-Class** (planned) - Multi-class classification
- **Temporal** (planned) - Sequence detection

See `../testing/README.md` for details.
