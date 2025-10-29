# GliaGL Training & Evolution Plan

This document summarizes the training pipeline implemented in `src/train/` and the roadmap to extend it with evolutionary search and multi-track optimization.

## Components

- `hebbian/training_config.h` — Hyperparameters for:
  - Output detection (EMA alpha/threshold)
  - Episode timing (warmup U, decision window W)
  - Learning (lr, eligibility lambda, weight decay)
  - Reward shaping (margin threshold, pos/neg rewards)
  - Intrinsic plasticity (target rate, EMA alpha, threshold/leak steps)
  - Sparsity/growth (prune epsilon/patience, edge growth count, init weight)
  - Batching (batch size, shuffle)
- `hebbian/trainer.h` — Training API:
  - `Trainer::evaluate()` — Run a single episode and return rates, margin, winner
  - `Trainer::trainEpisode()` — Per-episode learning (reward-modulated Hebbian with eligibility traces)
  - `Trainer::computeEpisodeDelta()` — Compute per-edge deltas without mutating weights
  - `Trainer::applyDeltas()` — Apply accumulated/averaged deltas with weight decay
  - `Trainer::trainBatch()` — Accumulate across a batch and apply once
  - `Trainer::trainEpoch()` — Iterate batches for N epochs with optional shuffle
- `eval_main.cpp` — CLI runner with training and evaluation modes
- `mini_world_main.cpp` — Dataset runner for Mini-World (.seq + labels)
- `gradient/PLAN.md` — Implementation roadmap for rate-based gradient descent
- `gradient/README.md` — Theory of surrogate gradient training on spiking RNNs

## Data Model

Each training example is an "episode":
- Input: `InputSequence` — events mapping tick -> {sensory neuron ID -> value}
- Label: `target_id` — expected output neuron ID (e.g., `O0`, `O1`, ...)

The trainer forms a dataset as a vector of `(InputSequence, target_id)` items and supports:
- Shuffling per epoch
- Batching (accumulation of deltas across the batch, then a single application)

## Episode Timing

- Warmup `U` ticks: allow state to settle while updating EMA output rates
- Decision window `W` ticks: continue updates and finalize the decision
- Total ticks per episode = `U + W`

## Output Decision and Scoring

- EMA tracking of output neuron firing rates during the episode
- Winner prediction via argmax(rate) with threshold abstention
- Confidence margin = top1_rate − top2_rate

## Learning Rule (current)

- Reward-modulated local Hebbian with short eligibility traces per edge:
  - Eligibility update per tick: `e = lambda * e + 1(pre_fire) * 1(post_fire)`
  - Reward after episode: discrete shaping based on correctness and margin:
    - If `winner == target` and `margin >= margin_delta`: `reward = reward_pos`
    - Else: `reward = reward_neg`
  - Weight update (applied at end): `w += lr * reward * e; w -= weight_decay * w`
- Intrinsic plasticity per batch:
  - Track per-neuron EMA firing rate `r_obs`
  - Optionally adjust threshold/leak toward a target rate (`eta_theta`, `eta_leak`)
- Structural plasticity per batch:
  - Prune edges whose |w| < `prune_epsilon` for `prune_patience` consecutive checks
  - Optionally grow `grow_edges` new edges with init magnitude `init_weight`, respecting `TopologyPolicy`

### Batch/Epoch Semantics

- `trainBatch` accumulates deltas over examples and applies their average (1/B) once
- `trainEpoch` iterates batches for N epochs with optional shuffle

## Mapping to the Requested Process

- "Dataset with (input sequence, expected output)" → `EpisodeData { seq, target_id }`
- "Feed to net, evaluate, score" → `evaluate()` and EMA rates + margin
- "Heavily modify bad, lightly tune good" → reward shaping (configurable via `reward_pos`/`reward_neg`) and optional margin threshold `margin_delta`. We can also add a margin-scaled reward variant if desired (see Open Questions).
- "Batch within epoch to generalize" → `trainBatch()` accumulation and `batch_size` setting
- "Repeat for epochs" → `trainEpoch()` with `epochs` from CLI

## CLI Usage (selected)

Build in `src/train/` with CMake (see docs/README.md for details). Then:

- XOR — train for 20 epochs with batch=4, then eval
```
# Windows (after building Release)
.\src\train\build\Release\glia_eval.exe --scenario xor --default O0 --train --epochs 20 --batch 4 --shuffle 1
.\src\train\build\Release\glia_eval.exe --scenario xor --default O0
```

- 3-class — train for 10 epochs with noise
```
.\src\train\build\Release\glia_eval.exe --scenario 3class --noise 0.10 --train --epochs 10 --batch 3 --shuffle 1
.\src\train\build\Release\glia_eval.exe --scenario 3class --noise 0.10
```

- Hyperparameters can be overridden, e.g. `--lr 0.01 --lambda 0.95 --weight_decay 0.0001 --margin 0.05 --reward_pos 1.2 --reward_neg -0.8 ...`

## Evolutionary Outer Loop (roadmap)

We will add an evolutionary algorithm on top of SGD training:

1. Population and Initialization
   - Maintain a population of networks. Each network is a `.net` configuration (serializable via `Glia::saveNetworkToFile`).
   - Initialize from a seed net or diverse seeds.

2. Inner Training
   - For each individual, run `trainEpoch()` for a small number of epochs (Lamarckian: learned weights persist).

3. Fitness
   - Multi-objective metrics (configurable): accuracy, average margin, sparsity (few edges), energy (#spikes), stability.
   - Combine via weighted sum or Pareto ranking.
   - Default weights (tunable): accuracy=1.0, margin=0.5, sparsity=0.1, energy=0.1.

4. Variation Operators
   - Mutation: jitter weights; grow/prune edges; adjust leak/threshold/refractory within bounds; obey `TopologyPolicy`.
   - Crossover: merge connections/parameters from two parents; ensure structural validity.
   - Include an elitist direct copy (unmodified parent) each generation.

5. Selection
   - Keep top-K fittest; optionally tournament selection to preserve diversity.

6. Iterate
   - Repeat for G generations; optionally reduce train epochs over time for efficiency.

## Multi-Track Evolution (roadmap)

- Maintain multiple tracks (sub-populations), each with distinct fitness priorities:
  - Example tracks: accuracy-focused, energy-efficient, sparse/compact, high-margin.
- Run fewer epochs per track (shorter inner training), then periodically:
  - Cross-track crossover among top performers.
  - Re-assess fitness in each track context.
- Outcome: a set of final networks, each Pareto-optimal for its respective track.

## Open Questions / Next Decisions

- Labeling and dataset IO
  - Should we support a file-based dataset format (e.g., list of `*.seq` + label), or will datasets be constructed programmatically?
  - Proposed: `.seq` files via `InputSequence::loadFromFile()`, plus a manifest with lines: `<seq_path> <target_id>`.
- Reward shaping variant
  - Do you want a margin-scaled reward (e.g., `reward = k * (rate[target] - max_other)`), so updates are graded by confidence/error? If yes, we’ll add a toggle and gain.
- Evolution hyperparameters
  - Population size, number of children per parent, mutation rates, crossover method, selection pressure, and generation budget.
- Multi-track details
  - Number of tracks and exact fitness weights per track.
- Logging/metrics
  - Preferred output format (e.g., JSON) and directories for checkpoints and metrics.
- Reproducibility
  - Random seed configuration for training/evolution.

## Notes

- The trainer currently accumulates deltas over a batch and applies once, then prunes/grows and performs intrinsic plasticity. This matches the requested “batch within epoch” behavior.
- The reward scheme is intentionally simple and tunable; we can switch to margin-scaled updates on request.
