from dataclasses import dataclass
from typing import Iterable, Tuple

@dataclass(frozen=True)
class SeqHeader:
    duration_ticks: int
    loop: bool = False

def write_seq(path: str, header: SeqHeader, rows: Iterable[Tuple[int, str, float]]):
    # Write a .seq file:
    # DURATION <ticks>
    # LOOP <true|false>
    # <tick> <sensory id> <injection>
    with open(path, "w", encoding="utf-8") as f:
        f.write(f"DURATION {header.duration_ticks}\n")
        f.write(f"LOOP {str(header.loop).lower()}\n\n")
        for tick, sid, inj in rows:
            f.write(f"{tick} {sid} {inj:.6f}\n")

