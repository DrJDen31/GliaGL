#!/usr/bin/env python3
import argparse, os, math, csv, random, sys
from pathlib import Path

# Optional imports guarded so the script still runs for synthetic / sklearn digits
try:
    import torch
    from torchvision import datasets, transforms
    _HAS_TORCHVISION = True
except Exception:
    _HAS_TORCHVISION = False

try:
    from sklearn.datasets import load_digits as _load_digits
    _HAS_SKLEARN = True
except Exception:
    _HAS_SKLEARN = False


# ------------------------------
# Encoding helpers
# ------------------------------
def image_to_vectors(img_2d):
    """img_2d: 2D list/array-like in [0,1]. Returns flat list row-major."""
    h = len(img_2d)
    w = len(img_2d[0])
    vec = []
    for r in range(h):
        for c in range(w):
            vec.append(float(img_2d[r][c]))
    return vec, h, w

def normalize01(x, eps=1e-9):
    mn, mx = float(min(x)), float(max(x))
    if mx - mn < eps:
        return [0.0 for _ in x]
    return [(v - mn)/(mx - mn) for v in x]

def poisson_encode(feature_vals, T, max_rate, amp, rng, min_intensity=0.0):
    """
    feature_vals in [0,1]; returns list of (tick, idx, amount) events.
    Each feature's Poisson rate = feature * max_rate (spikes/tick).
    """
    events = []
    for idx, val in enumerate(feature_vals):
        if val < min_intensity:
            continue
        lam = val * max_rate  # expected spikes per tick
        # For each tick, sample a Bernoulli with p ~ lam (if lam <= 1). If lam>1, allow multiple spikes.
        for t in range(T):
            # For generality, draw from Poisson(lam)
            k = rng.poisson(lam) if hasattr(rng, "poisson") else sum(1 for _ in range(int(lam)) if rng.random() < (lam - int(lam)))
            if lam < 1.0:
                # Approximate Poisson by Bernoulli for speed
                if rng.random() < lam:
                    k = 1
                else:
                    k = 0
            for _ in range(k):
                events.append((t, idx, amp))
    return events

def rate_encode(feature_vals, T, amp, min_intensity=0.0):
    """
    Dense: every tick inject val*amp for each feature >= min_intensity.
    """
    events = []
    for idx, val in enumerate(feature_vals):
        if val < min_intensity:
            continue
        amount = float(val) * amp
        if amount == 0.0:
            continue
        for t in range(T):
            events.append((t, idx, amount))
    return events

def latency_encode(feature_vals, T, amp, tick_min=0, tick_max=None, min_intensity=0.0):
    """
    One event per feature: brighter -> earlier tick.
    Maps val in [0,1] to t = tick_min + (1-val)*(tick_range-1)
    """
    events = []
    if tick_max is None:
        tick_max = T - 1
    tick_range = max(1, tick_max - tick_min + 1)
    for idx, val in enumerate(feature_vals):
        if val < min_intensity:
            continue
        # invert so 1.0 fires earliest
        t = tick_min + int(round((1.0 - float(val)) * (tick_range - 1)))
        events.append((t, idx, amp))
    return events


# ------------------------------
# Dataset loaders
# ------------------------------
def load_mnist(root, split, fashion=False):
    if not _HAS_TORCHVISION:
        raise RuntimeError("torchvision not available. Install it or use --dataset sklearn_digits/spiral/parity.")
    tfm = transforms.Compose([transforms.ToTensor()])  # [C,H,W] in [0,1]
    ds_cls = datasets.FashionMNIST if fashion else datasets.MNIST
    train = (split == "train")
    ds = ds_cls(root=root, train=train, download=True, transform=tfm)
    # Yield (2D grayscale array in [0,1], label)
    for img, label in ds:
        # img: [1,28,28]
        arr = img.squeeze(0).numpy().tolist()
        yield arr, int(label)

def load_sklearn_digits():
    if not _HAS_SKLEARN:
        raise RuntimeError("scikit-learn not available. Install it or use another dataset.")
    data = _load_digits()
    images = data.images  # (n, 8, 8), values 0..16
    labels = data.target.tolist()
    # scale to [0,1]
    for i, img in enumerate(images):
        arr = (img / 16.0).tolist()
        yield arr, int(labels[i])

def gen_spiral(n_per_class=500, n_classes=3, noise=0.2, seed=0):
    """
    Classic 2D spiral; outputs (x,y) in ~[-1,1], class in 0..C-1
    Encodes as 2 features.
    """
    rng = random.Random(seed)
    import math
    for cls in range(n_classes):
        for i in range(n_per_class):
            r = i / n_per_class
            theta = cls * 2*math.pi/n_classes + r * 4*math.pi
            x = r * math.cos(theta) + rng.gauss(0, noise)*0.05
            y = r * math.sin(theta) + rng.gauss(0, noise)*0.05
            # normalize to [0,1] per feature for encoders
            yield [ (x+1)/2, (y+1)/2 ], cls

def gen_parity(n_bits=12, n_samples=5000, seed=0):
    rng = random.Random(seed)
    for _ in range(n_samples):
        bits = [rng.randint(0,1) for _ in range(n_bits)]
        # even parity -> class 0, odd parity -> class 1
        label = sum(bits) % 2
        # Represent as features in [0,1]
        yield [float(b) for b in bits], label


# ------------------------------
# .seq writer
# ------------------------------
def write_seq(path, duration, events):
    """
    events: list of (tick:int, feature_idx:int, amount:float)
    """
    lines = []
    lines.append(f"DURATION {duration}")
    lines.append("LOOP false")
    lines.append("")
    # Sort by tick for readability (not required)
    events_sorted = sorted(events, key=lambda e: (e[0], e[1]))
    for t, idx, amt in events_sorted:
        lines.append(f"{t} S{idx} {amt:.6f}")
    Path(path).write_text("\n".join(lines), encoding="utf-8")


# ------------------------------
# Main
# ------------------------------
def main():
    p = argparse.ArgumentParser(description="Generate Glia .seq files for several benchmark tasks.")
    p.add_argument("--dataset", choices=["mnist", "fashion", "sklearn_digits", "spiral", "parity"], default="mnist")
    p.add_argument("--split", choices=["train","test"], default="test", help="For MNIST/Fashion.")
    p.add_argument("--outdir", type=str, default="out_seqs")
    p.add_argument("--max-samples", type=int, default=1000, help="Cap number of samples to export.")
    p.add_argument("--encoding", choices=["poisson","rate","latency"], default="poisson")
    p.add_argument("--duration", type=int, default=50, help="Total ticks per sample.")
    p.add_argument("--amp", type=float, default=200.0, help="Injection amount per event (scales lines’ third column).")
    p.add_argument("--max-rate", type=float, default=0.5, help="Max spikes/tick for poisson encoding when feature=1.0.")
    p.add_argument("--min-intensity", type=float, default=0.0, help="Ignore features/pixels below this intensity.")
    p.add_argument("--tick-min", type=int, default=0, help="Latency encoding start tick.")
    p.add_argument("--tick-max", type=int, default=None, help="Latency encoding end tick (inclusive).")
    p.add_argument("--rng-seed", type=int, default=0)
    p.add_argument("--root", type=str, default="~/.cache/glia_datasets", help="Where to store/download datasets.")
    # Spiral options
    p.add_argument("--spiral-classes", type=int, default=3)
    p.add_argument("--spiral-n-per-class", type=int, default=500)
    p.add_argument("--spiral-noise", type=float, default=0.2)
    # Parity options
    p.add_argument("--parity-bits", type=int, default=12)
    p.add_argument("--parity-samples", type=int, default=5000)

    args = p.parse_args()
    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    # RNG: use numpy if present for Poisson; fallback to random.Random
    try:
        import numpy as _np
        rng = _np.random.default_rng(args.rng_seed)
    except Exception:
        class _Fallback:
            def __init__(self, s): self.r = random.Random(s)
            def random(self): return self.r.random()
        rng = _Fallback(args.rng_seed)

    # Load/generate data
    if args.dataset == "mnist":
        it = load_mnist(os.path.expanduser(args.root), args.split, fashion=False)
    elif args.dataset == "fashion":
        it = load_mnist(os.path.expanduser(args.root), args.split, fashion=True)
    elif args.dataset == "sklearn_digits":
        it = load_sklearn_digits()
    elif args.dataset == "spiral":
        it = gen_spiral(n_per_class=args.spiral_n_per_class, n_classes=args.spiral_classes, noise=args.spiral_noise, seed=args.rng_seed)
    else:  # parity
        it = gen_parity(n_bits=args.parity_bits, n_samples=args.parity_samples, seed=args.rng_seed)

    label_path = outdir / "labels.csv"
    with open(label_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["filename","label"])

        count = 0
        for sample, label in it:
            if count >= args.max_samples:
                break

            # Normalize & flatten features
            if isinstance(sample, list) and isinstance(sample[0], list):
                # 2D (image)
                vec, h, w_ = image_to_vectors(sample)
                # already [0,1] for MNIST/Fashion/Sklearn; keep as-is
                feature_vals = vec
            else:
                # 1D (spiral 2D, parity bits, etc.)
                feature_vals = sample
                # Keep within [0,1]; if not, normalize:
                feature_vals = [min(1.0, max(0.0, v)) for v in feature_vals]

            # Encode
            if args.encoding == "poisson":
                events = poisson_encode(feature_vals, args.duration, args.max_rate, args.amp, rng, min_intensity=args.min_intensity)
            elif args.encoding == "rate":
                events = rate_encode(feature_vals, args.duration, args.amp, min_intensity=args.min_intensity)
            else:
                events = latency_encode(feature_vals, args.duration, args.amp,
                                        tick_min=args.tick_min,
                                        tick_max=(args.tick_max if args.tick_max is not None else args.duration-1),
                                        min_intensity=args.min_intensity)

            # Write .seq
            fname = f"{args.dataset}_{args.encoding}_{args.split if args.dataset in ['mnist','fashion'] else 'data'}_{count:06d}.seq"
            write_seq(outdir / fname, duration=args.duration, events=events)

            # Label row
            w.writerow([fname, label])
            count += 1

    print(f"Done. Wrote {count} .seq files to {outdir} and labels to {label_path}.")


if __name__ == "__main__":
    main()
