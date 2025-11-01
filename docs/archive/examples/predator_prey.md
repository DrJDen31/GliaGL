# Example: Predator–Prey Multi-Agent

**Goal**: count **two classes** (predators vs prey) or predict **capture events**.

## Observation → Events

- Simulate two object types with different brightness or motion statistics.
- Optional occlusions; different speeds.

## Labels

- `n_predators`, `n_prey` (two heads), or `capture` binary when distance < threshold.

## Files

- `.seq` as before.
- `labels_predator_prey.csv`: per-tick counts or capture flags.

## Baseline tweaks

- Split L2 into **two motion banks** with different preferred speeds.
- Add two readout heads (multi-task).

## Metrics

- Count MAE for both classes; capture precision/recall.

## Extensions

- Cooperative predators; evasive prey; curriculum on speed/number.
