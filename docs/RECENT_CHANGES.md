# Recent Changes Summary

## Training System Overhaul (Nov 2025)

### Core Issue Fixed

**Problem**: Python wrapper was using Hebbian/RL trainer instead of gradient-based trainer, causing accuracy to drop from 70-85% to ~10% (random chance).

**Solution**:

- Exposed `RateGDTrainer` (gradient-based supervised learning) to Python
- Made it the default trainer (`Trainer(net, use_gradient=True)`)
- Result: **Accuracy restored to 82-84%**

---

## New Features

### 1. Learning Rate Scheduling

Added automatic LR scheduling for better convergence without manual tuning:

**Cosine Annealing** (default):

```python
trainer.train(dataset, epochs=20, lr_schedule='cosine')
```

- Smooth decay from initial LR to 0.01× initial
- Best for stable convergence with fine-tuning at end

**Step Decay**:

```python
trainer.train(dataset, epochs=20, lr_schedule='step')
```

- Reduces LR by 50% every 1/3 of epochs
- More aggressive, good for exploration then refinement

**Constant LR**:

```python
trainer.train(dataset, epochs=20, lr_schedule=None)
```

- No scheduling, keeps initial LR throughout

### 2. Improved Training Progress Display

**Before**: Cumulative progress bar across all epochs

```
Epoch 20/20 [████████████████████] Acc: 1.34%, Margin: 0.008
```

**After**: Per-epoch status with dynamic learning rate

```
Epoch  1/20 [LR=0.050000]  →  Acc: 54.81%, Margin: 0.070
Epoch  2/20 [LR=0.049695]  →  Acc: 71.34%, Margin: 0.104
...
Epoch 20/20 [LR=0.000805]  →  Acc: 84.36%, Margin: 0.217
```

### 3. Gradient vs Hebbian Training

Users can now choose between training methods:

```python
# Gradient-based (default, recommended for supervised learning)
trainer = glia.Trainer(net, use_gradient=True)

# Hebbian/reinforcement learning (experimental)
trainer = glia.Trainer(net, use_gradient=False)
```

---

## API Changes

### Python Bindings

**Added**:

- `RateGDTrainer` class (gradient-based trainer)
- `lr_schedule` parameter in `Trainer.train()` method
- Exposed missing `TrainingConfig` parameters:
  - `no_update_if_satisfied`
  - `use_advantage_baseline`
  - `update_gating`
  - `reward_mode`, `reward_gain`, `reward_min`, `reward_max`
  - `margin_delta`

**Changed**:

- `Trainer` now uses gradient-based learning by default
- Training progress display is now per-epoch instead of cumulative

### Command Line

**New Options** in `train_digits.py`:

```bash
--lr-schedule MODE    # cosine, step, or none (default: cosine)
--optimizer TYPE      # sgd, adam, or adamw (default: adamw)
```

---

## Performance Improvements

### Digits Classification Results

| Configuration                  | Before Fix | After Fix  |
| ------------------------------ | ---------- | ---------- |
| Default (lr=0.01, 20 epochs)   | ~10%       | **82-83%** |
| Higher LR (lr=0.05, 20 epochs) | ~10%       | **83-84%** |
| With hidden layer              | N/A        | **85-90%** |

**Key Improvements**:

- Gradient descent is faster and more stable than Hebbian learning
- LR scheduling allows higher initial LR without divergence
- AdamW optimizer provides adaptive per-parameter learning rates

---

## Testing

### Reorganization
- **Moved all test files to `tests/` directory** for cleaner root folder
- Test files now use ASCII-safe markers (`[OK]`, `[FAIL]`) instead of Unicode symbols
- Fixed Windows CI encoding issues with UTF-8 stdout wrapper
- Updated GitHub Actions workflows to use new test paths

### Fixed Issues
- Updated test suite to handle floating-point precision
- All 6 test suites now pass (previously 2 failed)
- Added better error messages with approximate comparisons
- Fixed Unicode encoding errors that caused CI failures on Windows

### Test Structure
```
tests/
├── README.md                    # Test documentation
├── test_import.py              # Import and basic binding tests
├── test_api.py                 # High-level API tests
├── test_comprehensive.py       # Comprehensive functionality tests
├── test_optimizer_bindings.py  # Optimizer tests
└── test_quick.py               # Quick sanity checks
```

### Test Results
```
Results: 6 passed, 0 failed
[PASS] Network Wrapper
[PASS] Trainer Wrapper
[PASS] Dataset
[PASS] Config Helpers
[PASS] File I/O
[PASS] Evolution Wrapper
```

---

## Documentation Updates

### Updated Files

1. **`python/README.md`**

   - Added LR scheduling examples
   - Updated training section with gradient trainer usage
   - Added advanced options section

2. **`examples/seq_digits_poisson/README.md`**

   - Updated quick start commands
   - Added LR scheduling section
   - Updated performance table with new results
   - Updated console output examples

3. **`CHANGELOG.md`**

   - Added unreleased section with all changes
   - Documented the accuracy fix (10% → 82-84%)

4. **`docs/RECENT_CHANGES.md`** (this file)
   - Comprehensive summary of all changes

---

## Migration Guide

### For Existing Code

**If you were using default training**:
No changes needed! Your code now uses gradient-based training automatically.

**If you explicitly want Hebbian training**:

```python
# Old (implicit Hebbian)
trainer = glia.Trainer(net)

# New (explicit Hebbian)
trainer = glia.Trainer(net, use_gradient=False)
```

**To use LR scheduling** (recommended):

```python
# Old
history = trainer.train(dataset, epochs=20, config=config)

# New
history = trainer.train(dataset, epochs=20, config=config, lr_schedule='cosine')
```

---

## Command Line Examples

### Basic Training

```bash
python examples/seq_digits_poisson/train_digits.py --epochs 20
```

### High Accuracy Configuration

```bash
python examples/seq_digits_poisson/train_digits.py --epochs 20 --lr 0.05 --batch-size 32
```

### Custom LR Schedule

```bash
# Cosine (smooth decay)
python examples/seq_digits_poisson/train_digits.py --epochs 20 --lr-schedule cosine

# Step decay (periodic drops)
python examples/seq_digits_poisson/train_digits.py --epochs 15 --lr-schedule step

# Constant LR (no scheduling)
python examples/seq_digits_poisson/train_digits.py --epochs 10 --lr-schedule none
```

---

## Next Steps

1. ✅ Training system fixed and documented
2. ✅ Tests passing
3. ✅ Documentation updated
4. 🔲 Consider adding warmup phase to LR scheduling
5. 🔲 Add learning curves visualization to analysis script
6. 🔲 Benchmark against other SNN frameworks

---

_Last updated: November 2024_
