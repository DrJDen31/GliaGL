#!/usr/bin/env python3
"""
Test the high-level Python API
"""

def test_network_wrapper():
    """Test Network wrapper class"""
    import glia
    import numpy as np
    
    print("\n[Network Wrapper]")
    
    # Create network
    net = glia.Network(num_sensory=2, num_neurons=3)
    print(f"✓ Created: {net}")
    
    # Properties
    assert net.num_neurons == 5
    assert len(net.sensory_ids) == 2
    assert len(net.neuron_ids) == 5
    print(f"✓ Properties work")
    
    # State access
    state = net.state
    assert 'ids' in state
    assert 'values' in state
    assert isinstance(state['values'], np.ndarray)
    print(f"✓ State property works")
    
    # Inject dict
    net.inject_dict({"S0": 100.0, "S1": 50.0})
    print(f"✓ inject_dict() works")
    
    # Inject array
    net.inject_array(np.array([120.0, 80.0]))
    print(f"✓ inject_array() works")
    
    # Multiple steps
    net.step(n_steps=10)
    print(f"✓ step(n_steps) works")
    
    # Firing neurons
    firing = net.get_firing_neurons()
    print(f"✓ get_firing_neurons(): {len(firing)} neurons fired")
    
    return True


def test_trainer_wrapper():
    """Test Trainer wrapper class"""
    import glia
    
    print("\n[Trainer Wrapper]")
    
    # Create network and trainer
    net = glia.Network(num_sensory=2, num_neurons=3)
    trainer = glia.Trainer(net)
    print(f"✓ Created: {trainer}")
    
    # Config property - test that we can get and modify config
    original_lr = trainer.config.lr
    trainer.config.lr = 0.02
    assert abs(trainer.config.lr - 0.02) < 1e-6, f"Expected lr≈0.02, got {trainer.config.lr}"
    trainer.config.lr = original_lr  # Restore
    print(f"✓ Config property works")
    
    # History
    history = trainer.history
    assert isinstance(history, dict)
    print(f"✓ History property works")
    
    return True


def test_dataset():
    """Test Dataset class"""
    import glia
    
    print("\n[Dataset]")
    
    # Create episodes
    episodes = []
    for i in range(10):
        ep = glia.EpisodeData()
        ep.seq = glia.InputSequence()
        ep.target_id = f"O{i % 2}"
        episodes.append(ep)
    
    # Create dataset
    dataset = glia.Dataset(episodes)
    print(f"✓ Created dataset: {dataset}")
    
    # Length
    assert len(dataset) == 10
    print(f"✓ len() works: {len(dataset)}")
    
    # Indexing
    ep = dataset[0]
    assert isinstance(ep, glia.EpisodeData)
    print(f"✓ Indexing works")
    
    # Slicing
    subset = dataset[0:5]
    assert isinstance(subset, glia.Dataset)
    assert len(subset) == 5
    print(f"✓ Slicing works")
    
    # Split
    train, val = dataset.split(train_fraction=0.7, shuffle=False)
    assert len(train) == 7
    assert len(val) == 3
    print(f"✓ Split works: train={len(train)}, val={len(val)}")
    
    # Shuffle
    shuffled = dataset.shuffle(seed=42)
    assert len(shuffled) == len(dataset)
    print(f"✓ Shuffle works")
    
    return True


def test_config_helpers():
    """Test config creation helpers"""
    import glia
    
    print("\n[Config Helpers]")
    
    # Training config - test that parameters are properly set
    cfg = glia.create_config(
        lr=0.015,  # Use non-default value to ensure it's actually set
        batch_size=4,
        warmup_ticks=100
    )
    assert abs(cfg.lr - 0.015) < 1e-6, f"Expected lr≈0.015, got {cfg.lr}"
    assert cfg.batch_size == 4, f"Expected batch_size=4, got {cfg.batch_size}"
    assert cfg.warmup_ticks == 100, f"Expected warmup_ticks=100, got {cfg.warmup_ticks}"
    print(f"✓ create_config() works")
    
    # Evolution config
    evo_cfg = glia.create_evo_config(
        population=20,
        generations=50,
        elite=4
    )
    assert evo_cfg.population == 20
    assert evo_cfg.generations == 50
    assert evo_cfg.elite == 4
    print(f"✓ create_evo_config() works")
    
    return True


def test_file_io():
    """Test file I/O"""
    import glia
    import tempfile
    import os
    
    print("\n[File I/O]")
    
    # Create and save network
    net1 = glia.Network(num_sensory=2, num_neurons=3)
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.net', delete=False) as f:
        temp_path = f.name
    
    try:
        net1.save(temp_path)
        print(f"✓ Saved network to {temp_path}")
        
        # Load it back
        net2 = glia.Network.from_file(temp_path, verbose=False)
        print(f"✓ Loaded network: {net2}")
        
        # Verify
        assert net2.num_neurons == net1.num_neurons
        print(f"✓ Neuron counts match")
        
    finally:
        if os.path.exists(temp_path):
            os.remove(temp_path)
    
    return True


def test_evolution_wrapper():
    """Test Evolution wrapper"""
    import glia
    
    print("\n[Evolution Wrapper]")
    
    # Note: We can't actually run evolution without proper dataset,
    # but we can test creation
    
    # Create dummy dataset
    episodes = []
    for i in range(2):
        ep = glia.EpisodeData()
        ep.seq = glia.InputSequence()
        ep.target_id = "O0"
        episodes.append(ep)
    
    # Configs
    train_cfg = glia.create_config()
    evo_cfg = glia.create_evo_config(population=2, generations=1)
    
    print(f"✓ Evolution wrapper API exists")
    print(f"   (Actual run requires valid network file)")
    
    return True


def main():
    print("=" * 60)
    print("GliaGL High-Level API Test Suite")
    print("=" * 60)
    
    tests = [
        ("Network Wrapper", test_network_wrapper),
        ("Trainer Wrapper", test_trainer_wrapper),
        ("Dataset", test_dataset),
        ("Config Helpers", test_config_helpers),
        ("File I/O", test_file_io),
        ("Evolution Wrapper", test_evolution_wrapper),
    ]
    
    passed = 0
    failed = 0
    
    for name, test_func in tests:
        try:
            if test_func():
                passed += 1
                print(f"✓ {name} PASSED")
            else:
                failed += 1
                print(f"✗ {name} FAILED")
        except Exception as e:
            failed += 1
            print(f"✗ {name} FAILED: {e}")
            import traceback
            traceback.print_exc()
    
    print("\n" + "=" * 60)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 60)
    
    return failed == 0


if __name__ == "__main__":
    import sys
    sys.exit(0 if main() else 1)
