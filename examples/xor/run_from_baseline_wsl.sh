#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")"/../.. && pwd)"
BUILD_DIR="$ROOT_DIR/src/train/build"
EXE="$BUILD_DIR/glia_eval"
CFG="$ROOT_DIR/examples/xor/configs/train_xor_from_baseline_nojitter.json"
LOG_DIR="$ROOT_DIR/examples/xor/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

cmake -S "$ROOT_DIR/src/train" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -- -j

# Single run: train from baseline then evaluate and write summary JSON
"$EXE" --config "$CFG" \
       --net "$ROOT_DIR/examples/xor/xor_baseline.net" \
       --reward_mode softplus_margin --use_advantage_baseline 1 --baseline_beta 0.1 \
       --elig_post_use_rate 1 --no_update_if_satisfied 1 --weight_clip 2.0 \
       --save_net "$LOG_DIR/xor_trained_from_baseline.net" \
       --metrics_json "$LOG_DIR/eval_summary_from_baseline.json"

echo "Done. Metrics: $LOG_DIR/eval_summary_from_baseline.json"
