# GliaGL Test Suite

This directory contains all test scripts for the GliaGL project.

## Test Files

### Core Tests

**`test_import.py`** - Basic import and binding verification
- Tests Python module import
- Tests network creation
- Tests basic simulation
- Tests NumPy interface
- Tests configuration objects

**`test_api.py`** - High-level API tests
- Network wrapper class tests
- Trainer wrapper tests
- Dataset functionality tests
- Config helper functions
- File I/O operations
- Evolution wrapper tests

### Additional Tests

**`test_comprehensive.py`** - Comprehensive functionality tests
**`test_optimizer_bindings.py`** - Optimizer binding tests
**`test_quick.py`** - Quick sanity checks

## Running Tests

### Run All Core Tests

```bash
python tests/test_import.py
```

```bash
python tests/test_api.py
```

### Run with pytest

```bash
pytest tests/ -v
```

### CI/CD

Tests are automatically run on:
- Ubuntu (Python 3.8, 3.9, 3.10, 3.11, 3.12)
- macOS (Python 3.8, 3.11)
- Windows (Python 3.8, 3.11)

See `.github/workflows/build-and-test.yml` for configuration.

## Test Requirements

- Python >= 3.8
- NumPy >= 1.20
- pytest (optional, for running full test suite)

## Expected Output

All tests use ASCII-safe output markers for cross-platform compatibility:
- `[OK]` - Test passed
- `[FAIL]` - Test failed
- `[PASS]` - Test suite passed
- Results summary at the end

## Adding New Tests

When adding new tests:
1. Use ASCII-safe output (no Unicode symbols for CI compatibility)
2. Add UTF-8 encoding header: `# -*- coding: utf-8 -*-`
3. Set up stdout encoding for Windows compatibility
4. Follow existing test patterns
5. Update this README

## Troubleshooting

### Unicode Errors on Windows CI

Tests include UTF-8 encoding setup:
```python
if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
```

### Import Errors

Ensure the package is installed:
```bash
pip install -e .
```
