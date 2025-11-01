"""
Visualization utilities for networks and training
"""
from typing import Optional, List, Tuple
import numpy as np
from .network import Network


def plot_network_graph(
    network: Network,
    layout: str = 'spring',
    node_size: int = 300,
    show_weights: bool = True,
    save_path: Optional[str] = None
):
    """
    Plot network as a graph (requires networkx and matplotlib)
    
    Args:
        network: Network to visualize
        layout: Layout algorithm ('spring', 'circular', 'shell', 'kamada_kawai')
        node_size: Size of nodes
        show_weights: Show edge weights as labels
        save_path: Optional path to save figure
    """
    try:
        import networkx as nx
        import matplotlib.pyplot as plt
    except ImportError:
        raise ImportError(
            "networkx and matplotlib required for visualization. "
            "Install with: pip install networkx matplotlib"
        )
    
    # Create directed graph
    G = nx.DiGraph()
    
    # Add nodes
    neuron_ids = network.neuron_ids
    for nid in neuron_ids:
        G.add_node(nid)
    
    # Add edges with weights
    from_ids, to_ids, weights = network.get_weights()
    edge_labels = {}
    for f, t, w in zip(from_ids, to_ids, weights):
        G.add_edge(f, t, weight=w)
        if show_weights:
            edge_labels[(f, t)] = f"{w:.2f}"
    
    # Create layout
    if layout == 'spring':
        pos = nx.spring_layout(G, k=1, iterations=50)
    elif layout == 'circular':
        pos = nx.circular_layout(G)
    elif layout == 'shell':
        # Group by neuron type
        sensory = [nid for nid in neuron_ids if nid.startswith('S')]
        hidden = [nid for nid in neuron_ids if nid.startswith('H') or nid.startswith('N')]
        output = [nid for nid in neuron_ids if nid.startswith('O')]
        shells = [s for s in [sensory, hidden, output] if s]
        pos = nx.shell_layout(G, nlist=shells)
    elif layout == 'kamada_kawai':
        pos = nx.kamada_kawai_layout(G)
    else:
        raise ValueError(f"Unknown layout: {layout}")
    
    # Color nodes by type
    node_colors = []
    for nid in G.nodes():
        if nid.startswith('S'):
            node_colors.append('lightblue')
        elif nid.startswith('H') or nid.startswith('N'):
            node_colors.append('lightgreen')
        elif nid.startswith('O'):
            node_colors.append('lightcoral')
        else:
            node_colors.append('gray')
    
    # Draw
    plt.figure(figsize=(12, 8))
    nx.draw(
        G, pos,
        node_color=node_colors,
        node_size=node_size,
        with_labels=True,
        font_size=10,
        font_weight='bold',
        arrows=True,
        arrowsize=20,
        edge_color='gray',
        alpha=0.7
    )
    
    if show_weights:
        nx.draw_networkx_edge_labels(G, pos, edge_labels, font_size=8)
    
    plt.title(f"Network Graph ({len(neuron_ids)} neurons, {len(from_ids)} connections)")
    plt.axis('off')
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved to {save_path}")
    
    plt.show()


def plot_weight_distribution(
    network: Network,
    bins: int = 50,
    save_path: Optional[str] = None
):
    """
    Plot distribution of synaptic weights
    
    Args:
        network: Network to analyze
        bins: Number of histogram bins
        save_path: Optional path to save figure
    """
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        raise ImportError("matplotlib required. Install with: pip install matplotlib")
    
    _, _, weights = network.get_weights()
    
    if len(weights) == 0:
        print("No connections in network")
        return
    
    fig, axes = plt.subplots(1, 2, figsize=(12, 4))
    
    # Histogram
    axes[0].hist(weights, bins=bins, alpha=0.7, edgecolor='black')
    axes[0].axvline(0, color='r', linestyle='--', linewidth=2, label='Zero')
    axes[0].set_xlabel('Weight')
    axes[0].set_ylabel('Count')
    axes[0].set_title('Weight Distribution')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)
    
    # Excitatory vs Inhibitory
    excitatory = (weights > 0).sum()
    inhibitory = (weights < 0).sum()
    zero = (weights == 0).sum()
    
    axes[1].bar(['Excitatory', 'Inhibitory', 'Zero'], 
                [excitatory, inhibitory, zero],
                color=['green', 'red', 'gray'],
                alpha=0.7,
                edgecolor='black')
    axes[1].set_ylabel('Count')
    axes[1].set_title('Connection Types')
    axes[1].grid(True, alpha=0.3, axis='y')
    
    # Add statistics text
    stats_text = f"Mean: {weights.mean():.3f}\n"
    stats_text += f"Std: {weights.std():.3f}\n"
    stats_text += f"Min: {weights.min():.3f}\n"
    stats_text += f"Max: {weights.max():.3f}"
    axes[0].text(0.02, 0.98, stats_text,
                 transform=axes[0].transAxes,
                 verticalalignment='top',
                 bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5),
                 fontfamily='monospace')
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved to {save_path}")
    
    plt.show()


def plot_training_history(
    history: dict,
    save_path: Optional[str] = None
):
    """
    Plot training history
    
    Args:
        history: Dictionary from Trainer.history
        save_path: Optional path to save figure
    """
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        raise ImportError("matplotlib required. Install with: pip install matplotlib")
    
    metrics = ['accuracy', 'margin']
    available = [m for m in metrics if m in history and history[m]]
    
    if not available:
        print("No training history to plot")
        return
    
    fig, axes = plt.subplots(1, len(available), figsize=(6*len(available), 4))
    if len(available) == 1:
        axes = [axes]
    
    for ax, metric in zip(axes, available):
        values = history[metric]
        ax.plot(values, linewidth=2)
        ax.set_xlabel('Epoch')
        ax.set_ylabel(metric.capitalize())
        ax.set_title(f'Training {metric.capitalize()}')
        ax.grid(True, alpha=0.3)
        
        # Add final value
        if values:
            final = values[-1]
            ax.axhline(final, color='r', linestyle='--', alpha=0.5)
            ax.text(0.02, 0.98, f'Final: {final:.4f}',
                   transform=ax.transAxes,
                   verticalalignment='top',
                   bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved to {save_path}")
    
    plt.show()


def plot_neuron_activity(
    network: Network,
    simulation_steps: int = 100,
    input_pattern: Optional[dict] = None,
    neuron_ids: Optional[List[str]] = None,
    save_path: Optional[str] = None
):
    """
    Plot neuron activity over time
    
    Args:
        network: Network to simulate
        simulation_steps: Number of timesteps to simulate
        input_pattern: Optional dict of sensory inputs to inject each step
        neuron_ids: Optional list of neurons to plot (plots all if None)
        save_path: Optional path to save figure
    """
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        raise ImportError("matplotlib required. Install with: pip install matplotlib")
    
    if neuron_ids is None:
        neuron_ids = network.neuron_ids
    
    # Track activity
    activity = {nid: [] for nid in neuron_ids}
    
    for step in range(simulation_steps):
        # Inject inputs
        if input_pattern:
            network.inject_dict(input_pattern)
        
        # Step
        network.step()
        
        # Record activity
        for nid in neuron_ids:
            neuron = network.get_neuron(nid)
            if neuron:
                activity[nid].append(1.0 if neuron.did_fire() else 0.0)
    
    # Plot
    fig, ax = plt.subplots(figsize=(14, 6))
    
    for i, nid in enumerate(neuron_ids):
        # Offset each neuron for visibility
        spikes = np.array(activity[nid])
        spike_times = np.where(spikes > 0)[0]
        ax.scatter(spike_times, [i] * len(spike_times), 
                  marker='|', s=100, linewidths=2, label=nid)
    
    ax.set_xlabel('Timestep')
    ax.set_ylabel('Neuron')
    ax.set_yticks(range(len(neuron_ids)))
    ax.set_yticklabels(neuron_ids)
    ax.set_title(f'Neuron Activity ({simulation_steps} steps)')
    ax.grid(True, alpha=0.3, axis='x')
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved to {save_path}")
    
    plt.show()
