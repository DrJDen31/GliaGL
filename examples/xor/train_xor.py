#!/usr/bin/env python
"""
XOR Problem Example

Train a network to learn the XOR function using spiking neurons.
This is the Python version of the CLI-based XOR example.

XOR truth table:
  A  B | Output
  0  0 | 0
  0  1 | 1
  1  0 | 1
  1  1 | 0
"""

import glia
import numpy as np
import argparse


def create_xor_dataset(n_episodes=40):
    """Create dataset for XOR problem"""
    episodes = []
    
    # XOR patterns
    patterns = [
        ({'S0': 0.0, 'S1': 0.0}, 'N2'),    # 0 XOR 0 = 0 → N2
        ({'S0': 100.0, 'S1': 0.0}, 'N3'),  # 1 XOR 0 = 1 → N3
        ({'S0': 0.0, 'S1': 100.0}, 'N3'),  # 0 XOR 1 = 1 → N3
        ({'S0': 100.0, 'S1': 100.0}, 'N2') # 1 XOR 1 = 0 → N2
    ]
    
    for i in range(n_episodes):
        ep = glia.EpisodeData()
        seq = glia.InputSequence()
        
        # Select pattern
        inputs, target = patterns[i % 4]
        seq.add_timestep(inputs)
        
        ep.seq = seq
        ep.target_id = target
        episodes.append(ep)
    
    return glia.Dataset(episodes)


def main():
    parser = argparse.ArgumentParser(description='XOR Problem Example')
    parser.add_argument('--train-episodes', type=int, default=40, help='Training episodes')
    parser.add_argument('--test-episodes', type=int, default=20, help='Test episodes')
    parser.add_argument('--epochs', type=int, default=100, help='Training epochs')
    parser.add_argument('--lr', type=float, default=0.02, help='Learning rate')
    parser.add_argument('--save', type=str, default='nets/xor_trained.net', help='Save path')
    parser.add_argument('--load', type=str, default='nets/xor_baseline.net', help='Load baseline')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    args = parser.parse_args()
    
    print("=" * 60)
    print("XOR Problem with Spiking Neural Network")
    print("=" * 60)
    
    # Load network
    print(f"\nLoading network from: {args.load}")
    net = glia.Network.from_file(args.load)
    print(f"Network: {net.num_neurons} neurons, {net.num_connections} connections")
    
    # Create datasets
    print(f"\nGenerating datasets...")
    train_data = create_xor_dataset(args.train_episodes)
    test_data = create_xor_dataset(args.test_episodes)
    print(f"Training: {len(train_data)} episodes")
    print(f"Testing: {len(test_data)} episodes")
    
    # Training configuration
    config = glia.create_config(
        lr=args.lr,
        batch_size=4,
        warmup_ticks=10,
        eval_ticks=50,
        verbose=False
    )
    
    # Evaluate before training
    print("\n" + "-" * 60)
    print("Before Training")
    print("-" * 60)
    trainer = glia.Trainer(net, config)
    
    correct = sum(1 for ep in test_data 
                  if trainer.evaluate(ep.seq, config).winner_id == ep.target_id)
    test_acc_before = correct / len(test_data)
    print(f"Test accuracy: {test_acc_before:.1%}")
    
    # Train
    print("\n" + "-" * 60)
    print(f"Training for {args.epochs} epochs")
    print("-" * 60)
    trainer.train_epoch(train_data, epochs=args.epochs, config=config)
    print(f"\nFinal training accuracy: {trainer.epoch_accuracy[-1]:.1%}")
    
    # Evaluate after training
    print("\n" + "-" * 60)
    print("After Training")
    print("-" * 60)
    
    correct = 0
    results = []
    for ep in test_data:
        metrics = trainer.evaluate(ep.seq, config)
        is_correct = (metrics.winner_id == ep.target_id)
        correct += int(is_correct)
        results.append({
            'target': ep.target_id,
            'predicted': metrics.winner_id,
            'correct': is_correct
        })
        
        if args.verbose:
            status = "✓" if is_correct else "✗"
            print(f"  {status} Target: {ep.target_id}, Predicted: {metrics.winner_id}")
    
    test_acc_after = correct / len(test_data)
    print(f"Test accuracy: {test_acc_after:.1%}")
    print(f"Improvement: {(test_acc_after - test_acc_before)*100:.1f} percentage points")
    
    # Save
    net.save(args.save)
    print(f"\n✓ Trained network saved to: {args.save}")
    
    print("\n" + "=" * 60)
    print("✓ XOR example complete!")
    print("=" * 60)


if __name__ == '__main__':
    main()
