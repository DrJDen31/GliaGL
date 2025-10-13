# Event-Camera Mini-World — Config & File Formats

This document explains all knobs for the event-camera mini-world generator and how they map to the files Glia consumes.

---

## 1) Simulator overview

We simulate a small 2-D world with bright objects moving over a darker background. From consecutive frames we generate **DVS-style ON/OFF events** whenever the pixel intensity changes by at least a threshold.

**Key ideas**

- Each clip is `duration_s` seconds at `fps` frames per second → `T = round(duration_s * fps)` frames.
- ON event at `(t, y, x)` means intensity increased by `>= event_threshold` from frame `t-1` to `t`.
- OFF event → decreased by `<= -event_threshold`.
- Events are mapped to **sensory neuron IDs** (`S<index>`) and **binned into ticks** of length `bin_ms` milliseconds to produce `.seq` inputs for Glia.

---

## 2) CLI flags (generator/event_world.py)

```
--out                Output folder (required). Creates: seq/, labels/, npz/, config.json
--seed               RNG seed (int). Affects object count/shape/pose/motion.
--clips              Number of clips to generate.

# World
--width              Image width in pixels (W).
--height             Image height in pixels (H).
--fps                Frames per second (float).
--duration-s         Seconds per clip.

# Events / binning
--bin-ms             Tick length in ms for .seq binning (int).
--event-threshold    Per-pixel intensity delta to trigger events (0..1).
--polarity           "both" | "on" | "off".
--max-events-per-bin Optional cap per (tick, sensory id).

# Objects
--max-objects        Max objects per clip (actual n is random in [1..max]).
--size-min/max       Shape radius (circle) or half-side (square) in px.
--speed-min/max      Pixel speed (px/s).
--brightness-min/max Additive brightness (0..1) for object interior.
--bg-drift           Optional slow drift of background brightness per second.

# Mapping / export
--id-prefix          Prefix for sensory neuron IDs in .seq (default "S").
--injection-scale    Value for third column in .seq (e.g., 200.0).
```

### Practical defaults

- `fps=200`, `bin_ms=10` ⇒ **20 ticks / second**, easy mental math.
- `event_threshold=0.12` is conservative; lower if you want denser events.
- `max-events-per-bin` helps tame bursty frames if you use very small `bin_ms`.

---

## 3) Dataset layout

```
<out>/
  config.json                 # copy of all flags
  seq/
    clip_00000.seq            # Glia input (<tick> <sid> <inj>)
    clip_00001.seq
    ...
  labels/
    labels_counts.csv         # per-tick labels for "count" head
  npz/
    clip_00000.npz            # frames, on, off (raw simulator tensors)
    clip_00000.meta.json      # {"n_objects": int}
    ...
```

### `config.json`

Contains: `width, height, fps, duration_s, bin_ms, ...`. Use this to derive tick ↔ time mappings in tools/vis.

---

## 4) The `.seq` file format (for Glia)

Each `.seq` is plain text with a header and event rows:

```
DURATION <n_ticks>
LOOP <true|false>

<tick> <sensory_id> <injection>
```

- **Tick**: integer `floor( (t / fps) * 1000 / bin_ms )`, where `t` is the _frame index_ of an event.
- **Sensory ID**: `S<index>` by default. Index mapping described below.
- **Injection**: constant amplitude per event (default `injection_scale`). You can change this if you want magnitude to depend on, e.g., delta size or per-pixel energy.

> Header `DURATION` is computed from `duration_s` and `bin_ms`: `ceil(duration_s * 1000 / bin_ms)`.

---

## 5) Pixel → Sensory neuron mapping

We interleave ON/OFF neurons per pixel in **row-major** order:

```
index = (y * W + x) * 2 + pol     # pol: 0 = ON, 1 = OFF
sensory id = f"{id_prefix}{index}" # e.g., S0, S1, S2, ...
```

This mapping is consistent across generator, visualizer, and the .seq writer.

---

## 6) Labels

For the starter task we label **object count** per tick:

```
labels_counts.csv   # CSV with header
clip_id,tick,true_count
0,0,2
0,1,2
...
```

- `true_count` is the _sampled number of objects in that clip_.
- We keep it constant over ticks of a clip (objects do not spawn/despawn mid-clip in the default generator). If you later implement birth/death events, emit per-tick counts instead.

---

## 7) Visualization tools

### Static viewer

`python generator/visualize_world.py --in <out> --clip 0`

- Shows first/middle frames and event rasters (frame index on x-axis).

### Animation viewer

`python generator/animate_world.py --in <out> --clip 0 [--show-events] [--theme light|dark] [--blit]`

- Plays frames as an animation.
- Optional event dots per frame.
- HUD below the image shows **frame, time (ms), tick (current/total)**.
- Progress bar indicates position through the clip.
- `--save scene.gif` or `scene.mp4` to export.

> On some Tk backends, `--blit` can be finicky; it’s off by default.

---

## 8) Reproducibility

- Use `--seed` to deterministically regenerate the same dataset.
- For exact re-runs across machines, keep `config.json` with the seed and the binary `npz` files next to the `.seq` you trained on.

---

## 9) Tips & gotchas

- If you increase `fps` without adjusting `bin_ms`, you’ll pack more frames per tick → fewer emitted events after binning. Tune jointly.
- Very small `event_threshold` can blow up event counts. Use `--max-events-per-bin` as a safety valve or bump the threshold.
- Larger `brightness_max` gives stronger edges (more ON/OFF when moving). Balance this with `injection_scale` so your network input isn’t saturated.
- If you turn on `bg_drift`, expect more OFF/ON noise as the whole scene shifts slowly—good for robustness tests.
