# Training Strategy (Practical, fast)

## Readout & Reward

- Output decision from **EMA firing rates** (α≈1/20).
- Use argmax(rate); optionally require margin `Δ` to commit.
- **Reward shaping**: `+1.2` if `rate[y]−max(others) ≥ Δ`, else `−0.8`.

## Local Synaptic Learning

- **Reward-modulated Hebb/STDP** with short eligibility traces per synapse.
- Update: `Δw ∝ reward × eligibility(pre, post)`; include small weight decay/L1.

## Intrinsic Plasticity (per neuron)

- Maintain healthy rates via **threshold/leak homeostasis**:
  - `θ ← θ + ηθ (r_obs − r_target)`
  - `leak ← leak + ηℓ (r_target − r_obs)` (optional)
- Keeps units out of dead/saturated regimes; stabilizes dynamics.

## Structural Plasticity (sparsity)

- **Prune** edges with |w| < ε for K steps.
- **Grow** a few candidate edges/step (random or locality-biased) with tiny |w|.
- Enforce per-source **outgoing “mass” target** (homeostasis) or L1 to stay sparse.

## Competition

- Add **lateral inhibition** between outputs or a **shared inhibitory pool**.
- This makes argmax(rate) reliable and reduces readout complexity.

## Evolutionary Outer Loop (small, Lamarckian)

- Population 10–50; mutate structure (grow/prune), θ/leak/refractory bounds, pool strength.
- **Lamarckian**: let local learning happen during evaluation; inherit learned state.
- Multi-objective: maximize accuracy/margin; minimize edges/energy (#spikes).

## Curriculum

- Start simple/short windows; introduce noise and longer contexts; anneal rewire rate.
