#!/usr/bin/env python3
"""
Visualize an evolutionary lineage JSON produced by EvolutionEngine.

Usage:
  python tools/evo_tree_viz.py --lineage examples/3class/logs/evolve_lineage.json --out examples/3class/logs/evo_tree.png

If matplotlib is available, renders a layered graph by generation.
If not, prints a Graphviz DOT to stdout as a fallback.
"""

import json
import argparse
from collections import defaultdict

def load_lineage(path):
    with open(path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    nodes = data.get('nodes', [])
    # Normalize parent -1 to None
    for n in nodes:
        if n.get('parent', None) == -1:
            n['parent'] = None
    return nodes

def to_layers(nodes):
    layers = defaultdict(list)
    id2node = {}
    for n in nodes:
        g = int(n.get('gen', 0))
        layers[g].append(n)
        id2node[n['id']] = n
    gens = sorted(layers.keys())
    # Sort each gen by id for stable layout
    for g in gens:
        layers[g].sort(key=lambda x: x['id'])
    return gens, layers, id2node

def try_matplotlib(nodes, out_path=None, show=False):
    try:
        import matplotlib.pyplot as plt
        import matplotlib as mpl
    except Exception as e:
        return False
    gens, layers, id2node = to_layers(nodes)
    if not gens:
        print('No nodes to plot')
        return True
    # Compute fitness range
    fits = [float(n.get('fitness', 0.0)) for n in nodes]
    fmin = min(fits) if fits else 0.0
    fmax = max(fits) if fits else 1.0
    if fmax <= fmin:
        fmax = fmin + 1.0
    cmap = mpl.cm.viridis

    fig, ax = plt.subplots(figsize=(max(6, len(nodes)*0.15), max(4, len(gens)*1.5)))
    ax.set_title('Evolutionary Tree')
    ax.set_axis_off()

    # Assign positions per generation
    positions = {}
    max_nodes_in_layer = max(len(layers[g]) for g in gens)
    for gi, g in enumerate(gens):
        layer = layers[g]
        nL = len(layer)
        for i, n in enumerate(layer):
            x = (i + 1) / (nL + 1)
            y = 1.0 - gi / max(1, (len(gens)-1) if len(gens) > 1 else 1)
            positions[n['id']] = (x, y)

    # Draw edges
    for n in nodes:
        pid = n.get('parent')
        if pid is None:
            continue
        if pid not in positions:
            continue
        x0, y0 = positions[pid]
        x1, y1 = positions[n['id']]
        ax.plot([x0, x1], [y0, y1], color='#888888', linewidth=1.0, zorder=1)

    # Draw nodes
    for n in nodes:
        x, y = positions[n['id']]
        f = float(n.get('fitness', 0.0))
        t = (f - fmin) / (fmax - fmin)
        color = cmap(t)
        ax.scatter([x], [y], s=120, c=[color], edgecolors='k', linewidths=0.5, zorder=2)
        label = f"g{n.get('gen', 0)}\nid{n['id']}\nf={f:.3f}\nacc={float(n.get('acc',0.0)):.2f}"
        ax.text(x, y-0.05, label, ha='center', va='top', fontsize=7)

    plt.tight_layout()
    if out_path:
        fig.savefig(out_path, dpi=200)
        print(f'Saved plot -> {out_path}')
    if show:
        plt.show()
    plt.close(fig)
    return True

def to_dot(nodes):
    lines = ['digraph evo {', '  rankdir=TB;']
    for n in nodes:
        label = f"g{n.get('gen',0)} id{n['id']} f={float(n.get('fitness',0.0)):.3f}"
        lines.append(f'  n{n["id"]} [label="{label}"];')
    for n in nodes:
        pid = n.get('parent')
        if pid is None:
            continue
        lines.append(f'  n{pid} -> n{n["id"]};')
    lines.append('}')
    return '\n'.join(lines)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lineage', required=True, help='Path to lineage JSON written by EvolutionEngine')
    ap.add_argument('--out', default=None, help='Output image path (PNG). If omitted, shows window if possible.')
    ap.add_argument('--show', type=int, default=0, help='Show interactive window (1/0)')
    args = ap.parse_args()

    nodes = load_lineage(args.lineage)
    ok = try_matplotlib(nodes, out_path=args.out, show=bool(args.show))
    if not ok:
        print('# matplotlib not available, printing DOT to stdout:')
        print(to_dot(nodes))

if __name__ == '__main__':
    main()

