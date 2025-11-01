# Implementation Notes (Glia/Neuron)

## Rate Readout

- Maintain EMA per output: `rate[k] = (1−α) rate[k] + α * didFire(k)`; α≈1/20.
- Decisions:
  - Argmax(rate)
  - Threshold winner: `rate[k] ≥ θ`
  - Margin: `rate[best] − rate[second] ≥ Δ`

## Teacher Nudges (optional)

- If target y and current best != y:
  - inject small +pulse into OUT_y
  - inject small −pulse into current best
- Use only during train.

## Logging

- Per tick: `t, pred, target, reward, per-class rates, #edges_to_outputs, sum_incoming`.
- Plot: rolling accuracy, per-class rates, topology/weight signals.

## Structural Sync

- After trainer updates weights/edges, call `glia.syncBookkeepingFromNeurons()` to keep maps current for printing/saving.

## Config Format (suggested)

```ini
[neuron_params]
# id, threshold, leak, refractory, bias(optional)
A, 90, 0.0, 0
O1, 50, 1.0, 0
O0, 60, 1.0, 0

[edges]
# pre, post, weight, delay(optional)
S0, O1, 60
S1, O1, 60
S0, A,  60
S1, A,  60
A,  O1, -120
A,  O0, 120
```

### Save/Load: parse blocks, create neurons if absent, set params, then add edges.

---

### `CONFIG_MANUAL_XOR_RATE.cfg`

```ini
# Manual XOR-by-rate network (coincidence A version)
# Neurons: S0, S1 (sensory); A (AND), O1 (XOR true), O0 (XOR false)

[neuron_params]
A, 90, 0.0, 0
O1, 50, 1.0, 0
O0, 60, 1.0, 0

[edges]
S0, O1, 60
S1, O1, 60
S0, A,  60
S1, A,  60
A,  O1, -120
A,  O0, 120

[readout]
alpha=0.05
mode=margin
margin=0.20
```
