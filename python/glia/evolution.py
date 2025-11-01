"""
High-level Evolution wrapper with Python callbacks
"""
from typing import List, Optional, Callable, Dict, Any
import numpy as np
from . import _core


class Evolution:
    """
    Evolutionary trainer with Python callback support
    
    This wrapper adds:
    - Generation-level Python callbacks
    - Better result tracking
    - Convenience methods
    
    Example:
        >>> evo = Evolution(
        ...     network_path="baseline.net",
        ...     train_data=train_episodes,
        ...     val_data=val_episodes,
        ...     config=evo_config
        ... )
        >>> result = evo.run(
        ...     on_generation=lambda gen, best: print(f"Gen {gen}: {best.fitness:.3f}")
        ... )
    """
    
    def __init__(
        self,
        network_path: str,
        train_data: List[_core.EpisodeData],
        val_data: List[_core.EpisodeData],
        train_config: _core.TrainingConfig,
        evo_config: _core.EvolutionConfig,
    ):
        """
        Create evolution engine
        
        Args:
            network_path: Path to base network file
            train_data: Training episodes for Lamarckian loop
            val_data: Validation episodes for fitness evaluation
            train_config: Configuration for inner training
            evo_config: Evolution parameters
        """
        self._network_path = network_path
        self._train_data = train_data
        self._val_data = val_data
        self._train_config = train_config
        self._evo_config = evo_config
        
        # Create C++ engine
        callbacks = _core.EvolutionCallbacks()
        self._engine = _core.EvolutionEngine(
            network_path,
            train_data,
            val_data,
            train_config,
            evo_config,
            callbacks
        )
    
    def run(
        self,
        on_generation: Optional[Callable[[int, _core.NetworkSnapshot, _core.EvoMetrics], None]] = None,
        verbose: bool = True
    ) -> _core.EvolutionResult:
        """
        Run evolutionary training
        
        Note: This currently runs without Python callbacks to get full GIL release.
        For callback support, we would need to modify the C++ side or implement
        a manual evolution loop in Python.
        
        Args:
            on_generation: Callback(generation, best_genome, metrics) 
            verbose: Print progress
            
        Returns:
            Evolution result with best genome and history
        """
        if on_generation is not None:
            print("Warning: Python callbacks during evolution not yet supported.")
            print("Evolution will run with full GIL release (faster) but no callbacks.")
        
        # Run evolution (full GIL release)
        result = self._engine.run()
        
        # Print summary if verbose
        if verbose:
            print(f"\nEvolution complete!")
            print(f"Best fitness: {result.best_fitness_hist[-1]:.4f}")
            print(f"Best accuracy: {result.best_acc_hist[-1]:.2%}")
            print(f"Best margin: {result.best_margin_hist[-1]:.3f}")
            print(f"Generations: {len(result.best_fitness_hist)}")
        
        return result
    
    @staticmethod
    def load_best_genome(
        network_path: str,
        result: _core.EvolutionResult
    ) -> 'Network':
        """
        Load the best genome from evolution result into a network
        
        Args:
            network_path: Base network file
            result: Evolution result
            
        Returns:
            Network with best genome loaded
        """
        from .network import Network
        
        # Load base network
        net = Network.from_file(network_path, verbose=False)
        
        # Apply best genome
        best = result.best_genome
        
        # Set neuron parameters
        ids = [n.id for n in best.neurons]
        thresholds = np.array([n.thr for n in best.neurons], dtype=np.float32)
        leaks = np.array([n.leak for n in best.neurons], dtype=np.float32)
        net.set_state(ids, thresholds, leaks)
        
        # Set weights
        # Note: accessing edge attributes - 'from' is a Python keyword so use getattr
        from_ids = [getattr(e, 'from') for e in best.edges]
        to_ids = [e.to for e in best.edges]
        weights = np.array([e.w for e in best.edges], dtype=np.float32)
        net.set_weights(from_ids, to_ids, weights)
        
        return net
    
    def plot_history(self, show: bool = True, save_path: Optional[str] = None):
        """
        Plot evolution history (requires matplotlib)
        
        Args:
            show: Show the plot
            save_path: Optional path to save figure
        """
        try:
            import matplotlib.pyplot as plt
        except ImportError:
            raise ImportError(
                "matplotlib required for plotting. Install with: pip install matplotlib"
            )
        
        # Run if not already run
        # (This would need state tracking - simplified for now)
        print("Run evolution first, then access result.best_fitness_hist, etc.")
        print("Plotting from result object coming in future update.")
    
    @property
    def config(self) -> _core.EvolutionConfig:
        """Get evolution configuration"""
        return self._evo_config
    
    def __repr__(self) -> str:
        return (
            f"<Evolution pop={self._evo_config.population} "
            f"gens={self._evo_config.generations}>"
        )
    
    # Access to underlying C++ object
    @property
    def _cpp(self) -> _core.EvolutionEngine:
        """Access underlying C++ EvolutionEngine object"""
        return self._engine


def plot_evolution_result(result: _core.EvolutionResult, save_path: Optional[str] = None):
    """
    Plot evolution result history
    
    Args:
        result: Evolution result from Evolution.run()
        save_path: Optional path to save figure
    """
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        raise ImportError(
            "matplotlib required for plotting. Install with: pip install matplotlib"
        )
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 4))
    
    # Fitness
    axes[0].plot(result.best_fitness_hist, 'b-', linewidth=2)
    axes[0].set_xlabel('Generation')
    axes[0].set_ylabel('Fitness')
    axes[0].set_title('Best Fitness Over Generations')
    axes[0].grid(True, alpha=0.3)
    
    # Accuracy
    axes[1].plot(result.best_acc_hist, 'g-', linewidth=2)
    axes[1].set_xlabel('Generation')
    axes[1].set_ylabel('Accuracy')
    axes[1].set_title('Best Accuracy Over Generations')
    axes[1].set_ylim([0, 1])
    axes[1].grid(True, alpha=0.3)
    
    # Margin
    axes[2].plot(result.best_margin_hist, 'r-', linewidth=2)
    axes[2].set_xlabel('Generation')
    axes[2].set_ylabel('Margin')
    axes[2].set_title('Best Margin Over Generations')
    axes[2].grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved to {save_path}")
    
    plt.show()
