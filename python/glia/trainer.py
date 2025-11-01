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
    
    def __init__(self, network: Network, config: Optional[_core.TrainingConfig] = None):
        """
        Create trainer
        
        Args:
            network: Network to train
            config: Training configuration (uses defaults if None)
        """
        self._network = network
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
        verbose: bool = True
    ) -> Dict[str, List[float]]:
        """
        Train for multiple epochs with Python callback support
        
        This method manually implements the training loop to support
        Python callbacks, unlike train_epoch_fast() which releases the GIL.
        
        Args:
            dataset: Training episodes
            epochs: Number of epochs
            config: Training configuration
            on_epoch: Callback(epoch, accuracy, margin) called after each epoch
            verbose: Print progress
            
        Returns:
            Training history dictionary
        """
        cfg = config or self._config
        
        for epoch in range(epochs):
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
                
                # Callback
                if on_epoch:
                    on_epoch(epoch, accuracy, margin)
                
                # Print
                if verbose and (epoch % max(1, epochs // 10) == 0 or epoch == epochs - 1):
                    print(f"Epoch {epoch+1}/{epochs}: "
                          f"Acc={accuracy:.2%}, Margin={margin:.3f}")
        
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
