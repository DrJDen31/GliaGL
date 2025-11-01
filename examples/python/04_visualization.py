#!/usr/bin/env python3
"""
Example 4: Visualization

This example demonstrates:
- Network graph visualization
- Weight distribution plots
- Training history plots
- Neuron activity raster plots
"""

import glia
import numpy as np

print("=" * 60)
print("Example 4: Visualization")
print("=" * 60)
print()

# Check if visualization is available
try:
    import glia.viz as viz
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    viz_available = True
    print("[✓] Visualization modules available")
except ImportError as e:
    viz_available = False
    print(f"[!] Visualization not available: {e}")
    print(f"    Install with: pip install matplotlib networkx")
    print()

if not viz_available:
    print("This example requires matplotlib and networkx.")
    print("Exiting...")
    exit(0)

print()

# ========== Create and Setup Network ==========
print("[Step 1] Creating network with connections...")

net = glia.Network(num_sensory=2, num_neurons=8)

# Add some random connections
neuron_ids = net.neuron_ids
n_neurons = len(neuron_ids)

# Create connectivity
rng = np.random.RandomState(42)
n_connections = 15

from_indices = rng.randint(0, n_neurons, size=n_connections)
to_indices = rng.randint(0, n_neurons, size=n_connections)
weights = rng.uniform(-1.0, 2.0, size=n_connections)

from_ids = [neuron_ids[i] for i in from_indices]
to_ids = [neuron_ids[i] for i in to_indices]

net.set_weights(from_ids, to_ids, weights)

print(f"   Created network: {net}")
print(f"   Connections: {net.num_connections}")
print()

# ========== Network Graph Visualization ==========
print("[Step 2] Creating network graph visualization...")

try:
    viz.plot_network_graph(
        net,
        layout='spring',
        node_size=800,
        show_weights=True,
        save_path='network_graph.png'
    )
    print(f"   ✓ Saved: network_graph.png")
except Exception as e:
    print(f"   ✗ Failed to create network graph: {e}")

print()

# ========== Weight Distribution ==========
print("[Step 3] Creating weight distribution plot...")

try:
    viz.plot_weight_distribution(
        net,
        bins=20,
        save_path='weight_distribution.png'
    )
    print(f"   ✓ Saved: weight_distribution.png")
except Exception as e:
    print(f"   ✗ Failed to create weight distribution: {e}")

print()

# ========== Training History Visualization ==========
print("[Step 4] Creating training history plot...")

# Create dummy training dataset
episodes = []
for i in range(30):
    ep = glia.EpisodeData()
    seq = glia.InputSequence()
    for t in range(10):
        seq.add_timestep({"S0": 100.0, "S1": 50.0})
    ep.seq = seq
    ep.target_id = "N0" if i % 2 == 0 else "N1"
    episodes.append(ep)

dataset = glia.Dataset(episodes)

# Train briefly
config = glia.create_config(lr=0.01, batch_size=4, warmup_ticks=5, decision_window=10)
trainer = glia.Trainer(net, config)

print(f"   Training for 30 epochs...")
history = trainer.train(
    dataset.episodes,
    epochs=30,
    verbose=False
)

try:
    viz.plot_training_history(
        history,
        save_path='training_history.png'
    )
    print(f"   ✓ Saved: training_history.png")
except Exception as e:
    print(f"   ✗ Failed to create training history: {e}")

print()

# ========== Neuron Activity Raster Plot ==========
print("[Step 5] Creating neuron activity raster plot...")

# Reset network and run simulation
net2 = glia.Network(num_sensory=2, num_neurons=6)

# Add some connections to make it interesting
from_ids_sim = ["S0", "S1", "S0", "S1"]
to_ids_sim = ["N0", "N1", "N2", "N3"]
weights_sim = np.array([1.5, 1.2, 1.8, 1.0])
net2.set_weights(from_ids_sim, to_ids_sim, weights_sim)

try:
    viz.plot_neuron_activity(
        net2,
        simulation_steps=100,
        input_pattern={"S0": 120.0, "S1": 100.0},
        neuron_ids=None,  # Plot all neurons
        save_path='neuron_activity.png'
    )
    print(f"   ✓ Saved: neuron_activity.png")
except Exception as e:
    print(f"   ✗ Failed to create activity raster: {e}")

print()

# ========== Custom Visualization Example ==========
print("[Step 6] Creating custom visualization...")

try:
    import matplotlib.pyplot as plt
    
    # Get weight statistics over time (simulate training)
    weight_history = []
    for epoch in range(20):
        from_ids, to_ids, weights = net.get_weights()
        if len(weights) > 0:
            weight_history.append({
                'mean': weights.mean(),
                'std': weights.std(),
                'max': weights.max(),
                'min': weights.min()
            })
    
    # Plot
    fig, ax = plt.subplots(figsize=(10, 6))
    
    epochs = list(range(len(weight_history)))
    means = [h['mean'] for h in weight_history]
    stds = [h['std'] for h in weight_history]
    
    ax.plot(epochs, means, 'b-', linewidth=2, label='Mean')
    ax.fill_between(epochs,
                     [m - s for m, s in zip(means, stds)],
                     [m + s for m, s in zip(means, stds)],
                     alpha=0.3, color='b')
    
    ax.set_xlabel('Epoch')
    ax.set_ylabel('Weight Value')
    ax.set_title('Weight Statistics Over Training')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('custom_plot.png', dpi=150)
    plt.close()
    
    print(f"   ✓ Saved: custom_plot.png")
except Exception as e:
    print(f"   ✗ Failed to create custom plot: {e}")

print()

# ========== Summary ==========
print("=" * 60)
print("Summary:")
print(f"  - Network graph visualization")
print(f"  - Weight distribution analysis")
print(f"  - Training history plots")
print(f"  - Neuron activity raster plots")
print(f"  - Custom matplotlib integration")
print()
print("Generated visualizations:")
print(f"  - network_graph.png")
print(f"  - weight_distribution.png")
print(f"  - training_history.png")
print(f"  - neuron_activity.png")
print(f"  - custom_plot.png")
print()
print("This demonstrates visualization capabilities in GliaGL.")
print("=" * 60)
