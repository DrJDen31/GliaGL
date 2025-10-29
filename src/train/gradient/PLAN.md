# Gradient Training Plan (Rate-based Surrogate)

This plan outlines how to add a gradient-descent-style trainer for GliaGL using differentiable firing-rate proxies. We start with plain SGD, then optionally add Momentum and Adam.

## Milestones
- M0: Scaffolding and config plumbing
- M1: Rate collection and loss (SGD)
- M2: Local gradient using eligibility traces (e-prop-style)
- M3: Optimizers (Momentum, Adam), gradient clipping
- M4: Synapse usage/rate integration and regularizers
- M5: Metrics, JSON logs, CLI flags, docs

## M0 — Scaffolding
- Add `src/train/gradient/` with a new trainer `RateGDTrainer` mirroring the Hebbian `Trainer` API:
  - `evaluate(seq, cfg)`
  - `trainBatch(batch, cfg, metrics_out)`
  - `trainEpoch(dataset, epochs, cfg)`
- Extend config with a nested gradient subgroup (non-breaking):
  - `grad.loss`: `softmax_xent` | `margin_hinge`
  - `grad.temperature`: float (softmax temperature)
  - `grad.optimizer`: `sgd` | `momentum` | `adam`
  - `grad.momentum`, `grad.adam_beta1`, `grad.adam_beta2`, `grad.adam_eps`
  - `grad.clip_grad_norm`: float (0=disabled)
  - `grad.surrogate`: `sigmoid` | `piecewise_linear` (for spike derivative surrogate)

## M1 — Rates and Loss
- Track EMA firing rate per neuron (already in Hebbian): `r_j ← (1−α) r_j + α · spike_j`.
- Output rates: average over decision window W or use end-of-window value.
- Loss choices:
  - Softmax-Xent: `p = softmax(r_O / T)`, `L = −log p[target]`.
  - Margin-Hinge: `m = r_target − max_{k≠t} r_k`, `L = max(0, δ − m)`.
- Compute dL/dr for output neurons; cache per-episode gradients.

## M2 — Local Gradients with Eligibility Traces
- Use existing eligibility traces `e_ij(t) = λ e_ij(t−1) + pre_i(t)·post_j(t_surrogate)` as credit assignment.
- Local learning signal for neuron j at episode end:
  - `g_j = dL/dr_j` aggregated over window (e.g., average or sum).
- Approx weight gradient:
  - `∂L/∂w_ij ≈ (Σ_t e_ij(t)) · g_j` (scale by window length if averaged)
- Batch accumulation: sum over examples; apply averaged gradient with learning rate.
- Optional weight decay as in Hebbian path.

## M3 — Optimizers and Stability
- Implement `sgd`, `momentum` (velocity v), and `adam` (m,v with bias correction).
- Add `grad.clip_grad_norm` (global or per-parameter) to stabilize training.

## M4 — Synapse Usage and Regularizers
- Synapse rate/usage: normalize eligibility as a proxy for synapse firing rate; add auxiliary losses:
  - L2 on weights; sparsity: L1 on weights or penalty on average usage.
  - Energy proxy: penalty on sum of neuron rates.
- Combine via weighted sum with existing config weights if desired.

## M5 — Instrumentation and CLI
- Write per-epoch `{accuracy[], margin[], loss[]}` to JSON (similar to Hebbian).
- CLI flags: `--gd 1`, `--gd_loss softmax_xent`, `--gd_opt adam`, etc.
- Ensure both trainers can be selected at runtime without rebuild; default remains Hebbian.

## Integration Notes
- Keep `src/train/hebbian/` intact; add `src/train/gradient/` with a separate trainer.
- Do not alter network core (`src/arch/`) for first pass.
- Begin with outputs-only gradients; later propagate to all neurons with surrogate derivative.
- Unit-test on XOR and 3-class; compare to Hebbian baseline.
