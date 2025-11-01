# Migration Guide: CLI to Python API

Guide for migrating from legacy CLI tools to the new Python API.

---

## Overview

GliaGL has transitioned from CLI-based tools to a modern Python-first API. This guide helps you migrate existing workflows.

**What's Changed**:
- ❌ Old: Standalone C++ executables with command-line flags
- ✅ New: Python package with comprehensive API

**Benefits**:
- Better integration with Python ecosystem (NumPy, matplotlib, Jupyter)
- More flexible and composable
- Easier debugging and prototyping
- Access to modern ML tools

---

## Quick Migration Table

| Old CLI | New Python API |
|---------|----------------|
| `./train_main --net network.net --data train.seq` | `trainer.train_epoch(dataset, epochs=100)` |
| `./eval_main --net network.net --eval-seq test.seq` | `metrics = trainer.evaluate(sequence)` |
| `./evo_main --baseline net.net --gens 100` | `result = evo.run()` |
| Config files (`.cfg`) | Python `TrainingConfig` objects |
| Sequence files (`.seq`) | `InputSequence` + `Dataset` objects |
| Network files (`.net`) | Same format, but use `Network.from_file()` |

---

## Training Migration

### Old: train_main CLI

```bash
# Old CLI approach
./train_main \
    --net baseline.net \
    --train-data train.seq \
    --val-data val.seq \
    --lr 0.01 \
    --batch-size 32 \
    --epochs 100 \
    --warmup-ticks 10 \
    --eval-ticks 50 \
    --output trained.net
```

###

 New: Python API

```python
import glia

# Load network
net = glia.Network.from_file('baseline.net')

# Load or create dataset
# (Previously .seq files, now programmatically created)
train_data = load_dataset('train.seq')  # See data loading section
val_data = load_dataset('val.seq')

# Configure training
config = glia.create_config(
    lr=0.01,
    batch_size=32,
    warmup_ticks=10,
    eval_ticks=50
)

# Train
trainer = glia.Trainer(net, config)
trainer.train_epoch(train_data, epochs=100, config=config)

# Save
net.save('trained.net')
```

---

## Evaluation Migration

### Old: eval_main CLI

```bash
# Old CLI approach
./eval_main \
    --net trained.net \
    --eval-seq test.seq \
    --target-id OUT0 \
    --warmup 10 \
    --eval 50
```

### New: Python API

```python
import glia

# Load network and data
net = glia.Network.from_file('trained.net')
test_seq = load_sequence('test.seq')

# Configure
config = glia.create_config(
    warmup_ticks=10,
    eval_ticks=50
)

# Evaluate
trainer = glia.Trainer(net, config)
metrics = trainer.evaluate(test_seq, config)

print(f"Winner: {metrics.winner_id}")
print(f"Margin: {metrics.margin:.3f}")
print(f"Firing rates: {metrics.firing_rates}")
```

---

## Evolution Migration

### Old: evo_main CLI

```bash
# Old CLI approach
./evo_main \
    --baseline baseline.net \
    --train-data train.seq \
    --val-data val.seq \
    --population 20 \
    --generations 100 \
    --elite 4 \
    --train-epochs 10 \
    --sigma-w 0.1 \
    --sigma-thr 2.0 \
    --lamarckian 1 \
    --output-best evolved_best.net
```

### New: Python API

```python
import glia

# Load baseline and data
train_data = load_dataset('train.seq')
val_data = load_dataset('val.seq')

# Configure training
train_cfg = glia.create_config(lr=0.01, batch_size=8)

# Configure evolution
evo_cfg = glia.create_evolution_config(
    population=20,
    generations=100,
    elite=4,
    train_epochs=10,
    sigma_w=0.1,
    sigma_thr=2.0,
    lamarckian=True
)

# Run evolution
evo = glia.Evolution(
    'baseline.net',
    train_data,
    val_data,
    train_cfg,
    evo_cfg
)

result = evo.run()

# Load and save best
best_net = glia.Network()
best_net.load_from_genome(result.best_genome)
best_net.save('evolved_best.net')
```

---

## Data Format Migration

### Sequence Files (.seq)

**Old format** (parsed from text files):
```
# Example .seq file
TICK 0
S0 100.0
S1 50.0

TICK 10
S0 0.0
S1 100.0
```

**New format** (Python objects):
```python
import glia

# Create sequence
seq = glia.InputSequence()

# Tick 0
seq.add_timestep({'S0': 100.0, 'S1': 50.0})

# Tick 1 (auto-increments)
seq.add_timestep({'S0': 0.0, 'S1': 100.0})
```

### Loading Old .seq Files

If you have existing `.seq` files, convert them:

```python
def load_sequence_from_file(filepath):
    """Load old .seq format into InputSequence"""
    import re
    
    seq = glia.InputSequence()
    current_inputs = {}
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            
            # Skip comments and empty lines
            if not line or line.startswith('#'):
                continue
            
            # New tick
            if line.startswith('TICK'):
                if current_inputs:
                    seq.add_timestep(current_inputs)
                    current_inputs = {}
                continue
            
            # Parse input: "S0 100.0"
            parts = line.split()
            if len(parts) == 2:
                neuron_id, value = parts
                current_inputs[neuron_id] = float(value)
        
        # Add final tick
        if current_inputs:
            seq.add_timestep(current_inputs)
    
    return seq
```

### Dataset Migration

**Old approach** (multiple .seq files + labels):
```
data/
├── example_0.seq  (label: OUT0)
├── example_1.seq  (label: OUT1)
├── example_2.seq  (label: OUT0)
...
```

**New approach** (Python Dataset):
```python
import glia
import glob

def load_dataset_from_directory(directory):
    """Load old .seq files into Dataset"""
    episodes = []
    
    # Assume naming: class_X_example_Y.seq
    for filepath in glob.glob(f"{directory}/*.seq"):
        # Extract label from filename
        filename = filepath.split('/')[-1]
        # e.g., "class_0_example_5.seq" → "OUT0"
        label = filename.split('_')[1]
        target_id = f"OUT{label}"
        
        # Load sequence
        seq = load_sequence_from_file(filepath)
        
        # Create episode
        ep = glia.EpisodeData()
        ep.seq = seq
        ep.target_id = target_id
        episodes.append(ep)
    
    return glia.Dataset(episodes)

# Usage
train_data = load_dataset_from_directory('data/train')
val_data = load_dataset_from_directory('data/val')
```

---

## Configuration Migration

### Old: Config Files (.cfg)

**Old format**:
```ini
# train.cfg
[training]
lr = 0.01
batch_size = 32
warmup_ticks = 10
eval_ticks = 50
reward_pos = 1.0
reward_neg = -0.5
```

**New format** (Python):
```python
import glia

config = glia.create_config(
    lr=0.01,
    batch_size=32,
    warmup_ticks=10,
    eval_ticks=50,
    reward_pos=1.0,
    reward_neg=-0.5
)
```

### Loading Old Config Files

```python
import configparser

def load_config_from_file(filepath):
    """Load old .cfg format into TrainingConfig"""
    parser = configparser.ConfigParser()
    parser.read(filepath)
    
    # Extract training section
    training = parser['training']
    
    config = glia.create_config(
        lr=float(training.get('lr', 0.01)),
        batch_size=int(training.get('batch_size', 32)),
        warmup_ticks=int(training.get('warmup_ticks', 10)),
        eval_ticks=int(training.get('eval_ticks', 50)),
        reward_pos=float(training.get('reward_pos', 1.0)),
        reward_neg=float(training.get('reward_neg', -0.5)),
        margin_delta=float(training.get('margin_delta', 0.1)),
        weight_decay=float(training.get('weight_decay', 0.0001)),
        verbose=bool(int(training.get('verbose', 1)))
    )
    
    return config

# Usage
config = load_config_from_file('configs/train.cfg')
```

---

## Workflow Examples

### Example 1: Basic Training

**Old CLI workflow**:
```bash
#!/bin/bash
./train_main --net baseline.net --train train.seq --val val.seq --epochs 100 --output trained.net
./eval_main --net trained.net --eval val.seq --target OUT0
```

**New Python workflow**:
```python
import glia

# Setup
net = glia.Network.from_file('baseline.net')
train_data = load_dataset('train.seq')
val_data = load_dataset('val.seq')

# Train
config = glia.create_config(lr=0.01, batch_size=32)
trainer = glia.Trainer(net, config)
trainer.train_epoch(train_data, epochs=100, config=config)

# Evaluate
for episode in val_data:
    metrics = trainer.evaluate(episode.seq, config)
    print(f"Target: {episode.target_id}, Winner: {metrics.winner_id}, Margin: {metrics.margin:.3f}")

# Save
net.save('trained.net')
```

### Example 2: Evolutionary Training

**Old CLI workflow**:
```bash
#!/bin/bash
./evo_main \
    --baseline baseline.net \
    --train train.seq \
    --val val.seq \
    --population 20 \
    --generations 100 \
    --output evolved.net
```

**New Python workflow**:
```python
import glia

# Setup
train_data = load_dataset('train.seq')
val_data = load_dataset('val.seq')

train_cfg = glia.create_config(lr=0.02, batch_size=8)
evo_cfg = glia.create_evolution_config(
    population=20,
    generations=100,
    elite=4
)

# Run evolution
evo = glia.Evolution('baseline.net', train_data, val_data, train_cfg, evo_cfg)
result = evo.run()

# Save best
best_net = glia.Network()
best_net.load_from_genome(result.best_genome)
best_net.save('evolved.net')

print(f"Best fitness: {result.best_fitness:.4f}")
print(f"Best accuracy: {result.best_acc_hist[-1]:.1%}")
```

---

## Shell Script to Python

Many users had shell scripts automating CLI tools. Here's how to migrate:

### Old: Shell Script

```bash
#!/bin/bash
# run_experiments.sh

for seed in 1 2 3 4 5; do
    echo "Running experiment with seed $seed"
    
    ./train_main \
        --net baseline.net \
        --train data/train_$seed.seq \
        --val data/val.seq \
        --seed $seed \
        --epochs 100 \
        --output results/net_seed_$seed.net
    
    ./eval_main \
        --net results/net_seed_$seed.net \
        --eval data/test.seq \
        --output results/metrics_seed_$seed.txt
done
```

### New: Python Script

```python
#!/usr/bin/env python
# run_experiments.py

import glia

val_data = load_dataset('data/val.seq')
test_data = load_dataset('data/test.seq')

for seed in range(1, 6):
    print(f"Running experiment with seed {seed}")
    
    # Load data
    train_data = load_dataset(f'data/train_{seed}.seq')
    
    # Setup
    net = glia.Network.from_file('baseline.net')
    config = glia.create_config(lr=0.01, batch_size=32)
    
    # Train
    trainer = glia.Trainer(net, config)
    trainer.reseed(seed)
    trainer.train_epoch(train_data, epochs=100, config=config)
    
    # Save
    net.save(f'results/net_seed_{seed}.net')
    
    # Evaluate
    test_metrics = evaluate_dataset(trainer, test_data, config)
    
    # Save metrics
    with open(f'results/metrics_seed_{seed}.txt', 'w') as f:
        f.write(f"Seed: {seed}\n")
        f.write(f"Test Accuracy: {test_metrics['accuracy']:.4f}\n")
        f.write(f"Test Margin: {test_metrics['margin_mean']:.4f}\n")
```

---

## Visualization Migration

### Old: Manual plotting from CLI output

**Old approach**:
```bash
# Save metrics to file
./train_main ... > metrics.log

# Parse and plot with external script
python plot_metrics.py metrics.log
```

### New: Integrated visualization

**New approach**:
```python
import glia
import glia.viz as viz

# Train
trainer = glia.Trainer(net, config)
trainer.train_epoch(train_data, epochs=100, config=config)

# Plot built-in
viz.plot_training_history(trainer, 'training_history.png')
viz.plot_network(net, 'network_structure.png')
viz.plot_weights(net, 'weight_distribution.png')
```

---

## Common Migration Patterns

### Pattern 1: Batch Processing

**Old**:
```bash
for file in data/*.seq; do
    ./eval_main --net model.net --eval $file
done
```

**New**:
```python
import glob

for filepath in glob.glob('data/*.seq'):
    seq = load_sequence_from_file(filepath)
    metrics = trainer.evaluate(seq, config)
    print(f"{filepath}: {metrics.winner_id} (margin: {metrics.margin:.3f})")
```

### Pattern 2: Parameter Sweeps

**Old**:
```bash
for lr in 0.001 0.01 0.1; do
    ./train_main --lr $lr --output net_lr_${lr}.net
done
```

**New**:
```python
for lr in [0.001, 0.01, 0.1]:
    net = glia.Network.from_file('baseline.net')
    config = glia.create_config(lr=lr)
    trainer = glia.Trainer(net, config)
    trainer.train_epoch(train_data, epochs=50, config=config)
    net.save(f'net_lr_{lr}.net')
```

### Pattern 3: Multiple Trials

**Old**:
```bash
for trial in {1..10}; do
    ./evo_main --seed $trial --output trial_${trial}.net
done
```

**New**:
```python
for trial in range(1, 11):
    evo_cfg = glia.create_evolution_config(seed=trial)
    evo = glia.Evolution('baseline.net', train_data, val_data, train_cfg, evo_cfg)
    result = evo.run()
    
    best_net = glia.Network()
    best_net.load_from_genome(result.best_genome)
    best_net.save(f'trial_{trial}.net')
```

---

## Debugging Migration

### Old: Print statements in C++

**Limited debugging in compiled binaries**

### New: Full Python debugging

```python
import pdb

# Set breakpoint
pdb.set_trace()

# Or use IPython
from IPython import embed
embed()

# Or modern debugger
import ipdb
ipdb.set_trace()

# Inspect state
ids, values, thresholds, leaks = net.get_state()
print(f"State: values={values}, thresholds={thresholds}")
```

---

## Benefits of Migration

### 1. **Interactive Development**

**Old**: Edit, recompile, run, repeat
**New**: Jupyter notebooks, instant feedback

```python
# In Jupyter
import glia

net = glia.Network(num_sensory=2, num_neurons=5)
net.step()  # Instant feedback

# Visualize immediately
import glia.viz as viz
viz.plot_network(net)
```

### 2. **Better Integration**

```python
# NumPy
ids, values, _, _ = net.get_state()
import numpy as np
print(f"Mean: {np.mean(values)}")

# Pandas
import pandas as pd
df = pd.DataFrame({
    'id': ids,
    'value': values
})

# Scikit-learn
from sklearn.preprocessing import StandardScaler
scaler = StandardScaler()
scaled_values = scaler.fit_transform(values.reshape(-1, 1))
```

### 3. **Modern ML Workflows**

```python
# MLflow tracking
import mlflow

with mlflow.start_run():
    trainer.train_epoch(train_data, epochs=100, config=config)
    mlflow.log_metric("accuracy", trainer.epoch_accuracy[-1])
    mlflow.log_artifact("trained.net")
```

---

## Getting Help

### Documentation
- [API Reference](API_REFERENCE.md) - Complete API docs
- [Quickstart](QUICKSTART.md) - Getting started
- [Advanced Usage](ADVANCED_USAGE.md) - Advanced techniques

### Examples
- Check `examples/python/` for complete examples
- Compare with old examples in `docs/archive/`

### Support
- GitHub Issues: Report bugs or ask questions
- GitHub Discussions: Community help

---

## Migration Checklist

- [ ] Install Python package: `pip install -e .`
- [ ] Convert `.seq` files to `InputSequence` + `Dataset`
- [ ] Convert config files to Python `Config` objects
- [ ] Replace CLI commands with Python API calls
- [ ] Update shell scripts to Python scripts
- [ ] Add visualization using `glia.viz`
- [ ] Test migrated workflows
- [ ] Update documentation/README

---

**Migration complete?** You now have access to the full power of Python + GliaGL! 🎉
