# Example: Cellular Automata Future Prediction

**Goal**: predict **next state** (or class of rule) from a short observed event history of a CA (e.g., Game of Life).

## Observation → Events

- Show CA states as frames; emit ON when a cell turns alive, OFF when it dies.
- Windowed world with wraparound boundaries.

## Labels

- Next-step classification (alive/dead per cell → can be subsampled to a count), or **rule ID**.

## Files

- `.seq` generated from CA transitions.
- `labels_ca.csv`: `clip_id,tick,target` (task-dependent).

## Baseline tweaks

- L1 edge detectors capture neighborhood changes well.
- For per-cell prediction, add a **convolutional readout** (local head). For rule ID, keep a global class head.

## Metrics

- Per-cell accuracy / IoU; rule ID accuracy.

## Extensions

- Noisy updates; partial observation masks.
