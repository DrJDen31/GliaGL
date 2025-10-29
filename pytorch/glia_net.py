"""PyTorch reimplementation of the Glia spiking recurrent network."""

import torch
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple, Union


def _parse_net(path: Union[str, Path]) -> Tuple[List[str], List[float], List[float], List[float], List[Tuple[int, int, float]]]:
    """Parse a legacy ``.net`` configuration file into typed tensors.

    Parameters
    ----------
    path:
        Filesystem location of the Glia network description. Each non-comment
        line is expected to be either ``NEURON`` (threshold/leak/resting) or
        ``CONNECTION`` (directed synapse + weight).

    Returns
    -------
    ids, thresholds, leaks, resting, connections:
        Flat metadata lists that mirror the C++ loader. ``connections`` stores
        triples of (src_idx, dst_idx, weight) for later conversion into a dense
        adjacency matrix.
    """
    ids: List[str] = []
    thresholds: List[float] = []
    leaks: List[float] = []
    resting: List[float] = []
    connections: List[Tuple[int, int, float]] = []
    id_to_index: Dict[str, int] = {}
    for raw in Path(path).read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if parts[0] == "NEURON" and len(parts) >= 5:
            ident = parts[1]
            threshold = float(parts[2])
            leak = float(parts[3])
            rest = float(parts[4])
            if ident in id_to_index:
                idx = id_to_index[ident]
                thresholds[idx] = threshold
                leaks[idx] = leak
                resting[idx] = rest
            else:
                idx = len(ids)
                ids.append(ident)
                id_to_index[ident] = idx
                thresholds.append(threshold)
                leaks.append(leak)
                resting.append(rest)
        elif parts[0] == "CONNECTION" and len(parts) >= 4:
            src = parts[1]
            dst = parts[2]
            weight = float(parts[3])
            if src not in id_to_index or dst not in id_to_index:
                raise ValueError(f"Connection references undefined neuron: {src}->{dst}")
            connections.append((id_to_index[src], id_to_index[dst], weight))
    if not ids:
        raise ValueError("No neurons defined in net file")
    return ids, thresholds, leaks, resting, connections


class GliaTorch(torch.nn.Module):
    """Time-stepped spiking network faithful to the Glia C++ implementation.

    The module manages per-neuron membrane voltages, synaptic delays via a
    one-tick buffer, and optional surrogate gradients for differentiability.
    """

    def __init__(
        self,
        ids: Sequence[str],
        thresholds: Sequence[float],
        leaks: Sequence[float],
        resting: Sequence[float],
        connections: Sequence[Tuple[int, int, float]],
        device: Optional[Union[str, torch.device]] = None,
        dtype: Optional[torch.dtype] = None,
        surrogate_beta: Optional[float] = None,
    ) -> None:
        """Construct the recurrent core from explicit neuron metadata.

        Parameters closely mirror the pre-parsed values produced by
        :func:`_parse_net`. Buffers are registered to ensure they move across
        devices together with the module (``.to(device)``).
        """
        super().__init__()
        target_device = torch.device(device) if device is not None else torch.device("cpu")
        target_dtype = dtype if dtype is not None else torch.float32

        # Map string identifiers (``S0``/``N3``/``O1``) to contiguous indices.
        self.ids: List[str] = list(ids)
        self.id_to_index: Dict[str, int] = {ident: idx for idx, ident in enumerate(self.ids)}

        # Register neuron attributes so they participate in state dict exports.
        thr = torch.tensor(thresholds, dtype=target_dtype, device=target_device)
        leak = torch.tensor(leaks, dtype=target_dtype, device=target_device)
        rest = torch.tensor(resting, dtype=target_dtype, device=target_device)
        self.register_buffer("thresholds", thr)
        self.register_buffer("leaks", leak)
        self.register_buffer("resting", rest)

        # Dense weight matrix: row = presynaptic neuron, column = postsynaptic.
        weight = torch.zeros((len(ids), len(ids)), dtype=target_dtype, device=target_device)
        for src, dst, val in connections:
            weight[src, dst] = val
        self.weight = torch.nn.Parameter(weight)
        # Runtime state buffers mirror the C++ ``Neuron`` members.
        self.register_buffer("membrane", rest.clone())
        self.register_buffer("delta", torch.zeros_like(rest))
        self.register_buffer("on_deck", torch.zeros_like(rest))
        self.register_buffer("refractory", torch.zeros_like(rest, dtype=torch.int64))
        self.register_buffer("last_spike", torch.zeros_like(rest))
        self.surrogate_beta = surrogate_beta

    @classmethod
    def from_net(
        cls,
        path: Union[str, Path],
        device: Optional[Union[str, torch.device]] = None,
        dtype: Optional[torch.dtype] = None,
        surrogate_beta: Optional[float] = None,
    ) -> "GliaTorch":
        """Instantiate a model directly from a ``.net`` disk artifact."""
        ids, thresholds, leaks, resting, connections = _parse_net(path)
        return cls(ids, thresholds, leaks, resting, connections, device=device, dtype=dtype, surrogate_beta=surrogate_beta)

    def reset_state(self) -> None:
        """Reset dynamic buffers so the network can process a fresh episode."""
        self.membrane.copy_(self.resting)
        self.delta.zero_()
        self.on_deck.zero_()
        self.refractory.zero_()
        self.last_spike.zero_()

    def to_device(self, device: Union[str, torch.device]) -> "GliaTorch":
        """Return ``self`` placed on ``device`` for fluent chaining."""
        return self.to(device)

    def inject(self, mapping: Dict[str, float]) -> None:
        """Add raw sensory current into the synaptic staging buffer."""
        if not mapping:
            return
        for ident, value in mapping.items():
            idx = self.id_to_index[ident]
            # ``on_deck`` mirrors the C++ ``Neuron::on_deck`` delayed accumulator.
            self.on_deck[idx] = self.on_deck[idx] + float(value)

    def tick(self, sensory: Optional[Dict[str, float]] = None) -> torch.Tensor:
        """Advance the network by one tick and return spike indicators.

        The update order matches the simulator: apply staged currents, decay
        membrane via leak, threshold-and-fire, then route spikes with a one-tick
        synaptic delay.
        """
        if sensory is not None:
            self.inject(sensory)
        self.last_spike.zero_()
        # ``delta`` holds the synaptic current to apply this tick; ``on_deck`` is
        # populated for the *next* tick, guaranteeing a uniform delay.
        incoming = self.delta.clone()
        self.delta.copy_(self.on_deck)
        self.on_deck.zero_()
        refractory_mask = self.refractory > 0
        if refractory_mask.any():
            self.refractory[refractory_mask] = self.refractory[refractory_mask] - 1
        update_mask = ~refractory_mask
        # Membrane integration replicates ``value = leak * value + incoming``.
        updated = torch.maximum(torch.zeros_like(self.membrane), self.leaks * self.membrane + incoming)
        self.membrane = torch.where(update_mask, updated, self.membrane)
        diff = self.membrane - self.thresholds
        if self.surrogate_beta is None:
            fired_mask = (diff > 0) & update_mask
            self.last_spike[fired_mask] = 1.0
        else:
            sigmoid = torch.sigmoid(diff * self.surrogate_beta)
            hard_mask = (sigmoid > 0.5) & update_mask
            self.last_spike.copy_(sigmoid)
            if not self.training:
                self.last_spike.zero_()
                self.last_spike[hard_mask] = 1.0
            fired_mask = hard_mask
        if fired_mask.any():
            self.membrane[fired_mask] = self.resting[fired_mask]
            spike_vector = torch.zeros_like(self.membrane)
            spike_vector[fired_mask] = 1.0
            # Postsynaptic current is staged for the next step (1-tick delay).
            transmissions = torch.matmul(spike_vector, self.weight)
            self.on_deck += transmissions
        return self.last_spike.clone()

    def run(self, sensory_events: Sequence[Optional[Dict[str, float]]]) -> torch.Tensor:
        """Simulate a sequence of sensory injections and collect spikes."""
        spikes = []
        for event in sensory_events:
            spikes.append(self.tick(event))
        return torch.stack(spikes)

    def simulate_events(self, sensory_events: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor]:
        """Functional simulator driven by tensor-coded stimuli for training loops.

        Parameters
        ----------
        sensory_events:
            Either ``(steps, neurons)`` or ``(batch, steps, neurons)`` tensor of
            injected currents. Values are interpreted exactly as calls to
            :meth:`inject`, i.e. they are staged into the one-tick delay buffer
            before influencing membrane voltages.

        Returns
        -------
        spikes, final_membrane:
            ``spikes`` is a tensor matching the batch/time dimensions of the
            input and contains surrogate spike amplitudes (continuous in
            training mode, binary otherwise). ``final_membrane`` captures the
            last membrane potential for each neuron in the batch.
        """

        # Normalise input shape to ``(batch, steps, neurons)`` and place it on
        # the same device/dtype as the network state.
        if sensory_events.dim() == 2:
            sensory_events = sensory_events.unsqueeze(0)
        events = sensory_events.to(device=self.resting.device, dtype=self.resting.dtype)

        batch, steps, neurons = events.shape
        membrane = self.resting.unsqueeze(0).expand(batch, neurons).clone()
        delta = torch.zeros_like(membrane)
        on_deck = torch.zeros_like(membrane)
        spikes: List[torch.Tensor] = []

        # Pre-compute broadcastable attribute buffers.
        leak = self.leaks.unsqueeze(0)
        threshold = self.thresholds.unsqueeze(0)
        resting = self.resting.unsqueeze(0)

        for t in range(steps):
            # Inject external current for this tick before the synaptic delay is
            # applied (mirrors the staging performed by ``tick``).
            on_deck = on_deck + events[:, t, :]

            incoming = delta
            delta = on_deck
            on_deck = torch.zeros_like(on_deck)

            # Leak-integrate and rectify toward zero.
            membrane_candidate = torch.maximum(torch.zeros_like(membrane), leak * membrane + incoming)

            diff = membrane_candidate - threshold
            if self.surrogate_beta is not None:
                spike_prob = torch.sigmoid(diff * self.surrogate_beta)
                spike_drive = spike_prob if self.training else (spike_prob > 0.5).to(events.dtype)
                fire_mask = spike_prob > 0.5
            else:
                spike_drive = (diff > 0).to(events.dtype)
                fire_mask = spike_drive.bool()

            # Reset membranes that fired back toward their resting potential.
            membrane = torch.where(fire_mask, resting, membrane_candidate)

            spikes.append(spike_drive)

            # Propagate spikes forward with a one-tick delay.
            transmissions = torch.matmul(spike_drive, self.weight)
            on_deck = on_deck + transmissions

        spike_stack = torch.stack(spikes, dim=1)
        if sensory_events.dim() == 2:
            spike_stack = spike_stack.squeeze(0)
            membrane = membrane.squeeze(0)
        return spike_stack, membrane

    def set_weight(self, src: str, dst: str, value: float) -> None:
        """Update a synaptic weight in-place using neuron identifiers."""
        self.weight.data[self.id_to_index[src], self.id_to_index[dst]] = float(value)

    def get_weight(self, src: str, dst: str) -> float:
        """Query the weight connecting ``src`` to ``dst``."""
        return float(self.weight[self.id_to_index[src], self.id_to_index[dst]].item())

    def get_membrane(self) -> torch.Tensor:
        """Return a snapshot of membrane voltages for inspection/plots."""
        return self.membrane.clone()

    def get_spike_vector(self) -> torch.Tensor:
        """Get the last spike vector (binary or surrogate activations)."""
        return self.last_spike.clone()

    def get_indices(self, ids: Iterable[str]) -> List[int]:
        """Translate neuron identifiers into integer indices for tensor slicing."""
        return [self.id_to_index[ident] for ident in ids]
