#!/usr/bin/env python
"""Test that optimizer bindings work correctly"""

import glia

# Test creating config with optimizer
print("Testing optimizer bindings...")

# Create config with SGD
config_sgd = glia.create_config(optimizer="sgd")
print(f"✓ SGD config: {config_sgd.grad.optimizer}")
assert config_sgd.grad.optimizer == "sgd"

# Create config with Adam
config_adam = glia.create_config(optimizer="adam", adam_beta1=0.9, adam_beta2=0.999)
print(f"✓ Adam config: {config_adam.grad.optimizer}, beta1={config_adam.grad.adam_beta1}")
assert config_adam.grad.optimizer == "adam"
assert abs(config_adam.grad.adam_beta1 - 0.9) < 1e-6  # Floating point comparison

# Create config with AdamW
config_adamw = glia.create_config(optimizer="adamw")
print(f"✓ AdamW config: {config_adamw.grad.optimizer}")
assert config_adamw.grad.optimizer == "adamw"

# Test modifying optimizer
config = glia.create_config()
config.grad.optimizer = "adam"
print(f"✓ Modified optimizer: {config.grad.optimizer}")
assert config.grad.optimizer == "adam"

# Test accessing GradConfig directly
grad_cfg = glia.GradConfig()
grad_cfg.optimizer = "adamw"
grad_cfg.adam_beta1 = 0.95
print(f"✓ Direct GradConfig: {grad_cfg.optimizer}, beta1={grad_cfg.adam_beta1}")

# Test OutputDetectorConfig
detector_cfg = glia.OutputDetectorConfig()
detector_cfg.alpha = 0.1
detector_cfg.threshold = 0.02
print(f"✓ OutputDetectorConfig: alpha={detector_cfg.alpha}, threshold={detector_cfg.threshold}")

print("\n✓ All optimizer bindings working correctly!")
