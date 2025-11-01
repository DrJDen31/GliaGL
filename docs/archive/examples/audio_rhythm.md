# Example: Synthetic Audio Events / Rhythms

**Goal**: detect rhythm class or tempo from **eventized audio** (spike-like onsets).

## Observation → Events

- Synthesize short audio (click tracks, drum patterns, sine sweeps).
- Convert to a **time×frequency** patch (STFT or mel). Generate ON events on energy rises and OFF on falls across bands → treated like a 2-D sensor.

## Labels

- Rhythm class (e.g., 4/4, 3/4, syncopated), or **tempo bin**.

## Files

- `.seq` from eventized spectrogram cells.
- `labels_audio.csv`: `clip_id,tick,class_id` (kept constant per clip for global class).

## Baseline tweaks

- Treat frequency like the Y dimension; motion units now track **spectral edges** and **onset slopes**.
- Increase L1 size (×2) to cover more bands; keep Integrator similar.

## Metrics

- Accuracy on class/tempo; optionally f-score on onset detection.

## Extensions

- Add noise, detuning, variable reverb tails → robustness to decay dynamics.
