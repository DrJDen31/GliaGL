# FeatureColumn Microcircuit (per tile)

## Inputs

- ON event, OFF event (signed spikes) from Eventizer for each pixel in tile.

## Units

- Edge detectors (e.g., 0°, 45°, 90°, 135°): coincidence of aligned pixels; lateral inhibition among orientations.
- Motion detectors (e.g., L→R, R→L, U→D, D→U): 2-tap delay + coincidence (Reichardt-like).
- Short-term memory units for persistence (leak≈0.6–0.95).
- Inhibitory pool per tile to enforce WTA and sparsity.

## Outputs

- Tile-level features (edges, corners, motion vectors) → pooled upward.
- Optional “assembly” neurons that latch on sustained evidence.

## Default Params (starting point)

- Edge coincidence: leak=0.0, TH tuned so 2–3 aligned ONs trigger.
- Motion: delay=1–3 ticks on one arm, coincidence leak=0.0, TH matched to arm strengths.
- Memory: leak=0.8, TH just above background.
- Pool gains: O→I ≈ +30~+40; I→O ≈ −40~−60 (tune per density).
