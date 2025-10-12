#!/usr/bin/env bash
set -euo pipefail
# Resolve repo root from this script location
ROOT_DIR="$(cd "$(dirname "$0")"/../.. && pwd)"
BUILD_DIR="$ROOT_DIR/src/train/build"
EXE="$BUILD_DIR/glia_eval"
CFG="$ROOT_DIR/examples/xor/configs/train_xor.json"
LOG_DIR="$ROOT_DIR/examples/xor/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

# Configure and build (Makefiles generator on WSL)
cmake -S "$ROOT_DIR/src/train" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -- -j

# Train using JSON config (override net with absolute path for robustness)
"$EXE" --config "$CFG" --net "$ROOT_DIR/examples/xor/xor_network.net"

# Evaluate XOR and write a summary JSON
"$EXE" --net "$ROOT_DIR/examples/xor/xor_network.net" --scenario xor --default O0 --metrics_json "$LOG_DIR/eval_summary.json"

echo "Done. Metrics: $LOG_DIR/eval_summary.json"
