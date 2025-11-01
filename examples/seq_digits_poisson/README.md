# Handwritten Digits Classification (Poisson-Encoded)

Train spiking neural networks to classify handwritten digits using Poisson-encoded spike sequences.

---

## Overview

This example demonstrates digit classification (0-9) using spiking neural networks with:
- **Poisson-encoded input**: Handwritten digits converted to spike trains
- **Gradient-based training**: Backpropagation through time for SNNs
- **Rate-coded readout**: Classification from firing rates

---

## Dataset

The dataset consists of:
- **Training set**: `data/train/` (sequences + labels)
- **Test set**: `data/test/` (sequences + labels)
- **Format**: `.seq` files with Poisson spike trains
- **Labels**: CSV files mapping filenames to digit classes (0-9)

Expected structure:
```
data/
├── train/
│   ├── labels.csv
│   └── *.seq (many sequence files)
└── test/
    ├── labels.csv
    └── *.seq
```

---

## Quick Start

### Using Python (Recommended)

```bash
# Train on digit sequences
python train_digits.py --net nets/readout.net --data data --epochs 10

# With custom parameters
python train_digits.py \
    --net nets/readout.net \
    --data data \
    --epochs 20 \
    --batch-size 32 \
    --lr 0.02 \
    --save nets/my_trained_model.net
```

### Using C++ (Legacy)

```bash
cd src
# Compile (see main.cpp for build instructions)
# Run training
./train_digits --net ../nets/readout.net --data ../data --epochs 10
```

---

## Network Architectures

### `readout.net`
Simple readout layer:
- Input neurons for pixel intensities
- Output neurons O0-O9 for digit classes
- Minimal architecture for fast training

### `readout_hidden.net`
Hidden layer architecture:
- Input layer
- Hidden layer for feature extraction
- Output layer O0-O9
- Better accuracy, slower training

---

## Training Options

### Python Script Options

```bash
python train_digits.py --help

Options:
  --net PATH          Network file (default: nets/readout.net)
  --data PATH         Data root directory (default: data)
  --epochs N          Training epochs (default: 10)
  --batch-size N      Batch size (default: 16)
  --lr FLOAT          Learning rate (default: 0.01)
  --warmup N          Warmup ticks (default: 20)
  --eval-ticks N      Evaluation window (default: 80)
  --save PATH         Save trained network (default: nets/digits_trained.net)
  --results PATH      Results directory (default: results)
  --seed N            Random seed (default: 42)
  --verbose           Verbose evaluation output
```

---

## Output

### Trained Network
- Saved to `nets/digits_trained.net` (or custom path)
- Can be loaded with `glia.Network.from_file()`

### Metrics (`results/metrics.json`)
```json
{
  "test_accuracy_before": 0.12,
  "test_accuracy_after": 0.89,
  "train_accuracy_final": 0.95,
  "epochs": 10,
  "improvement": 0.77,
  "confusion_matrix": [[...]]
}
```

### Predictions (`results/predictions_test.csv`)
```csv
target,predicted,correct,margin
O0,O0,True,0.234
O1,O1,True,0.456
O2,O7,False,0.012
...
```

### Console Output
- Training progress per epoch
- Confusion matrix
- Per-digit accuracy
- Overall metrics

---

## Results

### Typical Performance

| Network | Epochs | Test Accuracy | Training Time |
|---------|--------|---------------|---------------|
| readout | 10 | ~85-90% | ~2-5 minutes |
| readout_hidden | 20 | ~90-95% | ~10-20 minutes |

Performance varies with:
- Learning rate
- Batch size
- Network initialization
- Sequence encoding quality

---

## File Organization

```
seq_digits_poisson/
├── README.md              # This file
├── train_digits.py        # Python training script
├── analyze.py             # Results analysis script
├── data/                  # Dataset (3000+ sequences)
│   ├── train/
│   └── test/
├── nets/                  # Network files
│   ├── readout.net
│   ├── readout_hidden.net
│   └── digits_trained.net (after training)
├── results/               # Training outputs
│   ├── metrics.json
│   ├── predictions_test.csv
│   └── *.png (if visualization run)
└── src/                   # C++ source (legacy)
    └── main.cpp
```

---

## Analysis

Analyze training results:

```bash
python analyze.py --results results --data data/test
```

Generates:
- Confusion matrix visualization
- Per-digit accuracy plots
- Misclassification analysis
- Correct/incorrect example grids

---

## Advanced Usage

### Custom Network Architecture

Create your own network file:

```
# my_network.net
# 784 sensory (28x28 pixels) + hidden + 10 outputs

NEURON S0 100.0 1.0
...
NEURON H0 45.0 0.9
...
NEURON O0 20.0 1.0
...

SYNAPSE S0 H0 0.5
...
```

Then train:
```bash
python train_digits.py --net my_network.net
```

### Hyperparameter Tuning

Grid search example:

```bash
for lr in 0.005 0.01 0.02 0.05; do
    for bs in 8 16 32; do
        python train_digits.py \
            --lr $lr \
            --batch-size $bs \
            --save nets/trained_lr${lr}_bs${bs}.net \
            --results results/lr${lr}_bs${bs}
    done
done
```

### Transfer Learning

Fine-tune a pretrained network:

```bash
# Train base model
python train_digits.py --epochs 10 --save nets/base.net

# Fine-tune with different learning rate
python train_digits.py \
    --net nets/base.net \
    --epochs 5 \
    --lr 0.001 \
    --save nets/finetuned.net
```

---

## Troubleshooting

### "Labels file not found"
Ensure data structure matches expected format:
```
data/train/labels.csv
data/test/labels.csv
```

### Low Accuracy
Try:
- Increase epochs (`--epochs 20`)
- Adjust learning rate (`--lr 0.02` or `--lr 0.005`)
- Use larger network (`readout_hidden.net`)
- Check dataset quality

### Slow Training
- Reduce batch size (`--batch-size 8`)
- Use simpler network (`readout.net`)
- Reduce eval window (`--eval-ticks 50`)

---

## Next Steps

- **Experiment** with different architectures
- **Try** evolutionary training (see evolution examples)
- **Visualize** network activity during classification
- **Compare** with other examples (3class, xor)

---

## References

- Main documentation: `docs/user-guide/`
- API reference: `docs/user-guide/API_REFERENCE.md`
- Training guide: `docs/user-guide/ADVANCED_USAGE.md`
