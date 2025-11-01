# NumPy-Compatible Data Interface

## Overview

The GliaGL C++ core now exposes NumPy-compatible interfaces for efficient data exchange with Python. All data is returned as flat arrays that can be zero-copied into NumPy arrays via pybind11's buffer protocol.

## Network State Access

### Get State
```python
import glia
import numpy as np

net = glia.Network.from_file("network.net")

# Get complete network state
ids, values, thresholds, leaks = net.get_state()

# Convert to NumPy arrays (zero-copy via pybind11)
values_np = np.array(values, copy=False)
thresholds_np = np.array(thresholds, copy=False)
leaks_np = np.array(leaks, copy=False)

print(f"Network has {len(ids)} neurons")
print(f"Average threshold: {thresholds_np.mean():.2f}")
```

### Set State
```python
# Modify parameters
new_thresholds = thresholds_np * 1.1  # Increase all thresholds by 10%
new_leaks = leaks_np * 0.95           # Decrease leaks by 5%

# Update network
net.set_state(ids, new_thresholds.tolist(), new_leaks.tolist())
```

## Weight Matrix Access

### Get Weights (COO Sparse Format)
```python
# Get all synaptic weights as edge list
from_ids, to_ids, weights = net.get_weights()

# Convert to NumPy
weights_np = np.array(weights)

print(f"Network has {len(weights)} connections")
print(f"Weight range: [{weights_np.min():.3f}, {weights_np.max():.3f}]")
print(f"Mean absolute weight: {np.abs(weights_np).mean():.3f}")

# Convert to scipy sparse matrix (if needed)
import scipy.sparse as sp

# Create ID to index mapping
all_ids = net.get_all_neuron_ids()
id_to_idx = {id: i for i, id in enumerate(all_ids)}

# Convert to indices
from_indices = [id_to_idx[id] for id in from_ids]
to_indices = [id_to_idx[id] for id in to_ids]

# Create sparse matrix
n_neurons = len(all_ids)
weight_matrix = sp.coo_matrix(
    (weights_np, (from_indices, to_indices)),
    shape=(n_neurons, n_neurons)
)

print(f"Sparsity: {1 - weight_matrix.nnz / (n_neurons**2):.2%}")
```

### Set Weights
```python
# Modify weights
new_weights = weights_np * 0.9  # Reduce all weights by 10%

# Apply to network
net.set_weights(from_ids, to_ids, new_weights.tolist())

# Or add new connections
net.set_weights(
    ["S0", "S1"],  # from
    ["H0", "H1"],  # to
    [0.5, -0.3]    # weights
)
```

## Network Analysis

### Connectivity Analysis
```python
from collections import Counter

# Analyze connectivity
from_ids, to_ids, weights = net.get_weights()

# Fan-out (outgoing connections per neuron)
fan_out = Counter(from_ids)
print(f"Max fan-out: {max(fan_out.values())}")

# Fan-in (incoming connections per neuron)
fan_in = Counter(to_ids)
print(f"Max fan-in: {max(fan_in.values())}")

# Weight distribution
weights_np = np.array(weights)
print(f"Excitatory synapses: {(weights_np > 0).sum()}")
print(f"Inhibitory synapses: {(weights_np < 0).sum()}")
```

### Neuron Parameter Analysis
```python
ids, values, thresholds, leaks = net.get_state()

# Separate by neuron type
sensory_mask = [id.startswith('S') for id in ids]
hidden_mask = [id.startswith('H') for id in ids]
output_mask = [id.startswith('O') for id in ids]

thresholds_np = np.array(thresholds)
leaks_np = np.array(leaks)

print("Thresholds by layer:")
print(f"  Sensory: {thresholds_np[sensory_mask].mean():.2f}")
print(f"  Hidden:  {thresholds_np[hidden_mask].mean():.2f}")
print(f"  Output:  {thresholds_np[output_mask].mean():.2f}")
```

## Batch Processing

### Efficient State Snapshots
```python
# Save network state for later restoration
snapshot = {
    'ids': ids,
    'thresholds': np.array(thresholds),
    'leaks': np.array(leaks),
    'weights': {
        'from': from_ids,
        'to': to_ids,
        'values': np.array(weights)
    }
}

# ... training or experiments ...

# Restore state
net.set_state(snapshot['ids'], 
              snapshot['thresholds'].tolist(),
              snapshot['leaks'].tolist())
net.set_weights(snapshot['weights']['from'],
                snapshot['weights']['to'],
                snapshot['weights']['values'].tolist())
```

## Performance Notes

### Zero-Copy Transfer
When using pybind11, arrays returned from C++ can be accessed by NumPy without copying:

```cpp
// C++ side (in pybind11 binding):
.def("get_state", [](Glia &self) {
    std::vector<std::string> ids;
    std::vector<float> values, thresholds, leaks;
    self.getState(ids, values, thresholds, leaks);
    
    // Return as tuple of NumPy arrays (zero-copy)
    return py::make_tuple(
        ids,
        py::array_t<float>(values.size(), values.data()),  // zero-copy view
        py::array_t<float>(thresholds.size(), thresholds.data()),
        py::array_t<float>(leaks.size(), leaks.data())
    );
})
```

### Batch Updates
For efficiency, update weights in batches rather than one-by-one:

```python
# SLOW: One update at a time
for i in range(len(weights)):
    net.set_weights([from_ids[i]], [to_ids[i]], [new_weights[i]])

# FAST: Batch update
net.set_weights(from_ids, to_ids, new_weights)
```

## Data Format Summary

| Method | Input/Output | Format | NumPy Compatible |
|--------|-------------|--------|------------------|
| `get_state()` | Output | 4 parallel arrays | Yes (float32) |
| `set_state()` | Input | 3 parallel arrays | Yes |
| `get_weights()` | Output | COO sparse (3 arrays) | Yes (float32) |
| `set_weights()` | Input | COO sparse (3 arrays) | Yes |
| `get_all_neuron_ids()` | Output | String array | Yes |
| `get_neuron_count()` | Output | Integer | Yes |
| `get_connection_count()` | Output | Integer | Yes |

All float data is `float32` (C++ `float` = Python `np.float32`).
