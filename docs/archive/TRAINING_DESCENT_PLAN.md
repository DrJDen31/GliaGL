# Training Descent Plan (Implementation Notes)

This document tracks the incremental changes added to make training more "descent-like" while staying compatible with the existing pipeline. All changes are behind config flags so you can A/B them.

## Goals

- Reduce update variance and make steps correlate with improving the margin.
- Provide a smooth reward signal even when the argmax decision is discrete.
- Preserve existing behavior by default; opt-in improvements via flags.

## Changes Implemented

- Reward mode `softplus_margin` (new)
  - `reward = sigmoid(gain * (delta - target_margin))` in [0,1]
  - Encourages increasing target margin; smooth signal vs binary.
  - Config: `--reward_mode softplus_margin` and optional `--reward_gain`.

- Advantage baseline (optional)
  - Center reward by subtracting an EMA baseline: `adv = r - baseline`.
  - Flags: `--use_advantage_baseline 1`, `--baseline_beta 0.1`.

- No-update-if-satisfied gate (optional)
  - If correct and `margin >= delta`, skip weight update to reduce drift.
  - Flag: `--no_update_if_satisfied 1`.

- Rate-based eligibility (optional)
  - Replace post spike indicator with post EMA rate for denser/local signal:
    `e = lambda * e + pre_spike * post_rate`.
  - Flag: `--elig_post_use_rate 1`.

## CLI Examples

```bash
# XOR, enable all new options
glia_eval \
  --net examples/xor/xor_network.net --scenario xor --default O0 \
  --train --epochs 20 --batch 4 --shuffle 1 \
  --reward_mode softplus_margin --reward_gain 1.0 \
  --use_advantage_baseline 1 --baseline_beta 0.1 \
  --elig_post_use_rate 1 --no_update_if_satisfied 1 \
  --verbose 1 --log_every 1
```

## Flags Summary

- `--reward_mode {binary|margin_linear|softplus_margin}`
- `--reward_gain F` (used by margin modes)
- `--use_advantage_baseline 0|1`
- `--baseline_beta F`
- `--no_update_if_satisfied 0|1`
- `--elig_post_use_rate 0|1`

## Notes

- Defaults preserve prior behavior (binary rewards, spike-based eligibility).
- Combine `softplus_margin` with the advantage baseline for signed updates.
- Keep `margin_delta` meaningful; it is used by the gate and for softplus center.

