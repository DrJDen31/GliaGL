#!/usr/bin/env python
"""
3-Class Classification Example

Train a network to classify 3 one-hot encoded classes with noise robustness.
This is the Python version of the CLI-based 3class example.

Network Architecture:
- 3 sensory neurons (S0, S1, S2) for one-hot input
- 1 inhibitory pool neuron (I)
- 3 output neurons (N0/O0, N1/O1, N2/O2)

The inhibitory pool provides winner-take-all dynamics for robust classification.
"""

import glia
import numpy as np
import argparse


def create_3class_network():
    """Create the 3-class classification network"""
    # Create network: 3 sensory + 4 internal (1 inhibitory + 3 outputs)
    net = glia.Network(num_sensory=3, num_neurons=4)
    
    # Connection topology:
    # S0 -> N1 (O0), S1 -> N2 (O1), S2 -> N3 (O2)  [feedforward]
    # N1,N2,N3 -> N0 (I)  [output to inhibitory pool]
    # N0 (I) -> N1,N2,N3  [inhibitory feedback]
    
    from_ids = [
        # Feedforward (sensory -> output)
        'S0', 'S1', 'S2',
        # Output -> Inhibitory
        'N1', 'N2', 'N3',
        # Inhibitory -> Output
        'N0', 'N0', 'N0'
    ]
    
    to_ids = [
        # Feedforward
        'N1', 'N2', 'N3',
        # Output -> Inhibitory
        'N0', 'N0', 'N0',
        # Inhibitory -> Output
        'N1', 'N2', 'N3'
    ]
    
    weights = np.array([
        # Feedforward (strong excitatory)
        60.0, 60.0, 60.0,
        # Output -> Inhibitory (moderate excitatory)
        35.0, 35.0, 35.0,
        # Inhibitory -> Output (strong inhibitory)
        -45.0, -45.0, -45.0
    ])
    
    net.set_weights(from_ids, to_ids, weights)
    
    # Set neuron parameters
    ids, _, thresholds, leaks = net.get_state()
    
    # Inhibitory pool: lower threshold, higher leak
    # Outputs: higher threshold, no leak
    for i, nid in enumerate(ids):
        if nid == 'N0':  # Inhibitory pool
            thresholds[i] = 40.0
            leaks[i] = 0.8
        elif nid in ['N1', 'N2', 'N3']:  # Outputs
            thresholds[i] = 50.0
            leaks[i] = 1.0
    
    net.set_state(ids, thresholds, leaks)
    
    return net


def create_3class_dataset(n_episodes=60, noise_level=0.1):
    """
    Create dataset for 3-class classification
    
    Args:
        n_episodes: Number of training episodes
        noise_level: Probability of noise in non-target inputs (0.0 to 1.0)
    """
    episodes = []
    
    for i in range(n_episodes):
        ep = glia.EpisodeData()
        seq = glia.InputSequence()
        
        # Determine class
        class_id = i % 3
        
        # Create one-hot input with noise
        inputs = {}
        for j in range(3):
            if j == class_id:
                # Target input (strong)
                inputs[f'S{j}'] = 100.0
            else:
                # Non-target with potential noise
                if np.random.rand() < noise_level:
                    inputs[f'S{j}'] = np.random.uniform(20.0, 60.0)
                else:
                    inputs[f'S{j}'] = 0.0
        
        seq.add_timestep(inputs)
        ep.seq = seq
        ep.target_id = f'N{class_id + 1}'  # N1, N2, or N3
        
        episodes.append(ep)
    
    return glia.Dataset(episodes)


def evaluate_network(net, dataset, config, verbose=False):
    """Evaluate network on dataset"""
    trainer = glia.Trainer(net, config)
    correct = 0
    results = []
    
    for ep in dataset:
        metrics = trainer.evaluate(ep.seq, config)
        is_correct = (metrics.winner_id == ep.target_id)
        correct += int(is_correct)
        
        results.append({
            'target': ep.target_id,
            'predicted': metrics.winner_id,
            'correct': is_correct,
            'margin': metrics.margin
        })
        
        if verbose:
            status = "✓" if is_correct else "✗"
            print(f"  {status} Target: {ep.target_id}, Predicted: {metrics.winner_id}, Margin: {metrics.margin:.3f}")
    
    accuracy = correct / len(dataset)
    return accuracy, results


def main():
    parser = argparse.ArgumentParser(description='3-Class Classification Example')
    parser.add_argument('--noise', type=float, default=0.1, help='Noise level (0.0-1.0)')
    parser.add_argument('--train-episodes', type=int, default=60, help='Training episodes')
    parser.add_argument('--test-episodes', type=int, default=30, help='Test episodes')
    parser.add_argument('--epochs', type=int, default=50, help='Training epochs')
    parser.add_argument('--lr', type=float, default=0.01, help='Learning rate')
    parser.add_argument('--save', type=str, default='nets/3class_trained.net', help='Save path')
    parser.add_argument('--load', type=str, default=None, help='Load pretrained network')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    args = parser.parse_args()
    
    print("=" * 60)
    print("3-Class Classification with Inhibitory Pool")
    print("=" * 60)
    
    # Create or load network
    if args.load:
        print(f"\nLoading network from: {args.load}")
        net = glia.Network.from_file(args.load)
    else:
        print("\nCreating 3-class network...")
        net = glia.Network()
        net.load('nets/3class_network.net')
    
    print(f"Network: {net.num_neurons} neurons, {net.num_connections} connections")
    
    # Create datasets
    print(f"\nGenerating datasets (noise level: {args.noise:.1%})...")
    train_data = create_3class_dataset(args.train_episodes, noise_level=args.noise)
    test_data = create_3class_dataset(args.test_episodes, noise_level=args.noise)
    print(f"Training: {len(train_data)} episodes")
    print(f"Testing: {len(test_data)} episodes")
    
    # Training configuration
    config = glia.create_config(
        lr=args.lr,
        batch_size=8,
        warmup_ticks=10,
        eval_ticks=50,
        verbose=False
    )
    
    # Evaluate before training
    print("\n" + "-" * 60)
    print("Before Training")
    print("-" * 60)
    train_acc_before, _ = evaluate_network(net, train_data, config, verbose=False)
    test_acc_before, _ = evaluate_network(net, test_data, config, verbose=False)
    print(f"Training accuracy: {train_acc_before:.1%}")
    print(f"Test accuracy: {test_acc_before:.1%}")
    
    # Train
    print("\n" + "-" * 60)
    print(f"Training for {args.epochs} epochs")
    print("-" * 60)
    trainer = glia.Trainer(net, config)
    trainer.train_epoch(train_data, epochs=args.epochs, config=config)
    
    print(f"\nFinal training accuracy: {trainer.epoch_accuracy[-1]:.1%}")
    
    # Evaluate after training
    print("\n" + "-" * 60)
    print("After Training")
    print("-" * 60)
    train_acc_after, _ = evaluate_network(net, train_data, config, verbose=False)
    test_acc_after, results = evaluate_network(net, test_data, config, verbose=args.verbose)
    print(f"Training accuracy: {train_acc_after:.1%}")
    print(f"Test accuracy: {test_acc_after:.1%}")
    print(f"Improvement: {(test_acc_after - test_acc_before)*100:.1f} percentage points")
    
    # Save trained network
    net.save(args.save)
    print(f"\n✓ Trained network saved to: {args.save}")
    
    # Confusion matrix
    print("\n" + "-" * 60)
    print("Confusion Matrix (Test Set)")
    print("-" * 60)
    confusion = {}
    for r in results:
        key = (r['target'], r['predicted'])
        confusion[key] = confusion.get(key, 0) + 1
    
    targets = ['N1', 'N2', 'N3']
    print(f"{'':>10} | {'Predicted':^30}")
    print(f"{'Target':>10} | " + " ".join([f"{t:>8}" for t in targets]))
    print("-" * 50)
    for target in targets:
        counts = [confusion.get((target, pred), 0) for pred in targets]
        print(f"{target:>10} | " + " ".join([f"{c:>8}" for c in counts]))
    
    print("\n" + "=" * 60)
    print("✓ Example complete!")
    print("=" * 60)


if __name__ == '__main__':
    main()
