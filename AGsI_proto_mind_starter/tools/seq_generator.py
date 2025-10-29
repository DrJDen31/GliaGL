#!/usr/bin/env python3
"""Generate .seq trials for paired binding and recall drills.
Edit encoders & class patterns to match your setup.
"""
import random, pathlib, argparse

def emit_header(duration=200, loop=False):
    return [f"DURATION {duration}", f"LOOP {'true' if loop else 'false'}", ""]

def emit_spike(t, neuron, amp=200.0):
    return f"{t} {neuron} {amp:.1f}"

def paired_trial(image_neurons, audio_neurons, gate_window=120, duration=200):
    lines = ["# Paired binding trial"] + emit_header(duration)
    # inputs
    for t, n in enumerate(image_neurons[:100]):
        lines.append(emit_spike(t, n))
    for t, n in enumerate(audio_neurons[:100]):
        lines.append(emit_spike(t, n))
    # gates
    lines.append(emit_spike(0, "PLASTICITY_GATE", 400.0))
    lines.append(emit_spike(0, "DOPAMINE", 150.0))
    lines.append(emit_spike(0, "NE", 90.0))
    lines.append(emit_spike(gate_window, "PLASTICITY_GATE", -400.0))
    lines.append("")
    return "\n".join(lines)

def recall_trial(image_only_neurons, duration=200):
    lines = ["# Recall (image only)"] + emit_header(duration)
    for t, n in enumerate(image_only_neurons[:100]):
        lines.append(emit_spike(t, n))
    return "\n".join(lines)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("out", type=pathlib.Path)
    args = parser.parse_args()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    img = [f"V{i}" for i in range(0, 60, 3)]
    aud = [f"A{i}" for i in range(0, 60, 3)]
    with args.out.open("w") as f:
        f.write(paired_trial(img, aud))
        f.write("\n\n")
        f.write(recall_trial(img))
