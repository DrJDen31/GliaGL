# Getting Started with GliaGL

This tutorial will walk you through the basics of using GliaGL in Python.

## Installation

```bash
pip install glia
```

Or from source:
```bash
git clone https://github.com/yourusername/GliaGL.git
cd GliaGL
pip install -e .
```

## Your First Network

Let's create and simulate a simple spiking neural network.

### Step 1: Import GliaGL

```python
import glia
import numpy as np

# Check installation
glia.info()
```

### Step 2: Create a Network

```python
# Create network with 2 sensory neurons and 5 internal neurons
net = glia.Network(num_sensory=2, num_neurons=5)

print(net)
# Output: <Network neurons=7 connections=0>
```

### Step 3: Inspect the Network

```python
# Get neuron IDs
print(f"Sensory neurons: {net.sensory_ids}")
# Output: ['S0', 'S1']

print(f"All neurons: {net.neuron_ids}")
# Output: ['S0', 'S1', 'N0', 'N1', 'N2', 'N3', 'N4']

# Get network state
state = net.state
print(f"Thresholds: {state['thresholds']}")
print(f"Leaks: {state['leaks']}")
```

### Step 4: Add Connections

```python
# Connect sensory neurons to internal neurons
net.set_weights(
    from_ids=["S0", "S0", "S1", "S1"],
    to_ids=["N0", "N1", "N2", "N3"],
    weights=np.array([1.5, 1.2, 1.8, 1.0])
)

print(f"Connections: {net.num_connections}")
# Output: 4
```

### Step 5: Run Simulation

```python
# Inject input
net.inject_dict({"S0": 150.0, "S1": 100.0})

# Run 10 timesteps
for step in range(10):
    net.step()
    
    # Check which neurons fired
    firing = net.get_firing_neurons()
    if firing:
        print(f"Step {step}: {firing} fired")
```

### Step 6: Save Network

```python
net.save("my_first_network.net")
print("Network saved!")
```

## Training a Network

Now let's train a network on a simple task.

### Step 1: Load or Create Network

```python
# Load existing network
net = glia.Network.from_file("my_first_network.net")

# Or create new one
net = glia.Network(num_sensory=2, num_neurons=5)
```

### Step 2: Create Training Data

```python
# Create simple dataset
episodes = []

for i in range(20):
    # Create episode
    ep = glia.EpisodeData()
    
    # Create input sequence
    seq = glia.InputSequence()
    for t in range(10):
        seq.add_timestep({
            "S0": 100.0 if i % 2 == 0 else 50.0,
            "S1": 50.0 if i % 2 == 0 else 100.0,
        })
    
    ep.seq = seq
    ep.target_id = "N0" if i % 2 == 0 else "N1"
    episodes.append(ep)

# Create dataset
dataset = glia.Dataset(episodes)
print(f"Dataset size: {len(dataset)}")
```

### Step 3: Configure Training

```python
config = glia.create_config(
    lr=0.01,              # Learning rate
    batch_size=4,         # Batch size
    warmup_ticks=10,      # Warmup period
    decision_window=20    # Decision window
)
```

### Step 4: Train

```python
# Create trainer
trainer = glia.Trainer(net, config)

# Train with progress callback
def on_epoch(epoch, accuracy, margin):
    if epoch % 10 == 0:
        print(f"Epoch {epoch}: Accuracy={accuracy:.2%}, Margin={margin:.3f}")

history = trainer.train(
    dataset.episodes,
    epochs=50,
    on_epoch=on_epoch
)

print(f"Training complete!")
print(f"Final accuracy: {history['accuracy'][-1]:.2%}")
```

### Step 5: Evaluate

```python
# Evaluate on dataset
results = trainer.evaluate_dataset(dataset.episodes, config)

print(f"Accuracy: {results['accuracy']:.2%}")
print(f"Correct: {results['correct']}/{results['total']}")
```

### Step 6: Save Trained Network

```python
net.save("trained_network.net")
```

## Working with Data

### Loading Sequence Files

```python
# Load a single .seq file
seq = glia.load_sequence_file("input.seq")

# Load directory of sequences
dataset = glia.load_dataset_from_directory(
    "data/train/",
    pattern="*.seq",
    target_mapping=lambda f: "O0" if "class0" in f else "O1"
)
```

### Creating Sequences from NumPy

```python
import numpy as np

# Create random data
data = np.random.rand(100, 2)  # 100 timesteps, 2 inputs

# Convert to sequence
seq = glia.create_sequence_from_array(data, ["S0", "S1"])

# Use in dataset
ep = glia.EpisodeData()
ep.seq = seq
ep.target_id = "N0"
```

### Dataset Operations

```python
# Create dataset
dataset = glia.Dataset(episodes)

# Split
train, val = dataset.split(train_fraction=0.8, shuffle=True, seed=42)

# Shuffle
shuffled = dataset.shuffle(seed=42)

# Index
episode = dataset[0]
subset = dataset[0:10]
```

## Visualization

```python
import glia.viz as viz

# Network graph
viz.plot_network_graph(net, save_path="network.png")

# Weight distribution
viz.plot_weight_distribution(net, save_path="weights.png")

# Training history
viz.plot_training_history(trainer.history, save_path="training.png")

# Neuron activity
viz.plot_neuron_activity(
    net,
    simulation_steps=100,
    input_pattern={"S0": 100.0},
    save_path="activity.png"
)
```

## NumPy Integration

GliaGL provides zero-copy NumPy integration:

```python
# Get state as NumPy arrays
ids, values, thresholds, leaks = net.get_state()

# Modify with NumPy
new_thresholds = thresholds * 1.1  # Increase by 10%

# Apply changes
net.set_state(ids, new_thresholds, leaks)

# Get weights
from_ids, to_ids, weights = net.get_weights()

# Analyze
print(f"Mean weight: {weights.mean():.3f}")
print(f"Std weight: {weights.std():.3f}")
```

## Evolution

For evolutionary training:

```python
# Configure evolution
train_cfg = glia.create_config(lr=0.01)
evo_cfg = glia.create_evo_config(
    population=10,
    generations=20,
    elite=2,
    train_epochs=5
)

# Run evolution
evo = glia.Evolution(
    network_path="baseline.net",
    train_data=train_episodes,
    val_data=val_episodes,
    train_config=train_cfg,
    evo_config=evo_cfg
)

result = evo.run(verbose=True)

# Load best genome
best_net = glia.Evolution.load_best_genome("baseline.net", result)
best_net.save("evolved.net")
```

## Common Patterns

### Pattern 1: Load, Train, Save

```python
import glia

# Load
net = glia.Network.from_file("network.net")
dataset = glia.load_dataset_from_directory("data/")

# Train
trainer = glia.Trainer(net)
config = glia.create_config(lr=0.01)
history = trainer.train(dataset.episodes, epochs=100)

# Save
net.save("trained_network.net")
```

### Pattern 2: Experiment with Parameters

```python
# Try different learning rates
for lr in [0.001, 0.01, 0.1]:
    net = glia.Network.from_file("baseline.net")
    config = glia.create_config(lr=lr)
    trainer = glia.Trainer(net, config)
    history = trainer.train(data.episodes, epochs=50, verbose=False)
    print(f"lr={lr}: Final accuracy={history['accuracy'][-1]:.2%}")
```

### Pattern 3: Interactive Exploration

```python
import glia
import numpy as np

# Load network
net = glia.Network.from_file("network.net")

# Explore
>>> net.num_neurons
50

>>> net.num_connections
200

>>> state = net.state
>>> state['thresholds'].mean()
100.0

# Modify and test
>>> state['thresholds'] *= 1.2
>>> net.set_state(state['ids'], state['thresholds'], state['leaks'])
>>> # Test the change...
```

## Next Steps

1. **Run Examples**
   ```bash
   cd examples/python
   python 01_basic_simulation.py
   ```

2. **Read Documentation**
   - `docs/python_api_guide.md` - Complete API reference
   - `docs/numpy_interface.md` - NumPy integration
   - `docs/cli_to_python_migration.md` - Migration from CLI

3. **Try Notebooks**
   - `examples/notebooks/` - Interactive tutorials

4. **Build Your Application**
   - Start with example templates
   - Customize for your task
   - Refer to API docs

## Troubleshooting

### Import Error
```python
ImportError: No module named 'glia'
```
**Solution:** Install package
```bash
pip install glia
# or from source:
pip install -e .
```

### C++ Module Not Found
```python
ImportError: cannot import name '_core'
```
**Solution:** Rebuild C++ extension
```bash
pip install -e . --force-reinstall --no-deps
```

### Visualization Not Available
```python
ImportError: No module named 'matplotlib'
```
**Solution:** Install visualization dependencies
```bash
pip install matplotlib networkx
```

## Getting Help

- **Documentation**: `docs/` directory
- **Examples**: `examples/python/`
- **API Help**: `python -c "import glia; help(glia.Network)"`
- **Issues**: GitHub Issues

## Summary

You've learned:
- ✅ Creating and simulating networks
- ✅ Training with datasets
- ✅ Working with NumPy arrays
- ✅ Visualization
- ✅ Evolutionary training
- ✅ Common patterns

**Next**: Explore the examples and build your own applications!
