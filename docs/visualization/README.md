# Visualization Docs Index

- Build & run
  - docs/BUILD_INSTRUCTIONS.md – Platform setup and building the visualizer
  - src/vis/CMakeLists.txt – Visualization build configuration

- Usage guides
  - docs/RENDERING_USAGE.md – Rendering usage
  - docs/archive/UI_INDICATORS.md – Winner/tick indicators (historical)
  - docs/archive/FINAL_UI_POLISH.md – Final indicator sizes and sticky logic (historical)

- Progress & phases (archived)
  - docs/archive/PHASE1_COMPLETE.md – Core data structures
  - docs/archive/PHASE2_COMPLETE.md – Rendering integration
  - docs/archive/VISUALIZATION_PROGRESS.md – Implementation progress
  - docs/archive/STATUS_SUMMARY.md – Summary status

- Code entry points
  - src/vis/main.cpp – App entry
  - src/vis/argparser.cpp – Load network mode
  - src/vis/network_graph.cpp – Graph build + activation updates
  - src/vis/OpenGLRenderer.cpp – Rendering

Notes:
- Visual indicators now rely on EMA-based winner tracking (see `src/arch/output_detection.h`)
- OPENGL errors on startup should be resolved by skipping cloth VBO updates in network mode
