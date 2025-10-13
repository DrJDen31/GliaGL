# Baseline Network — Populations & Wiring

This baseline targets the **counting** task (predict number of moving objects per tick). It’s deliberately simple and scalable.

---

## 1) Populations (for 32×32, ON/OFF)

- **Sensory**: `2 × 32 × 32 = 2,048` (fixed by input mapping)
- **L1 (edges/contrast)**
  - Excitatory: **512**
  - Inhibitory: **128**
- **L2 (motion/direction)**
  - Excitatory: **256**
  - Inhibitory: **64**
- **Integrator / working memory**
  - Excitatory: **256**
  - Inhibitory: **64**
- **Readout (count head)**: **K+1** classes (e.g., 0–4 ⇒ 5)

_Total non-sensory: 1,024 excitatory / 256 inhibitory (≈ 80/20 E/I)._

> For 64×64, roughly 4× these counts; for 128×128, 16×.

---

## 2) Connectivity sketch

- **Sensory → L1 (RF ~5×5)**: sparse 10–20% fan-in per L1 neuron; ON and OFF both project.
- **L1 → L2 (pool stride 2–4)**: build motion selectivity via **delayed** connections (short positive delay in the preferred direction).
- **L2 ↔ Integrator**: bidirectional sparse 5–10%; integrator neurons have **longer time constant** (slow decay) to accumulate evidence.
- **Inhibitory**: local lateral inhibition within each layer (surround kernels) plus a mild global inhibitory projection to stabilise the integrator.
- **Integrator → Readout**: dense to a small softmax head (`K+1`). Optionally concatenate L2→Readout for a “fast” path.

---

## 3) Time constants & injection

- Suggested **sensory injection** (from `.seq`): **200.0** (tune ±2×).
- L1 membrane/decay: **fast** (respond to bursts).
- L2: **medium** (bridge brief gaps).
- Integrator: **slow** (hold count across ~200–400 ms).
- Add small jitter to delays to discourage brittle synchrony.

---

## 4) Plasticity (optional)

- Hebbian/STDP in **Sensory→L1** and **L1→L2** helps discover edges/motion without labels.
- Use supervised (or reinforcement) shaping on **Integrator→Readout** for the count task.
- If the network under-counts: increase Integrator size or time constant; if it over-counts: strengthen lateral inhibition or add a learned “null” suppressor in Readout.

---

## 5) Scaling rules

Let area \(A = W·H\). With ON/OFF interleaving:

- L1_exc ≈ `0.5–1.0 × A / 4`
- L2_exc ≈ `0.25–0.5 × A / 16`
- Integrator_exc ≈ **same** as L2_exc
- Inhibitory ≈ **¼** of excitatory per layer

This keeps fan-in/fan-out reasonable as resolution grows.

---

## 6) Training recipe (suggested)

1. Start with simulator defaults: `fps=200, bin_ms=10, event_threshold=0.12, max_objects=3`.
2. Train the count head for 10–20 epochs with cross-entropy on `labels_counts.csv`.
3. If validation accuracy stalls:
   - double Integrator exc (256→512),
   - or increase `brightness_max` to strengthen edges,
   - or raise `max-events-per-bin` to avoid clipping.
4. For robustness, add clips with `bg_drift` and random `polarity` dropouts (on/off only).

**Expected result (32×32, K=4)**: >90% tick-wise accuracy after short training on a few thousand clips.
