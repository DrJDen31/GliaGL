# Example: GridWorld Navigation (Partial Observability)

**Goal**: classify or predict the agent’s intended direction or the presence of a goal while observing only **local events** (partial window). This stresses integration over time.

## Observation → Events

- Render a tiny grid; the agent is a bright square walking per a policy.
- Only a small **fovea** (e.g., 10×10) around the agent is rendered; rest stays dark.
- Movement produces ON/OFF edges at step boundaries.

## Labels

- **Direction** (N/E/S/W) per tick, or **is-on-goal** boolean.
- For sequence-level tasks, label final goal reached (yes/no).

## Files

- `.seq` as usual (events from the fovea only).
- `labels_gridworld.csv`: `clip_id,tick,label` with `label ∈ {N,E,S,W}` (or 0/1).

## Baseline tweaks

- Keep the same network; increase Integrator size (×1.5) so it can carry context between steps.
- L2 motion neurons with 4 preferred directions map cleanly to the label space.

## Metrics

- Tick-wise accuracy; sequence accuracy (goal reached).

## Extensions

- Random obstacles; stochastic slip (adds label noise).
- Reward-modulated plasticity for policy learning.
