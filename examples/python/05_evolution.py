#!/usr/bin/env python3
"""
Example 5: Evolutionary Training

This example demonstrates:
- Setting up evolutionary training
- Configuring evolution parameters
- Running evolution
- Analyzing and visualizing results
- Loading the best genome
"""

import glia
import numpy as np
from pathlib import Path

print("=" * 60)
print("Example 5: Evolutionary Training")
print("=" * 60)
print()

# ========== Prepare Baseline Network ==========
print("[Step 1] Creating baseline network...")

baseline_file = "evolution_baseline.net"

# Create a simple network as baseline
net = glia.Network(num_sensory=2, num_neurons=8)

# Add initial sparse connectivity
neuron_ids = net.neuron_ids
rng = np.random.RandomState(42)

n_init_connections = 10
from_indices = rng.randint(0, len(neuron_ids), size=n_init_connections)
to_indices = rng.randint(0, len(neuron_ids), size=n_init_connections)
init_weights = rng.uniform(-0.5, 1.5, size=n_init_connections)

from_ids = [neuron_ids[i] for i in from_indices]
to_ids = [neuron_ids[i] for i in to_indices]
net.set_weights(from_ids, to_ids, init_weights)

# Save baseline
net.save(baseline_file)
print(f"   Baseline network: {net}")
print(f"   Saved to: {baseline_file}")
print()

# ========== Create Dataset ==========
print("[Step 2] Creating dataset...")

# Create simple classification problem
# Class 0: S0 high, S1 low → Target N0
# Class 1: S0 low, S1 high → Target N1

train_episodes = []
val_episodes = []

for i in range(40):  # Training data
    ep = glia.EpisodeData()
    seq = glia.InputSequence()
    
    is_class0 = (i % 2 == 0)
    
    for t in range(15):
        seq.add_timestep({
            "S0": 120.0 if is_class0 else 40.0,
            "S1": 40.0 if is_class0 else 120.0,
        })
    
    ep.seq = seq
    ep.target_id = "N0" if is_class0 else "N1"
    train_episodes.append(ep)

for i in range(10):  # Validation data
    ep = glia.EpisodeData()
    seq = glia.InputSequence()
    
    is_class0 = (i % 2 == 0)
    
    for t in range(15):
        seq.add_timestep({
            "S0": 120.0 if is_class0 else 40.0,
            "S1": 40.0 if is_class0 else 120.0,
        })
    
    ep.seq = seq
    ep.target_id = "N0" if is_class0 else "N1"
    val_episodes.append(ep)

print(f"   Training episodes: {len(train_episodes)}")
print(f"   Validation episodes: {len(val_episodes)}")
print()

# ========== Configure Training ==========
print("[Step 3] Configuring training parameters...")

train_config = glia.create_config(
    lr=0.02,
    batch_size=4,
    warmup_ticks=10,
    decision_window=20
)

print(f"   Learning rate: {train_config.lr}")
print(f"   Batch size: {train_config.batch_size}")
print()

# ========== Configure Evolution ==========
print("[Step 4] Configuring evolution parameters...")

evo_config = glia.create_evo_config(
    population=8,      # Small population for quick demo
    generations=15,     # Few generations for quick demo
    elite=2,           # Top 2 preserved each generation
    train_epochs=5     # Brief training for each individual
)

# Set additional evolution parameters
evo_config.sigma_w = 0.1       # Weight mutation strength
evo_config.sigma_thr = 2.0     # Threshold mutation strength
evo_config.sigma_leak = 0.01   # Leak mutation strength
evo_config.w_acc = 1.0         # Accuracy weight in fitness
evo_config.w_margin = 0.5      # Margin weight in fitness
evo_config.w_sparsity = 0.01   # Sparsity penalty weight
evo_config.lamarckian = True   # Use Lamarckian evolution
evo_config.seed = 42

print(f"   Population: {evo_config.population}")
print(f"   Generations: {evo_config.generations}")
print(f"   Elite count: {evo_config.elite}")
print(f"   Training epochs per individual: {evo_config.train_epochs}")
print(f"   Lamarckian: {evo_config.lamarckian}")
print()

# ========== Run Evolution ==========
print("[Step 5] Running evolutionary training...")
print(f"   This will take a minute...")
print()

evo = glia.Evolution(
    network_path=baseline_file,
    train_data=train_episodes,
    val_data=val_episodes,
    train_config=train_config,
    evo_config=evo_config
)

# Run evolution (GIL released - can run in background)
result = evo.run(verbose=True)

print()

# ========== Analyze Results ==========
print("[Step 6] Analyzing evolution results...")

print(f"   Best fitness progression:")
for gen in range(0, len(result.best_fitness_hist), max(1, len(result.best_fitness_hist)//5)):
    fitness = result.best_fitness_hist[gen]
    accuracy = result.best_acc_hist[gen]
    margin = result.best_margin_hist[gen]
    print(f"      Gen {gen:2d}: Fitness={fitness:.4f}, Acc={accuracy:.2%}, Margin={margin:.3f}")

print()
print(f"   Final results:")
print(f"      Best fitness: {result.best_fitness_hist[-1]:.4f}")
print(f"      Best accuracy: {result.best_acc_hist[-1]:.2%}")
print(f"      Best margin: {result.best_margin_hist[-1]:.3f}")
print()

# ========== Load Best Genome ==========
print("[Step 7] Loading best evolved network...")

best_net = glia.Evolution.load_best_genome(baseline_file, result)
print(f"   Loaded: {best_net}")

# Save it
best_file = "evolution_best.net"
best_net.save(best_file)
print(f"   Saved to: {best_file}")
print()

# ========== Visualize Results ==========
print("[Step 8] Creating visualizations...")

try:
    glia.plot_evolution_result(result, save_path='evolution_history.png')
    print(f"   ✓ Saved: evolution_history.png")
except ImportError:
    print(f"   ✗ Matplotlib not available for plotting")
except Exception as e:
    print(f"   ✗ Failed to plot: {e}")

print()

# ========== Compare Baseline vs Evolved ==========
print("[Step 9] Comparing baseline vs evolved network...")

# Evaluate baseline
baseline_net = glia.Network.from_file(baseline_file, verbose=False)
baseline_trainer = glia.Trainer(baseline_net)
baseline_results = baseline_trainer.evaluate_dataset(val_episodes, train_config)

# Evaluate evolved
evolved_trainer = glia.Trainer(best_net)
evolved_results = evolved_trainer.evaluate_dataset(val_episodes, train_config)

print(f"   Baseline Network:")
print(f"      Accuracy: {baseline_results['accuracy']:.2%}")
print(f"      Margin: {baseline_results['margin']:.3f}")
print()
print(f"   Evolved Network:")
print(f"      Accuracy: {evolved_results['accuracy']:.2%}")
print(f"      Margin: {evolved_results['margin']:.3f}")
print()
print(f"   Improvement:")
print(f"      Accuracy: {evolved_results['accuracy'] - baseline_results['accuracy']:+.2%}")
print(f"      Margin: {evolved_results['margin'] - baseline_results['margin']:+.3f}")
print()

# ========== Summary ==========
print("=" * 60)
print("Summary:")
print(f"  - Ran {evo_config.generations} generations of evolution")
print(f"  - Population: {evo_config.population} individuals")
print(f"  - Final fitness: {result.best_fitness_hist[-1]:.4f}")
print(f"  - Final accuracy: {result.best_acc_hist[-1]:.2%}")
print(f"  - Improvement: {evolved_results['accuracy'] - baseline_results['accuracy']:+.2%}")
print()
print("Files created:")
print(f"  - {baseline_file} (baseline network)")
print(f"  - {best_file} (best evolved network)")
print(f"  - evolution_history.png (visualization)")
print()
print("This demonstrates evolutionary training with GliaGL.")
print("=" * 60)
