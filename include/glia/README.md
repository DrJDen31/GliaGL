# GliaGL Public API Headers

This directory contains the clean, public-facing API headers for GliaGL. These headers are designed for:

1. **Python bindings** via pybind11
2. **C++ library usage** by external projects
3. **API stability** - changes here affect the public interface

## Header Organization

### `glia.h` - Main Entry Point
Include this single header to get the complete API:
```cpp
#include <glia/glia.h>
```

### Core Components

- **`types.h`** - Common types and structures
  - `EpisodeMetrics` - Results from episode evaluation
  - `EvolutionMetrics` - Evolutionary training metrics
  - `NetworkSnapshot` - Network state for checkpointing

- **`network.h`** - Neural network interface
  - `Network` class - Load, save, simulate spiking networks
  - Main simulation loop (`step()`)
  - Sensory input injection

- **`trainer.h`** - Training interface
  - `Trainer` class - Gradient-based training
  - Batch and epoch-level training
  - Training history and checkpointing

- **`evolution.h`** - Evolutionary training interface
  - `Evolution` class - Lamarckian evolution
  - Custom fitness functions
  - Generation callbacks

## Design Principles

### 1. PIMPL Pattern
All classes use the Pointer to Implementation (PIMPL) pattern:
```cpp
class Network {
    std::shared_ptr<class Glia> impl_;  // Internal implementation
};
```
This provides:
- **ABI stability** - internal changes don't break binary compatibility
- **Fast compilation** - implementation headers not exposed
- **Clean Python bindings** - only public interface is bound

### 2. Smart Pointers
All object ownership uses `std::shared_ptr`:
- Safe for Python/C++ interop
- Automatic memory management
- Thread-safe reference counting

### 3. Namespace Isolation
- **Public API**: `glia::` namespace
- **Internal implementation**: Global namespace (not exposed to Python)

## For Python Bindings

The pybind11 binding code should only include these headers:
```cpp
#include <glia/glia.h>
// Bindings use glia::Network, glia::Trainer, glia::Evolution
```

Implementation headers (`src/`) should NOT be included in binding code.

## For C++ Users

Link against the GliaGL library and include the public headers:
```cpp
#include <glia/glia.h>

int main() {
    auto net = std::make_shared<glia::Network>();
    net->load("network.net");
    net->step();
}
```

## Internal Implementation

The actual implementation lives in `src/`:
- `src/arch/` - Core network engine (Glia, Neuron classes)
- `src/train/` - Training algorithms
- `src/evo/` - Evolution engine

These are NOT part of the public API and can change without notice.
