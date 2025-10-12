#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")"/../.. && pwd)"
BUILD_DIR="$ROOT_DIR/src/train/build"
EXE="$BUILD_DIR/glia_eval"
CFG="$ROOT_DIR/examples/3class/configs/train_3class.json"
LOG_DIR="$ROOT_DIR/examples/3class/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

cmake -S "$ROOT_DIR/src/train" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -- -j

"$EXE" --config "$CFG" --net "$ROOT_DIR/examples/3class/3class_network.net"
"$EXE" --net "$ROOT_DIR/examples/3class/3class_network.net" --scenario 3class --default O0 --metrics_json "$LOG_DIR/eval_summary.json"

echo "Done. Metrics: $LOG_DIR/eval_summary.json"
