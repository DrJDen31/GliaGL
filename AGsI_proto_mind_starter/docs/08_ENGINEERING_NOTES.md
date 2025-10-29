# 08 — Engineering Notes (starting constants)

- Index size: 512; k=24; τ_e=400 ms; η=2e-4
- Cortex size: 2048; k=64; τ_e=300 ms; η=1e-5..3e-5
- STDP: τ+ = 20 ms (A+=1.0), τ- = 40 ms (A-=0.8); scale base increments by 1e-3 pretrain, 1e-4 online.
- Norm: L2→1 per post every 25 ms; clamp w∈[0,1].
- Gates: token bucket = 3 pulses capacity; refill 1 pulse/10 s; 120 ms window.
- Predictive loop: low‑gain feedback; measure residual energy each step.
- Winner fatigue: raise threshold slightly for recent winners to improve coverage.
