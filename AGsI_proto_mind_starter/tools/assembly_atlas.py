#!/usr/bin/env python3
"""Build a simple 'assembly atlas' from spike-count CSV logs.
Input format: rows=timestamps, cols=neuron IDs (N columns), values=spike counts in window.
Outputs: top-k assemblies per concept and drift/overlap CSVs.
"""
import argparse, pathlib, csv, numpy as np

def load_csv(p):
    with open(p) as f:
        return np.array([[float(x) for x in row] for row in csv.reader(f)])

def topk_indices(vec, k):
    return np.argsort(-vec)[:k]

def jaccard(a, b):
    inter = len(set(a) & set(b))
    union = len(set(a) | set(b))
    return inter / max(union, 1)

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("spike_csv", type=pathlib.Path)
    ap.add_argument("--k", type=int, default=64)
    ap.add_argument("--slice", type=int, default=-1, help="row index to analyze (default last)" )
    args = ap.parse_args()
    X = load_csv(args.spike_csv)
    v = X[args.slice]
    idx = topk_indices(v, args.k)
    print("TOPK_NEURONS,", ",".join(map(str, idx)))
