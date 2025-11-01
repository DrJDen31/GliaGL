#!/usr/bin/env python
"""Quick test to check module loading"""

try:
    import glia
    print(f"SUCCESS: Imported glia from {glia.__file__}")
    print(f"Version: {glia.__version__}")
    
    # Try creating a network
    net = glia.Network(num_sensory=2, num_neurons=3)
    print(f"SUCCESS: Created network: {net}")
    
except Exception as e:
    print(f"ERROR: {e}")
    import traceback
    traceback.print_exc()
