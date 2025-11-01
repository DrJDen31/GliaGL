# GliaGL Quickstart Guide

Get up and running with GliaGL in 5 minutes.

---

## Installation

### Prerequisites

- Python 3.8+
- C++14 compiler (gcc, clang, or MSVC)
- pip

### Install from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/GliaGL.git
cd GliaGL

# Create virtual environment (recommended)
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install in editable mode
pip install -e ".[viz]"
```

### Verify Installation

```bash
python -c "import glia; print(glia.__version__)"
```

---

## Your First Network

### Create and Simulate

```python
import glia

# Create a network with 2 sensory neurons and 5 internal neurons
net = glia.Network(num_sensory=2, num_neurons=5)

# Inject current into sensory neurons
net.inject_dict({
    'S0': 100.0,  # Strong input
    'S1': 50.0    # Weak input
})

# Run simulation
net.step(n_steps=10)

# Check which neurons fired
fired = net.get_firing_neurons()
print(f"Firing neurons: {fired}")
```

**Output**:
```
Firing neurons: ['S0', 'S1']
```

---

## Add Connections

```python
import numpy as np

# Create connections: S0 → N0, S1 → N1
net.set_weights(
    from_ids=['S0', 'S1'],
    to_ids=['N0', 'N1'],
    weights=np.array([2.0, 1.5])
)

# Verify
print(f"Network now has {net.num_connections} connections")
```

---

## Training a Network

### Prepare Data

```python
# Create training dataset
episodes = []

for i in range(20):
    # Create episode
    ep = glia.EpisodeData()
    
    # Define input sequence
    seq = glia.InputSequence()
    seq.add_timestep({'S0': 100.0, 'S1': 0.0})
    seq.add_timestep({'S0': 0.0, 'S1': 100.0})
    
    ep.seq = seq
    ep.target_id = f"N{i % 3}"  # Rotate through 3 outputs
    
    episodes.append(ep)

# Create dataset
dataset = glia.Dataset(episodes)
train_data, val_data = dataset.split(train_frac=0.8, seed=42)
```

### Train

```python
# Configure training
config = glia.create_config(
    lr=0.01,
    batch_size=4,
    warmup_ticks=10,
    eval_ticks=50
)

# Create trainer
trainer = glia.Trainer(net, config)

# Train for 50 epochs
trainer.train_epoch(train_data, epochs=50, config=config)

# Check accuracy
print(f"Final accuracy: {trainer.epoch_accuracy[-1]:.1%}")
```

---

## Save and Load

### Save Network

```python
net.save('my_network.net')
```

### Load Network

```python
# Load from file
loaded_net = glia.Network.from_file('my_network.net')

# Use it
loaded_net.step(10)
```

---

## Visualization

### Network Structure

```python
import glia.viz as viz

# Visualize network graph
viz.plot_network(net, 'network.png')
```

### Training Progress

```python
# Plot training history
viz.plot_training_history(trainer, 'training.png')
```

### Weight Distribution

```python
# Analyze weights
viz.plot_weights(net, 'weights.png')
```

---

## NumPy Integration

### Access State as Arrays

```python
import numpy as np

# Get current state
ids, values, thresholds, leaks = net.get_state()

print(f"Average membrane voltage: {values.mean():.2f}")
print(f"Threshold range: [{thresholds.min():.0f}, {thresholds.max():.0f}]")
```

### Modify State

```python
# Increase all thresholds by 10%
ids, values, thresholds, leaks = net.get_state()
thresholds *= 1.1
net.set_state(ids, thresholds, leaks)
```

### Vectorized Operations

```python
# Normalize thresholds to [90, 110]
ids, _, thresholds, leaks = net.get_state()
thresholds = 90 + 20 * (thresholds - thresholds.min()) / (thresholds.max() - thresholds.min())
net.set_state(ids, thresholds, leaks)
```

---

## Complete Example

Here's a complete workflow from scratch:

```python
import glia
import numpy as np

# 1. Create network
net = glia.Network(num_sensory=3, num_neurons=10)

# 2. Add random connections
n_connections = 15
from_ids = np.random.choice(net.sensory_ids, n_connections)
to_ids = np.random.choice([n for n in net.neuron_ids if n not in net.sensory_ids], n_connections)
weights = np.random.randn(n_connections)
net.set_weights(from_ids.tolist(), to_ids.tolist(), weights)

print(f"Created network with {net.num_connections} connections")

# 3. Create dataset
episodes = []
for i in range(50):
    ep = glia.EpisodeData()
    seq = glia.InputSequence()
    
    # Random input pattern
    for t in range(5):
        inputs = {f'S{j}': np.random.rand() * 100 for j in range(3)}
        seq.add_timestep(inputs)
    
    ep.seq = seq
    ep.target_id = f"N{i % 3}"
    episodes.append(ep)

dataset = glia.Dataset(episodes)
train_data, val_data = dataset.split(0.8, seed=42)

# 4. Train
config = glia.create_config(lr=0.01, batch_size=8)
trainer = glia.Trainer(net, config)
trainer.train_epoch(train_data, epochs=30, config=config)

# 5. Evaluate
correct = 0
for ep in val_data:
    metrics = trainer.evaluate(ep.seq, config)
    if metrics.winner_id == ep.target_id:
        correct += 1

accuracy = correct / len(val_data)
print(f"Validation accuracy: {accuracy:.1%}")

# 6. Save
net.save('trained_network.net')

# 7. Visualize
import glia.viz as viz
viz.plot_network(net, 'final_network.png')
viz.plot_training_history(trainer, 'training_history.png')
```

---

## Next Steps

Now that you've got the basics:

1. **Explore Examples**: Check out `examples/python/` for more complex scenarios
2. **Read API Reference**: See [API_REFERENCE.md](API_REFERENCE.md) for detailed documentation
3. **Advanced Techniques**: Learn about evolutionary training in [ADVANCED_USAGE.md](ADVANCED_USAGE.md)
4. **Migrate from CLI**: See [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) if coming from the old CLI tools

---

## Common Patterns

### Pattern 1: Periodic Input

```python
# Inject inputs every 10 steps
for step in range(100):
    if step % 10 == 0:
        net.inject_dict({'S0': 100.0})
    net.step()
```

### Pattern 2: Activity Monitoring

```python
# Track firing over time
activity_log = []
for t in range(100):
    net.step()
    fired = net.get_firing_neurons()
    if fired:
        activity_log.append((t, fired))

print(f"Total firing events: {len(activity_log)}")
```

### Pattern 3: State Snapshots

```python
# Save state at intervals
snapshots = []
for epoch in range(50):
    trainer.train_epoch(train_data, epochs=1, config=config)
    ids, values, thresholds, leaks = net.get_state()
    snapshots.append({
        'epoch': epoch,
        'thresholds': thresholds.copy(),
        'accuracy': trainer.epoch_accuracy[-1]
    })
```

### Pattern 4: Batch Processing

```python
# Process multiple networks
networks = [glia.Network(num_sensory=2, num_neurons=5) for _ in range(10)]

for i, net in enumerate(networks):
    trainer = glia.Trainer(net, config)
    trainer.train_epoch(train_data, epochs=20, config=config)
    net.save(f'network_{i}.net')
```

---

## Troubleshooting

### Import Error

**Problem**: `ModuleNotFoundError: No module named 'glia'`

**Solution**: 
```bash
pip install -e .
```

### Build Error

**Problem**: Compilation fails during installation

**Solution**: 
```bash
# Make sure you have a C++ compiler
# On Ubuntu/Debian:
sudo apt-get install build-essential

# On macOS:
xcode-select --install

# On Windows: Install Visual Studio Build Tools
```

### Visualization Error

**Problem**: `ImportError: No module named 'matplotlib'`

**Solution**:
```bash
pip install matplotlib networkx
# Or install with viz extras:
pip install -e ".[viz]"
```

---

## Quick Reference

### Essential Imports

```python
import glia
import glia.viz as viz
import numpy as np
```

### Create Network

```python
net = glia.Network(num_sensory=N_in, num_neurons=N_hidden)
```

### Simulate

```python
net.inject_dict({'S0': 100.0})
net.step(n_steps=10)
```

### Train

```python
config = glia.create_config(lr=0.01, batch_size=32)
trainer = glia.Trainer(net, config)
trainer.train_epoch(dataset, epochs=50, config=config)
```

### Visualize

```python
viz.plot_network(net, 'network.png')
viz.plot_training_history(trainer, 'history.png')
```

---

## Getting Help

- **API Reference**: [API_REFERENCE.md](API_REFERENCE.md)
- **Examples**: `examples/python/`
- **Issues**: GitHub Issues
- **Discussions**: GitHub Discussions

---

**Happy coding with GliaGL!** 🧠⚡
