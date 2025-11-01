# Virtual Environment Setup for GliaGL

## Why Use a Virtual Environment?

Using a virtual environment (venv) for GliaGL development:
- ✅ Isolates dependencies from your system Python
- ✅ Prevents version conflicts with other projects
- ✅ Makes it easy to reproduce the exact environment
- ✅ Allows testing different Python versions

## Recommended Setup for WSL/Linux (Primary Development)

### Option 1: Standard venv (Recommended)

```bash
# Navigate to project root
cd ~/path/to/GliaGL  # or /mnt/c/Users/jaden/OneDrive/Documents/AI/GliaGL

# Create virtual environment
python3 -m venv venv

# Activate it
source venv/bin/activate

# Upgrade pip
pip install --upgrade pip

# Install GliaGL in editable mode with dev dependencies
pip install -e ".[dev,viz]"

# Verify installation
python -c "import glia; print(glia.__version__)"
```

**To deactivate**:
```bash
deactivate
```

**To activate later**:
```bash
source venv/bin/activate
```

### Option 2: Using conda/anaconda (If you prefer)

```bash
# Create conda environment
conda create -n gliagl python=3.12

# Activate it
conda activate gliagl

# Install dependencies
pip install -e ".[dev,viz]"
```

## Setup for Windows (Secondary Development)

### Using PowerShell

```powershell
# Navigate to project root
cd C:\Users\jaden\OneDrive\Documents\AI\GliaGL

# Create virtual environment
python -m venv venv

# Activate it
.\venv\Scripts\Activate.ps1

# If you get execution policy error:
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# Upgrade pip
pip install --upgrade pip

# Install GliaGL
pip install -e ".[dev,viz]"
```

## What Should Be in the Venv?

### Core Dependencies
```
numpy>=1.20          # Required
```

### Development Dependencies
```
pytest>=7.0          # Testing
black>=23.0          # Code formatting
mypy>=1.0            # Type checking
jupyter>=1.0         # Notebooks
```

### Visualization Dependencies
```
matplotlib>=3.5      # Plotting
networkx>=2.6        # Graph visualization
```

## .gitignore Configuration

The venv directory should already be ignored. Verify `.gitignore` contains:

```gitignore
# Virtual environments
venv/
.venv/
env/
ENV/
```

✅ This is already in your `.gitignore`

## IDE Configuration

### VS Code

Create/update `.vscode/settings.json`:

```json
{
    "python.defaultInterpreterPath": "${workspaceFolder}/venv/bin/python",
    "python.terminal.activateEnvironment": true,
    "python.testing.pytestEnabled": true,
    "python.testing.pytestArgs": [
        "tests"
    ]
}
```

The IDE should automatically detect and offer to use the venv.

## Current Situation Analysis

Based on your error output, you're using:
```
/root/anaconda3/bin/python (found version "3.12.2")
```

This is **anaconda3 in WSL**, not a project-specific venv.

### Recommendation: Create a WSL venv

```bash
# In WSL, at project root
cd /mnt/c/Users/jaden/OneDrive/Documents/AI/GliaGL

# Create venv
python3 -m venv venv

# Activate
source venv/bin/activate

# You should see (venv) in your prompt
# (venv) user@machine:/mnt/c/.../GliaGL$

# Clean install
pip install --upgrade pip setuptools wheel
pip install -e ".[dev,viz]"

# Test
python python/test_import.py
python test_comprehensive.py
```

## Switching Between Environments

### If you want to keep using anaconda:
```bash
# Create conda env
conda create -n gliagl python=3.12
conda activate gliagl
pip install -e ".[dev,viz]"
```

### If you want to use standard venv (recommended):
```bash
# Deactivate conda if active
conda deactivate

# Use project venv
source venv/bin/activate
```

## Testing the Environment

After activation, verify:

```bash
# Check Python location (should be in venv/)
which python

# Check glia is installed
pip list | grep glia

# Run tests
python python/test_import.py
python test_comprehensive.py

# Run examples
cd examples/python
python 01_basic_simulation.py
```

## Benefits of Project-Specific Venv

1. **Isolation**: Won't interfere with anaconda base or other projects
2. **Reproducibility**: Anyone can recreate exact environment
3. **Clean testing**: Know exactly what dependencies are needed
4. **Version control**: Can commit `requirements.txt` or `environment.yml`

## Requirements Files

### Generate requirements.txt

```bash
# After installing everything you need
pip freeze > requirements.txt
```

### Or create manually

`requirements-dev.txt`:
```txt
# Core
numpy>=1.20

# Development
pytest>=7.0
black>=23.0
mypy>=1.0
jupyter>=1.0

# Visualization  
matplotlib>=3.5
networkx>=2.6

# Build tools (handled by pyproject.toml, but for reference)
scikit-build-core>=0.5
pybind11>=2.11
```

## Quick Reference

### Daily Workflow

```bash
# WSL/Linux
cd /mnt/c/Users/jaden/OneDrive/Documents/AI/GliaGL
source venv/bin/activate
# Do work...
deactivate

# Windows
cd C:\Users\jaden\OneDrive\Documents\AI\GliaGL
.\venv\Scripts\Activate.ps1
# Do work...
deactivate
```

### When Dependencies Change

```bash
# Reinstall in editable mode
pip install -e ".[dev,viz]" --force-reinstall --no-deps

# Or just rebuild C++ extension
pip install -e . --force-reinstall --no-deps
```

## Troubleshooting

### "command not found: python"
```bash
# Use python3 explicitly
python3 -m venv venv
```

### "Cannot activate virtualenv"
```bash
# Make sure you're in the right directory
pwd
ls -la venv/  # Should see bin/, lib/, etc.

# Try explicit path
source ./venv/bin/activate
```

### "Module not found" after activation
```bash
# Reinstall
pip install -e ".[dev,viz]"
```

## Recommendation

**For your setup**, I recommend:

1. **Create a WSL venv** at project root:
   ```bash
   cd /mnt/c/Users/jaden/OneDrive/Documents/AI/GliaGL
   python3 -m venv venv
   source venv/bin/activate
   pip install -e ".[dev,viz]"
   ```

2. **Add to your shell profile** for easy activation:
   ```bash
   # Add to ~/.bashrc or ~/.zshrc
   alias gliagl='cd /mnt/c/Users/jaden/OneDrive/Documents/AI/GliaGL && source venv/bin/activate'
   ```

3. **Use it consistently** for all GliaGL work

This keeps your anaconda environment clean and makes GliaGL development reproducible.
