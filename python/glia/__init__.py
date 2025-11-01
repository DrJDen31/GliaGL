"""
GliaGL - Fast Spiking Neural Network Simulator

A Python package for simulating, training, and evolving spiking neural networks.

Example:
    >>> import glia
    >>> net = glia.Network.from_file("network.net")
    >>> net.step()
    >>> state = net.state  # Property access
    >>> 
    >>> trainer = glia.Trainer(net)
    >>> history = trainer.train(dataset, epochs=100)
"""

__version__ = "0.1.0"

# Import C++ core
try:
    from . import _core
except ImportError as e:
    raise ImportError(
        "Failed to import C++ core module. "
        "Make sure the package is properly installed with: pip install -e ."
    ) from e

# High-level Python wrappers (Pythonic API)
from .network import Network
from .trainer import Trainer
from .evolution import Evolution, plot_evolution_result
from .data import (
    Dataset,
    load_sequence_file,
    create_sequence_from_array,
    load_dataset_from_directory,
    create_config,
    create_evo_config,
)

# Re-export C++ types that don't have wrappers
from ._core import (
    Neuron,
    InputSequence,
    TrainingConfig,
    EpisodeData,
    EpisodeMetrics,
    EvolutionConfig,
    EvoMetrics,
    NetworkSnapshot,
    EvolutionResult,
    EdgeRecord,
    NeuronRecord,
)

# Visualization (optional - only if dependencies installed)
try:
    from . import viz
    _HAS_VIZ = True
except ImportError:
    _HAS_VIZ = False
    viz = None

__all__ = [
    "__version__",
    # High-level API (Python wrappers)
    "Network",
    "Trainer",
    "Evolution",
    "Dataset",
    # Utility functions
    "load_sequence_file",
    "create_sequence_from_array",
    "load_dataset_from_directory",
    "create_config",
    "create_evo_config",
    "plot_evolution_result",
    # C++ types (direct access)
    "Neuron",
    "InputSequence",
    "TrainingConfig",
    "EpisodeData",
    "EpisodeMetrics",
    "EvolutionConfig",
    "EvoMetrics",
    "NetworkSnapshot",
    "EvolutionResult",
    "EdgeRecord",
    "NeuronRecord",
    # Visualization (if available)
    "viz",
]


def info():
    """Print GliaGL package information"""
    print(f"GliaGL version {__version__}")
    print(f"C++ core available: {_core is not None}")
    print(f"Visualization available: {_HAS_VIZ}")
    
    if _HAS_VIZ:
        print("  - matplotlib: ✓")
        print("  - networkx: ✓")
    else:
        print("  Install viz dependencies: pip install matplotlib networkx")

