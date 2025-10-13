# Example: CartPole (Continuous Control → Events)

**Goal**: classify **balance state** (stable/unstable) or **action** (left/right) from eventized renderings of a simulated cart-pole.

## Observation → Events

- Render the pole/cart on dark background; small camera jitter for realism.
- ON/OFF edges happen at pole angle changes and cart motion.

## Labels

- Per-tick **action** (policy demo) or **is_stable** (0/1).

## Files

- `.seq` with events from the render.
- `labels_cartpole.csv`: `clip_id,tick,action_id` or `stable`.

## Baseline tweaks

- Add a tiny **velocity readout** head (regresses dx/dt) from L2 motion—helps with action inference.
- Integrator stays modest; dynamics are smooth.

## Metrics

- Action accuracy; stability AUC.

## Extensions

- Domain randomization: pole length, gravity, noise, motion blur.
