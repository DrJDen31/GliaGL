"""Train and validate a GliaTorch network on synthetic XOR spike sequences.

This script demonstrates an end-to-end workflow:

1. Load the handcrafted XOR baseline network.
2. Sample noisy stimulus episodes that inject currents into sensory neurons over
   time (including random delays and amplitude jitter).
3. Optimise the synaptic weight matrix via stochastic gradient descent with a
   surrogate spiking objective.
4. Report accuracy on a held-out validation split using the same temporal
   simulator employed by the Glia C++ engine.

Run with:
    python pytorch/train_xor.py
"""

from __future__ import annotations

import random
from pathlib import Path
from typing import Dict, Iterable, Tuple

import torch
import torch.nn.functional as F
from torch.utils.data import DataLoader, TensorDataset

from glia_net import GliaTorch

# ---------------------------------------------------------------------------
# Dataset utilities
# ---------------------------------------------------------------------------


def _gather_indices(model: GliaTorch, prefix: str) -> Iterable[int]:
    """Return indices of neurons whose identifier starts with ``prefix``."""

    return [idx for idx, ident in enumerate(model.ids) if ident.startswith(prefix)]


def build_episode_tensor(
    model: GliaTorch,
    bit_a: int,
    bit_b: int,
    steps: int,
    amplitude: float,
    delay_range: Tuple[int, int],
    noise_std: float,
) -> torch.Tensor:
    """Construct a single `(steps, neurons)` stimulus tensor for XOR inputs.

    Parameters
    ----------
    model:
        GliaTorch instance providing neuron identifiers.
    bit_a, bit_b:
        Binary inputs whose XOR defines the target label.
    steps:
        Number of simulated ticks for the episode.
    amplitude:
        Baseline current injected when a sensory bit is active.
    delay_range:
        Inclusive tick range from which to sample the starting pulse index. A
        non-zero delay encourages the recurrent dynamics to integrate over time.
    noise_std:
        Standard deviation of zero-mean Gaussian noise added to every tick to
        emulate jitter.
    """

    num_neurons = len(model.ids)
    tensor = torch.zeros(steps, num_neurons, dtype=torch.float32)

    sensory_indices = {ident: model.id_to_index[ident] for ident in model.ids if ident.startswith("S")}
    start_tick = random.randint(delay_range[0], delay_range[1])

    if bit_a:
        tensor[start_tick, sensory_indices["S0"]] = amplitude + random.uniform(-0.1 * amplitude, 0.1 * amplitude)
    if bit_b:
        tensor[start_tick, sensory_indices["S1"]] = amplitude + random.uniform(-0.1 * amplitude, 0.1 * amplitude)

    if noise_std > 0.0:
        tensor += torch.randn_like(tensor) * noise_std

    return tensor


def build_dataset(
    model: GliaTorch,
    samples: int,
    steps: int = 18,
    amplitude: float = 120.0,
    delay_range: Tuple[int, int] = (0, 4),
    noise_std: float = 2.5,
) -> Tuple[torch.Tensor, torch.Tensor]:
    """Generate a batch of XOR episodes and corresponding class labels."""

    episodes = []
    labels = []
    for _ in range(samples):
        bit_a = random.randint(0, 1)
        bit_b = random.randint(0, 1)
        label = bit_a ^ bit_b
        episodes.append(build_episode_tensor(model, bit_a, bit_b, steps, amplitude, delay_range, noise_std))
        labels.append(label)
    return torch.stack(episodes), torch.tensor(labels, dtype=torch.long)


# ---------------------------------------------------------------------------
# Training + evaluation loops
# ---------------------------------------------------------------------------


def compute_logits_from_spikes(spikes: torch.Tensor, model: GliaTorch) -> torch.Tensor:
    """Aggregate output neuron spike histories into classification logits."""

    output_indices = [model.id_to_index[nid] for nid in model.ids if nid.startswith("O")]
    # Average spike probability across time and rescale to improve logit spread.
    spike_rates = spikes[..., output_indices].mean(dim=-2)
    return spike_rates * 6.0


def evaluate(model: GliaTorch, loader: DataLoader, device: torch.device) -> Tuple[float, float]:
    """Evaluate accuracy and average loss on a dataloader."""

    model.eval()
    total_loss = 0.0
    total_correct = 0
    total_samples = 0
    criterion = torch.nn.CrossEntropyLoss()

    with torch.no_grad():
        for batch_events, batch_labels in loader:
            spikes, _ = model.simulate_events(batch_events.to(device))
            logits = compute_logits_from_spikes(spikes, model)
            loss = criterion(logits, batch_labels.to(device))
            total_loss += loss.item() * batch_events.size(0)
            preds = logits.argmax(dim=-1)
            total_correct += (preds.cpu() == batch_labels).sum().item()
            total_samples += batch_events.size(0)

    avg_loss = total_loss / max(1, total_samples)
    accuracy = total_correct / max(1, total_samples)
    return avg_loss, accuracy


def train_model(epochs: int = 25, batch_size: int = 32) -> None:
    """Entry-point that orchestrates dataset creation, training, and evaluation."""

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    net_path = Path(__file__).resolve().parent.parent / "examples" / "xor" / "xor_baseline.net"

    model = GliaTorch.from_net(net_path, device=device, surrogate_beta=25.0)
    model.train()

    train_events, train_labels = build_dataset(model, samples=2000)
    val_events, val_labels = build_dataset(model, samples=400)

    train_loader = DataLoader(TensorDataset(train_events, train_labels), batch_size=batch_size, shuffle=True)
    val_loader = DataLoader(TensorDataset(val_events, val_labels), batch_size=batch_size)

    optimizer = torch.optim.Adam(model.parameters(), lr=2e-3)
    criterion = torch.nn.CrossEntropyLoss()

    for epoch in range(1, epochs + 1):
        running_loss = 0.0
        for batch_events, batch_labels in train_loader:
            optimizer.zero_grad(set_to_none=True)
            spikes, _ = model.simulate_events(batch_events.to(device))
            logits = compute_logits_from_spikes(spikes, model)
            loss = criterion(logits, batch_labels.to(device))
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), max_norm=5.0)
            optimizer.step()
            running_loss += loss.item() * batch_events.size(0)

        # Validation runs with network in eval mode (binary spikes).
        val_loss, val_acc = evaluate(model, val_loader, device)
        model.train()

        avg_train_loss = running_loss / len(train_loader.dataset)
        print(f"Epoch {epoch:02d}: train_loss={avg_train_loss:.4f} val_loss={val_loss:.4f} val_acc={val_acc:.3f}")

    # Final validation summary.
    val_loss, val_acc = evaluate(model, val_loader, device)
    print(f"\nSummary: accuracy {val_acc * len(val_loader.dataset):.0f}/{len(val_loader.dataset)} ({val_acc * 100:.1f}%)")


if __name__ == "__main__":
    torch.manual_seed(42)
    random.seed(42)
    torch.set_printoptions(precision=3, sci_mode=False)
    train_model()
