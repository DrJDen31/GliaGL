# Gradient Training Theory (Rate-based Surrogate for Spiking RNNs)

This note summarizes the theory for a gradient-descent-style trainer over a spiking, recurrent network using differentiable firing-rate proxies and surrogate gradients.

## Modeling choices
- **Rate proxy**: For each neuron `j`, maintain an EMA firing rate `r_j(t) = (1−α) r_j(t−1) + α · spike_j(t)`.
- **Episode output**: Aggregate output rates over the decision window `W` (e.g., average) to get `r_O`.
- **Surrogate gradient**: Replace the non-differentiable spike with a smooth surrogate when needed:
  - Sigmoid: `σ(x) = 1/(1+e^{−x})`, use `σ'(x)` as a proxy for `∂spike/∂x`.
  - Piecewise-linear: triangular/rectifier-based derivative around threshold.

## Losses
- **Softmax cross-entropy** over output rates:
  - `p = softmax(r_O / T)` with temperature `T` (default 1.0).
  - `L = −log p[target]`.
  - Gradient at outputs: `∂L/∂r_O = p − y` where `y` is one-hot target.
- **Margin hinge** (episode-level):
  - Margin: `m = r_target − max_{k≠t} r_k`.
  - `L = max(0, δ − m)`; subgradient w.r.t. rates follows active hinge.

## Credit assignment (local, e-prop style)
- Maintain per-edge eligibility `e_ij(t) = λ e_ij(t−1) + pre_i(t) · post_j(t_surrogate)`.
- Episode learning signal for neuron `j`: `g_j = Σ_{t∈W} ∂L/∂r_j(t)` (or the average).
- Approximate weight gradient:
  - `∂L/∂w_ij ≈ (Σ_t e_ij(t)) · g_j`.
  - Intuition: `e_ij` tracks how `w_ij` affects `j`'s activity; `g_j` tells whether increasing `j` helps or hurts the loss.

## Update rules
- **SGD**: `w ← w − η · ∂L/∂w − ζ · w` (weight decay `ζ`).
- **Momentum**: `v ← μ v + ∂L/∂w`, `w ← w − η v`.
- **Adam**: `m ← β1 m + (1−β1) g`, `v ← β2 v + (1−β2) g²`, bias-correct `m̂, v̂`, then `w ← w − η · m̂/(√(v̂)+ε)`.
- **Clipping**: optional global or per-parameter `||g||` clipping for stability.

## Regularization and auxiliary signals
- **Sparsity/energy**: penalties on `|w|`, on average neuron rate `Σ_j r_j`, or on average synapse usage (normalized eligibility).
- **Topology policy**: respect existing `TopologyPolicy` when growing connections, identical to Hebbian path.

## Relationship to existing Hebbian trainer
- Hebbian uses reward-modulated eligibility: `Δw ∝ reward · Σ_t e_ij(t)`.
- Gradient mode uses supervised loss to produce `g_j` and replaces reward with `g_j`.
- Both rely on the same event-driven forward pass and EMA rate tracking.

## Implementation notes
- Reuse `InputSequence`, `Glia`, EMA tracking, and batching infrastructure.
- Extend `TrainingConfig` with a `grad` subgroup for optimizer and loss settings.
- Begin with outputs-only gradient; later consider full network rate-based propagation with surrogate `∂spike/∂u`.
