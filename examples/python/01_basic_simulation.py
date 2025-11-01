#!/usr/bin/env python3
"""
Example 1: Basic Network Simulation

This example demonstrates:
- Creating a simple network
- Injecting sensory inputs
- Running simulation
- Monitoring neuron activity
"""

import glia
import numpy as np

print("=" * 60)
print("Example 1: Basic Network Simulation")
print("=" * 60)
print()

# ========== Create Network ==========
print("[Step 1] Creating a small network...")
# 2 sensory neurons, 5 internal neurons
net = glia.Network(num_sensory=2, num_neurons=5)
print(f"   Created: {net}")
print(f"   Sensory neurons: {net.sensory_ids}")
print(f"   Total neurons: {net.neuron_ids}")
print()

# ========== Inspect Initial State ==========
print("[Step 2] Initial network state...")
state = net.state
print(f"   Thresholds: {state['thresholds'][:5]}...")
print(f"   Leaks: {state['leaks'][:5]}...")
print()

# ========== Run Simulation ==========
print("[Step 3] Running simulation with inputs...")
print()

# Simulation parameters
n_steps = 50
input_current = 150.0  # Strong input to cause firing

# Track firing activity
firing_history = []

for step in range(n_steps):
    # Inject input every 10 steps
    if step % 10 == 0:
        net.inject_dict({
            "S0": input_current,
            "S1": input_current * 0.5
        })
        print(f"   Step {step:3d}: Injected inputs")
    
    # Run one timestep
    net.step()
    
    # Check which neurons fired
    firing = net.get_firing_neurons()
    firing_history.append(firing)
    
    if firing:
        print(f"   Step {step:3d}: Firing → {firing}")

print()

# ========== Analyze Results ==========
print("[Step 4] Analyzing results...")

# Count how many times each neuron fired
from collections import Counter
all_firing = [nid for step_firing in firing_history for nid in step_firing]
firing_counts = Counter(all_firing)

print(f"   Total timesteps: {n_steps}")
print(f"   Firing events:")
for neuron_id, count in sorted(firing_counts.items()):
    print(f"      {neuron_id}: {count} times ({count/n_steps*100:.1f}%)")

if not firing_counts:
    print("      (No neurons fired - try increasing input_current)")

print()

# ========== State After Simulation ==========
print("[Step 5] Final network state...")
final_state = net.state
print(f"   Final membrane values: {final_state['values']}")
print()

# ========== Summary ==========
print("=" * 60)
print("Summary:")
print(f"  - Simulated {n_steps} timesteps")
print(f"  - {len(firing_counts)} unique neurons fired")
print(f"  - Total firing events: {sum(firing_counts.values())}")
print()
print("This demonstrates basic network simulation with GliaGL.")
print("=" * 60)
