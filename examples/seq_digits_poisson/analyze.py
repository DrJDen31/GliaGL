import argparse
import csv
import os
from collections import defaultdict

import matplotlib.pyplot as plt
import numpy as np


def read_predictions_csv(path):
    rows = []
    with open(path, 'r', newline='') as f:
        r = csv.DictReader(f)
        for row in r:
            rows.append(row)
    return rows


def id_to_label(s):
    if s and s[0] in ('O','o'):
        try:
            return int(s[1:])
        except Exception:
            return -1
    try:
        return int(s)
    except Exception:
        return -1


def build_confusion(rows, num_classes=10):
    cm = np.zeros((num_classes, num_classes), dtype=np.int64)
    for row in rows:
        t = id_to_label(row.get('true', ''))
        p = id_to_label(row.get('pred', ''))
        if 0 <= t < num_classes and 0 <= p < num_classes:
            cm[t, p] += 1
    return cm


def load_seq_grid(seq_path, size=8):
    vals = defaultdict(float)
    try:
        with open(seq_path, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                toks = line.split()
                if toks[0] == 'EVENT':
                    # EVENT tick id value
                    if len(toks) >= 4:
                        sid = toks[2]
                        v = float(toks[3])
                        vals[sid] += v
                else:
                    # tick id value
                    if len(toks) >= 3 and toks[0].isdigit():
                        sid = toks[1]
                        v = float(toks[2])
                        vals[sid] += v
    except Exception:
        pass
    grid = np.zeros((size * size,), dtype=np.float32)
    for i in range(size * size):
        key = f'S{i}'
        grid[i] = vals.get(key, 0.0)
    # normalize per-sample
    if grid.max() > 0:
        grid = grid / grid.max()
    return grid.reshape(size, size)


def plot_confusion(cm, out_path):
    fig, ax = plt.subplots(figsize=(6, 5))
    im = ax.imshow(cm, cmap='Blues')
    plt.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    ax.set_xlabel('Predicted')
    ax.set_ylabel('True')
    ax.set_xticks(range(cm.shape[1]))
    ax.set_yticks(range(cm.shape[0]))
    ax.set_title('Confusion Matrix')
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_examples_grid(rows, root_dir, out_path, correct=True, max_examples=40, grid_size=8):
    sel = []
    for row in rows:
        t = row.get('true', '')
        p = row.get('pred', '')
        ok = (t == p)
        if ok == correct:
            sel.append(row)
        if len(sel) >= max_examples:
            break
    if not sel:
        return
    cols = 10
    rows_n = int(np.ceil(len(sel) / cols))
    fig, axes = plt.subplots(rows_n, cols, figsize=(cols * 1.4, rows_n * 1.4))
    if rows_n == 1:
        axes = np.array([axes])
    for idx, row in enumerate(sel):
        r = idx // cols
        c = idx % cols
        ax = axes[r, c]
        fname = row.get('filename', '')
        seqp = os.path.join(root_dir, 'test', fname)
        grid = load_seq_grid(seqp, size=grid_size)
        ax.imshow(grid, cmap='gray', vmin=0, vmax=1)
        ax.set_axis_off()
        t = id_to_label(row.get('true', ''))
        p = id_to_label(row.get('pred', ''))
        ax.set_title(f'{t}->{p}', fontsize=8)
    # turn off remaining axes
    for idx in range(len(sel), rows_n * cols):
        r = idx // cols
        c = idx % cols
        axes[r, c].set_axis_off()
    title = 'Correct' if correct else 'Misclassified'
    fig.suptitle(f'{title} examples', fontsize=12)
    fig.tight_layout(rect=[0, 0.03, 1, 0.95])
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def read_metrics_csv(path):
    xs, loss, acc = [], [], []
    try:
        with open(path, 'r', newline='') as f:
            r = csv.DictReader(f)
            for row in r:
                xs.append(int(row.get('epoch', len(xs) + 1)))
                loss.append(float(row.get('loss', 0.0)))
                acc.append(float(row.get('accuracy', 0.0)))
    except Exception:
        pass
    return np.array(xs), np.array(loss), np.array(acc)


def plot_metrics(xs, loss, acc, out_path):
    if xs.size == 0:
        return
    fig, ax1 = plt.subplots(figsize=(7, 4))
    l1 = ax1.plot(xs, loss, 'r-', label='Loss')
    ax1.set_xlabel('Epoch')
    ax1.set_ylabel('Loss', color='r')
    ax2 = ax1.twinx()
    l2 = ax2.plot(xs, acc, 'b-', label='Accuracy')
    ax2.set_ylabel('Accuracy', color='b')
    ln = l1 + l2
    labs = [l.get_label() for l in ln]
    ax1.legend(ln, labs, loc='best')
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    ap.add_argument('--pred_csv', required=True)
    ap.add_argument('--metrics_csv', default='')
    ap.add_argument('--out_dir', default='.')
    args = ap.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)

    rows = read_predictions_csv(args.pred_csv)
    cm = build_confusion(rows)
    total = cm.sum()
    acc = (np.trace(cm) / total) if total > 0 else 0.0

    plot_confusion(cm, os.path.join(args.out_dir, 'confusion_matrix.png'))
    plot_examples_grid(rows, args.root, os.path.join(args.out_dir, 'correct_grid.png'), correct=True)
    plot_examples_grid(rows, args.root, os.path.join(args.out_dir, 'misclassified_grid.png'), correct=False)

    if args.metrics_csv:
        xs, loss, acc_hist = read_metrics_csv(args.metrics_csv)
        plot_metrics(xs, loss, acc_hist, os.path.join(args.out_dir, 'metrics_plot.png'))

    print(f'Confusion matrix saved to: {os.path.join(args.out_dir, "confusion_matrix.png")}')
    print(f'Example grids saved to: {os.path.join(args.out_dir, "correct_grid.png")}, {os.path.join(args.out_dir, "misclassified_grid.png")}')
    if args.metrics_csv:
        print(f'Metrics plot saved to: {os.path.join(args.out_dir, "metrics_plot.png")}')


if __name__ == '__main__':
    main()
