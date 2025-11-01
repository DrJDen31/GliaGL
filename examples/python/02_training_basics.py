#!/usr/bin/env python3
"""
Example 2: Training Basics

This example demonstrates:
- Loading a pre-trained network
- Creating a simple dataset
- Training with callbacks
- Evaluating performance
"""

import glia
import numpy as np
from pathlib import Path

print("=" * 60)
print("Example 2: Training Basics")
print("=" * 60)
print()

# ========== Load Network ==========
print("[Step 1] Loading network...")

# Try to find an example network
# In a real scenario, you'd have your own .net file
network_file = "example_network.net"

# Create a simple network if file doesn't exist
if not Path(network_file).exists():
    print(f"   Network file not found, creating one...")
    net = glia.Network(num_sensory=2, num_neurons=5)
    net.save(network_file)
    print(f"   Created and saved: {network_file}")
else:
    net = glia.Network.from_file(network_file, verbose=False)
    print(f"   Loaded from: {network_file}")

print(f"   Network: {net}")
print()

# ========== Create Dataset ==========
print("[Step 2] Creating a simple dataset...")

# For this example, we'll create dummy episodes
# In a real application, you'd load actual data
episodes = []

for i in range(20):
    # Create episode
    ep = glia.EpisodeData()
    
    # Create simple input sequence
    seq = glia.InputSequence()
    for t in range(10):
        # Add some time-varying input
        seq.add_timestep({
            "S0": 100.0 if i % 2 == 0 else 50.0,
            "S1": 50.0 if i % 2 == 0 else 100.0,
        })
    
    ep.seq = seq
    ep.target_id = "N0" if i % 2 == 0 else "N1"  # Alternate targets
    episodes.append(ep)

dataset = glia.Dataset(episodes)
print(f"   Created dataset with {len(dataset)} episodes")

# Split into train and validation
train_data, val_data = dataset.split(train_fraction=0.75, shuffle=True, seed=42)
print(f"   Train: {len(train_data)} episodes")
print(f"   Validation: {len(val_data)} episodes")
print()

# ========== Configure Training ==========
print("[Step 3] Configuring training...")

config = glia.create_config(
    lr=0.01,
    batch_size=4,
    warmup_ticks=10,
    decision_window=20,
    verbose=False
)

print(f"   Learning rate: {config.lr}")
print(f"   Batch size: {config.batch_size}")
print()

# ========== Train Network ==========
print("[Step 4] Training network...")
print()

trainer = glia.Trainer(net, config)
trainer.set_seed(42)  # For reproducibility

# Define callback for progress monitoring
def on_epoch_complete(epoch, accuracy, margin):
    # Print every 10 epochs
    if (epoch + 1) % 10 == 0:
        print(f"   Epoch {epoch+1:3d}: Accuracy={accuracy:6.2%}, Margin={margin:6.3f}")

# Train with callback
history = trainer.train(
    train_data.episodes,
    epochs=50,
    config=config,
    on_epoch=on_epoch_complete,
    verbose=False
)

print()

# ========== Evaluate ==========
print("[Step 5] Evaluating on validation set...")

results = trainer.evaluate_dataset(val_data.episodes, config)

print(f"   Validation Accuracy: {results['accuracy']:.2%}")
print(f"   Validation Margin: {results['margin']:.3f}")
print(f"   Correct: {results['correct']}/{results['total']}")
print()

# ========== Analyze Training History ==========
print("[Step 6] Training history...")

if history['accuracy']:
    initial_acc = history['accuracy'][0]
    final_acc = history['accuracy'][-1]
    best_acc = max(history['accuracy'])
    
    print(f"   Initial accuracy: {initial_acc:.2%}")
    print(f"   Final accuracy: {final_acc:.2%}")
    print(f"   Best accuracy: {best_acc:.2%}")
    print(f"   Improvement: {final_acc - initial_acc:+.2%}")
    
    if history['margin']:
        initial_margin = history['margin'][0]
        final_margin = history['margin'][-1]
        print(f"   Margin improvement: {final_margin - initial_margin:+.3f}")

print()

# ========== Save Trained Network ==========
print("[Step 7] Saving trained network...")
trained_file = "example_network_trained.net"
net.save(trained_file)
print(f"   Saved to: {trained_file}")
print()

# ========== Summary ==========
print("=" * 60)
print("Summary:")
print(f"  - Trained for 50 epochs")
print(f"  - Dataset: {len(train_data)} train, {len(val_data)} validation")
print(f"  - Final accuracy: {final_acc:.2%}")
print(f"  - Network saved to: {trained_file}")
print()
print("This demonstrates basic training workflow with GliaGL.")
print("=" * 60)
