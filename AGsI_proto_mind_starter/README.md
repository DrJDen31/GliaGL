# AGsI: Artificial General subIntelligence (Starter Pack)

This repo is a planning + scaffolding bundle to bootstrap a **multimodal, online-learning, assembly-based proto‑mind** in your Glia framework. 
It converts our chat plan into concrete files you can drop into Windsurf and iterate on.

## What’s inside
- **docs/**: Roadmap, architecture, “brain map,” learning rules, interpretability & safety, metrics, .seq spec.
- **design/**: Mermaid diagrams describing the system (paste into Mermaid previewer).
- **experiments/**: Config templates for milestone demos + expected proofs.
- **tools/**: Code stubs (Python) for .seq generation, metrics logging, and assembly-atlas analysis. Adapt as needed.
- **seq_examples/**: Tiny illustrative .seq fragments for gating, pairing, and recall tests.

## Quick start
1. Read `docs/01_ROADMAP.md` and pick the current milestone.
2. Open the matching `experiments/*_config.yaml` and fill in details.
3. Use `tools/seq_generator.py` to emit training/recall .seq files for that milestone.
4. Run Glia with logging hooks (see `docs/06_METRICS_EVAL.md`). 
5. Use `tools/assembly_atlas.py` to visualize assemblies over time (spike-count CSVs in → PNGs/CSVs out).
6. Iterate: tune k‑WTA, STDP rates, and gating budgets per milestone.

## AGsI definition
**AGsI (Artificial General subIntelligence)**: a *general* cognitive substrate—multimodal binding, online learning, internal assemblies, imagination, workspace/attention—operating **below human intelligence** and without claims of consciousness. A mind‑like system, but subhuman and safety‑instrumented by design.
