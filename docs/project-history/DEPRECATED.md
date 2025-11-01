# Deprecated Features

This document tracks features that have been deprecated or removed as part of the Python transformation.

## CLI Tools (Deprecated)

### Training Executable - REMOVED

**Old (CLI)**:
```bash
cd src/train/build
./glia_eval --scenario xor --epochs 100 --lr 0.01
```

**Removed files**:
- `src/train/eval_main.cpp`
- Associated build configurations

**Replacement (Python)**:
```python
import glia

net = glia.Network.from_file("network.net")
trainer = glia.Trainer(net)
config = glia.create_config(lr=0.01)
history = trainer.train(dataset.episodes, epochs=100)
```

**Why removed**: CLI tool had limited flexibility. Python API provides:
- Interactive experimentation
- Custom training loops
- Better error messages
- Integration with ML ecosystem

---

### Mini-World Training - REMOVED

**Old (CLI)**:
```bash
./glia_miniworld --config configs/default.yaml
```

**Removed files**:
- `src/train/mini_world_main.cpp`

**Replacement (Python)**:
```python
import glia

net = glia.Network.from_file("miniworld.net")
dataset = glia.load_dataset_from_directory("data/miniworld/")
trainer = glia.Trainer(net)
history = trainer.train(dataset.episodes, epochs=100)
```

---

### Evolutionary Training C++ - REMOVED

**Old (CLI)**:
```bash
cd examples/3class/evaluator
./3class_evo --population 10 --generations 20
```

**Removed files**:
- `examples/3class/evaluator/*.cpp`
- `examples/3class/evaluator/*.h`
- `examples/mini-world/evaluator/*.cpp`
- `examples/mini-world/evaluator/*.h`

**Replacement (Python)**:
```python
import glia

# Configure
train_cfg = glia.create_config(lr=0.01)
evo_cfg = glia.create_evo_config(
    population=10,
    generations=20,
    elite=2
)

# Run evolution
evo = glia.Evolution(
    "baseline.net",
    train_data,
    val_data,
    train_cfg,
    evo_cfg
)
result = evo.run()

# Get best network
best = glia.Evolution.load_best_genome("baseline.net", result)
```

**Why removed**: Python version provides:
- Progress monitoring
- Visualization
- Flexible callbacks
- Easy experimentation
- Better result analysis

---

### Build Scripts - REMOVED

**Old**:
```bash
# Various per-example build scripts
bash examples/3class/build.sh
powershell examples/3class/build.ps1
```

**Removed files**:
- `examples/3class/*.sh`
- `examples/3class/*.ps1`
- `examples/mini-world/*.ps1`
- Other scattered build scripts

**Replacement**:
```bash
# Single unified build
pip install -e .

# Or use provided scripts
bash scripts/build_local.sh
```

**Why removed**: 
- Fragmented build process
- Hard to maintain
- Modern Python tooling is better
- One command to build everything

---

## Visualization (Deprecated - OpenGL)

### OpenGL Network Visualizer - FROZEN

**Status**: Code kept but unmaintained

**Location**: `src/vis/`

**What it was**: Real-time 3D network visualization using OpenGL

**Why frozen**: 
- Complex dependencies (OpenGL, GLFW, GLM)
- Platform-specific issues
- Limited functionality

**Replacement (Python)**:
```python
import glia.viz as viz

# 2D network graph (better for analysis)
viz.plot_network_graph(net, save_path="network.png")

# Weight distribution
viz.plot_weight_distribution(net)

# Training curves
viz.plot_training_history(trainer.history)

# Activity rasters
viz.plot_neuron_activity(net, simulation_steps=100)
```

**Note**: OpenGL visualizer can still be built for those who need it, but it's not maintained or tested.

---

## Data Formats (Still Supported)

### Network Files (.net)

**Status**: ✅ Fully supported

Still the primary network storage format. Can be:
- Loaded in Python: `glia.Network.from_file("network.net")`
- Saved from Python: `net.save("network.net")`
- Hand-edited (text format)

### Sequence Files (.seq)

**Status**: ✅ Fully supported

Still used for input sequences. Can be:
- Loaded in Python: `glia.load_sequence_file("input.seq")`
- Created from NumPy: `glia.create_sequence_from_array(array, ids)`
- Loaded as dataset: `glia.load_dataset_from_directory("data/")`

---

## Configuration

### Command-Line Flags - REMOVED

**Old**:
```bash
./glia_eval \
    --epochs 100 \
    --lr 0.01 \
    --batch-size 4 \
    --warmup 50 \
    --decision-window 50
```

**Replacement (Python)**:
```python
config = glia.create_config(
    lr=0.01,
    batch_size=4,
    warmup_ticks=50,
    decision_window=50
)
trainer.train(dataset.episodes, epochs=100, config=config)
```

**Benefits**:
- Type-safe
- Documented (docstrings)
- Autocomplete in IDEs
- Can be serialized/loaded
- Programmatically modifiable

---

## Build System

### Manual CMake Builds - PARTIALLY DEPRECATED

**Old workflow**:
```bash
cd src/train
mkdir build && cd build
cmake ..
make
./glia_eval ...
```

**New workflow**:
```bash
# Python package handles everything
pip install -e .
python examples/python/01_basic_simulation.py
```

**Note**: Manual CMake builds still work for C++ development, but not needed for normal use.

---

## Testing

### Manual Test Programs - REMOVED

**Old**: Scattered C++ test executables

**Removed**:
- Various test main files
- Manual test scripts

**Replacement**:
```bash
# Python test suite
python python/test_import.py
python python/test_api.py

# Or with pytest
pytest tests/
```

---

## Documentation

### CLI-Focused Docs - UPDATED

**Changed**: All documentation now Python-first

**Updates**:
- README.md → Python quick start
- Examples → Python scripts
- Tutorials → Python workflows
- API docs → Python API reference

**Legacy docs**: Archived in `docs/archive/` if needed

---

## Migration Guide

### For Existing Users

If you have existing workflows:

1. **Installation**:
   ```bash
   pip install glia  # or pip install -e .
   ```

2. **Convert scripts**:
   - See `docs/cli_to_python_migration.md`
   - Examples in `examples/python/`
   - API reference in `docs/python_api_guide.md`

3. **Data compatibility**:
   - `.net` files work unchanged
   - `.seq` files work unchanged
   - Just load them in Python

4. **Performance**:
   - Python API has <1% overhead
   - Training/evolution speed identical
   - GIL released during compute

---

## Why These Changes?

### Benefits of Python API

1. **Accessibility**: Python is easier to learn than C++
2. **Flexibility**: Custom training loops, callbacks, analysis
3. **Integration**: Works with NumPy, Pandas, Matplotlib, Jupyter
4. **Experimentation**: Interactive notebooks, rapid iteration
5. **Community**: Larger ML/Python ecosystem
6. **Maintenance**: Single codebase instead of CLI + Python

### What We Kept

- ✅ All core C++ performance
- ✅ All network formats
- ✅ All data formats
- ✅ All algorithms
- ✅ All functionality (via Python)

### What We Gained

- ✅ NumPy integration
- ✅ Visualization tools
- ✅ Interactive exploration
- ✅ Better error messages
- ✅ Type hints (future)
- ✅ Easier debugging

---

## FAQ

### Can I still use the CLI tools?

The CLI tools have been removed. The Python API provides all the same functionality with better flexibility. See migration guide.

### Will old .net files work?

Yes! All network files are compatible.

### Is the Python version slower?

No. The Python overhead is <1%, and the GIL is released during training/evolution, so it's essentially the same speed.

### Can I still build the C++ code directly?

Yes, for C++ development. But normal users should use `pip install`.

### Where are the old files?

They're in git history. You can check out old commits if needed. But we recommend migrating to Python.

### What about the OpenGL visualizer?

It's frozen but still in the repository. You can build it if needed, but it's not maintained. Use `glia.viz` instead.

---

## Timeline

- **Before 2025**: CLI-only C++ tools
- **January 2025**: Python transformation (Stages 1-5)
- **January 2025**: Repository cleanup (Stage 6)
- **Current**: Python-first, CLI deprecated

---

## Support

- **Python API**: Full support
- **CLI tools**: Removed, use Python
- **Migration help**: See `docs/cli_to_python_migration.md`
- **Questions**: File GitHub issue

---

**Last Updated**: January 2025  
**Version**: 0.1.0
