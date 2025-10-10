# Neomorphic Hardware Concept (v0)

## Cores & State

- Many small neuron cores (e.g., 1–8k neurons/core).
- Local SRAM for synapses & states (V, θ, leak, refractory, traces).
- Synapse sign flexible; optional per-edge delay slots (1..N ticks).

## Event Network-on-Chip (NoC)

- **AER** packets: (src_id, fanout class). Hardware multicast.
- Hierarchical fabric (tile crossbar → cluster mesh → chip ring).
- Credit-based flow control; per-dest event queues (throttle bursts).
- Retinotopic placement for locality; long-range edges rare and budgeted.

## Plasticity

- On-core **eligibility traces**; **three-factor** updates via broadcast reward/neuromodulator.
- Intrinsic plasticity microcode (rate homeostasis via θ/leak).
- Background structural tasks (prune/add) with safety limits.

## Timing

- Global tick (or bounded async windows) with deterministic update ordering.
- Optional dynamic time scaling with input load.

## Energy & Performance

- Work ∝ **#events × path length**; near-zero when scenes are static.
- µs-scale local decisions (few ticks latency).
- Leverage on-device learning to avoid off-chip retraining.

## Suggested Packet Fields

- 32-bit `src`, 16-bit `fanout_class`, 8-bit `delay_slot`, 8-bit `priority`.
- Optional `modulator_id` for reward broadcasts.

## Floorplanning Notes

- Tile = FeatureColumn array + small pool/collision domain.
- Keep fan-out ≤64 where possible; encourage nearest-neighbor wiring.
