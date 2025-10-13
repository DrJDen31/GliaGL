import argparse, json, pathlib
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, PillowWriter
from matplotlib.patches import Rectangle
from matplotlib.lines import Line2D

def load_clip(run_dir: pathlib.Path, clip: int):
    npz = np.load(run_dir / "npz" / f"clip_{clip:05d}.npz")
    frames = npz["frames"]; on = npz["on"]; off = npz["off"]
    cfg = json.load(open(run_dir / "config.json", "r"))
    return frames, on, off, cfg

def group_events_by_frame(events, T):
    buckets = [[] for _ in range(T)]
    if events.size == 0: return buckets
    for t, y, x in events.tolist():
        if 0 <= t < T: buckets[int(t)].append((int(y), int(x)))
    return buckets

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="indir", required=True)
    ap.add_argument("--clip", type=int, default=0)
    ap.add_argument("--speed", type=float, default=1.0)
    ap.add_argument("--show-events", dest="show_events", action="store_true")
    ap.add_argument("--save", type=str, default=None)
    ap.add_argument("--dpi", type=int, default=120)
    ap.add_argument("--figsize", type=float, nargs=2, default=(6.0,6.8))
    ap.add_argument("--marker-size", type=float, default=4.0)
    ap.add_argument("--theme", type=str, default="dark", choices=["light","dark"])
    ap.add_argument("--blit", action="store_true")
    args = ap.parse_args()

    run_dir = pathlib.Path(args.indir)
    frames, on, off, cfg = load_clip(run_dir, args.clip)
    T, H, W = frames.shape
    fps = float(cfg.get("fps", 200.0))
    bin_ms = float(cfg.get("bin_ms", 10))
    interval_ms = (1000.0 / fps) / max(args.speed, 1e-6)

    on_by_t = group_events_by_frame(on, T)
    off_by_t = group_events_by_frame(off, T)

    # Layout: image + progress bar
    fig = plt.figure(figsize=tuple(args.figsize), dpi=args.dpi)
    ax_img = fig.add_axes([0.05, 0.20, 0.90, 0.75])
    ax_bar = fig.add_axes([0.05, 0.10, 0.90, 0.05])

    # Theme
    if args.theme == "dark":
        fig.patch.set_facecolor("#111111")
        bar_bg, bar_fg, text_color, border_col = "#333333", "#02d5ff", "#eaeaea", "#eaeaea"
    else:
        fig.patch.set_facecolor("#ffffff")
        bar_bg, bar_fg, text_color, border_col = "#e6e6e6", "#2b7cff", "#111111", "#888888"

    # Image + optional events
    im = ax_img.imshow(frames[0], vmin=0, vmax=1, animated=args.blit)
    ax_img.set_axis_off()

    scat_on = scat_off = None
    if args.show_events:
        scat_on = ax_img.scatter([], [], s=args.marker_size, animated=args.blit, label="ON")
        scat_off = ax_img.scatter([], [], s=args.marker_size, animated=args.blit, label="OFF")
        leg = ax_img.legend(loc="upper right", facecolor="white", framealpha=0.7)
        for txt in leg.get_texts(): txt.set_color("#000000")

    # HUD: below image; works in both modes
    if args.blit:
        hud = ax_img.text(0.02, -0.12, "", transform=ax_img.transAxes, clip_on=False,
                          ha="left", va="bottom", family="monospace", fontsize=10,
                          color=text_color, animated=True)
    else:
        hud = fig.text(0.05, 0.04, "", ha="left", va="bottom", family="monospace",
                       fontsize=10, color=text_color)

    # Progress bar track + fill
    ax_bar.set_xlim(0, 1); ax_bar.set_ylim(0, 1)
    ax_bar.set_xticks([]); ax_bar.set_yticks([])
    for s in ax_bar.spines.values(): s.set_visible(False)

    bg_rect = Rectangle((0.00, 0.15), 1.00, 0.70,
                        transform=ax_bar.transAxes, clip_on=False,
                        facecolor=bar_bg, edgecolor="none", zorder=1)
    fg_rect = Rectangle((0.00, 0.15), 0.00, 0.70,
                        transform=ax_bar.transAxes, clip_on=False,
                        facecolor=bar_fg, edgecolor="none", zorder=2)
    ax_bar.add_patch(bg_rect); ax_bar.add_patch(fg_rect)
    fg_rect.set_animated(args.blit)

    # ---- Uniform border (filled rects) ----
    if args.theme == "dark":
        border_col = "#eaeaea"
        th = 0.010
        draw_side_borders = True
    else:
        border_col = "#aaaaaa"   # lighter for top/bottom
        th = 0.010
        draw_side_borders = False   # <-- hide side borders in light mode

    y0, y1 = 0.15, 0.85

    # Top / bottom borders (always)
    top_border = Rectangle((0.0, y1 - th), 1.0, th,
                        transform=ax_bar.transAxes, clip_on=False,
                        facecolor=border_col, edgecolor="none", zorder=4)
    bot_border = Rectangle((0.0, y0), 1.0, th,
                        transform=ax_bar.transAxes, clip_on=False,
                        facecolor=border_col, edgecolor="none", zorder=4)
    ax_bar.add_patch(top_border)
    ax_bar.add_patch(bot_border)

    # Optional left/right borders (dark theme only)
    left_border = right_border = None
    if draw_side_borders:
        left_border  = Rectangle((0.0, y0), th, (y1 - y0),
                                transform=ax_bar.transAxes, clip_on=False,
                                facecolor=border_col, edgecolor="none", zorder=4)
        right_border = Rectangle((1.0 - th, y0), th, (y1 - y0),
                                transform=ax_bar.transAxes, clip_on=False,
                                facecolor=border_col, edgecolor="none", zorder=4)
        ax_bar.add_patch(left_border)
        ax_bar.add_patch(right_border)

    # If blitting, include whichever borders exist
    if args.blit:
        animated_borders = [top_border, bot_border]
        if left_border is not None:  animated_borders += [left_border]
        if right_border is not None: animated_borders += [right_border]
        for p in animated_borders: p.set_animated(True)

    empty_offsets = np.empty((0, 2))

    def update(t):
        im.set_array(frames[t])

        if args.show_events:
            if on_by_t[t]:
                ys, xs = zip(*on_by_t[t]); scat_on.set_offsets(np.c_[xs, ys])
                scat_on.set_sizes([args.marker_size] * len(xs))
            else:
                scat_on.set_offsets(empty_offsets)
            if off_by_t[t]:
                ys, xs = zip(*off_by_t[t]); scat_off.set_offsets(np.c_[xs, ys])
                scat_off.set_sizes([args.marker_size] * len(xs))
            else:
                scat_off.set_offsets(empty_offsets)

        time_ms = (t / fps) * 1000.0
        tick = int(time_ms // bin_ms)
        tick_total = int(np.ceil((T / fps) * 1000.0 / bin_ms))
        hud.set_text(f"frame: {t:5d} / {T-1}    time: {time_ms:7.2f} ms    tick: {tick:5d} / {tick_total}")

        prog = min(1.0, max(0.0, t / max(1, T-1)))
        fg_rect.set_width(prog)

        if args.blit:
            arts = [im, fg_rect, hud, top_border, bot_border]
            if left_border is not None:  arts.append(left_border)
            if right_border is not None: arts.append(right_border)
            if args.show_events: arts += [scat_on, scat_off]
            return arts
        return ()

    anim = FuncAnimation(fig, update, frames=T, interval=interval_ms, blit=args.blit)

    if args.save:
        out = pathlib.Path(args.save)
        if out.suffix.lower() == ".gif":
            anim.save(str(out), writer=PillowWriter(fps=max(1, int(fps*args.speed))))
        else:
            try:
                from matplotlib.animation import FFMpegWriter
                anim.save(str(out), writer=FFMpegWriter(fps=max(1, int(fps*args.speed))))
            except Exception as e:
                alt = out.with_suffix(".gif")
                print(f"FFmpeg unavailable ({e}); falling back to GIF: {alt}")
                anim.save(str(alt), writer=PillowWriter(fps=max(1, int(fps*args.speed))))
    else:
        plt.show()

if __name__ == "__main__":
    main()
