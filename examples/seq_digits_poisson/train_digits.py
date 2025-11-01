#!/usr/bin/env python
"""
Handwritten Digits Classification Example

Train a spiking neural network to classify handwritten digits (0-9) from
Poisson-encoded spike sequences. This is the Python version of the C++ digits
training example.

Dataset: Poisson-encoded digit sequences (*.seq files)
Task: 10-class classification
"""

import glia
import numpy as np
import argparse
import csv
import json
from pathlib import Path


def load_dataset(data_root, split='train'):
    """
    Load digit sequences from data_root/{split}/
    
    Expected structure:
      data_root/train/labels.csv  (filename,label)
      data_root/train/*.seq       (sequence files)
      data_root/test/labels.csv
      data_root/test/*.seq
    """
    split_dir = Path(data_root) / split
    labels_file = split_dir / 'labels.csv'
    
    if not labels_file.exists():
        raise FileNotFoundError(f"Labels file not found: {labels_file}")
    
    episodes = []
    
    # Read labels
    with open(labels_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            filename = row['filename']
            label = int(row['label'])
            
            seq_path = split_dir / filename
            if not seq_path.exists():
                print(f"Warning: Sequence file not found: {seq_path}")
                continue
            
            # Load sequence
            try:
                seq = glia.InputSequence()
                seq.load_from_file(str(seq_path))
            except Exception as e:
                print(f"Warning: Failed to load {seq_path}: {e}")
                continue
            
            # Create episode
            ep = glia.EpisodeData()
            ep.seq = seq
            ep.target_id = f'O{label}'  # Output neurons O0-O9
            episodes.append(ep)
    
    print(f"Loaded {len(episodes)} episodes from {split_dir}")
    return glia.Dataset(episodes)


def evaluate_model(net, dataset, config, verbose=False):
    """Evaluate network and return accuracy, loss, predictions"""
    trainer = glia.Trainer(net, config)
    correct = 0
    predictions = []
    
    for ep in dataset:
        metrics = trainer.evaluate(ep.seq, config)
        predicted = metrics.winner_id
        target = ep.target_id
        is_correct = (predicted == target)
        correct += int(is_correct)
        
        predictions.append({
            'target': target,
            'predicted': predicted,
            'correct': is_correct,
            'margin': metrics.margin
        })
        
        if verbose:
            status = "✓" if is_correct else "✗"
            print(f"  {status} Target: {target}, Predicted: {predicted}, Margin: {metrics.margin:.3f}")
    
    accuracy = correct / len(dataset) if len(dataset) > 0 else 0.0
    return accuracy, predictions


def compute_confusion_matrix(predictions):
    """Compute 10x10 confusion matrix"""
    matrix = np.zeros((10, 10), dtype=int)
    
    for pred in predictions:
        target_digit = int(pred['target'][1])  # Extract digit from 'O0', 'O1', etc.
        predicted_digit = int(pred['predicted'][1]) if pred['predicted'] else -1
        
        if 0 <= predicted_digit <= 9:
            matrix[target_digit, predicted_digit] += 1
    
    return matrix


def print_confusion_matrix(matrix):
    """Pretty print confusion matrix"""
    print("\nConfusion Matrix:")
    print("       " + " ".join(f"{i:>4}" for i in range(10)))
    print("     " + "-" * 51)
    for i in range(10):
        print(f"{i:>4} | " + " ".join(f"{matrix[i,j]:>4}" for j in range(10)))


def main():
    # Get script directory for relative paths
    script_dir = Path(__file__).parent
    
    parser = argparse.ArgumentParser(description='Handwritten Digits Classification')
    parser.add_argument('--net', type=str, default=None, help='Network file')
    parser.add_argument('--data', type=str, default=None, help='Data root directory')
    parser.add_argument('--epochs', type=int, default=10, help='Training epochs')
    parser.add_argument('--batch-size', type=int, default=16, help='Batch size')
    parser.add_argument('--lr', type=float, default=0.01, help='Learning rate')
    parser.add_argument('--warmup', type=int, default=20, help='Warmup ticks')
    parser.add_argument('--decision-window', type=int, default=80, help='Decision window for output detection')
    parser.add_argument('--save', type=str, default=None, help='Save path')
    parser.add_argument('--results', type=str, default=None, help='Results directory')
    parser.add_argument('--seed', type=int, default=42, help='Random seed')
    parser.add_argument('--verbose', action='store_true', help='Verbose evaluation')
    args = parser.parse_args()
    
    # Resolve paths relative to script directory if not provided
    if args.net is None:
        args.net = str(script_dir / 'nets' / 'readout.net')
    if args.data is None:
        args.data = str(script_dir / 'data')
    if args.save is None:
        args.save = str(script_dir / 'nets' / 'digits_trained.net')
    if args.results is None:
        args.results = str(script_dir / 'results')
    
    print("=" * 70)
    print("Handwritten Digits Classification with Spiking Neural Network")
    print("=" * 70)
    
    # Set seed
    np.random.seed(args.seed)
    
    # Load network
    print(f"\nLoading network from: {args.net}")
    if not Path(args.net).exists():
        print(f"Error: Network file not found: {args.net}")
        print("\nPlease provide a network file with --net PATH")
        print("Or ensure nets/readout.net exists in the example directory.")
        return 1
    
    net = glia.Network.from_file(args.net)
    print(f"Network: {net.num_neurons} neurons, {net.num_connections} connections")
    
    # Load datasets
    print("\nLoading datasets...")
    try:
        train_data = load_dataset(args.data, 'train')
        test_data = load_dataset(args.data, 'test')
    except FileNotFoundError as e:
        print(f"Error: {e}")
        print("\nExpected structure:")
        print("  data/train/labels.csv")
        print("  data/train/*.seq")
        print("  data/test/labels.csv")
        print("  data/test/*.seq")
        return 1
    
    # Training configuration
    config = glia.create_config(
        lr=args.lr,
        batch_size=args.batch_size,
        warmup_ticks=args.warmup,
        decision_window=args.decision_window,
        verbose=False
    )
    
    # Evaluate before training
    print("\n" + "-" * 70)
    print("Before Training")
    print("-" * 70)
    train_acc_before, _ = evaluate_model(net, train_data, config)
    test_acc_before, test_preds_before = evaluate_model(net, test_data, config)
    print(f"Training accuracy: {train_acc_before:.2%}")
    print(f"Test accuracy: {test_acc_before:.2%}")
    
    # Train
    print("\n" + "-" * 70)
    print(f"Training for {args.epochs} epochs")
    print("-" * 70)
    trainer = glia.Trainer(net, config)
    
    # Train with progress reporting
    history = trainer.train(
        train_data.episodes,
        epochs=args.epochs,
        config=config,
        on_epoch=lambda e, acc, margin: print(f"Epoch {e+1}/{args.epochs}: Train acc={acc:.2%}, Margin={margin:.3f}")
    )
    
    # Final evaluation
    print("\n" + "-" * 70)
    print("After Training")
    print("-" * 70)
    train_acc_after, train_preds = evaluate_model(net, train_data, config)
    test_acc_after, test_preds = evaluate_model(net, test_data, config, verbose=args.verbose)
    
    print(f"Training accuracy: {train_acc_after:.2%}")
    print(f"Test accuracy: {test_acc_after:.2%}")
    print(f"Improvement: {(test_acc_after - test_acc_before)*100:.1f} percentage points")
    
    # Confusion matrix
    conf_matrix = compute_confusion_matrix(test_preds)
    print_confusion_matrix(conf_matrix)
    
    # Per-digit accuracy
    print("\nPer-Digit Accuracy:")
    for digit in range(10):
        correct = conf_matrix[digit, digit]
        total = conf_matrix[digit, :].sum()
        acc = correct / total if total > 0 else 0.0
        print(f"  Digit {digit}: {acc:.1%} ({correct}/{total})")
    
    # Save results
    results_dir = Path(args.results)
    results_dir.mkdir(exist_ok=True)
    
    # Save trained network
    net.save(args.save)
    print(f"\n✓ Trained network saved to: {args.save}")
    
    # Save metrics
    metrics = {
        'test_accuracy_before': float(test_acc_before),
        'test_accuracy_after': float(test_acc_after),
        'train_accuracy_final': float(train_acc_after),
        'epochs': args.epochs,
        'batch_size': args.batch_size,
        'learning_rate': args.lr,
        'improvement': float(test_acc_after - test_acc_before),
        'confusion_matrix': conf_matrix.tolist()
    }
    
    metrics_file = results_dir / 'metrics.json'
    with open(metrics_file, 'w') as f:
        json.dump(metrics, f, indent=2)
    print(f"✓ Metrics saved to: {metrics_file}")
    
    # Save predictions
    preds_file = results_dir / 'predictions_test.csv'
    with open(preds_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['target', 'predicted', 'correct', 'margin'])
        writer.writeheader()
        writer.writerows(test_preds)
    print(f"✓ Predictions saved to: {preds_file}")
    
    print("\n" + "=" * 70)
    print("✓ Training complete!")
    print("=" * 70)
    
    return 0


if __name__ == '__main__':
    exit(main())
