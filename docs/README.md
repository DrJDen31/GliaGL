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
