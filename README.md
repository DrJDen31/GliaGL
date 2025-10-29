# GliaGL

## Overview

Event-driven spiking network simulator with:
- Gradient-descent training (current focus)
- Evolutionary trainer (planned, layered on top)
- OpenGL visualization (frozen for now; to be refreshed later)

## Current status

- Train and evaluate via the CLI in `src/train/`.
- Examples live under `examples/` (replacing older `src/testing/` paths).
- Visualization in `src/vis/` remains but may be out-of-date.

## Quick start (WSL/Linux)

```bash
# Build training CLI
cd src/train
mkdir -p build && cd build
cmake ..
cmake --build . -j

# Run examples
# XOR (manual network)
./glia_eval --scenario xor --default O0

# 3-class (manual network with noise)
./glia_eval --scenario 3class --noise 0.10

# Evaluate a custom .net (example path)
./glia_eval --net ../../examples/xor/xor_network.net --default O0
```

Notes:
- Set `--warmup` and `--window` to control episode timing; see `src/train/README.md`.
- The evolutionary trainer will wrap the inner gradient-descent loop (see roadmap in `src/train/README.md`).

## Repository layout

```
GliaGL/
├── src/
│   ├── arch/        # Core network engine
│   ├── train/       # CLI runner, trainers, configs
│   └── vis/         # OpenGL visualizer (frozen)
├── examples/        # XOR, 3class, seq_digits_poisson, mini-world, etc.
├── docs/            # Specs, plans, usage (kept in sync with examples/)
├── build/           # Local builds (ignored)
└── tools/           # Utilities (e.g., evo_tree_viz.py)
```

## Visualization (status)

- OpenGL viewer under `src/vis/` builds via CMake but is considered frozen/out-of-date.
- Keep using the training CLI for core workflows.

## Housekeeping

- Local build directories (`build/`, `src/train/build/`, `src/vis/build/`) are ignored by git.
- Examples should not commit compiled binaries, logs, or analysis outputs.
- See `.gitignore` for the full list of ignored artifacts.
