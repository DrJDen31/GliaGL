# Example: IMU Gesture Segmentation

**Goal**: detect and segment **gestures** (e.g., shake, swipe) from IMU sequences converted to events.

## Observation → Events

- Take tri-axial accelerometer + gyroscope; treat each channel as a row (Y) and time as X.
- Emit ON/OFF events for positive/negative thresholded derivatives.

## Labels

- Gesture class per tick (`none`, `shake`, `twist`, ...).

## Files

- `.seq` from eventized IMU grid.
- `labels_imu.csv`: `clip_id,tick,class_id`.

## Baseline tweaks

- Increase Integrator’s time constant and size (gestures extend longer).
- Use stronger local inhibition to sharpen onsets.

## Metrics

- Framewise f1; segment IoU.

## Extensions

- Overlapping gestures; online change-point detection.
