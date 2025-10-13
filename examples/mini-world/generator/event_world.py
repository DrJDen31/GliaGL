import os, math, random, argparse, json, pathlib
from dataclasses import dataclass
from typing import List, Tuple, Dict
import numpy as np
from seq_writer import write_seq, SeqHeader

# ---------------------------
# Helpers
# ---------------------------

def clamp(v, lo, hi):
    # Works for scalars or numpy arrays
    return np.clip(v, lo, hi)

def seed_all(seed:int):
    random.seed(seed)
    np.random.seed(seed % (2**32 - 1))

# ---------------------------
# World + rendering
# ---------------------------

@dataclass
class ObjectCfg:
    shape: str          # "circle" or "square"
    size_px: int        # radius (circle) or half-side (square)
    brightness: float   # 0..1
    x: float; y: float  # center
    vx: float; vy: float

class EventWorld:
    def __init__(self, width:int, height:int, fps:float,
                 event_threshold:float=0.12,
                 polarity:str="both",
                 bg_drift:float=0.0):
        self.W, self.H = width, height
        self.fps = fps
        self.dt = 1.0 / fps
        self.event_threshold = event_threshold
        assert polarity in ("both","on","off")
        self.polarity = polarity
        self.bg_drift = bg_drift
        self.bg_level = 0.0

    def _render(self, objs: List[ObjectCfg]) -> np.ndarray:
        """Render grayscale frame in [0,1]."""
        frame = np.full((self.H, self.W), float(clamp(self.bg_level, 0.0, 1.0)), dtype=np.float32)
        yy, xx = np.mgrid[0:self.H, 0:self.W]
        for o in objs:
            if o.shape == "circle":
                mask = (xx - o.x)**2 + (yy - o.y)**2 <= (o.size_px)**2
            else:  # square
                mask = (np.abs(xx - o.x) <= o.size_px) & (np.abs(yy - o.y) <= o.size_px)
            # clamp() handles ndarray fine now
            frame[mask] = clamp(frame[mask] + o.brightness, 0.0, 1.0)
        return frame

    def _step_objs(self, objs: List[ObjectCfg]):
        for o in objs:
            o.x += o.vx * self.dt
            o.y += o.vy * self.dt
            # bounce on borders
            if o.x - o.size_px < 0 and o.vx < 0: o.vx *= -1
            if o.x + o.size_px > self.W-1 and o.vx > 0: o.vx *= -1
            if o.y - o.size_px < 0 and o.vy < 0: o.vy *= -1
            if o.y + o.size_px > self.H-1 and o.vy > 0: o.vy *= -1
        if self.bg_drift != 0.0:
            self.bg_level = float(clamp(self.bg_level + self.bg_drift * self.dt, 0.0, 1.0))

    def simulate(self, duration_s: float, make_objects_fn) -> Dict[str, np.ndarray]:
        """
        Returns:
          {
            "frames": [T,H,W] float32,
            "on":  [N_on, 3] int32   (t, y, x) with t in [1..T-1]
            "off": [N_off, 3] int32,
            "n_objects": int
          }
        """
        steps = int(round(duration_s * self.fps))
        objs = make_objects_fn()
        frames = []
        for _ in range(steps):
            frames.append(self._render(objs))
            self._step_objs(objs)
        frames = np.stack(frames, axis=0)  # [T,H,W]

        prev = frames[0]
        on_events = []
        off_events = []
        for t in range(1, steps):
            cur = frames[t]
            delta = cur - prev
            if self.polarity in ("both","on"):
                on_mask = delta >= self.event_threshold
                ys, xs = np.where(on_mask)
                on_events.extend([(t, int(y), int(x)) for y, x in zip(ys, xs)])
            if self.polarity in ("both","off"):
                off_mask = delta <= -self.event_threshold
                ys, xs = np.where(off_mask)
                off_events.extend([(t, int(y), int(x)) for y, x in zip(ys, xs)])
            prev = cur

        return {
            "frames": frames.astype(np.float32),
            "on": np.array(on_events, dtype=np.int32) if len(on_events) else np.zeros((0,3), dtype=np.int32),
            "off": np.array(off_events, dtype=np.int32) if len(off_events) else np.zeros((0,3), dtype=np.int32),
            "n_objects": len(objs),
        }

# ---------------------------
# Object sampling
# ---------------------------

def sample_objects(W,H,max_objects,size_min,size_max,speed_min,speed_max,bmin,bmax) -> List[ObjectCfg]:
    n = random.randint(1, max_objects)
    objs: List[ObjectCfg] = []
    for _ in range(n):
        shape = random.choice(["circle","square"])
        sz = random.randint(size_min, size_max)
        b = random.uniform(bmin, bmax)
        # keep away from borders a bit
        x = random.uniform(sz+1, W-1-sz-1)
        y = random.uniform(sz+1, H-1-sz-1)
        speed = random.uniform(speed_min, speed_max)
        th = random.uniform(0, 2*math.pi)
        vx = speed * math.cos(th)
        vy = speed * math.sin(th)
        objs.append(ObjectCfg(shape, sz, b, x, y, vx, vy))
    return objs

# ---------------------------
# Mapping to sensory neurons / .seq rows
# ---------------------------

def sensory_id(x, y, pol, W, id_prefix="S"):
    # pol: 0=ON, 1=OFF
    idx = (y * W + x) * 2 + pol
    return f"{id_prefix}{idx}"

def events_to_seq_rows(on_events: np.ndarray, off_events: np.ndarray,
                       bin_ms: int, fps: float, W: int,
                       id_prefix: str, injection_scale: float,
                       max_events_per_bin: int | None = None) -> List[Tuple[int,str,float]]:
    """
    Convert (t,y,x) with t in frame index into Glia .seq (<tick> <sid> <injection>).
    1 tick = bin_ms milliseconds. time_ms = (t/fps)*1000 → tick = floor(time_ms/bin_ms).
    """
    rows: List[Tuple[int,str,float]] = []

    def add(batch: np.ndarray, pol: int):
        for (t,y,x) in batch.tolist():
            time_ms = (t / fps) * 1000.0
            tick = int(time_ms // bin_ms)
            rows.append((tick, sensory_id(x, y, pol, W, id_prefix), float(injection_scale)))

    if on_events.size:  add(on_events, 0)
    if off_events.size: add(off_events, 1)

    rows.sort(key=lambda r: (r[0], r[1]))
    if max_events_per_bin is not None:
        capped = []
        counter: Dict[Tuple[int,str], int] = {}
        for r in rows:
            key = (r[0], r[1])
            c = counter.get(key, 0)
            if c < max_events_per_bin:
                capped.append(r)
                counter[key] = c + 1
        rows = capped
    return rows

# ---------------------------
# CLI
# ---------------------------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=str, required=True)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--clips", type=int, default=5)

    # world
    ap.add_argument("--width", type=int, default=32)
    ap.add_argument("--height", type=int, default=32)
    ap.add_argument("--fps", type=float, default=200.0)
    ap.add_argument("--duration-s", type=float, default=3.0)

    # events / binning
    ap.add_argument("--bin-ms", type=int, default=10)
    ap.add_argument("--event-threshold", type=float, default=0.12)
    ap.add_argument("--polarity", type=str, default="both", choices=["both","on","off"])
    ap.add_argument("--max-events-per-bin", type=int, default=None)

    # objects
    ap.add_argument("--max-objects", type=int, default=3)
    ap.add_argument("--size-min", type=int, default=3)
    ap.add_argument("--size-max", type=int, default=6)
    ap.add_argument("--speed-min", type=float, default=20.0)  # px/s
    ap.add_argument("--speed-max", type=float, default=80.0)  # px/s
    ap.add_argument("--brightness-min", type=float, default=0.6)
    ap.add_argument("--brightness-max", type=float, default=1.0)
    ap.add_argument("--bg-drift", type=float, default=0.0)

    # mapping/export
    ap.add_argument("--id-prefix", type=str, default="S")
    ap.add_argument("--injection-scale", type=float, default=200.0)

    args = ap.parse_args()
    seed_all(args.seed)

    out = pathlib.Path(args.out)
    (out / "seq").mkdir(parents=True, exist_ok=True)
    (out / "labels").mkdir(parents=True, exist_ok=True)
    (out / "npz").mkdir(parents=True, exist_ok=True)

    with open(out / "config.json", "w", encoding="utf-8") as f:
        json.dump(vars(args), f, indent=2)

    world = EventWorld(args.width, args.height, args.fps,
                       args.event_threshold, args.polarity, args.bg_drift)

    # labels header
    labels_rows = [("clip_id","tick","true_count")]

    for clip_id in range(args.clips):
        def make_objs():
            return sample_objects(args.width, args.height, args.max_objects,
                                  args.size_min, args.size_max,
                                  args.speed_min, args.speed_max,
                                  args.brightness_min, args.brightness_max)

        sim = world.simulate(args.duration_s, make_objs)

        # Save raw for visualization/debug
        np.savez_compressed(out / "npz" / f"clip_{clip_id:05d}.npz",
                            frames=sim["frames"], on=sim["on"], off=sim["off"])
        with open(out / "npz" / f"clip_{clip_id:05d}.meta.json", "w", encoding="utf-8") as f:
            json.dump({"n_objects": sim["n_objects"]}, f)

        # Export .seq
        rows = events_to_seq_rows(sim["on"], sim["off"],
                                  args.bin_ms, args.fps, args.width,
                                  args.id_prefix, args.injection_scale,
                                  args.max_events_per_bin)
        duration_ticks = int(math.ceil((args.duration_s*1000.0) / args.bin_ms))
        write_seq(str(out / "seq" / f"clip_{clip_id:05d}.seq"),
                  SeqHeader(duration_ticks=duration_ticks, loop=False),
                  rows)

        # Labels (count per tick, using the exact n_objects from this sim)
        for tick in range(duration_ticks):
            labels_rows.append((str(clip_id), str(tick), str(sim["n_objects"])))

    # Write labels CSV
    import csv
    with open(out / "labels" / "labels_counts.csv", "w", newline="", encoding="utf-8") as f:
        csv.writer(f).writerows(labels_rows)

if __name__ == "__main__":
    main()
