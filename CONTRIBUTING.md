# Contributing to GliaGL

Thank you for your interest in contributing to GliaGL! This document provides guidelines for contributing to the project.

## Code of Conduct

Be respectful, constructive, and professional in all interactions.

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in [Issues](https://github.com/yourusername/GliaGL/issues)
2. If not, create a new issue with:
   - Clear, descriptive title
   - Steps to reproduce
   - Expected vs actual behavior
   - Your environment (OS, Python version, GliaGL version)
   - Minimal code example if possible

### Suggesting Features

1. Check existing issues and discussions
2. Create a new issue with:
   - Clear description of the feature
   - Use cases and benefits
   - Possible implementation approach

### Pull Requests

1. Fork the repository
2. Create a new branch: `git checkout -b feature/your-feature-name`
3. Make your changes
4. Add tests if applicable
5. Run the test suite:
   ```bash
   python python/test_import.py
   python test_comprehensive.py
   python examples/python/01_basic_simulation.py  # Verify examples still work
   ```
6. Commit with clear messages
7. Push and create a pull request

## Development Setup

### Prerequisites

- Python 3.8+
- C++ compiler (GCC, Clang, or MSVC)
- CMake 3.15+
- Git

### Setup

```bash
# Clone repository
git clone https://github.com/yourusername/GliaGL.git
cd GliaGL

# Create virtual environment
python -m venv venv
source venv/bin/activate  # or `venv\Scripts\activate` on Windows

# Install in development mode
pip install -e ".[dev]"

# Run tests
python python/test_import.py
python python/test_api.py
```

## Code Style

### Python

- Follow PEP 8
- Use Black for formatting: `black python/glia/`
- Type hints encouraged
- Docstrings for all public functions/classes

Example:
```python
def train_network(
    net: Network,
    dataset: Dataset,
    epochs: int = 100
) -> dict:
    """
    Train a network on a dataset.
    
    Args:
        net: Network to train
        dataset: Training data
        epochs: Number of training epochs
        
    Returns:
        Training history dictionary
    """
    ...
```

### C++

- Follow existing code style
- Use C++14 features
- Comment complex logic
- Avoid raw pointers (use `std::shared_ptr`)

Example:
```cpp
/**
 * @brief Train network for one epoch
 * @param dataset Training episodes
 * @param config Training configuration
 */
void trainEpoch(
    const std::vector<EpisodeData>& dataset,
    const TrainingConfig& config
);
```

## Testing

### Running Tests

```bash
# Import and basic functionality
python python/test_import.py

# Comprehensive API tests
python test_comprehensive.py

# Quick smoke test
python test_quick.py

# Examples (verify all work)
python examples/python/01_basic_simulation.py
python examples/python/02_training_basics.py
python examples/python/03_numpy_integration.py
python examples/python/04_visualization.py
python examples/python/05_evolution.py

# Full test suite (if pytest installed)
pytest tests/ -v
```

### Writing Tests

Add tests for new features:

```python
def test_new_feature():
    """Test description"""
    import glia
    
    # Setup
    net = glia.Network(2, 3)
    
    # Test
    result = net.new_feature()
    
    # Assert
    assert result == expected_value
    print("✓ Test passed")
    
    return True
```

## Documentation

### Docstrings

All public APIs should have docstrings:

```python
class MyClass:
    """
    Brief description.
    
    Longer description with usage examples.
    
    Example:
        >>> obj = MyClass()
        >>> obj.method()
    """
    
    def method(self, arg: str) -> int:
        """
        Method description.
        
        Args:
            arg: Argument description
            
        Returns:
            Return value description
        """
```

### Documentation Files

- Update `docs/` when adding features:
  - `docs/API_REFERENCE.md` - For new API methods
  - `docs/QUICKSTART.md` - For beginner-friendly features
  - `docs/ADVANCED_USAGE.md` - For advanced techniques
  - `docs/MIGRATION_GUIDE.md` - For CLI migration patterns
- Keep `README.md` up to date
- Update `CHANGELOG.md` with changes
- Update examples in `examples/python/` if API changes affect them

## Commit Messages

Use clear, descriptive commit messages:

```
Add NumPy interface for network state access

- Add get_state() method returning NumPy arrays
- Add set_state() for updating from arrays
- Update documentation
- Add tests for new methods
```

Format:
- First line: Brief summary (50 chars or less)
- Blank line
- Detailed description with bullet points

## Pull Request Process

1. **Before submitting:**
   - Run tests locally
   - Update documentation
   - Add changelog entry if needed
   - Rebase on latest main if necessary

2. **PR description should include:**
   - What changed and why
   - Related issue numbers (fixes #123)
   - Testing done
   - Any breaking changes

3. **Review process:**
   - Maintainers will review your PR
   - Address feedback and comments
   - Once approved, PR will be merged

## Release Process

Releases are handled by maintainers:

1. Update version in `pyproject.toml`
2. Update `CHANGELOG.md`
3. Create git tag: `v0.1.0`
4. GitHub Actions builds and publishes to PyPI

## Areas Needing Help

Current priorities:

- [ ] More comprehensive test coverage
- [ ] Performance benchmarks
- [ ] Additional visualization functions
- [ ] Jupyter notebook tutorials (interactive quickstart)
- [ ] More example networks and datasets
- [ ] Additional examples (e.g., sequence learning, pattern recognition)
- [ ] Documentation improvements (more examples, clearer explanations)
- [ ] Type stub files (.pyi) for better IDE support
- [ ] Continuous integration setup (GitHub Actions)
- [ ] Pre-built wheels for common platforms

## Questions?

- Check documentation in `docs/`
- Search existing issues
- Create a new issue with your question
- Join discussions

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

Thank you for contributing to GliaGL!
