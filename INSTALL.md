# GliaGL Installation Guide

## Quick Install (Python Package)

### From Source

```bash
cd GliaGL

# Install in development mode (recommended)
pip install -e .
```

This will:
1. Fetch pybind11 automatically
2. Compile the C++ code
3. Build the Python extension module `_core`
4. Install the `glia` Python package in editable mode

### Verify Installation

```bash
python python/test_import.py
```

Expected output:
```
============================================================
GliaGL Python Binding Test Suite
============================================================

[Import]
✓ Successfully imported glia version 0.1.0

[Network Creation]
✓ Created network: <Network neurons=5 connections=0>
  - Neuron count: 5
  - Connection count: 0

[Network Step]
✓ Network stepped successfully

[NumPy Interface]
✓ Got network state
  - IDs: ['S0', 'S1', 'N0', 'N1', 'N2']
  - Values shape: (5,)
  - Thresholds: [100. 100. 100. 100. 100.]
✓ Got network weights
  - Weight count: 0

[Training Config]
✓ Created training config: <TrainingConfig lr=0.010000 batch=1>

============================================================
Results: 5 passed, 0 failed
============================================================
```

## Requirements

- **Python**: 3.8 or higher
- **NumPy**: 1.20 or higher
- **C++ Compiler**: Supporting C++14
- **CMake**: 3.15 or higher

### Platform-Specific Setup

#### WSL/Ubuntu (Recommended)

```bash
sudo apt update
sudo apt install build-essential cmake python3-dev python3-pip
pip3 install numpy
```

#### Windows

**Option 1: Use WSL (Recommended)**
- Install WSL2
- Follow Ubuntu instructions above

**Option 2: Native Windows**
- Install Visual Studio 2019+ with "Desktop development with C++"
- Install CMake
- Install Python from python.org
- Use PowerShell or CMD to run `pip install -e .`

#### macOS

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Python
brew install python

# Install package
pip3 install -e .
```

## Development Installation

### With Additional Dependencies

```bash
# Install with all development tools
pip install -e ".[dev]"

# This includes:
# - pytest (testing)
# - black (formatting)
# - mypy (type checking)
# - jupyter (notebooks)
```

### With Visualization Tools

```bash
pip install -e ".[viz]"

# This includes:
# - matplotlib (plotting)
# - networkx (network analysis)
```

### Everything

```bash
pip install -e ".[all]"
```

## Building C++ Only (Without Python)

If you just want to build the C++ executables:

```bash
cd src/train
mkdir -p build && cd build
cmake ..
cmake --build . -j4

# Executables:
# - glia_eval
# - glia_miniworld
# - glia_3class_evo
# etc.
```

## Troubleshooting

### Error: "pybind11 not found"

The build system should fetch pybind11 automatically. If it fails:

```bash
pip install pybind11
pip install -e . --force-reinstall
```

### Error: "Python.h not found"

Install Python development headers:

**Ubuntu/WSL:**
```bash
sudo apt install python3-dev
```

**macOS:**
```bash
brew install python
```

### Error: "CMake version too old"

Install newer CMake:

**Ubuntu/WSL:**
```bash
# Add Kitware APT repository
sudo apt install software-properties-common
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake
```

**macOS:**
```bash
brew install cmake
```

### Build Errors with MSVC

If using Visual Studio, make sure to:
1. Install "Desktop development with C++"
2. Use "x64 Native Tools Command Prompt for VS"
3. Run `pip install -e .` from that prompt

### Import Error After Installation

If `import glia` fails:

```bash
# Force rebuild
pip install -e . --force-reinstall --no-deps

# Check if _core module exists
python -c "from glia import _core; print(_core)"
```

## Uninstall

```bash
pip uninstall glia
```

## Virtual Environment (Recommended)

For isolated development:

```bash
# Create virtual environment
python -m venv venv

# Activate (Linux/macOS)
source venv/bin/activate

# Activate (Windows)
venv\Scripts\activate

# Install
pip install -e ".[viz]"
```

See [VENV_SETUP.md](VENV_SETUP.md) for detailed guidance.

## Next Steps

After installation:

1. **Try examples**:
   ```bash
   python examples/python/01_basic_simulation.py
   python examples/python/02_training_basics.py
   python examples/python/03_numpy_integration.py
   python examples/python/04_visualization.py
   python examples/python/05_evolution.py
   ```

2. **Read documentation**:
   - [docs/QUICKSTART.md](docs/QUICKSTART.md) - 5-minute intro
   - [docs/API_REFERENCE.md](docs/API_REFERENCE.md) - Complete API
   - [docs/ADVANCED_USAGE.md](docs/ADVANCED_USAGE.md) - Advanced techniques
   - [docs/MIGRATION_GUIDE.md](docs/MIGRATION_GUIDE.md) - For CLI users

3. **Run tests**:
   ```bash
   python python/test_import.py
   python test_comprehensive.py
   ```

## Additional Resources

### Documentation

- **Quickstart**: [docs/QUICKSTART.md](docs/QUICKSTART.md)
- **API Reference**: [docs/API_REFERENCE.md](docs/API_REFERENCE.md)
- **Advanced Usage**: [docs/ADVANCED_USAGE.md](docs/ADVANCED_USAGE.md)
- **Migration Guide**: [docs/MIGRATION_GUIDE.md](docs/MIGRATION_GUIDE.md)
- **Examples**: [examples/python/README.md](examples/python/README.md)

### Support

If you encounter issues:
1. Check this troubleshooting guide
2. Verify requirements are met
3. Review [BUILD_FIXES.md](BUILD_FIXES.md) for known issues
4. Check GitHub issues
5. See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup

## Platform Notes

### WSL/Linux (Primary Development Platform)

GliaGL is developed primarily on WSL/Linux. All features are tested and supported:

```bash
# Recommended setup
sudo apt update
sudo apt install build-essential cmake python3-dev python3-pip python3-venv

# Clone and install
git clone <repo>
cd GliaGL
python3 -m venv venv
source venv/bin/activate
pip install -e ".[all]"

# Verify
python python/test_import.py
```

### Windows

Fully supported with MSVC:

```powershell
# Install Visual Studio Build Tools first
# Then:
pip install -e ".[all]"
```

### macOS

Fully supported with Xcode tools:

```bash
xcode-select --install
pip install -e ".[all]"
```
