# Toy Examples (Reproducible Specs)

All sims assume:

- Discrete ticks, **1-tick synaptic delay**.
- On spike: neuron **resets to 0**; **no refractory unless specified**.
- Membrane update: `V = max(0, V*leak + incoming)`. (Coincidence detectors use `leak≈0`.)
- Sensory S\* are exogenous spikes (we decide which ticks are 1).

Notation:

- Thresholds: `TH[Neuron] = value`
- Weights: `W[Pre -> Post] = value` (negative → inhibitory)

---

## XOR by Rates (2 inputs → 2 outputs)

**Neurons:** `S0, S1, A (AND), O1 (XOR true), O0 (XOR false)`

**Thresholds**

- `TH[A]=90`, `TH[O1]=50`, `TH[O0]=60`

**Weights**

- `S0->O1=+60`, `S1->O1=+60`
- `S0->A=+60`, `S1->A=+60`
- `A->O1=−120`, `A->O0=+120`

**Intuition**

- Single input (10 or 01): O1 crosses 50 at t+1; A does **not** fire (only +60).
- Double input (11): both O1 and A fire at t+1; at t+2 A suppresses O1 (−120) and excites O0 (+120).

**Measured steady rates (100 ticks, no refractory, leak=1)**

- 00 → `O1=0.00`, `O0=0.00` (false)
- 10/01 → **`O1≈0.50`**, `O0≈0.49`, `A≈0.49` (argmax → true)
- 11 → `O1≈0.01`, **`O0≈0.98`**, `A≈0.99` (false)

> Tip: make `A` a true coincidence detector by setting `leak[A]=0`. Then
> 10/01 → `O1≈1.00`, `O0≈0.00`, and 11 → `O0≈1.00`, `O1≈0.00` (clean margin).

**Tick-by-tick sketch (11 case)**

- t: S0=1, S1=1
- t+1: `O1` gets +120 → fires; `A` gets +120 → fires
- t+2: `A` sends `O1: −120` (suppresses), `O0: +120` (fires)

**Readout**

- Use EMA rate per output: `r_k ← (1−α)r_k + α·[fired_k]` (α≈1/20)
- Decision: `argmax(r)` or margin rule `r_best − r_second ≥ Δ`.

---

## 3-Class One-Hot with Noise + Inhibitory Pool

**Neurons:** `S0, S1, S2, I (pool), O0, O1, O2`

**Thresholds**

- `TH[I]=40`, `TH[O0]=TH[O1]=TH[O2]=50`

**Weights**

- Feedforward: `S0->O0=+60`, `S1->O1=+60`, `S2->O2=+60`
- Pool: `O*->I=+35` (each), `I->O*=−45` (each)

**Leak**

- `leak[I]=0.8`, others 1.0

**Input**

- Pick class c∈{0,1,2}; set `Sc=1` each tick; with probability p, spuriously set other S\* to 1.

**Behavior**

- With **5–20% noise**, the correct class output retains the highest rate; pool suppresses competitors.
- Readout by `argmax(rate)` is stable; add a margin if you want abstentions.

---

## Temporal Order: AB vs BA

**Neurons:** `S0, S1, M0, M1, C01, C10, I, O_AB, O_BA`

**Thresholds**

- `TH[M0]=TH[M1]=40`, `TH[C01]=TH[C10]=90`, `TH[I]=40`, `TH[O_AB]=TH[O_BA]=50`

**Weights**

- Memory: `S0->M0=+60`, `S1->M1=+60` (leaky memory)
- Coincidence: `M0->C01=+60`, `S1->C01=+60`; `M1->C10=+60`, `S0->C10=+60`
- Readout: `C01->O_AB=+120`, `C10->O_BA=+120`
- Pool: `O_AB->I=+30`, `O_BA->I=+30`, `I->O_AB=−40`, `I->O_BA=−40`

**Leak**

- `leak[M0]=leak[M1]=0.7`, `leak[C01]=leak[C10]=0.0` (true coincidence), `leak[I]=0.8`

**Patterns (period 4)**

- AB: t%4==0 → S0=1; t%4==1 → S1=1
- BA: t%4==0 → S1=1; t%4==1 → S0=1

**Behavior**

- AB → `O_AB` dominant; BA → `O_BA` dominant. Pool ensures clean separation.
