"""Demonstration script for the PyTorch Glia model using the XOR baseline network."""

from pathlib import Path
from typing import Dict, List

import torch

from glia_net import GliaTorch


def load_events(inputs: List[Dict[str, float]], ticks: int) -> List[Dict[str, float]]:
    """Expand sparse sensory pulses into tick-by-tick event dictionaries.

    Each element of ``inputs`` represents a stimulus injected at the start of a
    multi-tick window. The helper pads the remaining ticks with empty dicts so
    ``GliaTorch.run`` observes the same cadence as the C++ evaluator.
    """

    events: List[Dict[str, float]] = []
    for stimulus in inputs:
        events.append(stimulus)
        for _ in range(ticks - 1):
            events.append({})
    return events


def run_example() -> None:
    """Load the handcrafted XOR network and print spike/membrane traces."""

    net_path = Path(__file__).resolve().parent.parent / "examples" / "xor" / "xor_baseline.net"
    model = GliaTorch.from_net(net_path)

    # Driving sequence: excite each sensory neuron individually, then jointly,
    # followed by a quiescent window to observe persistence/decay behaviour.
    stimuli = [
        {"S0": 120.0},
        {"S1": 120.0},
        {"S0": 120.0, "S1": 120.0},
        {},
    ]

    events = load_events(stimuli, ticks=6)
    outputs = model.run(events)

    output_indices = model.get_indices(["O0", "O1"])
    print("spike trace:")
    print(outputs[:, output_indices])
    print("final membrane:")
    print(model.get_membrane()[output_indices])


if __name__ == "__main__":
    torch.set_printoptions(precision=3, sci_mode=False)
    run_example()
