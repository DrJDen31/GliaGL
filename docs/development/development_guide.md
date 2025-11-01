# GliaGL Development Guide

This guide is for developers working on GliaGL itself.

## Development Setup

### Prerequisites

- **Python**: 3.8 or higher
- **C++ Compiler**: 
  - Linux: GCC 7+ or Clang 5+
  - macOS: Xcode Command Line Tools
  - Windows: Visual Studio 2019+ with C++ tools
- **CMake**: 3.15 or higher
- **Git**: For version control

### Initial Setup

#### Linux/WSL (Recommended)

```bash
# Install system dependencies
sudo apt update
sudo apt install build-essential cmake python3-dev python3-pip git

# Clone repository
git clone https://github.com/yourusername/GliaGL.git
cd GliaGL

# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install in development mode with all extras
pip install -e ".[dev,viz,all]"

# Verify installation
python python/test_import.py
```

#### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake python git

# Clone and setup (same as Linux)
git clone https://github.com/yourusername/GliaGL.git
cd GliaGL
python3 -m venv venv
source venv/bin/activate
pip install -e ".[dev,viz,all]"
```

#### Windows

```powershell
# Install Visual Studio with C++ tools from:
# https://visualstudio.microsoft.com/downloads/

# Install CMake from:
# https://cmake.org/download/

# Install Python from:
# https://www.python.org/downloads/

# Clone repository
git clone https://github.com/yourusername/GliaGL.git
cd GliaGL

# Create virtual environment
python -m venv venv
venv\Scripts\activate

# Install in development mode
pip install -e ".[dev,viz,all]"

# Verify
python python\test_import.py
```

## Repository Structure

```
GliaGL/
├── include/glia/          # Public C++ API headers
├── src/                   # C++ implementation
│   ├── arch/              # Core network engine
│   ├── train/             # Training algorithms
│   └── evo/               # Evolution engine
├── python/
│   ├── src/               # pybind11 bindings
│   ├── glia/              # Python package
│   ├── examples/          # Python examples
│   ├── test_import.py     # Basic tests
│   └── test_api.py        # API tests
├── docs/                  # Documentation
├── examples/              # C++ examples and networks
├── scripts/               # Build scripts
└── .github/workflows/     # CI/CD
```

## Development Workflow

### Making Changes

1. **Create a branch**
   ```bash
   git checkout -b feature/my-feature
   ```

2. **Make changes**
   - C++ code in `src/` or `include/`
   - Python bindings in `python/src/`
   - Python API in `python/glia/`

3. **Test changes**
   ```bash
   # Rebuild after C++ changes
   pip install -e . --force-reinstall --no-deps
   
   # Run tests
   python python/test_import.py
   python python/test_api.py
   
   # Or use test script
   bash scripts/test_all.sh
   ```

4. **Commit and push**
   ```bash
   git add .
   git commit -m "Add feature: description"
   git push origin feature/my-feature
   ```

5. **Create pull request** on GitHub

### C++ Development

#### Compiling C++ Directly

For faster iteration on C++ code without Python:

```bash
cd src/train
mkdir -p build && cd build
cmake ..
cmake --build . -j4

# Run C++ executables
./glia_eval
./glia_miniworld
```

#### Adding New C++ Methods

1. **Add to header** (`src/arch/glia.h`):
   ```cpp
   // In public section
   void myNewMethod(int param);
   ```

2. **Implement** (`src/arch/glia.cpp`):
   ```cpp
   void Glia::myNewMethod(int param) {
       // Implementation
   }
   ```

3. **Add Python binding** (`python/src/bind_network.cpp`):
   ```cpp
   .def("my_new_method", &Glia::myNewMethod,
        py::arg("param"),
        "Method description")
   ```

4. **Add Python wrapper** (`python/glia/network.py`):
   ```python
   def my_new_method(self, param: int) -> None:
       """Pythonic wrapper with docs"""
       self._net.my_new_method(param)
   ```

5. **Rebuild and test**:
   ```bash
   pip install -e . --force-reinstall --no-deps
   python -c "import glia; net = glia.Network(); net.my_new_method(5)"
   ```

### Python Development

Python-only changes don't require rebuilding:

```bash
# Edit python/glia/*.py files
# Changes take effect immediately (dev mode)

python python/test_api.py
```

### Adding Dependencies

#### Python Dependencies

Edit `pyproject.toml`:
```toml
dependencies = [
    "numpy>=1.20",
    "your-new-package>=1.0",
]
```

Then:
```bash
pip install -e .
```

#### C++ Dependencies

For header-only libraries, add to `python/CMakeLists.txt`:
```cmake
include(FetchContent)
FetchContent_Declare(
    your_library
    GIT_REPOSITORY https://github.com/user/library.git
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(your_library)

target_link_libraries(_core PRIVATE your_library)
```

## Testing

### Running Tests

```bash
# Basic functionality
python python/test_import.py

# High-level API
python python/test_api.py

# All tests
bash scripts/test_all.sh

# With pytest (if available)
pytest tests/ -v
```

### Writing Tests

Add tests to `python/test_api.py`:

```python
def test_my_feature():
    """Test description"""
    import glia
    
    # Setup
    net = glia.Network(2, 3)
    
    # Test
    result = net.my_feature()
    
    # Verify
    assert result == expected
    print("✓ Test passed")
    
    return True
```

Then add to test list in `main()`.

### Performance Testing

```python
import time
import glia

net = glia.Network(100, 500)  # Larger network
dataset = [...]  # Large dataset

start = time.time()
trainer = glia.Trainer(net)
history = trainer.train_fast(dataset, epochs=100)
elapsed = time.time() - start

print(f"Training took {elapsed:.2f}s")
```

## Building and Packaging

### Local Build

```bash
# Clean build
bash scripts/build_local.sh

# Or manually
rm -rf build/ dist/ *.egg-info
python -m build
pip install dist/*.whl
```

### Building Wheels

```bash
# Install cibuildwheel
pip install cibuildwheel

# Build for current platform
cibuildwheel --platform linux  # or macos, windows

# Wheels will be in wheelhouse/
```

### Source Distribution

```bash
python -m build --sdist
# Creates dist/glia-0.1.0.tar.gz
```

## Documentation

### Adding Documentation

1. **Code docstrings**:
   ```python
   def my_function(arg: str) -> int:
       """
       Brief description.
       
       Args:
           arg: Argument description
           
       Returns:
           Return value description
           
       Example:
           >>> result = my_function("test")
           >>> print(result)
           42
       """
   ```

2. **Markdown docs** in `docs/`:
   - Add new `.md` file
   - Update relevant guides
   - Add to README if needed

3. **Examples** in `python/examples/`:
   - Create runnable example
   - Add comments explaining each step
   - Test that it works

### Documentation Build

Currently using Markdown. Future: Sphinx for API docs.

## Debugging

### C++ Debugging

```bash
# Build with debug symbols
cd src/train/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Run with gdb
gdb ./glia_eval
```

### Python Debugging

```python
import glia
import pdb

net = glia.Network(2, 3)
pdb.set_trace()  # Debugger breakpoint
net.step()
```

### Print Debugging

C++:
```cpp
std::cout << "Debug: value = " << value << std::endl;
```

Python:
```python
print(f"Debug: {net.state}")
```

## Code Style

### C++

- Use C++14 features
- `std::shared_ptr` for ownership
- Clear variable names
- Comment complex logic
- Follow existing patterns

### Python

- PEP 8 style
- Type hints encouraged
- Docstrings required for public APIs
- Use Black for formatting:
  ```bash
  pip install black
  black python/glia/
  ```

## Continuous Integration

### GitHub Actions

Workflows in `.github/workflows/`:
- `build-and-test.yml`: Tests on Linux/Mac/Windows
- `build-wheels.yml`: Build release wheels

### Running CI Locally

```bash
# Install act (GitHub Actions local runner)
# https://github.com/nektos/act

# Run workflow locally
act -j test-ubuntu
```

## Release Process

See `docs/release_process.md` for complete release workflow.

Quick version:
1. Update version in `pyproject.toml`
2. Update `CHANGELOG.md`
3. Create git tag: `git tag v0.1.0`
4. Push tag: `git push origin v0.1.0`
5. GitHub Actions builds and publishes

## Common Issues

### CMake can't find Python

```bash
export PYTHON_EXECUTABLE=$(which python3)
pip install -e .
```

### Build fails on Windows

- Use "x64 Native Tools Command Prompt for VS"
- Ensure CMake and Python are in PATH
- Try: `pip install -e . --no-build-isolation`

### Import error after rebuild

```bash
# Clear Python cache
rm -rf python/glia/__pycache__
pip install -e . --force-reinstall --no-deps
```

### Tests fail

```bash
# Verify installation
python -c "import glia; print(glia.__version__)"

# Check C++ module
python -c "from glia import _core; print(_core.__version__)"

# Reinstall clean
pip uninstall glia
pip install -e .
```

## Getting Help

- Check existing documentation in `docs/`
- Search [GitHub Issues](https://github.com/yourusername/GliaGL/issues)
- Create new issue with detailed description
- Join discussions

## Resources

- [pybind11 documentation](https://pybind11.readthedocs.io/)
- [CMake documentation](https://cmake.org/documentation/)
- [Python packaging guide](https://packaging.python.org/)
- [NumPy C API](https://numpy.org/doc/stable/reference/c-api/)

Happy coding!
