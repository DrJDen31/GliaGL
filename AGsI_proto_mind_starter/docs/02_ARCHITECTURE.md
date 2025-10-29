# 02 — Architecture Map (Brain‑like Parts)

## Modules
- **Encoders (per modality):** Vision/Audio/Text/Touch → sparse spikes; target sparsity via thresholds + inhibitory pool.
- **Index (fast plastic):** Small k‑WTA pool; builds co‑occurrence assemblies quickly; tagged synapses; brief persistence.
- **Cortex/Association (slow):** Larger pool; mild recurrence; stores stable concept assemblies; consolidates via replay.
- **Predictive Loops:** Assoc→Enc feedback; compute residual (prediction error) in simple subtractive circuits.
- **Global Workspace:** Competitive hub; ignition → broadcast to language/visual/motor heads; attention modulation.
- **Output Heads:** Language (phoneme/label), Visualization (templates), Motor (speech/act); mostly slow plastic.
- **Neuromodulators/Gates:** PLASTICITY, DOPAMINE (consistency/reward), NE (novelty/salience); token bucket budgets.
- **Replay (“sleep”):** Low‑duty oscillation to reactivate recent Index assemblies; tiny η to polish Cortex.

## Connectivity (high level)
- Enc→Index (fast‑plastic), Enc→Cortex (slow/frozen), Index↔Index (weak recur), Index→Cortex (replay), Cortex↔Cortex (weak), Cortex→Enc (predictive), Cortex/Index→GW, GW→Heads.

See `design/architecture.mmd` for a diagram.
