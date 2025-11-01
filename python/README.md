# GliaGL Python Bindings

Python interface for GliaGL spiking neural network simulator.

## Installation

### From Source (Development)

```bash
# Clone repository
cd GliaGL

# Install in editable mode (recommended for development)
pip install -e .

# Or build and install
pip install .
```

### Requirements

- Python >= 3.8
- NumPy >= 1.20
- C++ compiler with C++14 support
- CMake >= 3.15

## Quick Start

```python
import glia
import numpy as np

# Create a network
net = glia.Network(num_sensory=2, num_neurons=5)

# Or load from file
net = glia.Network()
net.load("examples/xor/xor_network.net")

# Simulate
net.inject("S0", 100.0)
net.step()

# Access state as NumPy arrays
ids, values, thresholds, leaks = net.get_state()
print(f"Network has {len(ids)} neurons")
print(f"Thresholds: {np.array(thresholds)}")

# Get weights (sparse COO format)
from_ids, to_ids, weights = net.get_weights()
print(f"Network has {len(weights)} connections")
```

## Training

### Basic Training

```python
import glia

# Setup network
net = glia.Network()
net.load("network.net")

# Create trainer (uses gradient-based learning by default)
trainer = glia.Trainer(net)

# Configure training using helper
config = glia.create_config(
    lr=0.01,
    batch_size=16,
    warmup_ticks=50,
    decision_window=50,
    optimizer='adamw'  # 'sgd', 'adam', or 'adamw'
)

# Load or create dataset
dataset = glia.load_dataset_from_directory("data/train")

# Train with learning rate scheduling
history = trainer.train(
    dataset.episodes,
    epochs=20,
    config=config,
    lr_schedule='cosine',  # 'cosine', 'step', or None
    verbose=True
)

# Check results
print(f"Final accuracy: {history['accuracy'][-1]:.2%}")
print(f"Final margin: {history['margin'][-1]:.3f}")
```

### Learning Rate Scheduling

```python
# Cosine annealing (default) - smooth decay
trainer.train(dataset.episodes, epochs=20, lr_schedule='cosine')

# Step decay - drops by 50% every 1/3 of epochs
trainer.train(dataset.episodes, epochs=20, lr_schedule='step')

# Constant LR - no scheduling
trainer.train(dataset.episodes, epochs=20, lr_schedule=None)
```

### Advanced Options

```python
# Use Hebbian/reinforcement learning instead of gradient descent
trainer = glia.Trainer(net, use_gradient=False)

# Custom epoch callback
def on_epoch(epoch, acc, margin):
    print(f"Epoch {epoch}: acc={acc:.2%}, margin={margin:.3f}")

history = trainer.train(
    dataset.episodes,
    epochs=100,
    config=config,
    on_epoch=on_epoch,
    lr_schedule='cosine'
)
```

## Evolution

```python
import glia

# Configure evolution
evo_config = glia.EvolutionConfig()
evo_config.population = 10
evo_config.generations = 20
evo_config.elite = 2
evo_config.train_epochs = 5

# Create engine
evo = glia.EvolutionEngine(
    network_path="baseline.net",
    train_set=train_dataset,
    val_set=val_dataset,
    train_config=train_config,
    evo_config=evo_config
)

# Run evolution (releases GIL - can run in background thread)
result = evo.run()

print(f"Best fitness: {result.best_fitness_hist[-1]}")
print(f"Best accuracy: {result.best_acc_hist[-1]}")
```

## Building from Source

### Using pip (Recommended)

```bash
pip install -e .
```

This will:
1. Compile the C++ code
2. Build the Python extension module
3. Install in development mode

### Manual CMake Build

```bash
cd python
mkdir build && cd build
cmake ..
cmake --build . -j4

# The _core module will be in build/
```

## Testing

```bash
# Run import test
python tests/test_import.py

# Run API tests
python tests/test_api.py

# Run full test suite (after installing pytest)
pip install pytest
pytest tests/
```

## Architecture

```
python/
├── CMakeLists.txt         # Build configuration
├── src/
│   ├── bind_core.cpp      # Main module definition
│   ├── bind_network.cpp   # Network bindings
│   ├── bind_neuron.cpp    # Neuron bindings
│   ├── bind_training.cpp  # Training bindings
│   └── bind_evolution.cpp # Evolution bindings
├── glia/
│   └── __init__.py        # Python package
└── test_import.py         # Basic tests
```

## Performance

The Python bindings use pybind11 with:
- **Zero-copy NumPy arrays** where possible
- **GIL release** during compute-heavy operations
- **Efficient data transfer** via `std::vector`

Training/evolution speed is identical to C++ (Python overhead < 1%).

## Thread Safety

Each `Network`, `Trainer`, and `EvolutionEngine` instance can be used from only one thread at a time. For multi-threading:

```python
import threading
import glia

def train_network(net_path, dataset):
    net = glia.Network()
    net.load(net_path)
    trainer = glia.Trainer(net)
    # GIL is released during training
    trainer.train_epoch(dataset, epochs=100, config=config)

# These run concurrently (GIL released)
t1 = threading.Thread(target=train_network, args=("net1.net", data1))
t2 = threading.Thread(target=train_network, args=("net2.net", data2))
t1.start()
t2.start()
t1.join()
t2.join()
```

## Troubleshooting

### Import Error

If you get `ImportError: cannot import name '_core'`:

```bash
# Rebuild the extension
pip install -e . --force-reinstall --no-deps
```

### Build Errors

Make sure you have:
- C++ compiler installed
- CMake >= 3.15
- Python development headers

**On Ubuntu/WSL:**
```bash
sudo apt install build-essential cmake python3-dev
```

**On Windows:**
- Install Visual Studio with C++ tools
- Or use WSL (recommended)

## Next Steps

See:
- `docs/numpy_interface.md` - NumPy usage examples
- `docs/gil_release_points.md` - Threading guide
- `examples/python/` - Complete examples (Stage 5)
