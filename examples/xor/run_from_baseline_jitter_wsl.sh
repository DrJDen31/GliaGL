#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")"/../.. && pwd)"
BUILD_DIR="$ROOT_DIR/src/train/build"
EXE="$BUILD_DIR/glia_eval"
CFG="$ROOT_DIR/examples/xor/configs/train_xor_from_baseline_jitter.json"
LOG_DIR="$ROOT_DIR/examples/xor/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

cmake -S "$ROOT_DIR/src/train" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -- -j

"$EXE" --config "$CFG" \
       --net "$ROOT_DIR/examples/xor/xor_baseline.net" \
       --metrics_json "$LOG_DIR/eval_summary_from_baseline_jitter.json"

echo "Done. Metrics: $LOG_DIR/eval_summary_from_baseline_jitter.json"
