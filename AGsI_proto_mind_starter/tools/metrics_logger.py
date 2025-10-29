#!/usr/bin/env python3
"""Lightweight metrics logger template.
You can pipe Glia's per-step summaries into this and compute curves offline.
"""
import sys, json

for line in sys.stdin:
    try:
        evt = json.loads(line)
    except Exception:
        continue
    # Expected fields: t, layer, event, value(s)
    # Extend as needed, e.g., accumulate ignition histograms, drift meters, etc.
    print(evt)
