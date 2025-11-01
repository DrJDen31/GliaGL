# GliaGL Advanced Usage Guide

Advanced techniques for working with GliaGL.

---

## Table of Contents

- [Evolutionary Training](#evolutionary-training)
- [Custom Training Loops](#custom-training-loops)
- [Network Architecture Design](#network-architecture-design)
- [Performance Optimization](#performance-optimization)
- [Advanced NumPy Integration](#advanced-numpy-integration)
- [Custom Metrics and Callbacks](#custom-metrics-and-callbacks)
- [Multi-Task Learning](#multi-task-learning)
- [Hyperparameter Tuning](#hyperparameter-tuning)

---

## Evolutionary Training

Evolutionary algorithms can discover network architectures and parameters that gradient-based methods cannot.

### Basic Evolution

```python
import glia

# Load baseline network
baseline_net = glia.Network.from_file('baseline.net')

# Create dataset
train_data, val_data = dataset.split(0.8, seed=42)

# Configure training
train_cfg = glia.create_config(
    lr=0.02,
    batch_size=8,
    warmup_ticks=20,
    eval_ticks=100
)

# Configure evolution
evo_cfg = glia.create_evolution_config(
    population=30,          # 30 individuals
    generations=100,        # 100 generations
    elite=6,                # Keep top 6
    parents_pool=12,        # 12 parents per generation
    train_epochs=10,        # Train each individual for 10 epochs
    sigma_w=0.15,           # Weight mutation strength
    sigma_thr=3.0,          # Threshold mutation strength
    sigma_leak=0.02,        # Leak mutation strength
    w_acc=1.0,              # Accuracy weight in fitness
    w_margin=0.5,           # Margin weight in fitness
    w_sparsity=0.02,        # Sparsity penalty
    lamarckian=True,        # Enable learned → genetic transfer
    seed=42
)

# Run evolution
evo = glia.Evolution(
    'baseline.net',
    train_data,
    val_data,
    train_cfg,
    evo_cfg
)

result = evo.run()

# Load best network
best_net = glia.Network()
best_net.load_from_genome(result.best_genome)
best_net.save('evolved_best.net')

print(f"Best fitness: {result.best_fitness:.4f}")
print(f"Best accuracy: {result.best_acc_hist[-1]:.1%}")
```

### Analyzing Evolution Results

```python
import matplotlib.pyplot as plt

# Plot fitness progression
plt.figure(figsize=(12, 4))

plt.subplot(1, 3, 1)
plt.plot(result.best_fitness_hist)
plt.xlabel('Generation')
plt.ylabel('Fitness')
plt.title('Best Fitness Over Time')

plt.subplot(1, 3, 2)
plt.plot(result.best_acc_hist)
plt.xlabel('Generation')
plt.ylabel('Accuracy')
plt.title('Best Accuracy Over Time')

plt.subplot(1, 3, 3)
plt.plot(result.best_margin_hist)
plt.xlabel('Generation')
plt.ylabel('Margin')
plt.title('Best Margin Over Time')

plt.tight_layout()
plt.savefig('evolution_analysis.png')
```

### Custom Fitness Functions

For custom fitness, modify the evolution configuration:

```python
# Emphasize sparsity more
evo_cfg = glia.create_evolution_config(
    w_acc=1.0,
    w_margin=0.3,
    w_sparsity=0.1,  # Stronger sparsity penalty
    # ... other params
)

# Or emphasize margin
evo_cfg = glia.create_evolution_config(
    w_acc=0.7,
    w_margin=1.0,  # Prioritize margin
    w_sparsity=0.01,
)
```

---

## Custom Training Loops

For maximum control, implement custom training loops.

### Manual Batch Training

```python
import numpy as np

def custom_train(net, dataset, config, epochs):
    trainer = glia.Trainer(net, config)
    
    best_acc = 0.0
    patience = 10
    patience_counter = 0
    
    for epoch in range(epochs):
        # Shuffle dataset
        dataset.shuffle(seed=epoch)
        
        # Process in batches
        batch_size = config.batch_size
        for i in range(0, len(dataset), batch_size):
            batch = dataset[i:i+batch_size]
            trainer.train_batch(batch, config)
        
        # Evaluate
        val_acc = evaluate_dataset(trainer, val_data, config)
        
        # Early stopping
        if val_acc > best_acc:
            best_acc = val_acc
            patience_counter = 0
            net.save('best_checkpoint.net')
        else:
            patience_counter += 1
        
        if patience_counter >= patience:
            print(f"Early stopping at epoch {epoch}")
            break
        
        # Adaptive learning rate
        if epoch % 20 == 0 and epoch > 0:
            config.lr *= 0.8
            print(f"Reduced learning rate to {config.lr}")
    
    return best_acc

def evaluate_dataset(trainer, dataset, config):
    correct = 0
    for episode in dataset:
        metrics = trainer.evaluate(episode.seq, config)
        if metrics.winner_id == episode.target_id:
            correct += 1
    return correct / len(dataset)
```

### Curriculum Learning

```python
def curriculum_train(net, easy_data, medium_data, hard_data, config):
    trainer = glia.Trainer(net, config)
    
    # Stage 1: Easy data
    print("Stage 1: Easy data")
    trainer.train_epoch(easy_data, epochs=30, config=config)
    
    # Stage 2: Medium data
    print("Stage 2: Medium data")
    config.lr *= 0.5  # Reduce learning rate
    trainer.train_epoch(medium_data, epochs=40, config=config)
    
    # Stage 3: Hard data
    print("Stage 3: Hard data")
    config.lr *= 0.5
    trainer.train_epoch(hard_data, epochs=50, config=config)
    
    # Stage 4: Mixed data
    print("Stage 4: All data")
    all_data = glia.Dataset(easy_data.episodes + medium_data.episodes + hard_data.episodes)
    all_data.shuffle(seed=42)
    trainer.train_epoch(all_data, epochs=30, config=config)
    
    return trainer
```

---

## Network Architecture Design

### Layered Networks

```python
def create_layered_network(n_input, n_hidden, n_output):
    """Create a feedforward layered network"""
    import numpy as np
    
    net = glia.Network(num_sensory=n_input, num_neurons=n_hidden + n_output)
    
    # Input → Hidden connections
    from_ids = []
    to_ids = []
    weights = []
    
    for i in range(n_input):
        for h in range(n_hidden):
            from_ids.append(f'S{i}')
            to_ids.append(f'N{h}')
            weights.append(np.random.randn() * 0.5)
    
    # Hidden → Output connections
    for h in range(n_hidden):
        for o in range(n_output):
            from_ids.append(f'N{h}')
            to_ids.append(f'N{n_hidden + o}')
            weights.append(np.random.randn() * 0.5)
    
    net.set_weights(from_ids, to_ids, np.array(weights))
    
    return net
```

### Sparse Networks

```python
def create_sparse_network(n_neurons, connection_prob=0.1):
    """Create a sparse random network"""
    import numpy as np
    
    net = glia.Network(num_sensory=0, num_neurons=n_neurons)
    
    # Generate sparse connections
    from_ids = []
    to_ids = []
    weights = []
    
    all_ids = net.neuron_ids
    
    for i, from_id in enumerate(all_ids):
        for j, to_id in enumerate(all_ids):
            if i != j and np.random.rand() < connection_prob:
                from_ids.append(from_id)
                to_ids.append(to_id)
                weights.append(np.random.randn())
    
    net.set_weights(from_ids, to_ids, np.array(weights))
    
    return net
```

### Recurrent Networks

```python
def add_recurrent_connections(net, recurrence_prob=0.2):
    """Add recurrent connections to existing network"""
    import numpy as np
    
    internal_ids = [nid for nid in net.neuron_ids if nid not in net.sensory_ids]
    
    from_ids = []
    to_ids = []
    weights = []
    
    for nid in internal_ids:
        if np.random.rand() < recurrence_prob:
            # Self-connection
            from_ids.append(nid)
            to_ids.append(nid)
            weights.append(np.random.randn() * 0.3)
    
    # Get existing weights
    existing_from, existing_to, existing_weights = net.get_weights()
    
    # Combine
    all_from = existing_from + from_ids
    all_to = existing_to + to_ids
    all_weights = np.concatenate([existing_weights, np.array(weights)])
    
    net.set_weights(all_from, all_to, all_weights)
    
    return net
```

---

## Performance Optimization

### Batch Simulation

```python
def batch_simulate(networks, input_sequence, n_steps):
    """Simulate multiple networks in parallel"""
    results = []
    
    for net in networks:
        # Apply inputs
        for t, inputs in enumerate(input_sequence):
            net.inject_dict(inputs)
            net.step()
        
        # Collect results
        ids, values, _, _ = net.get_state()
        results.append({
            'final_state': values.copy(),
            'network': net
        })
    
    return results
```

### Memory-Efficient Training

```python
def memory_efficient_train(net, large_dataset, config, epochs):
    """Train on large dataset without loading all into memory"""
    trainer = glia.Trainer(net, config)
    
    for epoch in range(epochs):
        # Process in chunks
        chunk_size = 100
        for chunk_start in range(0, len(large_dataset), chunk_size):
            chunk = large_dataset[chunk_start:chunk_start+chunk_size]
            
            # Train on chunk
            for i in range(0, len(chunk), config.batch_size):
                batch = chunk[i:i+config.batch_size]
                trainer.train_batch(batch, config)
            
            # Clear intermediate results
            del chunk
        
        if epoch % 10 == 0:
            print(f"Epoch {epoch}: Accuracy {trainer.epoch_accuracy[-1]:.1%}")
    
    return trainer
```

### Optimized State Access

```python
def batch_state_collection(net, n_timesteps):
    """Efficiently collect state over time"""
    import numpy as np
    
    # Pre-allocate arrays
    ids, values, thresholds, leaks = net.get_state()
    n_neurons = len(ids)
    
    value_history = np.zeros((n_timesteps, n_neurons))
    
    # Collect without repeated allocations
    for t in range(n_timesteps):
        net.step()
        _, values, _, _ = net.get_state()
        value_history[t] = values
    
    return value_history
```

---

## Advanced NumPy Integration

### Sparse Matrix Operations

```python
from scipy.sparse import coo_matrix, csr_matrix
import numpy as np

def get_adjacency_matrix(net):
    """Convert network to sparse adjacency matrix"""
    from_ids, to_ids, weights = net.get_weights()
    
    # Create ID to index mapping
    all_ids = net.neuron_ids
    id_to_idx = {nid: i for i, nid in enumerate(all_ids)}
    
    # Convert to indices
    row = [id_to_idx[fid] for fid in from_ids]
    col = [id_to_idx[tid] for tid in to_ids]
    
    # Create sparse matrix
    n = len(all_ids)
    adj_matrix = coo_matrix((weights, (row, col)), shape=(n, n))
    
    return adj_matrix, all_ids

def analyze_network_structure(net):
    """Analyze network connectivity"""
    adj_matrix, ids = get_adjacency_matrix(net)
    adj_csr = adj_matrix.tocsr()
    
    # In-degree and out-degree
    in_degree = np.array(adj_csr.sum(axis=0)).flatten()
    out_degree = np.array(adj_csr.sum(axis=1)).flatten()
    
    # Weight statistics
    weights = adj_matrix.data
    
    return {
        'in_degree': dict(zip(ids, in_degree)),
        'out_degree': dict(zip(ids, out_degree)),
        'weight_mean': weights.mean(),
        'weight_std': weights.std(),
        'sparsity': 1 - (len(weights) / (len(ids) ** 2))
    }
```

### Vectorized Parameter Updates

```python
def apply_weight_regularization(net, l1_lambda=0.01, l2_lambda=0.001):
    """Apply L1 and L2 regularization to weights"""
    from_ids, to_ids, weights = net.get_weights()
    
    # L1: Soft thresholding
    weights = np.sign(weights) * np.maximum(0, np.abs(weights) - l1_lambda)
    
    # L2: Scaling
    weights *= (1 - l2_lambda)
    
    # Remove near-zero weights
    mask = np.abs(weights) > 1e-6
    net.set_weights(
        [f for f, m in zip(from_ids, mask) if m],
        [t for t, m in zip(to_ids, mask) if m],
        weights[mask]
    )

def normalize_weights(net, method='l2'):
    """Normalize network weights"""
    from_ids, to_ids, weights = net.get_weights()
    
    if method == 'l1':
        weights = weights / (np.abs(weights).sum() + 1e-8)
    elif method == 'l2':
        weights = weights / (np.sqrt((weights ** 2).sum()) + 1e-8)
    elif method == 'max':
        weights = weights / (np.abs(weights).max() + 1e-8)
    
    net.set_weights(from_ids, to_ids, weights)
```

---

## Custom Metrics and Callbacks

### Custom Evaluation Metrics

```python
def compute_custom_metrics(trainer, dataset, config):
    """Compute detailed evaluation metrics"""
    import numpy as np
    
    metrics = {
        'accuracy': 0,
        'margin_mean': 0,
        'margin_std': 0,
        'firing_rate_mean': 0,
        'confusion_matrix': {}
    }
    
    margins = []
    firing_rates = []
    
    for episode in dataset:
        result = trainer.evaluate(episode.seq, config)
        
        # Accuracy
        if result.winner_id == episode.target_id:
            metrics['accuracy'] += 1
        
        # Confusion matrix
        key = (episode.target_id, result.winner_id)
        metrics['confusion_matrix'][key] = metrics['confusion_matrix'].get(key, 0) + 1
        
        # Margins
        margins.append(result.margin)
        
        # Firing rates
        total_rate = sum(result.firing_rates.values())
        firing_rates.append(total_rate)
    
    metrics['accuracy'] /= len(dataset)
    metrics['margin_mean'] = np.mean(margins)
    metrics['margin_std'] = np.std(margins)
    metrics['firing_rate_mean'] = np.mean(firing_rates)
    
    return metrics
```

### Training Callbacks

```python
class TrainingMonitor:
    """Monitor training progress with callbacks"""
    
    def __init__(self, checkpoint_dir='checkpoints'):
        self.checkpoint_dir = checkpoint_dir
        self.best_acc = 0.0
        self.history = []
    
    def on_epoch_end(self, epoch, net, trainer, val_data, config):
        """Called after each epoch"""
        # Evaluate
        val_metrics = compute_custom_metrics(trainer, val_data, config)
        
        # Log
        self.history.append({
            'epoch': epoch,
            'train_acc': trainer.epoch_accuracy[-1],
            'val_acc': val_metrics['accuracy'],
            'margin': val_metrics['margin_mean'],
            'firing_rate': val_metrics['firing_rate_mean']
        })
        
        # Checkpoint if improved
        if val_metrics['accuracy'] > self.best_acc:
            self.best_acc = val_metrics['accuracy']
            net.save(f'{self.checkpoint_dir}/best_epoch_{epoch}.net')
            print(f"✓ New best: {self.best_acc:.1%} (epoch {epoch})")
    
    def plot_history(self, output_path='training_monitor.png'):
        """Plot training history"""
        import matplotlib.pyplot as plt
        
        epochs = [h['epoch'] for h in self.history]
        train_acc = [h['train_acc'] for h in self.history]
        val_acc = [h['val_acc'] for h in self.history]
        
        plt.figure(figsize=(10, 4))
        plt.plot(epochs, train_acc, label='Train')
        plt.plot(epochs, val_acc, label='Validation')
        plt.xlabel('Epoch')
        plt.ylabel('Accuracy')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.savefig(output_path)

# Usage
monitor = TrainingMonitor()

for epoch in range(100):
    trainer.train_epoch(train_data, epochs=1, config=config)
    monitor.on_epoch_end(epoch, net, trainer, val_data, config)

monitor.plot_history()
```

---

## Multi-Task Learning

```python
def create_multitask_dataset(task_configs):
    """Create dataset for multiple tasks"""
    all_episodes = []
    
    for task_name, task_data in task_configs.items():
        for i, (inputs, target) in enumerate(task_data):
            ep = glia.EpisodeData()
            ep.seq = create_sequence_from_inputs(inputs)
            ep.target_id = f"{task_name}_{target}"
            all_episodes.append(ep)
    
    dataset = glia.Dataset(all_episodes)
    return dataset

def train_multitask(net, task_datasets, config, epochs):
    """Train on multiple tasks simultaneously"""
    trainer = glia.Trainer(net, config)
    
    for epoch in range(epochs):
        # Sample from each task
        for task_name, task_data in task_datasets.items():
            # Train on task batch
            batch = sample_batch(task_data, config.batch_size)
            trainer.train_batch(batch, config)
        
        if epoch % 10 == 0:
            # Evaluate on each task
            for task_name, task_data in task_datasets.items():
                acc = evaluate_dataset(trainer, task_data, config)
                print(f"Epoch {epoch}, {task_name}: {acc:.1%}")
    
    return trainer
```

---

## Hyperparameter Tuning

### Grid Search

```python
def grid_search(net_template, dataset, param_grid):
    """Search hyperparameter space"""
    best_acc = 0
    best_params = None
    results = []
    
    train_data, val_data = dataset.split(0.8, seed=42)
    
    for lr in param_grid['lr']:
        for batch_size in param_grid['batch_size']:
            for weight_decay in param_grid['weight_decay']:
                # Create fresh network
                net = glia.Network.from_file(net_template)
                
                # Configure
                config = glia.create_config(
                    lr=lr,
                    batch_size=batch_size,
                    weight_decay=weight_decay
                )
                
                # Train
                trainer = glia.Trainer(net, config)
                trainer.train_epoch(train_data, epochs=30, config=config)
                
                # Evaluate
                val_acc = evaluate_dataset(trainer, val_data, config)
                
                results.append({
                    'lr': lr,
                    'batch_size': batch_size,
                    'weight_decay': weight_decay,
                    'val_acc': val_acc
                })
                
                if val_acc > best_acc:
                    best_acc = val_acc
                    best_params = (lr, batch_size, weight_decay)
                    print(f"New best: {best_acc:.1%} with lr={lr}, batch={batch_size}, decay={weight_decay}")
    
    return best_params, results

# Usage
param_grid = {
    'lr': [0.001, 0.01, 0.1],
    'batch_size': [4, 8, 16, 32],
    'weight_decay': [0, 0.0001, 0.001]
}

best_params, results = grid_search('baseline.net', dataset, param_grid)
```

### Random Search

```python
import numpy as np

def random_search(net_template, dataset, n_trials=50):
    """Random hyperparameter search"""
    best_acc = 0
    best_params = None
    results = []
    
    train_data, val_data = dataset.split(0.8, seed=42)
    
    for trial in range(n_trials):
        # Sample random parameters
        lr = 10 ** np.random.uniform(-4, -1)
        batch_size = np.random.choice([4, 8, 16, 32])
        weight_decay = 10 ** np.random.uniform(-6, -2)
        warmup_ticks = np.random.choice([5, 10, 20])
        eval_ticks = np.random.choice([30, 50, 100])
        
        # Train and evaluate
        net = glia.Network.from_file(net_template)
        config = glia.create_config(
            lr=lr,
            batch_size=batch_size,
            weight_decay=weight_decay,
            warmup_ticks=warmup_ticks,
            eval_ticks=eval_ticks
        )
        
        trainer = glia.Trainer(net, config)
        trainer.train_epoch(train_data, epochs=30, config=config)
        val_acc = evaluate_dataset(trainer, val_data, config)
        
        results.append({
            'trial': trial,
            'params': config,
            'val_acc': val_acc
        })
        
        if val_acc > best_acc:
            best_acc = val_acc
            best_params = config
            print(f"Trial {trial}: New best {best_acc:.1%}")
    
    return best_params, results
```

---

## See Also

- [API Reference](API_REFERENCE.md) - Complete API documentation
- [Quickstart](QUICKSTART.md) - Getting started guide
- [Examples](../examples/python/README.md) - Code examples
