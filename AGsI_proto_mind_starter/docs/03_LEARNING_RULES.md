# 03 — Learning Rules & Gates

**Pair/Triplet STDP** with small base magnitudes → accumulated in **eligibility traces** `e_ij` (τ_e ~ 200–800 ms).  
**Three‑factor consolidation:** Δw = η · e_ij · M(t), where M(t) = PLASTICITY × DOPAMINE × NE.  
- η (Index) ~ 2e-4; η (Cortex) ~ 1e-5..3e-5; heads slow.  
- Per‑post weight normalization (L2→1.0) every ~25 ms; clamp [0,1].  
- **Heterosynaptic LTD** to compress assemblies; **winner fatigue** for coverage.

**Gating policy**
- Open only when **consistency high** (modalities agree) AND **novelty moderate**.  
- **Token bucket** limits consolidation windows per minute.  
- **Metaplastic anchor:** slow penalty toward baseline weights to prevent drift.
