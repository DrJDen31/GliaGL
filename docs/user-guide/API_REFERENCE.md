# GliaGL Python API Reference

Complete reference for the GliaGL Python API.

---

## Table of Contents

- [Network](#network)
- [Dataset](#dataset)
- [Trainer](#trainer)
- [Evolution](#evolution)
- [InputSequence](#inputsequence)
- [Configuration](#configuration)
- [Visualization](#visualization)

---

## Network

The main neural network class for creating and simulating spiking networks.

### Constructor

```python
glia.Network(num_sensory=0, num_neurons=0)
```

**Parameters**:
- `num_sensory` (int): Number of sensory input neurons
- `num_neurons` (int): Number of internal neurons

**Example**:
```python
net = glia.Network(num_sensory=3, num_neurons=10)
```

### Class Methods

#### `from_file(filepath, verbose=True)`

Load a network from a `.net` file.

**Parameters**:
- `filepath` (str): Path to network file
- `verbose` (bool): Print loading information

**Returns**: `Network` instance

**Example**:
```python
net = glia.Network.from_file('my_network.net')
```

### Instance Methods

#### Simulation

##### `step(n_steps=1)`

Advance the simulation by one or more timesteps.

**Parameters**:
- `n_steps` (int): Number of timesteps to simulate (default: 1)

**Example**:
```python
net.step()        # Single step
net.step(100)     # 100 steps
```

##### `inject(neuron_id, amount)`

Inject current into a specific neuron.

**Parameters**:
- `neuron_id` (str): ID of neuron to inject into
- `amount` (float): Current amount

**Example**:
```python
net.inject('S0', 150.0)
```

##### `inject_dict(inputs)`

Inject current into multiple neurons at once.

**Parameters**:
- `inputs` (dict): Mapping of neuron_id → current amount

**Example**:
```python
net.inject_dict({
    'S0': 100.0,
    'S1': 75.0,
    'S2': 50.0
})
```

#### State Access

##### `get_state()`

Get current network state as NumPy arrays.

**Returns**: Tuple of (ids, values, thresholds, leaks)
- `ids` (list[str]): Neuron IDs
- `values` (ndarray): Membrane voltages
- `thresholds` (ndarray): Firing thresholds
- `leaks` (ndarray): Leak parameters

**Example**:
```python
ids, values, thresholds, leaks = net.get_state()
print(f"Average voltage: {values.mean()}")
```

##### `set_state(ids, thresholds, leaks)`

Set neuron parameters from arrays.

**Parameters**:
- `ids` (list[str]): Neuron IDs
- `thresholds` (ndarray): New threshold values
- `leaks` (ndarray): New leak values

**Example**:
```python
ids, _, thresholds, leaks = net.get_state()
thresholds *= 1.1  # Increase all thresholds by 10%
net.set_state(ids, thresholds, leaks)
```

#### Weight Access

##### `get_weights()`

Get network weights in COO sparse format.

**Returns**: Tuple of (from_ids, to_ids, weights)
- `from_ids` (list[str]): Source neuron IDs
- `to_ids` (list[str]): Target neuron IDs
- `weights` (ndarray): Connection weights

**Example**:
```python
from_ids, to_ids, weights = net.get_weights()
print(f"Network has {len(weights)} connections")
```

##### `set_weights(from_ids, to_ids, weights)`

Set network weights from arrays.

**Parameters**:
- `from_ids` (list[str]): Source neuron IDs
- `to_ids` (list[str]): Target neuron IDs
- `weights` (ndarray): Connection weights

**Example**:
```python
import numpy as np
net.set_weights(
    ['S0', 'S1'],
    ['N0', 'N1'],
    np.array([1.5, -0.8])
)
```

#### Neuron Access

##### `get_neuron(neuron_id)`

Get a specific neuron by ID.

**Parameters**:
- `neuron_id` (str): Neuron ID

**Returns**: `Neuron` instance

**Example**:
```python
neuron = net.get_neuron('N0')
print(neuron.threshold)
```

##### `get_firing_neurons()`

Get list of neurons that fired in the last timestep.

**Returns**: list[str] of neuron IDs

**Example**:
```python
net.step()
fired = net.get_firing_neurons()
print(f"Firing: {fired}")
```

#### File I/O

##### `save(filepath)`

Save network to a `.net` file.

**Parameters**:
- `filepath` (str): Output file path

**Example**:
```python
net.save('trained_network.net')
```

##### `load(filepath, verbose=True)`

Load network configuration from file.

**Parameters**:
- `filepath` (str): Input file path
- `verbose` (bool): Print loading information

**Example**:
```python
net.load('baseline.net')
```

### Properties

#### `num_neurons`

Total number of neurons in the network (read-only).

**Type**: int

**Example**:
```python
print(f"Network has {net.num_neurons} neurons")
```

#### `num_connections`

Total number of connections in the network (read-only).

**Type**: int

**Example**:
```python
print(f"Network has {net.num_connections} connections")
```

#### `sensory_ids`

List of sensory neuron IDs (read-only).

**Type**: list[str]

**Example**:
```python
for sid in net.sensory_ids:
    print(f"Sensory neuron: {sid}")
```

#### `neuron_ids`

List of all neuron IDs (read-only).

**Type**: list[str]

**Example**:
```python
all_ids = net.neuron_ids
print(f"All neurons: {all_ids}")
```

---

## Dataset

Container for training episodes.

### Constructor

```python
glia.Dataset(episodes)
```

**Parameters**:
- `episodes` (list[EpisodeData]): List of training episodes

**Example**:
```python
episodes = []
for i in range(100):
    ep = glia.EpisodeData()
    ep.seq = create_sequence()
    ep.target_id = f"OUT{i % 3}"
    episodes.append(ep)

dataset = glia.Dataset(episodes)
```

### Methods

#### `split(train_frac=0.8, shuffle=True, seed=None)`

Split dataset into train and validation sets.

**Parameters**:
- `train_frac` (float): Fraction for training (default: 0.8)
- `shuffle` (bool): Shuffle before splitting (default: True)
- `seed` (int, optional): Random seed for reproducibility

**Returns**: Tuple of (train_dataset, val_dataset)

**Example**:
```python
train_data, val_data = dataset.split(train_frac=0.8, seed=42)
```

#### `shuffle(seed=None)`

Shuffle the dataset in-place.

**Parameters**:
- `seed` (int, optional): Random seed

**Example**:
```python
dataset.shuffle(seed=42)
```

### Special Methods

#### `len(dataset)`

Get number of episodes.

**Example**:
```python
print(f"Dataset size: {len(dataset)}")
```

#### `dataset[index]`

Access individual episodes.

**Example**:
```python
first_episode = dataset[0]
```

---

## Trainer

Hebbian trainer for spiking networks with reward-modulated learning.

### Constructor

```python
glia.Trainer(network, config=None)
```

**Parameters**:
- `network` (Network): Network to train
- `config` (TrainingConfig, optional): Training configuration

**Example**:
```python
trainer = glia.Trainer(net, config)
```

### Methods

#### `train_epoch(dataset, epochs, config=None)`

Train for multiple epochs on a dataset.

**Parameters**:
- `dataset` (Dataset): Training data
- `epochs` (int): Number of epochs
- `config` (TrainingConfig, optional): Training configuration

**Example**:
```python
trainer.train_epoch(train_data, epochs=100, config=cfg)
```

#### `train_batch(batch, config=None)`

Train on a single batch of episodes.

**Parameters**:
- `batch` (list[EpisodeData]): Batch of episodes
- `config` (TrainingConfig, optional): Training configuration

**Example**:
```python
batch = dataset[0:32]
trainer.train_batch(batch, config)
```

#### `evaluate(sequence, config=None)`

Evaluate network on a single episode.

**Parameters**:
- `sequence` (InputSequence): Input sequence
- `config` (TrainingConfig, optional): Evaluation configuration

**Returns**: `EpisodeMetrics` with accuracy, margin, firing rates

**Example**:
```python
metrics = trainer.evaluate(test_seq, config)
print(f"Margin: {metrics.margin}")
```

#### `reseed(seed)`

Set random seed for reproducibility.

**Parameters**:
- `seed` (int): Random seed

**Example**:
```python
trainer.reseed(42)
```

### Properties

#### `epoch_accuracy`

List of accuracy values per epoch (read-only).

**Type**: list[float]

**Example**:
```python
accuracies = trainer.epoch_accuracy
plt.plot(accuracies)
```

#### `epoch_margin`

List of margin values per epoch (read-only).

**Type**: list[float]

**Example**:
```python
margins = trainer.epoch_margin
plt.plot(margins)
```

---

## Evolution

Evolutionary trainer with Lamarckian learning.

### Constructor

```python
glia.Evolution(network_path, train_data, val_data, train_config, evo_config)
```

**Parameters**:
- `network_path` (str): Path to baseline network file
- `train_data` (Dataset): Training dataset
- `val_data` (Dataset): Validation dataset
- `train_config` (TrainingConfig): Training configuration
- `evo_config` (EvolutionConfig): Evolution configuration

**Example**:
```python
evo = glia.Evolution(
    'baseline.net',
    train_data,
    val_data,
    train_cfg,
    evo_cfg
)
```

### Methods

#### `run()`

Run evolutionary training.

**Returns**: `EvolutionResult` with best genome and history

**Example**:
```python
result = evo.run()
print(f"Best fitness: {result.best_fitness}")
```

---

## InputSequence

Sequence of timestamped inputs for simulation.

### Constructor

```python
glia.InputSequence()
```

**Example**:
```python
seq = glia.InputSequence()
```

### Methods

#### `add_timestep(inputs)`

Add inputs at the next timestep.

**Parameters**:
- `inputs` (dict): Mapping of neuron_id → value

**Example**:
```python
seq.add_timestep({'S0': 100.0, 'S1': 50.0})
seq.add_timestep({'S0': 50.0, 'S1': 100.0})
```

#### `is_empty()`

Check if sequence has no events.

**Returns**: bool

**Example**:
```python
if seq.is_empty():
    print("No inputs yet")
```

---

## Configuration

### TrainingConfig

Configuration for training parameters.

#### Creation

```python
config = glia.create_config(
    lr=0.01,
    batch_size=32,
    warmup_ticks=10,
    eval_ticks=50,
    reward_pos=1.0,
    reward_neg=-0.5,
    margin_delta=0.1,
    weight_decay=0.0001,
    eligibility_decay=0.9,
    checkpoint_every=10,
    verbose=True,
    log_every=5
)
```

**Parameters**:
- `lr` (float): Learning rate (default: 0.01)
- `batch_size` (int): Batch size (default: 32)
- `warmup_ticks` (int): Warmup timesteps (default: 10)
- `eval_ticks` (int): Evaluation timesteps (default: 50)
- `reward_pos` (float): Reward for correct output (default: 1.0)
- `reward_neg` (float): Penalty for incorrect output (default: -0.5)
- `margin_delta` (float): Margin threshold (default: 0.1)
- `weight_decay` (float): L2 regularization (default: 0.0001)
- `eligibility_decay` (float): Eligibility trace decay (default: 0.9)
- `checkpoint_every` (int): Checkpoint frequency (default: 10)
- `verbose` (bool): Print training info (default: True)
- `log_every` (int): Logging frequency (default: 5)

### EvolutionConfig

Configuration for evolutionary training.

#### Creation

```python
evo_cfg = glia.create_evolution_config(
    population=20,
    generations=50,
    elite=4,
    parents_pool=8,
    train_epochs=10,
    sigma_w=0.1,
    sigma_thr=2.0,
    sigma_leak=0.01,
    w_acc=1.0,
    w_margin=0.5,
    w_sparsity=0.01,
    lamarckian=True,
    seed=42
)
```

**Parameters**:
- `population` (int): Population size (default: 20)
- `generations` (int): Number of generations (default: 50)
- `elite` (int): Elite count to preserve (default: 4)
- `parents_pool` (int): Parent pool size (default: 8)
- `train_epochs` (int): Training epochs per individual (default: 10)
- `sigma_w` (float): Weight mutation std dev (default: 0.1)
- `sigma_thr` (float): Threshold mutation std dev (default: 2.0)
- `sigma_leak` (float): Leak mutation std dev (default: 0.01)
- `w_acc` (float): Accuracy weight in fitness (default: 1.0)
- `w_margin` (float): Margin weight in fitness (default: 0.5)
- `w_sparsity` (float): Sparsity penalty (default: 0.01)
- `lamarckian` (bool): Enable Lamarckian evolution (default: True)
- `seed` (int): Random seed (default: 42)

---

## Visualization

Visualization utilities for networks and training.

### Functions

#### `plot_network(network, output_path=None, **kwargs)`

Visualize network structure as a graph.

**Parameters**:
- `network` (Network): Network to visualize
- `output_path` (str, optional): Save location
- `**kwargs`: Additional matplotlib arguments

**Example**:
```python
glia.viz.plot_network(net, 'network.png', figsize=(12, 8))
```

#### `plot_weights(network, output_path=None, **kwargs)`

Plot weight distribution histogram.

**Parameters**:
- `network` (Network): Network to analyze
- `output_path` (str, optional): Save location
- `**kwargs`: Additional matplotlib arguments

**Example**:
```python
glia.viz.plot_weights(net, 'weights.png')
```

#### `plot_training_history(trainer, output_path=None, **kwargs)`

Plot training accuracy and margin over time.

**Parameters**:
- `trainer` (Trainer): Trainer with history
- `output_path` (str, optional): Save location
- `**kwargs`: Additional matplotlib arguments

**Example**:
```python
glia.viz.plot_training_history(trainer, 'history.png')
```

#### `plot_activity_raster(activity_log, output_path=None, **kwargs)`

Create raster plot of neuron firing activity.

**Parameters**:
- `activity_log` (list): List of (timestep, neuron_ids) tuples
- `output_path` (str, optional): Save location
- `**kwargs`: Additional matplotlib arguments

**Example**:
```python
activity = []
for t in range(100):
    net.step()
    activity.append((t, net.get_firing_neurons()))

glia.viz.plot_activity_raster(activity, 'raster.png')
```

---

## Data Structures

### EpisodeData

Container for a single training episode.

**Attributes**:
- `seq` (InputSequence): Input sequence
- `target_id` (str): Expected output neuron ID

**Example**:
```python
episode = glia.EpisodeData()
episode.seq = input_sequence
episode.target_id = 'OUT0'
```

### EpisodeMetrics

Metrics from episode evaluation.

**Attributes**:
- `winner_id` (str): ID of winning neuron
- `margin` (float): Margin between winner and runner-up
- `firing_rates` (dict): Neuron ID → firing rate
- `ticks_run` (int): Timesteps executed

### Neuron

Individual neuron (usually accessed via Network).

**Properties**:
- `id` (str): Neuron ID
- `value` (float): Current membrane voltage
- `threshold` (float): Firing threshold
- `leak` (float): Leak parameter
- `resting` (float): Resting voltage
- `did_fire` (bool): Fired in last timestep

**Example**:
```python
neuron = net.get_neuron('N0')
neuron.threshold = 105.0
print(f"Voltage: {neuron.value}")
```

---

## Error Handling

All GliaGL functions raise standard Python exceptions:

- `FileNotFoundError`: Network file not found
- `ValueError`: Invalid parameter values
- `RuntimeError`: Simulation or training errors
- `ImportError`: Missing dependencies (e.g., scipy, matplotlib)

**Example**:
```python
try:
    net = glia.Network.from_file('missing.net')
except FileNotFoundError:
    print("Network file not found, creating new network")
    net = glia.Network(num_sensory=3, num_neurons=10)
```

---

## Type Hints

GliaGL provides type hints for IDE support:

```python
from typing import Tuple, List
import numpy as np
import glia

def analyze_network(net: glia.Network) -> Tuple[float, float]:
    ids, values, thresholds, leaks = net.get_state()
    return float(np.mean(values)), float(np.std(values))
```

---

## Performance Tips

1. **Use NumPy operations** for state manipulation:
   ```python
   ids, values, thresholds, leaks = net.get_state()
   thresholds *= 1.1  # Faster than looping
   net.set_state(ids, thresholds, leaks)
   ```

2. **Batch injections** instead of individual:
   ```python
   # Good
   net.inject_dict({'S0': 100, 'S1': 50, 'S2': 75})
   
   # Slower
   net.inject('S0', 100)
   net.inject('S1', 50)
   net.inject('S2', 75)
   ```

3. **GIL is released** during C++ operations:
   ```python
   # Training releases GIL - safe for multithreading
   trainer.train_epoch(data, epochs=100)
   ```

4. **Sparse matrices** for large networks:
   ```python
   from scipy.sparse import coo_matrix
   from_ids, to_ids, weights = net.get_weights()
   # Convert to sparse format for efficient operations
   ```

---

## See Also

- [Quickstart Guide](QUICKSTART.md)
- [Examples](../examples/python/README.md)
- [Migration Guide](MIGRATION_GUIDE.md)
- [Advanced Usage](ADVANCED_USAGE.md)
