# 07 — .seq Format & Control Signals

```
# Comments
DURATION <ticks>
LOOP <true|false>

<tick> <NeuronName> <amplitude>
```

**Control neurons (reserve names)**
- `PLASTICITY_GATE`
- `DOPAMINE`
- `NE` (novelty/salience)
- (Optional) `SLEEP`, `INHIBIT_GLOBAL`, `ATTEND`

**Paired trial sketch**
- 0–100 ms: image & audio spikes (sparse)
- t=0: open PLASTICITY, DOPAMINE, NE for 120 ms
- 100–150 ms: mask one modality to train reconstruction (optional)

See `seq_examples/paired_binding.seq` and `tools/seq_generator.py`.
