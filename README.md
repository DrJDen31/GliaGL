# GliaGL

**Fast spiking neural network simulator with Python API**

[![Build Status](https://github.com/DrJDen31/GliaGL/workflows/build-and-test/badge.svg)](https://github.com/DrJDen31/GliaGL/actions)
[![PyPI version](https://badge.fury.io/py/glia.svg)](https://pypi.org/project/glia/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

GliaGL is a high-performance spiking neural network simulator with a clean Python API. It combines C++ speed with Python flexibility for research and applications.

## Features

✨ **Python-First API** - PyTorch-inspired interface  
⚡ **High Performance** - C++ core with <1% Python overhead  
🔢 **NumPy Integration** - Zero-copy array access  
📊 **Built-in Visualization** - Network graphs, training curves, activity rasters  
🧬 **Evolutionary Training** - Lamarckian evolution with population-based search  
🎯 **Gradient Training** - Eligibility traces and intrinsic plasticity  
🔧 **Flexible** - Custom training loops, callbacks, analysis

## Installation

```bash
pip install glia
```

Or from source:

```bash
git clone https://github.com/DrJDen31/GliaGL.git
cd GliaGL
pip install -e .
```

See [INSTALL.md](INSTALL.md) for detailed instructions.

## Quick Start

```python
import glia
import numpy as np

# Create or load network
net = glia.Network(num_sensory=2, num_neurons=5)
# or: net = glia.Network.from_file("network.net")

# Simple simulation
net.inject_dict({"S0": 100.0, "S1": 50.0})
net.step(n_steps=10)

# Access state as NumPy arrays
ids, values, thresholds, leaks = net.get_state()
print(f"Thresholds: {thresholds}")

# Training
dataset = glia.load_dataset_from_directory("data/")
trainer = glia.Trainer(net)
history = trainer.train(
    dataset.episodes,
    epochs=100,
    on_epoch=lambda e, acc, m: print(f"Epoch {e}: {acc:.2%}")
)

# Evolution
evo_cfg = glia.create_evo_config(population=10, generations=20)
evo = glia.Evolution("baseline.net", train_data, val_data, train_cfg, evo_cfg)
result = evo.run()
```

## Examples

Run the included examples:

```bash
cd examples/python

python 01_basic_simulation.py    # Network basics
python 02_training_basics.py     # Training workflow
python 03_numpy_integration.py   # NumPy integration
python 04_visualization.py       # Plotting
python 05_evolution.py           # Evolutionary training
```

Or try interactive Jupyter notebooks:

```bash
cd examples/notebooks
jupyter notebook
# Open 01_quickstart.ipynb
```

See [examples/python/README.md](examples/python/README.md) and [examples/notebooks/README.md](examples/notebooks/README.md) for details.

## Documentation

- **[Quickstart](docs/user-guide/QUICKSTART.md)** - Get started in 5 minutes
- **[API Reference](docs/user-guide/API_REFERENCE.md)** - Complete API documentation
- **[Advanced Usage](docs/user-guide/ADVANCED_USAGE.md)** - Advanced techniques
- **[Migration Guide](docs/user-guide/MIGRATION_GUIDE.md)** - Migrating from CLI
- **[All Documentation](docs/README.md)** - Complete documentation index

## Architecture

```
GliaGL/
├── include/glia/      # Public C++ API headers
├── src/               # C++ implementation
│   ├── arch/          # Core network engine
│   ├── train/         # Training algorithms
│   └── evo/           # Evolution engine
├── python/
│   ├── src/           # pybind11 bindings
│   └── glia/          # Python package
├── examples/
│   ├── python/        # Python examples
│   ├── notebooks/     # Jupyter notebooks
│   ├── 3class/        # 3-class classification
│   ├── xor/           # XOR problem
│   └── seq_digits_poisson/  # Digit classification
├── docs/              # Documentation
│   ├── user-guide/    # User documentation
│   ├── development/   # Development docs
│   └── reference/     # Technical reference
└── tests/             # Test suite
```

## Performance

- **Training**: Full C++ speed with GIL release
- **NumPy**: Zero-copy array access
- **Overhead**: <1% Python wrapper cost
- **Threading**: Multi-threaded capable

## Key Capabilities

### Network Simulation

- Leaky integrate-and-fire neurons
- Flexible connectivity
- Sensory input injection
- State inspection and modification

### Training

- Gradient-based learning with eligibility traces
- Intrinsic plasticity (threshold/leak adaptation)
- Structural plasticity (pruning/growth)
- Custom callbacks and monitoring

### Evolution

- Population-based training
- Lamarckian evolution (learned weights inherited)
- Multi-objective fitness (accuracy, margin, sparsity)
- Progress visualization

### Visualization

```python
import glia.viz as viz

viz.plot_network_graph(net)
viz.plot_weight_distribution(net)
viz.plot_training_history(trainer.history)
viz.plot_neuron_activity(net, simulation_steps=100)
```

## Migrating from CLI

If you previously used the C++ CLI tools, see our [migration guide](docs/user-guide/MIGRATION_GUIDE.md).

**Old (CLI)**:

```bash
./glia_eval network.net sequences.seq epochs=100 lr=0.01
```

**New (Python)**:

```python
import glia
net = glia.Network.from_file("network.net")
trainer = glia.Trainer(net)
history = trainer.train(dataset, epochs=100)
```

## Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Development

```bash
# Clone repository
git clone https://github.com/DrJDen31/GliaGL.git
cd GliaGL

# Install in development mode
pip install -e ".[dev]"

# Run tests
python python/test_import.py
python python/test_api.py

# Build and test
bash scripts/build_local.sh
```

## Citation

If you use GliaGL in your research, please cite:

```bibtex
@software{gliagl2025,
  title = {GliaGL: Fast Spiking Neural Network Simulator},
  author = {GliaGL Contributors},
  year = {2025},
  url = {https://github.com/DrJDen31/GliaGL}
}
```

## License

MIT License - see [LICENSE](LICENSE) for details.

## Links

- **Documentation**: [docs/README.md](docs/README.md)
- **Python Examples**: [examples/python/](examples/python/)
- **Jupyter Notebooks**: [examples/notebooks/](examples/notebooks/)
- **Issue Tracker**: [GitHub Issues](https://github.com/DrJDen31/GliaGL/issues)
- **Discussions**: [GitHub Discussions](https://github.com/DrJDen31/GliaGL/discussions)

## Acknowledgments

Built on:

- [pybind11](https://github.com/pybind/pybind11) - C++/Python bindings
- [NumPy](https://numpy.org/) - Numerical computing
- C++14 standard library

---

**Status**: Active development | **Version**: 0.1.0 | **Python**: 3.8+
