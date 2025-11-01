# Repository Cleanup Audit - Stage 6

## Overview

This document identifies obsolete files and directories that can be removed now that GliaGL has transitioned to a Python-first workflow.

## Files to Remove

### Obsolete C++ CLI Executables

**Location**: `src/train/`

These were the old CLI entry points, now replaced by Python scripts:

- [ ] `eval_main.cpp` - Main training executable (replaced by `python/glia/trainer.py`)
- [ ] `mini_world_main.cpp` - Mini world training (replaced by Python examples)
- [ ] Old shell scripts for building/running

**Reason**: All functionality now accessible via Python API

### Obsolete Evaluator C++ Files

**Location**: `examples/*/evaluator/`

Evolutionary runners that are now Python scripts:

- [ ] `examples/3class/evaluator/*.cpp` - 3-class evolution runner
- [ ] `examples/mini-world/evaluator/*.cpp` - Mini-world evolution runner
- [ ] Associated build files and scripts

**Reason**: Evolution now handled by `glia.Evolution` in Python

### Old Build Scripts

**Scattered locations**:

- [ ] `examples/3class/*.sh` - Old bash scripts
- [ ] `examples/3class/*.ps1` - Old PowerShell scripts
- [ ] `examples/mini-world/*.ps1` - Build scripts
- [ ] Any other `*.sh` or `*.ps1` not in `scripts/`

**Reason**: Build now handled by `scripts/build_local.sh` and modern Python tooling

### Build Directory Contents

**Keep directories, delete contents**:

- [ ] `build/*` - Root build artifacts
- [ ] `src/train/build/*` - Training build artifacts
- [ ] `src/vis/build/*` - Visualization build artifacts

**Reason**: Build artifacts should not be in git, keep dirs for structure

### Obsolete Documentation

**Check and remove**:

- [ ] `docs/archive/` files that reference old CLI
- [ ] Any markdown files mentioning `src/testing` (should be `examples/`)
- [ ] Old build instructions for CLI-only workflows

**Reason**: Documentation should reflect current Python-first workflow

## Files to Keep

### Core C++ Implementation

✅ **Keep all of**:

- `src/arch/` - Core network engine
- `src/train/gradient/` - Gradient algorithms
- `src/train/hebbian/` - Hebbian learning
- `src/evo/` - Evolution engine
- `include/glia/` - Public API headers

### Network and Data Files

✅ **Keep all**:

- `*.net` files - Network definitions
- `*.seq` files - Sequence data
- Configuration files

### Documentation

✅ **Keep and update**:

- `docs/architecture/` - Architecture docs
- `docs/*.md` - Updated documentation
- `README.md` - Updated for Python

### Examples

✅ **Keep**:

- Python examples in `examples/python/`
- Network files needed by examples
- Data files needed by examples

### New Python Infrastructure

✅ **Keep all**:

- `python/` - Python package
- `pyproject.toml` - Build config
- CI/CD workflows
- All Stage 1-5 additions

## Consolidation Tasks

### Network Files

**Current** (scattered):

```
examples/3class/3class_baseline.net
examples/newnet/*.net
examples/perm3/perm3_baseline.net
examples/seq_digits_poisson/*.net
```

**Proposed** (consolidated):

```
examples/networks/
├── 3class_baseline.net
├── perm3_baseline.net
├── digits_readout_h1.net
└── ...
```

**Action**: Move all `.net` files to `examples/networks/`, update references

### Sequence Files

**Current** (scattered):

```
examples/3class/configs/*.seq
examples/*/data/*.seq
```

**Proposed** (consolidated):

```
examples/data/
├── 3class/
│   ├── train/*.seq
│   └── val/*.seq
├── digits/
│   └── *.seq
└── ...
```

**Action**: Organize `.seq` files by dataset, update references

### Documentation Paths

**Update all references from**:

- `src/testing/` → `examples/`
- Old CLI commands → Python examples
- Build instructions → `pip install`

## AGsI_proto_mind_starter Folder

**Status**: IGNORE for now (as per previous decision)

**Location**: `AGsI_proto_mind_starter/`

**Action**: Leave untouched until final cleanup step

## OpenGL Visualization

**Status**: FROZEN (as per previous decision)

**Location**: `src/vis/`

**Action**: Keep code but mark as unmaintained in documentation

## Updated .gitignore

Already updated in Stage 4 with:

```
# Python packaging
dist/
*.egg-info/
python/build/

# Build artifacts
build/
src/train/build/
src/vis/build/
```

## Verification Steps

After cleanup:

1. **Build test**:

   ```bash
   pip install -e .
   ```

2. **Import test**:

   ```bash
   python python/test_import.py
   ```

3. **Examples test**:

   ```bash
   python examples/python/01_basic_simulation.py
   ```

4. **Documentation check**:
   - No broken links
   - All paths are correct
   - References are up-to-date

## Estimated Impact

**Files to remove**: ~15-20 files  
**Build artifacts to clear**: ~100+ generated files  
**Documentation to update**: ~10 files  
**Disk space saved**: ~50-100 MB (mostly build artifacts)

## Safety Notes

- ✅ All obsolete functionality is replaced by Python equivalents
- ✅ Network and data files are preserved
- ✅ Core C++ implementation is untouched
- ✅ Git history preserves everything
- ✅ Can revert if needed

## Approval Checklist

Before executing cleanup:

- [ X ] Review this audit document
- [ X ] Confirm no needed functionality is removed
- [ X ] Backup if desired (git handles this)
- [ ] Run cleanup script
- [ ] Verify build still works
- [ ] Verify examples still work
- [ ] Update documentation references

## Next Steps

1. Review this audit
2. Run `scripts/cleanup_repository.sh` (created below)
3. Test build and examples
4. Update documentation
5. Commit changes with clear message
