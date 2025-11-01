# GliaGL Python Examples

This directory contains Python examples demonstrating GliaGL's capabilities.

## Quick Start

Run examples in order:

```bash
cd examples/python

# Basic usage
python 01_basic_simulation.py
python 02_training_basics.py
python 03_numpy_integration.py

# Advanced features
python 04_visualization.py      # Requires matplotlib, networkx
python 05_evolution.py
```

## Examples Overview

### 01_basic_simulation.py
**What it demonstrates:**
- Creating a simple network
- Injecting sensory inputs
- Running simulation timesteps
- Monitoring neuron firing activity

**Key concepts:**
- Network initialization
- Input injection
- Simulation loop
- Activity analysis

**Duration:** ~5 seconds

---

### 02_training_basics.py
**What it demonstrates:**
- Loading/creating networks
- Creating datasets
- Training with callbacks
- Evaluation
- Saving trained networks

**Key concepts:**
- Dataset creation
- Training configuration
- Epoch callbacks
- Train/validation split
- Model persistence

**Duration:** ~10 seconds

---

### 03_numpy_integration.py
**What it demonstrates:**
- Zero-copy NumPy array access
- State manipulation with NumPy
- Weight matrix operations
- Sparse matrix conversion
- Vectorized operations

**Key concepts:**
- NumPy integration
- Zero-copy data access
- Sparse matrices (scipy)
- Batch operations
- Temporal analysis

**Duration:** ~5 seconds

---

### 04_visualization.py
**What it demonstrates:**
- Network graph visualization
- Weight distribution plots
- Training history plots
- Neuron activity raster plots
- Custom matplotlib integration

**Key concepts:**
- Network visualization
- Training monitoring
- Activity analysis
- Custom plotting

**Requirements:** `matplotlib`, `networkx`

**Duration:** ~15 seconds

---

### 05_evolution.py
**What it demonstrates:**
- Setting up evolutionary training
- Configuring evolution parameters
- Running evolution
- Analyzing results
- Loading best genome

**Key concepts:**
- Evolutionary algorithms
- Lamarckian evolution
- Fitness evaluation
- Genome extraction
- Baseline comparison

**Duration:** ~1-2 minutes

---

## Installation

### Basic (Required)
```bash
pip install glia numpy
```

### With Visualization
```bash
pip install glia[viz]
# or manually:
pip install glia numpy matplotlib networkx
```

### With All Features
```bash
pip install glia[all]
```

## Running Examples

### Individual Examples
```bash
python 01_basic_simulation.py
```

### All Examples
```bash
for example in 0*.py; do
    echo "Running $example..."
    python "$example"
    echo ""
done
```

### Quick Start Example
```bash
# From repository root
python python/examples/quick_start.py
```

## Example Output

Each example produces:
- **Console output**: Progress and results
- **Files**: Networks, visualizations, data
- **Exit code**: 0 on success

### Generated Files

Examples may create:
- `*.net` files (network snapshots)
- `*.png` files (visualizations)
- `*.json` files (metrics)

These are in `.gitignore` and safe to delete.

## Customization

All examples are fully commented and easy to modify:

```python
# Change network size
net = glia.Network(num_sensory=5, num_neurons=20)  # Bigger network

# Change training duration
history = trainer.train(dataset.episodes, epochs=200)  # More epochs

# Change evolution parameters
evo_cfg.population = 20  # Larger population
evo_cfg.generations = 50  # More generations
```

## Interactive Usage

Examples work great in Jupyter:

```bash
jupyter notebook
# Open any example and run cell-by-cell
```

Or IPython:

```bash
ipython
%run 01_basic_simulation.py
```

## Troubleshooting

### Import Error
```
ImportError: No module named 'glia'
```
**Solution:** Install package
```bash
pip install -e .  # From repo root
```

### Visualization Error
```
ImportError: No module named 'matplotlib'
```
**Solution:** Install visualization dependencies
```bash
pip install matplotlib networkx
```

### Performance Issues
- Examples use small networks for speed
- Increase sizes for more realistic scenarios
- Use `train_fast()` instead of `train()` for speed

## Next Steps

After running examples:

1. **Read Documentation**
   - [docs/QUICKSTART.md](../../docs/QUICKSTART.md) - 5-minute intro
   - [docs/API_REFERENCE.md](../../docs/API_REFERENCE.md) - Complete API reference
   - [docs/ADVANCED_USAGE.md](../../docs/ADVANCED_USAGE.md) - Advanced techniques
   - [docs/MIGRATION_GUIDE.md](../../docs/MIGRATION_GUIDE.md) - For CLI users

2. **Explore More**
   - Try modifying examples for your use case
   - Combine techniques from multiple examples
   - Create your own custom workflows

3. **Build Your Own**
   - Start with `02_training_basics.py` as template
   - Modify for your specific task
   - Refer to API docs as needed

## Example Data

Examples create synthetic data for demonstration. For real tasks:

1. **Load your own data:**
   ```python
   dataset = glia.load_dataset_from_directory("my_data/")
   ```

2. **Use existing GliaGL datasets:**
   ```bash
   # Example networks in parent directory
   ls ../*/readout*.net
   ls ../*/baseline*.net
   ```

3. **Create from arrays:**
   ```python
   import numpy as np
   data = np.load("my_data.npy")
   seq = glia.create_sequence_from_array(data, neuron_ids)
   ```

## Performance Notes

**Example timings** (approximate):
- Basic simulation: 5 seconds
- Training: 10-30 seconds
- Evolution: 1-2 minutes

**Factors affecting speed:**
- Network size (neurons, connections)
- Dataset size (episodes, timesteps)
- Training duration (epochs)
- Population size (evolution)

**To speed up:**
- Use `trainer.train_fast()` (no callbacks)
- Reduce epochs for experimentation
- Smaller networks for prototyping
- GIL is released during compute

## Contributing

Found an issue or have an improvement?
- File an issue on GitHub
- Submit a pull request
- See `CONTRIBUTING.md`

## License

Examples are part of GliaGL and use the same MIT License.

---

**Happy experimenting with GliaGL!** 🚀

For questions, see:
- **Documentation**: [docs/](../../docs/)
  - [QUICKSTART.md](../../docs/QUICKSTART.md) - Get started in 5 minutes
  - [API_REFERENCE.md](../../docs/API_REFERENCE.md) - Complete API
  - [ADVANCED_USAGE.md](../../docs/ADVANCED_USAGE.md) - Advanced patterns
- **GitHub**: https://github.com/yourusername/GliaGL
- **Contributing**: [CONTRIBUTING.md](../../CONTRIBUTING.md)
