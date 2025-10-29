# PyTorch Glia Experiments

This directory contains PyTorch-based reproductions of the Glia spiking network and supporting scripts.

## Components

- **`glia_net.py`** — neural module mirroring the C++ implementation with tick-driven membrane updates, surrogate gradients, and tensorised simulators (see `GliaTorch.simulate_events()`).
- **`example_xor.py`** — minimal demo loading `examples/xor/xor_baseline.net` and printing spike/membrane traces for a handcrafted stimulus schedule.
- **`train_xor.py`** — end-to-end training example that synthesises noisy XOR spike episodes, optimises weights with Adam, and reports validation accuracy.

## Usage

```bash
python pytorch/example_xor.py
python pytorch/train_xor.py
```

Both scripts honour the existing `.net` topology files. `train_xor.py` auto-detects CUDA when available and otherwise runs on CPU.
