"""
High-level Trainer wrapper with Python callbacks
"""
from typing import List, Optional, Callable, Dict, Any
import numpy as np
from . import _core
from .network import Network


class Trainer:
    """
    Neural network trainer with Python callback support
    
    Uses gradient-based learning (RateGDTrainer) by default for supervised learning.
    This wrapper adds:
    - Epoch-level Python callbacks during training
    - Better history tracking
    - Convenience methods
    
    Example:
        >>> trainer = Trainer(network)
        >>> history = trainer.train(
        ...     dataset, 
        ...     epochs=100,
        ...     on_epoch=lambda e, acc, margin: print(f"Epoch {e}: {acc:.2%}")
        ... )
    """
    
    def __init__(self, network: Network, config: Optional[_core.TrainingConfig] = None, use_gradient: bool = True):
        """
        Create trainer
        
        Args:
            network: Network to train
            config: Training configuration (uses defaults if None)
            use_gradient: If True, uses gradient-based trainer (recommended for supervised learning).
                         If False, uses Hebbian/reinforcement learning trainer.
        """
        self._network = network
        # Use gradient trainer by default (was causing the bug!)
        if use_gradient:
            self._trainer = _core.RateGDTrainer(network._cpp)
        else:
            self._trainer = _core.Trainer(network._cpp)
        self._config = config or _core.TrainingConfig()
        self._history: Dict[str, List[float]] = {
            'accuracy': [],
            'margin': [],
            'loss': []
        }
    
    def set_seed(self, seed: int) -> None:
        """Set random seed for reproducibility"""
        self._trainer.reseed(seed)
    
    def evaluate(
        self, 
        sequence: _core.InputSequence,
        config: Optional[_core.TrainingConfig] = None
    ) -> _core.EpisodeMetrics:
        """
        Evaluate network on a single episode
        
        Args:
            sequence: Input sequence to evaluate
            config: Training config (uses self._config if None)
            
        Returns:
            Episode metrics
        """
        cfg = config or self._config
        return self._trainer.evaluate(sequence, cfg)
    
    def train_batch(
        self,
        batch: List[_core.EpisodeData],
        config: Optional[_core.TrainingConfig] = None
    ) -> List[_core.EpisodeMetrics]:
        """
        Train on a single batch
        
        Args:
            batch: List of episodes
            config: Training config (uses self._config if None)
            
        Returns:
            List of episode metrics
        """
        cfg = config or self._config
        metrics_out = []
        self._trainer.train_batch(batch, cfg, metrics_out)
        return metrics_out
    
    def train(
        self,
        dataset: List[_core.EpisodeData],
        epochs: int = 10,
        config: Optional[_core.TrainingConfig] = None,
        on_epoch: Optional[Callable[[int, float, float], None]] = None,
        verbose: bool = True,
        lr_schedule: Optional[str] = 'cosine'  # 'cosine', 'step', or None
    ) -> Dict[str, List[float]]:
        """
        Train for multiple epochs with Python callback support and LR scheduling
        
        This method manually implements the training loop to support
        Python callbacks, unlike train_epoch_fast() which releases the GIL.
        
        Args:
            dataset: Training episodes
            epochs: Number of epochs
            config: Training configuration
            on_epoch: Callback(epoch, accuracy, margin) called after each epoch
            verbose: Print progress with progress bar (per-epoch progress)
            lr_schedule: Learning rate schedule ('cosine', 'step', or None)
                        'cosine': Smooth cosine annealing from initial LR to 0.01x
                        'step': Decay by 0.5 every 1/3 of epochs
            
        Returns:
            Training history dictionary
        """
        cfg = config or self._config
        initial_lr = cfg.lr
        
        # Learning rate schedule function
        def get_lr(epoch: int) -> float:
            if lr_schedule == 'cosine':
                # Cosine annealing: smooth decay from initial_lr to initial_lr * 0.01
                import math
                min_lr = initial_lr * 0.01
                return min_lr + (initial_lr - min_lr) * 0.5 * (1 + math.cos(math.pi * epoch / epochs))
            elif lr_schedule == 'step':
                # Step decay: reduce by 0.5 every 1/3 of epochs
                decay_epochs = max(1, epochs // 3)
                factor = 0.5 ** (epoch // decay_epochs)
                return initial_lr * factor
            else:
                return initial_lr
        
        for epoch in range(epochs):
            # Update learning rate
            cfg.lr = get_lr(epoch)
            
            # Progress tracking for this epoch
            num_batches = (len(dataset) + cfg.batch_size - 1) // cfg.batch_size
            
            if verbose:
                # Start-of-epoch message
                epoch_width = len(str(epochs))
                print(f"Epoch {epoch+1:>{epoch_width}}/{epochs} [LR={cfg.lr:.6f}]", end='')
            
            # Train one epoch (GIL released in C++)
            self._trainer.train_epoch(dataset, 1, cfg)
            
            # Get metrics
            acc_hist = self._trainer.get_epoch_acc_history()
            margin_hist = self._trainer.get_epoch_margin_history()
            
            if acc_hist:
                accuracy = acc_hist[-1]
                margin = margin_hist[-1] if margin_hist else 0.0
                
                # Update history
                self._history['accuracy'].append(accuracy)
                self._history['margin'].append(margin)
                
                # End-of-epoch status
                if verbose:
                    print(f"  →  Acc: {accuracy:>6.2%}, Margin: {margin:.3f}")
                
                # Custom callback
                if on_epoch:
                    on_epoch(epoch, accuracy, margin)
        
        # Restore original learning rate
        cfg.lr = initial_lr
        
        return self._history
    
    def train_fast(
        self,
        dataset: List[_core.EpisodeData],
        epochs: int = 10,
        config: Optional[_core.TrainingConfig] = None
    ) -> Dict[str, List[float]]:
        """
        Fast training without callbacks (full GIL release)
        
        This is faster than train() because it releases the GIL for the
        entire training run, but you can't monitor progress with Python callbacks.
        
        Args:
            dataset: Training episodes
            epochs: Number of epochs  
            config: Training configuration
            
        Returns:
            Training history dictionary
        """
        cfg = config or self._config
        
        # C++ train_epoch releases GIL for entire duration
        self._trainer.train_epoch(dataset, epochs, cfg)
        
        # Get final history
        acc_hist = self._trainer.get_epoch_acc_history()
        margin_hist = self._trainer.get_epoch_margin_history()
        
        self._history['accuracy'] = list(acc_hist)
        self._history['margin'] = list(margin_hist)
        
        return self._history
    
    def evaluate_dataset(
        self,
        dataset: List[_core.EpisodeData],
        config: Optional[_core.TrainingConfig] = None
    ) -> Dict[str, Any]:
        """
        Evaluate network on entire dataset
        
        Args:
            dataset: Evaluation episodes
            config: Training config
            
        Returns:
            Dictionary with accuracy, margin, and per-episode metrics
        """
        cfg = config or self._config
        
        metrics_list = []
        correct = 0
        total_margin = 0.0
        
        for episode in dataset:
            metrics = self._trainer.evaluate(episode.seq, cfg)
            metrics_list.append(metrics)
            
            if metrics.winner_id == episode.target_id:
                correct += 1
            total_margin += metrics.margin
        
        accuracy = correct / len(dataset) if dataset else 0.0
        avg_margin = total_margin / len(dataset) if dataset else 0.0
        
        return {
            'accuracy': accuracy,
            'margin': avg_margin,
            'episodes': metrics_list,
            'correct': correct,
            'total': len(dataset)
        }
    
    def revert_checkpoint(self) -> bool:
        """
        Revert to last checkpoint (if checkpointing enabled in config)
        
        Returns:
            True if revert succeeded
        """
        return self._trainer.revert_checkpoint()
    
    @property
    def history(self) -> Dict[str, List[float]]:
        """Get training history"""
        # Sync with C++ history
        acc_hist = self._trainer.get_epoch_acc_history()
        margin_hist = self._trainer.get_epoch_margin_history()
        
        if acc_hist:
            self._history['accuracy'] = list(acc_hist)
        if margin_hist:
            self._history['margin'] = list(margin_hist)
        
        return self._history
    
    @property
    def config(self) -> _core.TrainingConfig:
        """Get training configuration"""
        return self._config
    
    @config.setter
    def config(self, cfg: _core.TrainingConfig) -> None:
        """Set training configuration"""
        self._config = cfg
    
    def __repr__(self) -> str:
        return f"<Trainer network={self._network}>"
    
    # Access to underlying C++ object
    @property
    def _cpp(self) -> _core.Trainer:
        """Access underlying C++ Trainer object"""
        return self._trainer
