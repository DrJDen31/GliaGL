# Testing Docs Index

- Examples and specs
  - docs/TOY_EXAMPLES.md – XOR, 3-class, temporal AB/BA
  - src/testing/xor/ – XOR network, README, tests, baselines
  - src/testing/3class/ – 3-class network, README, tests, baselines

- CLI training/evaluation
  - src/train/eval_main.cpp – Runner entrypoint
  - Build: docs/BUILD_INSTRUCTIONS.md (glia_eval)
  - Usage examples (from docs/README.md):
    - glia_eval --scenario xor --default O0
    - glia_eval --scenario 3class --noise 0.10

- Output detection
  - src/arch/output_detection.h – EMA detector with default-id
  - docs/OUTPUT_DETECTION_OPTIONS.md – Detector options & best practices

- Framework
  - docs/SCALABLE_TESTING_FRAMEWORK.md – Overview
  - docs/TESTING_FRAMEWORK_SUMMARY.md – Consolidated plan
  - docs/TESTING_QUICK_REFERENCE.md – Shortcuts and references
