#!/usr/bin/env python3
"""
GliaGL Quick Start Example

This demonstrates the basic Python API for GliaGL.
"""

import glia
import numpy as np

print("=" * 60)
print("GliaGL Quick Start Example")
print("=" * 60)
print()

# Display package info
glia.info()
print()

# ========== 1. Create a Network ==========
print("[1] Creating a simple network...")
net = glia.Network(num_sensory=2, num_neurons=3)
print(f"   {net}")
print(f"   Neurons: {net.num_neurons}")
print(f"   Connections: {net.num_connections}")
print()

# ========== 2. Access Network State ==========
print("[2] Accessing network state...")
state = net.state
print(f"   Neuron IDs: {state['ids']}")
print(f"   Thresholds: {state['thresholds']}")
print(f"   Leaks: {state['leaks']}")
print()

# ========== 3. Inject Input and Simulate ==========
print("[3] Running simulation...")
# Inject current into sensory neurons
net.inject_dict({"S0": 150.0, "S1": 100.0})

# Simulate 10 timesteps
for step in range(10):
    net.step()
    firing = net.get_firing_neurons()
    if firing:
        print(f"   Step {step}: Firing neurons: {firing}")

print()

# ========== 4. NumPy Integration ==========
print("[4] NumPy integration...")
# Get state as arrays
ids, values, thresholds, leaks = net.get_state()
print(f"   Values (NumPy): {values}")
print(f"   Type: {type(values)}, Shape: {values.shape}")

# Modify thresholds
new_thresholds = thresholds * 1.1  # Increase by 10%
net.set_state(ids, new_thresholds, leaks)
print(f"   Updated thresholds: {net.state['thresholds']}")
print()

# ========== 5. Weight Access ==========
print("[5] Accessing weights...")
from_ids, to_ids, weights = net.get_weights()
if len(weights) > 0:
    print(f"   Total connections: {len(weights)}")
    print(f"   Weight range: [{weights.min():.3f}, {weights.max():.3f}]")
else:
    print(f"   No connections yet (empty network)")
print()

# ========== 6. Configuration Objects ==========
print("[6] Creating configurations...")

# Training config
train_config = glia.create_config(
    lr=0.01,
    batch_size=4,
    warmup_ticks=50,
    decision_window=50
)
print(f"   Training config: lr={train_config.lr}, batch={train_config.batch_size}")

# Evolution config
evo_config = glia.create_evo_config(
    population=10,
    generations=20,
    elite=2
)
print(f"   Evolution config: pop={evo_config.population}, gens={evo_config.generations}")
print()

# ========== 7. Load/Save Networks ==========
print("[7] Network I/O...")
# Save current network
net.save("example_network.net")
print("   Saved to: example_network.net")

# Load it back
net2 = glia.Network.from_file("example_network.net", verbose=False)
print(f"   Loaded: {net2}")
print()

# ========== Summary ==========
print("=" * 60)
print("Quick start complete!")
print()
print("Next steps:")
print("  - See examples/ for training and evolution demos")
print("  - Read docs/numpy_interface.md for data access patterns")
print("  - Try: python examples/xor_train.py")
print("=" * 60)
