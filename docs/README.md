# Continuous Image Analysis — Proof of Concept (PoC)

## Why this architecture (short)

- **Continuous time**: the net is always “ticking,” so state persists across frames.
- **Rate-coded outputs**: decisions emerge from sustained firing rates (argmax or margin).
- **Implicit memory**: loops + leak/thresholds encode short/long horizons without giant contexts.
- **Local learning ready**: reward-modulated Hebb/STDP + intrinsic plasticity (threshold/leak).
- **Hardware-aligned**: sparse, event-driven, amenable to neuromorphic/“neomorphic” chips.

## PoC goals

1. Drive the network with **eventized video** (frame deltas → spikes).
2. Use **tiled feature microcircuits** (edges, motion, inhibitory pools).
3. Perform **rate-based readout** with **margin thresholds** for classification/tracking.
4. Log per-tile activity, output rates, energy proxy (#spikes), and decision margins.

## Minimal milestone list

- Eventizer (ΔI > θ) producing ON/OFF spikes.
- FeatureColumn microcircuit replicated across tiles.
- Rate readout (EMA rates + argmax/margin).
- Basic intrinsic plasticity (rate homeostasis).
- Optional: reward-modulated Hebbian updates + sparse rewire (grow/prune).

---

## Documentation map

- **Architecture**
  - Index: `docs/architecture/README.md`
  - `src/arch/README.md` – Core components (`Glia`, `Neuron`, `output_detection.h`)
  - `docs/TOY_EXAMPLES.md` – Reproducible specs (XOR, 3-class, temporal)
  - `docs/OUTPUT_DETECTION_OPTIONS.md` – Detector strategies (EMA default)

- **Testing**
  - Index: `docs/testing/README.md`
  - `src/testing/` – Example nets and harnesses
  - XOR: `src/testing/xor/` (crafted + baseline)
  - 3-class: `src/testing/3class/` (crafted + baseline)

- **Training/Evaluation**
  - CLI runner: `src/train/eval_main.cpp`
  - Config: `src/train/training_config.h`
  - Trainer: `src/train/trainer.h`

- **Visualization**
  - Index: `docs/visualization/README.md`
  - `src/vis/` – OpenGL UI and network viewer
  - Build: `docs/BUILD_INSTRUCTIONS.md`

- **Archive**
  - `docs/archive/` – Historical progress/fixes/status docs

---

## CLI runner (training/eval) quick start

Build (Windows/MSVC or Linux/clang++/g++):

```bash
cd src/train
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

Usage examples:

```bash
# XOR crafted (default O0 fallback)
glia_eval[.exe] --scenario xor --default O0

# XOR baseline (dysfunctional)
glia_eval[.exe] --scenario xor --baseline --default O0

# 3-class crafted with 10% noise
glia_eval[.exe] --scenario 3class --noise 0.10

# Evaluate custom .net
glia_eval[.exe] --net ..\src\testing\xor\xor_network.net --default O0
```
