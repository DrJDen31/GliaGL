"""
High-level Network wrapper with convenience methods
"""
from typing import List, Tuple, Optional, Dict
import numpy as np
from . import _core


class Network:
    """
    Spiking neural network with Pythonic interface
    
    This is a thin wrapper around the C++ Network class that provides
    convenience methods and better integration with NumPy/Python.
    
    Example:
        >>> net = Network.from_file("network.net")
        >>> net.inject_dict({"S0": 100.0, "S1": 50.0})
        >>> net.step()
        >>> state = net.state  # Property access
    """
    
    def __init__(self, num_sensory: int = 0, num_neurons: int = 0):
        """
        Create a new network
        
        Args:
            num_sensory: Number of sensory input neurons
            num_neurons: Number of internal neurons
        """
        if num_sensory == 0 and num_neurons == 0:
            self._net = _core.Network()
        else:
            self._net = _core.Network(num_sensory, num_neurons)
    
    @classmethod
    def from_file(cls, filepath: str, verbose: bool = True) -> 'Network':
        """
        Load network from file
        
        Args:
            filepath: Path to .net file
            verbose: Print loading information
            
        Returns:
            Loaded network
        """
        net = cls()
        net._net.load(filepath, verbose)
        return net
    
    def save(self, filepath: str) -> None:
        """Save network to file"""
        self._net.save(filepath)
    
    def step(self, n_steps: int = 1) -> None:
        """
        Run simulation steps
        
        Args:
            n_steps: Number of timesteps to simulate
        """
        for _ in range(n_steps):
            self._net.step()
    
    def inject(self, neuron_id: str, amount: float) -> None:
        """Inject current into a sensory neuron"""
        self._net.inject(neuron_id, amount)
    
    def inject_dict(self, inputs: Dict[str, float]) -> None:
        """
        Inject currents from a dictionary
        
        Args:
            inputs: Dictionary mapping neuron IDs to current values
            
        Example:
            >>> net.inject_dict({"S0": 100.0, "S1": 50.0})
        """
        for neuron_id, amount in inputs.items():
            self._net.inject(neuron_id, amount)
    
    def inject_array(self, values: np.ndarray) -> None:
        """
        Inject currents from NumPy array to sensory neurons in order
        
        Args:
            values: Array of current values (one per sensory neuron)
        """
        sensory_ids = self.sensory_ids
        if len(values) != len(sensory_ids):
            raise ValueError(
                f"Array length {len(values)} doesn't match sensory neuron count {len(sensory_ids)}"
            )
        for neuron_id, value in zip(sensory_ids, values):
            self._net.inject(neuron_id, float(value))
    
    def reset(self) -> None:
        """Reset network to initial state (reload from last load/save)"""
        # For now, just set all values to resting
        ids, _, thresholds, leaks = self.get_state()
        self._net.set_state(ids, thresholds, leaks)
    
    # ========== State Access ==========
    
    def get_state(self) -> Tuple[List[str], np.ndarray, np.ndarray, np.ndarray]:
        """
        Get complete network state
        
        Returns:
            Tuple of (neuron_ids, values, thresholds, leaks)
            where values/thresholds/leaks are NumPy arrays
        """
        return self._net.get_state()
    
    def set_state(
        self, 
        neuron_ids: List[str], 
        thresholds: np.ndarray, 
        leaks: np.ndarray
    ) -> None:
        """Set neuron parameters"""
        self._net.set_state(neuron_ids, thresholds, leaks)
    
    def get_weights(self) -> Tuple[List[str], List[str], np.ndarray]:
        """
        Get all synaptic weights as edge list (COO sparse format)
        
        Returns:
            Tuple of (from_ids, to_ids, weights) where weights is a NumPy array
        """
        return self._net.get_weights()
    
    def set_weights(
        self, 
        from_ids: List[str], 
        to_ids: List[str], 
        weights: np.ndarray
    ) -> None:
        """Set synaptic weights from edge list"""
        self._net.set_weights(from_ids, to_ids, weights)
    
    # ========== Properties ==========
    
    @property
    def state(self) -> Dict[str, np.ndarray]:
        """
        Get state as dictionary of NumPy arrays
        
        Returns:
            Dictionary with keys: 'ids', 'values', 'thresholds', 'leaks'
        """
        ids, values, thresholds, leaks = self.get_state()
        return {
            'ids': ids,
            'values': values,
            'thresholds': thresholds,
            'leaks': leaks
        }
    
    @property
    def weights(self) -> Dict[str, any]:
        """
        Get weights as dictionary
        
        Returns:
            Dictionary with keys: 'from', 'to', 'values'
        """
        from_ids, to_ids, weights = self.get_weights()
        return {
            'from': from_ids,
            'to': to_ids,
            'values': weights
        }
    
    @property
    def sensory_ids(self) -> List[str]:
        """Get sensory neuron IDs"""
        return self._net.get_sensory_ids()
    
    @property
    def neuron_ids(self) -> List[str]:
        """Get all neuron IDs"""
        return self._net.get_all_neuron_ids()
    
    @property
    def num_neurons(self) -> int:
        """Get total neuron count"""
        return self._net.get_neuron_count()
    
    @property
    def num_connections(self) -> int:
        """Get total connection count"""
        return self._net.get_connection_count()
    
    def get_neuron(self, neuron_id: str) -> _core.Neuron:
        """Get neuron by ID"""
        return self._net.get_neuron(neuron_id)
    
    # ========== Analysis Methods ==========
    
    def get_firing_neurons(self) -> List[str]:
        """
        Get IDs of neurons that fired in the last timestep
        
        Returns:
            List of neuron IDs
        """
        firing = []
        for neuron_id in self.neuron_ids:
            neuron = self.get_neuron(neuron_id)
            if neuron and neuron.did_fire():
                firing.append(neuron_id)
        return firing
    
    def to_adjacency_matrix(self, dense: bool = False) -> np.ndarray:
        """
        Convert weight matrix to adjacency matrix
        
        Args:
            dense: If True, return dense matrix. Otherwise sparse COO.
            
        Returns:
            Adjacency matrix (dense or scipy.sparse if installed)
        """
        from_ids, to_ids, weights = self.get_weights()
        neuron_ids = self.neuron_ids
        id_to_idx = {nid: i for i, nid in enumerate(neuron_ids)}
        
        n = len(neuron_ids)
        
        if dense:
            # Dense matrix
            adj = np.zeros((n, n), dtype=np.float32)
            for from_id, to_id, weight in zip(from_ids, to_ids, weights):
                i = id_to_idx[from_id]
                j = id_to_idx[to_id]
                adj[i, j] = weight
            return adj
        else:
            # Return as COO format (compatible with scipy.sparse)
            try:
                import scipy.sparse as sp
                from_indices = [id_to_idx[fid] for fid in from_ids]
                to_indices = [id_to_idx[tid] for tid in to_ids]
                return sp.coo_matrix((weights, (from_indices, to_indices)), shape=(n, n))
            except ImportError:
                raise ImportError(
                    "scipy required for sparse matrices. Install with: pip install scipy\n"
                    "Or use dense=True for dense matrix"
                )
    
    def __repr__(self) -> str:
        return (
            f"<Network neurons={self.num_neurons} "
            f"connections={self.num_connections}>"
        )
    
    # Access to underlying C++ object for advanced use
    @property
    def _cpp(self) -> _core.Network:
        """Access underlying C++ Network object"""
        return self._net
