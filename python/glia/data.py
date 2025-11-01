"""
Data utilities for loading and creating datasets
"""
from typing import List, Dict, Tuple, Optional, Union
import numpy as np
from pathlib import Path
from . import _core


class Dataset:
    """
    Dataset container (PyTorch-like API)
    
    Example:
        >>> dataset = Dataset.from_sequences(sequences, targets)
        >>> print(f"Dataset size: {len(dataset)}")
        >>> episode = dataset[0]
    """
    
    def __init__(self, episodes: List[_core.EpisodeData]):
        """
        Create dataset from episodes
        
        Args:
            episodes: List of EpisodeData objects
        """
        self.episodes = episodes
    
    @classmethod
    def from_sequences(
        cls,
        sequences: List[_core.InputSequence],
        targets: List[str]
    ) -> 'Dataset':
        """
        Create dataset from sequences and target IDs
        
        Args:
            sequences: List of input sequences
            targets: List of target output neuron IDs
            
        Returns:
            Dataset
        """
        if len(sequences) != len(targets):
            raise ValueError(
                f"Sequences ({len(sequences)}) and targets ({len(targets)}) "
                "must have same length"
            )
        
        episodes = []
        for seq, target in zip(sequences, targets):
            ep = _core.EpisodeData()
            ep.seq = seq
            ep.target_id = target
            episodes.append(ep)
        
        return cls(episodes)
    
    @classmethod
    def from_files(
        cls,
        seq_files: List[str],
        targets: List[str]
    ) -> 'Dataset':
        """
        Load dataset from .seq files
        
        Args:
            seq_files: List of paths to .seq files
            targets: List of target output neuron IDs
            
        Returns:
            Dataset
        """
        sequences = []
        for filepath in seq_files:
            seq = _core.InputSequence()
            seq.load_from_file(filepath)
            sequences.append(seq)
        
        return cls.from_sequences(sequences, targets)
    
    def split(
        self,
        train_fraction: float = 0.8,
        shuffle: bool = True,
        seed: Optional[int] = None
    ) -> Tuple['Dataset', 'Dataset']:
        """
        Split dataset into train and validation sets
        
        Args:
            train_fraction: Fraction for training (0-1)
            shuffle: Shuffle before splitting
            seed: Random seed for shuffling
            
        Returns:
            Tuple of (train_dataset, val_dataset)
        """
        episodes = self.episodes.copy()
        
        if shuffle:
            rng = np.random.RandomState(seed)
            rng.shuffle(episodes)
        
        split_idx = int(len(episodes) * train_fraction)
        train_episodes = episodes[:split_idx]
        val_episodes = episodes[split_idx:]
        
        return Dataset(train_episodes), Dataset(val_episodes)
    
    def shuffle(self, seed: Optional[int] = None) -> 'Dataset':
        """
        Return shuffled copy of dataset
        
        Args:
            seed: Random seed
            
        Returns:
            New shuffled dataset
        """
        episodes = self.episodes.copy()
        rng = np.random.RandomState(seed)
        rng.shuffle(episodes)
        return Dataset(episodes)
    
    def __len__(self) -> int:
        return len(self.episodes)
    
    def __getitem__(self, idx: Union[int, slice]) -> Union[_core.EpisodeData, 'Dataset']:
        if isinstance(idx, slice):
            return Dataset(self.episodes[idx])
        return self.episodes[idx]
    
    def __repr__(self) -> str:
        return f"<Dataset size={len(self)}>"


def load_sequence_file(filepath: str) -> _core.InputSequence:
    """
    Load a single .seq file
    
    Args:
        filepath: Path to .seq file
        
    Returns:
        InputSequence
    """
    seq = _core.InputSequence()
    seq.load_from_file(filepath)
    return seq


def create_sequence_from_array(
    inputs: np.ndarray,
    neuron_ids: List[str]
) -> _core.InputSequence:
    """
    Create input sequence from NumPy array
    
    Args:
        inputs: Array of shape (timesteps, num_inputs)
        neuron_ids: List of sensory neuron IDs
        
    Returns:
        InputSequence
    """
    if inputs.shape[1] != len(neuron_ids):
        raise ValueError(
            f"Input array has {inputs.shape[1]} features but "
            f"{len(neuron_ids)} neuron IDs provided"
        )
    
    seq = _core.InputSequence()
    
    for t in range(inputs.shape[0]):
        timestep_inputs = {
            neuron_id: float(inputs[t, i])
            for i, neuron_id in enumerate(neuron_ids)
        }
        seq.add_timestep(timestep_inputs)
    
    return seq


def load_dataset_from_directory(
    directory: str,
    pattern: str = "*.seq",
    target_mapping: Optional[Dict[str, str]] = None
) -> Dataset:
    """
    Load dataset from a directory of .seq files
    
    Args:
        directory: Directory containing .seq files
        pattern: Glob pattern for files (default: *.seq)
        target_mapping: Dictionary mapping filename -> target_id
                       If None, attempts to infer from filename
        
    Returns:
        Dataset
        
    Example:
        >>> # Files: class0_001.seq, class0_002.seq, class1_001.seq, ...
        >>> dataset = load_dataset_from_directory(
        ...     "data/train",
        ...     target_mapping=lambda f: "O0" if "class0" in f else "O1"
        ... )
    """
    dir_path = Path(directory)
    seq_files = sorted(dir_path.glob(pattern))
    
    if not seq_files:
        raise ValueError(f"No files matching '{pattern}' found in {directory}")
    
    sequences = []
    targets = []
    
    for filepath in seq_files:
        # Load sequence
        seq = _core.InputSequence()
        seq.load_from_file(str(filepath))
        sequences.append(seq)
        
        # Determine target
        if target_mapping:
            if callable(target_mapping):
                target = target_mapping(filepath.name)
            else:
                target = target_mapping.get(filepath.name)
                if target is None:
                    raise ValueError(f"No target mapping for {filepath.name}")
        else:
            # Try to infer from filename (e.g., "class0_001.seq" -> "O0")
            name = filepath.stem
            if '_' in name:
                class_name = name.split('_')[0]
                # Extract digit from class name
                import re
                match = re.search(r'\d+', class_name)
                if match:
                    target = f"O{match.group()}"
                else:
                    raise ValueError(
                        f"Could not infer target from filename: {filepath.name}. "
                        "Provide target_mapping."
                    )
            else:
                raise ValueError(
                    f"Could not infer target from filename: {filepath.name}. "
                    "Provide target_mapping."
                )
        
        targets.append(target)
    
    return Dataset.from_sequences(sequences, targets)


def create_config(
    lr: float = 0.01,
    batch_size: int = 1,
    warmup_ticks: int = 50,
    decision_window: int = 50,
    **kwargs
) -> _core.TrainingConfig:
    """
    Create training configuration with sensible defaults
    
    Args:
        lr: Learning rate
        batch_size: Batch size
        warmup_ticks: Warmup timesteps before decision window
        decision_window: Window for output detection
        **kwargs: Additional config parameters
        
    Returns:
        TrainingConfig
    """
    config = _core.TrainingConfig()
    config.lr = lr
    config.batch_size = batch_size
    config.warmup_ticks = warmup_ticks
    config.decision_window = decision_window
    
    # Apply additional kwargs
    for key, value in kwargs.items():
        if hasattr(config, key):
            setattr(config, key, value)
        else:
            print(f"Warning: Unknown config parameter '{key}'")
    
    return config


def create_evo_config(
    population: int = 10,
    generations: int = 20,
    elite: int = 2,
    train_epochs: int = 5,
    **kwargs
) -> _core.EvolutionConfig:
    """
    Create evolution configuration with sensible defaults
    
    Args:
        population: Population size
        generations: Number of generations
        elite: Elite count (preserved each generation)
        train_epochs: Training epochs per individual
        **kwargs: Additional config parameters
        
    Returns:
        EvolutionConfig
    """
    config = _core.EvolutionConfig()
    config.population = population
    config.generations = generations
    config.elite = elite
    config.train_epochs = train_epochs
    
    # Apply additional kwargs
    for key, value in kwargs.items():
        if hasattr(config, key):
            setattr(config, key, value)
        else:
            print(f"Warning: Unknown evo config parameter '{key}'")
    
    return config
