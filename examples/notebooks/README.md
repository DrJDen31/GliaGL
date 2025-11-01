# GliaGL Jupyter Notebook Examples

Interactive tutorials for learning GliaGL in Jupyter notebooks.

---

## Notebooks

### 01_quickstart.ipynb
**Perfect for**: First-time users  
**Duration**: 15 minutes  
**Topics**:
- Creating networks
- Running simulations
- Adding connections
- Inspecting state
- Basic visualization
- Saving/loading

### 02_training.ipynb
**Perfect for**: Learning training workflows  
**Duration**: 20 minutes  
**Topics**:
- Creating datasets
- Training configuration
- Running training loops
- Evaluating performance
- Monitoring progress
- Visualizing results

### 03_evolution.ipynb
**Perfect for**: Evolutionary algorithms  
**Duration**: 25 minutes  
**Topics**:
- Setting up evolution
- Configuring parameters
- Running evolution
- Analyzing results
- Comparing with gradient training
- Lamarckian evolution

### 04_advanced.ipynb
**Perfect for**: Advanced users  
**Duration**: 30 minutes  
**Topics**:
- Custom training loops
- Sparse matrix operations
- Memory-efficient collection
- Vectorized updates
- Custom architectures
- Performance profiling

---

## Getting Started

### Installation

```bash
# Install GliaGL with visualization
pip install -e ".[viz]"

# Or install Jupyter separately
pip install -e .
pip install jupyter matplotlib networkx
```

### Running Notebooks

```bash
# Start Jupyter
jupyter notebook

# Or use JupyterLab
jupyter lab
```

Then navigate to `examples/notebooks/` and open any notebook.

---

## Learning Path

**Recommended order**:

```
01_quickstart.ipynb
    ↓
02_training.ipynb
    ↓
03_evolution.ipynb
    ↓
04_advanced.ipynb
```

**Time to completion**: ~90 minutes total

---

## Interactive Features

These notebooks are designed for **interactive exploration**:

- **Modify code** and re-run cells
- **Change parameters** to see different results
- **Visualize** networks and training progress
- **Experiment** with your own data
- **Save** your modified notebooks

---

## Tips for Jupyter

### Useful Shortcuts

- `Shift+Enter`: Run cell and move to next
- `Ctrl+Enter`: Run cell and stay
- `Alt+Enter`: Run cell and insert below
- `A`: Insert cell above
- `B`: Insert cell below
- `DD`: Delete cell
- `M`: Convert to markdown
- `Y`: Convert to code

### Best Practices

1. **Run cells in order** on first pass
2. **Restart kernel** if things break (`Kernel → Restart`)
3. **Save frequently** (`Ctrl+S`)
4. **Clear output** before committing to git
5. **Use markdown cells** to document your experiments

---

## Troubleshooting

### Kernel Not Found

```bash
# Install ipykernel in your environment
pip install ipykernel
python -m ipykernel install --user --name=glia
```

### Import Error

```bash
# Make sure GliaGL is installed
pip install -e .

# Restart Jupyter kernel
```

### Visualization Not Working

```bash
# Install visualization dependencies
pip install matplotlib networkx

# Restart kernel
```

---

## Extending the Notebooks

Feel free to:

- **Add your own cells** with experiments
- **Modify examples** for your use case
- **Save variations** with different names
- **Share** your notebooks with the community

---

## Converting to Scripts

Convert any notebook to a Python script:

```bash
jupyter nbconvert --to script 01_quickstart.ipynb
```

This creates `01_quickstart.py` you can run with:

```bash
python 01_quickstart.py
```

---

## Next Steps

After completing these notebooks:

1. **Explore Python examples**: `examples/python/`
2. **Read documentation**: `docs/user-guide/`
3. **Try advanced techniques**: `docs/user-guide/ADVANCED_USAGE.md`
4. **Build your own project**: Use notebooks as templates

---

## Contributing

Found an issue or have an improvement?

1. File an issue on GitHub
2. Submit a pull request
3. See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Requirements

- Python 3.8+
- GliaGL (`pip install -e .`)
- Jupyter (`pip install jupyter`)
- NumPy (required)
- Matplotlib (for visualization)
- NetworkX (for visualization)
- SciPy (for advanced notebook)

---

**Happy learning with GliaGL!** 🚀
