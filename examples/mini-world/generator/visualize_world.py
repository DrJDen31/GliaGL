import argparse, pathlib, numpy as np
import matplotlib.pyplot as plt

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="indir", type=str, required=True)
    ap.add_argument("--clip", type=int, default=0)
    args = ap.parse_args()
    indir = pathlib.Path(args.indir)
    data = np.load(indir/"npz"/f"clip_{args.clip:05d}.npz")
    frames, on, off = data["frames"], data["on"], data["off"]
    T,H,W = frames.shape; mid = T//2

    plt.figure()
    plt.title(f"Frames t=0 and t={mid}")
    plt.subplot(1,2,1); plt.imshow(frames[0], vmin=0, vmax=1); plt.axis("off")
    plt.subplot(1,2,2); plt.imshow(frames[mid], vmin=0, vmax=1); plt.axis("off")
    plt.tight_layout(); plt.show()

    def raster(events, title):
        if events.size == 0: print("No events for", title); return
        t = events[:,0]; yx = events[:,1]*W + events[:,2]
        plt.figure(); plt.scatter(t, yx, s=1); plt.xlabel("Frame index"); plt.ylabel("Pixel index"); plt.title(title); plt.tight_layout(); plt.show()
    raster(on, "ON events"); raster(off, "OFF events")

if __name__ == "__main__":
    main()

