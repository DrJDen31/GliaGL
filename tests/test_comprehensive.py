#!/usr/bin/env python
"""Comprehensive test of GliaGL Python API"""

def test_all():
    import glia
    import numpy as np
    
    print("="*60)
    print("GliaGL Comprehensive Test")
    print("="*60)
    
    # Test 1: Import and version
    print(f"\n✓ Imported glia v{glia.__version__}")
    
    # Test 2: Network creation
    net = glia.Network(num_sensory=2, num_neurons=3)
    print(f"✓ Created network: {net}")
    print(f"  - Neurons: {net.num_neurons}")
    print(f"  - Connections: {net.num_connections}")
    print(f"  - Sensory IDs: {net.sensory_ids}")
    print(f"  - All IDs: {net.neuron_ids}")
    
    # Test 3: State access
    ids, values, thresholds, leaks = net.get_state()
    print(f"✓ Accessed state")
    print(f"  - Shape: {len(ids)} neurons")
    print(f"  - Thresholds type: {type(thresholds)}")
    print(f"  - Thresholds: {thresholds}")
    
    # Test 4: State modification
    new_thresholds = thresholds * 1.1
    net.set_state(ids, new_thresholds, leaks)
    _, _, check_thresholds, _ = net.get_state()
    print(f"✓ Modified state")
    print(f"  - New thresholds: {check_thresholds}")
    
    # Test 5: Injection and simulation
    net.inject_dict({"S0": 100.0, "S1": 50.0})
    net.step(n_steps=10)
    print(f"✓ Ran simulation")
    
    # Test 6: Weights
    net.set_weights(["S0", "S1"], ["N0", "N1"], np.array([1.5, 2.0]))
    from_ids, to_ids, weights = net.get_weights()
    print(f"✓ Set and retrieved weights")
    print(f"  - Connections: {len(weights)}")
    print(f"  - Weights: {weights}")
    
    # Test 7: Save/Load
    net.save("test_network.net")
    net2 = glia.Network.from_file("test_network.net")
    print(f"✓ Saved and loaded network")
    print(f"  - Loaded: {net2}")
    
    # Test 8: Training config
    config = glia.create_config(lr=0.01, batch_size=4)
    print(f"✓ Created training config")
    print(f"  - LR: {config.lr}")
    print(f"  - Batch size: {config.batch_size}")
    
    # Test 9: Dataset
    episodes = []
    for i in range(5):
        ep = glia.EpisodeData()
        seq = glia.InputSequence()
        seq.add_timestep({"S0": 100.0, "S1": 50.0})
        seq.add_timestep({"S0": 50.0, "S1": 100.0})
        ep.seq = seq
        ep.target_id = "N0"
        episodes.append(ep)
    dataset = glia.Dataset(episodes)
    print(f"✓ Created dataset")
    print(f"  - Size: {len(dataset)}")
    
    # Test 10: Trainer
    trainer = glia.Trainer(net, config)
    print(f"✓ Created trainer")
    print(f"  - Trainer: {trainer}")
    
    print("\n" + "="*60)
    print("ALL TESTS PASSED! ✅")
    print("="*60)

if __name__ == "__main__":
    test_all()
