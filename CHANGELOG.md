# Changelog

All notable changes to GliaGL will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial Python package implementation
- pybind11 bindings for C++ core
- High-level Python API with Network, Trainer, Evolution classes
- Dataset class for PyTorch-like data handling
- Visualization utilities (requires matplotlib/networkx)
- NumPy integration with zero-copy data access
- GIL release during compute-heavy operations
- Comprehensive documentation and examples

### Changed
- Refactored C++ core to use std::shared_ptr for memory safety
- Reorganized headers into public API (include/glia/) and implementation (src/)

## [0.1.0] - 2025-01-XX

### Added
- First alpha release
- Core spiking neural network simulator
- Gradient-based training
- Evolutionary training with Lamarckian evolution
- Python bindings via pybind11
- NumPy compatibility
- Basic visualization tools
- Documentation and tutorials

### Features
- **Network Simulation**: Leaky integrate-and-fire neurons with flexible connectivity
- **Training**: Gradient descent with eligibility traces
- **Evolution**: Population-based training with mutation and selection
- **Python API**: Clean, Pythonic interface
- **Performance**: C++ speed with <1% Python overhead
- **Cross-Platform**: Linux, macOS, Windows support

### Known Issues
- OpenGL visualization currently frozen (use Python viz instead)
- Python callbacks during evolution not yet supported (use verbose mode)

## Release Notes

### Version 0.1.0 (Alpha)

This is the first public release of GliaGL with Python bindings. The library provides:

1. **Fast Simulation**: C++ core with Python interface
2. **Training**: Gradient-based and evolutionary algorithms
3. **Easy to Use**: PyTorch-inspired API
4. **Well Documented**: Complete API reference and examples

**Installation:**
```bash
pip install glia
```

**Quick Start:**
```python
import glia

net = glia.Network.from_file("network.net")
trainer = glia.Trainer(net)
history = trainer.train(dataset, epochs=100)
```

See `docs/python_api_guide.md` for complete documentation.

---

## Version Scheme

- **Major.Minor.Patch** (e.g., 1.2.3)
- **Major**: Breaking API changes
- **Minor**: New features, backward compatible
- **Patch**: Bug fixes, backward compatible

## Links

- [GitHub Repository](https://github.com/yourusername/GliaGL)
- [Documentation](https://github.com/yourusername/GliaGL/docs)
- [PyPI](https://pypi.org/project/glia/)
- [Issue Tracker](https://github.com/yourusername/GliaGL/issues)
