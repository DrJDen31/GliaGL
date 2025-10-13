#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")"/../.. && pwd)"
BUILD_DIR="$ROOT_DIR/src/train/build"
EXE="$BUILD_DIR/glia_eval"
CFG="$ROOT_DIR/examples/3class/configs/train_3class_from_baseline_nojitter.json"
LOG_DIR="$ROOT_DIR/examples/3class/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

cmake -S "$ROOT_DIR/src/train" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -- -j

"$EXE" --config "$CFG" \
       --net "$ROOT_DIR/examples/3class/3class_baseline_biased.net" \
       --reward_mode softplus_margin --use_advantage_baseline 1 --baseline_beta 0.1 \
       --elig_post_use_rate 1 --no_update_if_satisfied 1 --weight_clip 2.5 \
       --save_net "$LOG_DIR/3class_trained_from_biased_baseline.net" \
       --train_metrics_json "$LOG_DIR/3class_biased_train_metrics.json" \
       --metrics_json "$LOG_DIR/3class_biased_eval_summary.json"

echo "Done. Trained net: $LOG_DIR/3class_trained_from_biased_baseline.net"
echo "Metrics: $LOG_DIR/3class_biased_eval_summary.json, $LOG_DIR/3class_biased_train_metrics.json"

