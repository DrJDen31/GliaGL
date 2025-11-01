#!/usr/bin/env python3
"""
Example 3: NumPy Integration

This example demonstrates:
- Zero-copy NumPy array access
- State manipulation with NumPy
- Weight matrix operations
- Creating sparse weight matrices
"""

import glia
import numpy as np

print("=" * 60)
print("Example 3: NumPy Integration")
print("=" * 60)
print()

# ========== Create Network ==========
print("[Step 1] Creating network...")
net = glia.Network(num_sensory=3, num_neurons=10)
print(f"   {net}")
print()

# ========== Get State as NumPy Arrays ==========
print("[Step 2] Accessing state as NumPy arrays...")

ids, values, thresholds, leaks = net.get_state()

print(f"   Data types:")
print(f"      values:     {type(values)} with shape {values.shape}")
print(f"      thresholds: {type(thresholds)} with shape {thresholds.shape}")
print(f"      leaks:      {type(leaks)} with shape {leaks.shape}")
print()

print(f"   Statistics:")
print(f"      Threshold mean: {thresholds.mean():.2f}")
print(f"      Threshold std:  {thresholds.std():.2f}")
print(f"      Leak mean:      {leaks.mean():.3f}")
print(f"      Leak std:       {leaks.std():.3f}")
print()

# ========== Manipulate State with NumPy ==========
print("[Step 3] Manipulating state with NumPy...")

# Example: Increase all thresholds by 10%
new_thresholds = thresholds * 1.1
new_leaks = leaks  # Keep leaks the same

# Apply changes
net.set_state(ids, new_thresholds, new_leaks)

# Verify
_, _, updated_thresholds, _ = net.get_state()
print(f"   Original threshold mean: {thresholds.mean():.2f}")
print(f"   Updated threshold mean:  {updated_thresholds.mean():.2f}")
print(f"   Change: {(updated_thresholds.mean() / thresholds.mean() - 1) * 100:.1f}%")
print()

# ========== Vectorized Operations ==========
print("[Step 4] Vectorized operations...")

# Example: Normalize thresholds to [50, 150] range
min_threshold, max_threshold = 50.0, 150.0
normalized_thresholds = (
    min_threshold + 
    (thresholds - thresholds.min()) / 
    (thresholds.max() - thresholds.min()) * 
    (max_threshold - min_threshold)
)

print(f"   Original range: [{thresholds.min():.2f}, {thresholds.max():.2f}]")
print(f"   Normalized range: [{normalized_thresholds.min():.2f}, {normalized_thresholds.max():.2f}]")
print()

# ========== Weight Matrix Access ==========
print("[Step 5] Accessing weights...")

from_ids, to_ids, weights = net.get_weights()

if len(weights) > 0:
    print(f"   Number of connections: {len(weights)}")
    print(f"   Weight statistics:")
    print(f"      Mean: {weights.mean():.3f}")
    print(f"      Std:  {weights.std():.3f}")
    print(f"      Min:  {weights.min():.3f}")
    print(f"      Max:  {weights.max():.3f}")
    print()
    
    # Excitatory vs Inhibitory
    excitatory = (weights > 0).sum()
    inhibitory = (weights < 0).sum()
    print(f"   Connection types:")
    print(f"      Excitatory: {excitatory} ({excitatory/len(weights)*100:.1f}%)")
    print(f"      Inhibitory: {inhibitory} ({inhibitory/len(weights)*100:.1f}%)")
else:
    print(f"   No connections in network (empty)")

print()

# ========== Create Custom Weights ==========
print("[Step 6] Creating custom weight matrix...")

# Create random sparse connectivity
neuron_ids = net.neuron_ids
n_neurons = len(neuron_ids)
n_connections = 20

# Random source and target indices
rng = np.random.RandomState(42)
from_indices = rng.randint(0, n_neurons, size=n_connections)
to_indices = rng.randint(0, n_neurons, size=n_connections)

# Create weight values (80% excitatory, 20% inhibitory)
random_weights = rng.uniform(-0.5, 2.0, size=n_connections)
random_weights[random_weights < 0.2] = -random_weights[random_weights < 0.2]  # Make some inhibitory

# Convert indices to neuron IDs
from_neuron_ids = [neuron_ids[i] for i in from_indices]
to_neuron_ids = [neuron_ids[i] for i in to_indices]

print(f"   Created {n_connections} random connections")
print(f"   Weight range: [{random_weights.min():.3f}, {random_weights.max():.3f}]")
print()

# Apply to network
net.set_weights(from_neuron_ids, to_neuron_ids, random_weights)
print(f"   Applied weights to network")
print(f"   Network now has {net.num_connections} connections")
print()

# ========== Sparse Matrix Conversion ==========
print("[Step 7] Converting to sparse matrix (requires scipy)...")

try:
    import scipy.sparse as sp
    
    # Get adjacency matrix as sparse COO
    adj_matrix = net.to_adjacency_matrix(dense=False)
    
    print(f"   Adjacency matrix:")
    print(f"      Format: {type(adj_matrix)}")
    print(f"      Shape: {adj_matrix.shape}")
    print(f"      Non-zero: {adj_matrix.nnz}")
    print(f"      Sparsity: {(1 - adj_matrix.nnz / (adj_matrix.shape[0] * adj_matrix.shape[1])) * 100:.1f}%")
    
    # Convert to different formats
    csr_matrix = adj_matrix.tocsr()
    print(f"   Converted to CSR format: {type(csr_matrix)}")
    
except ImportError:
    print(f"   scipy not installed - sparse matrices unavailable")
    print(f"   Install with: pip install scipy")
    
    # Show dense matrix instead
    adj_dense = net.to_adjacency_matrix(dense=True)
    print(f"   Dense adjacency matrix:")
    print(f"      Shape: {adj_dense.shape}")
    print(f"      Dtype: {adj_dense.dtype}")
    print(f"      Non-zero: {(adj_dense != 0).sum()}")

print()

# ========== Batch Operations ==========
print("[Step 8] Batch operations on state...")

# Simulate and collect states over time
states_over_time = []
n_steps = 10

for step in range(n_steps):
    if step % 3 == 0:
        net.inject_array(np.array([100.0, 50.0, 75.0]))
    
    net.step()
    _, values, _, _ = net.get_state()
    states_over_time.append(values.copy())

# Stack into 2D array: (timesteps, neurons)
state_matrix = np.vstack(states_over_time)
print(f"   Collected states shape: {state_matrix.shape}")
print(f"   (timesteps × neurons)")
print()

# Analyze temporal statistics
temporal_mean = state_matrix.mean(axis=0)  # Mean across time for each neuron
temporal_std = state_matrix.std(axis=0)    # Std across time for each neuron

print(f"   Temporal statistics per neuron:")
print(f"      Mean activity: {temporal_mean}")
print(f"      Std activity:  {temporal_std}")
print()

# ========== Summary ==========
print("=" * 60)
print("Summary:")
print(f"  - Zero-copy NumPy integration demonstrated")
print(f"  - State manipulation with vectorized operations")
print(f"  - Weight matrix access and modification")
print(f"  - Sparse matrix support (if scipy available)")
print(f"  - Batch operations over time")
print()
print("This demonstrates NumPy integration for efficient data access.")
print("=" * 60)
