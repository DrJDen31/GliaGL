#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple test to verify Python bindings work
Run this after building: pip install -e .
"""
import sys
import os

# Ensure UTF-8 encoding for output (fixes Windows CI issues)
if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

def test_import():
    """Test that we can import the module"""
    try:
        import glia
        print(f"[OK] Successfully imported glia version {glia.__version__}")
        return True
    except ImportError as e:
        print(f"[FAIL] Failed to import glia: {e}")
        return False

def test_network_creation():
    """Test creating a network"""
    try:
        import glia
        net = glia.Network(num_sensory=2, num_neurons=3)
        print(f"[OK] Created network: {net}")
        print(f"  - Neuron count: {net.num_neurons}")
        print(f"  - Connection count: {net.num_connections}")
        return True
    except Exception as e:
        print(f"[FAIL] Failed to create network: {e}")
        return False

def test_network_step():
    """Test network simulation"""
    try:
        import glia
        net = glia.Network(num_sensory=2, num_neurons=3)
        net.inject("S0", 100.0)
        net.step()
        print("[OK] Network stepped successfully")
        return True
    except Exception as e:
        print(f"[FAIL] Failed to step network: {e}")
        return False

def test_numpy_interface():
    """Test NumPy data interface"""
    try:
        import glia
        import numpy as np
        
        net = glia.Network(num_sensory=2, num_neurons=3)
        
        # Get state
        ids, values, thresholds, leaks = net.get_state()
        print(f"[OK] Got network state")
        print(f"  - IDs: {ids}")
        print(f"  - Values shape: {np.array(values).shape}")
        print(f"  - Thresholds: {np.array(thresholds)}")
        
        # Get weights
        from_ids, to_ids, weights = net.get_weights()
        print(f"[OK] Got network weights")
        print(f"  - Weight count: {len(weights)}")
        
        return True
    except Exception as e:
        print(f"[FAIL] Failed NumPy interface test: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_training_config():
    """Test training configuration"""
    try:
        import glia
        config = glia.TrainingConfig()
        config.lr = 0.01
        config.batch_size = 4
        print(f"[OK] Created training config: {config}")
        return True
    except Exception as e:
        print(f"[FAIL] Failed to create training config: {e}")
        return False

def main():
    print("=" * 60)
    print("GliaGL Python Binding Test Suite")
    print("=" * 60)
    print()
    
    tests = [
        ("Import", test_import),
        ("Network Creation", test_network_creation),
        ("Network Step", test_network_step),
        ("NumPy Interface", test_numpy_interface),
        ("Training Config", test_training_config),
    ]
    
    passed = 0
    failed = 0
    
    for name, test_func in tests:
        print(f"\n[{name}]")
        if test_func():
            passed += 1
        else:
            failed += 1
    
    print("\n" + "=" * 60)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 60)
    
    return failed == 0

if __name__ == "__main__":
    sys.exit(0 if main() else 1)
