# GliaGL Network Visualization - Quick Reference

**Version:** 2.0  
**Last Updated:** 2025-10-05

---

## Quick Start

```bash
# Build
cd src/vis && mkdir build && cd build
cmake .. && make

# Run XOR network
./vis --network ../../../testing/xor/xor_network.net

# Test without OpenGL
cd src/vis
make -f Makefile.simple && ./test_network_simple
```

---

## Keyboard Controls

### Essential
| Key | Action | Description |
|-----|--------|-------------|
| `A` | Start animation | Begin continuous simulation |
| `X` | Stop animation | Pause simulation |
| `Q` | Quit | Exit program |
| `Space` | Single step | Execute one frame |

### Modes (Network Only)
| Key | Action | Description |
|-----|--------|-------------|
| `T` | Training mode | Neurons move via physics |
| `I` | Inference mode | Show neuron activations |
| `N` | Network step | Execute one network tick |

### Input Injection (Network Only)
| Key | Action | Description |
|-----|--------|-------------|
| `1` | Inject S0 | Send 200.0 to first sensory neuron |
| `2` | Inject S1 | Send 200.0 to second sensory neuron |
| `0` | Clear | Stop all stimulation |

### Display Toggles
| Key | Action | Description |
|-----|--------|-------------|
| `P` | Particles | Toggle neurons on/off |
| `W` | Wireframe | Toggle connections on/off |
| `B` | Bounding box | Toggle box on/off |
| `S` | Surface | Toggle surface rendering |
| `V` | Velocity | Toggle velocity vectors |
| `F` | Forces | Toggle force vectors |

### Simulation
| Key | Action | Description |
|-----|--------|-------------|
| `+` | Speed up | Double timestep |
| `-` | Slow down | Halve timestep |
| `R` | Reset | Reload from file |

---

## Mouse Controls

| Action | Description |
|--------|-------------|
| **Left drag** | Rotate camera around center |
| **Right drag** | Pan camera (truck) |
| **Scroll** | Zoom in/out (dolly) |
| **Shift + drag** | Alternative zoom |
| **Ctrl + drag** | Alternative pan |
| **Alt + drag** | Alternative dolly |

---

## Command Line

### Syntax
```bash
./vis [options]
```

### Options
| Flag | Argument | Description |
|------|----------|-------------|
| `--network` | `<file.net>` | Load neural network |
| `--cloth` | `<file.txt>` | Load cloth simulation |
| `--size` | `<width> <height>` | Window dimensions |

### Examples
```bash
# XOR network
./vis --network ../../../testing/xor/xor_network.net

# Custom window size
./vis --network network.net --size 1024 768

# Original cloth sim
./vis --cloth ../cloth_files/tablecloth.txt
```

---

## Color Coding

### Neurons
| Type | Base Color | Active Color | Fixed? |
|------|------------|--------------|--------|
| **Sensory** | 🔵 Blue | 🔷 Bright Blue | ✅ Yes |
| **Interneuron** | ⚫ Gray | 🟡 Yellow | ❌ No |
| **Output** | 🟣 Purple | 🟪 Magenta | ✅ Yes |

### Connections
| Type | Color | Meaning |
|------|-------|---------|
| **Excitatory** | 🟢 Green | Positive weight |
| **Inhibitory** | 🔴 Red | Negative weight |
| **Bright** | High opacity | Strong weight |
| **Dim** | Low opacity | Weak weight |

---

## Spatial Layout

```
     LEFT              CENTER             RIGHT
   (Fixed)            (Moving)          (Fixed)
     
     S0  ────────┐                    
                  \                    
     S1  ───────┐ \     I0 ────┐      
                 \│             │      
                  X ──> I1 ────┼────> O0
                 /│             │      
     S2  ───────┘ /     I2 ────┘      
                  /                    
     S3  ────────┘                   
     
  SENSORY      INTERNEURONS        OUTPUT
 (Blue, fixed)  (Gray, free)   (Purple, fixed)
```

---

## Modes Explained

### Training Mode (Press `T`)
**Purpose:** Organize network spatially

**Behavior:**
- Neurons move based on connection weights
- Stronger connections pull neurons closer
- System settles into equilibrium
- Physics runs continuously
- Colors static (gray/blue/purple)

**When to use:**
- First time viewing network
- Understanding topology
- Finding clusters
- Adjusting layout

**Visualization:**
- Watch neurons move and settle
- Connected neurons group together
- Network structure becomes clear

---

### Inference Mode (Press `I`)
**Purpose:** Observe network activity

**Behavior:**
- Neurons are stationary
- Network executes (ticks)
- Colors change on firing
- Connections pulse with spikes
- Shows information flow

**When to use:**
- Testing network computation
- Debugging behavior
- Demonstrating function
- Analyzing activity patterns

**Visualization:**
- Inject inputs (1, 2 keys)
- Watch spikes propagate
- See neurons light up
- Observe output response

---

## Typical Workflow

### 1. Launch and Explore
```bash
./vis --network xor_network.net
# Window opens with default view
```

### 2. Start Animation
```
Press 'A'
# Network starts moving (training mode by default)
```

### 3. Let it Settle
```
Wait 5-10 seconds
# Neurons find equilibrium positions
```

### 4. Switch to Inference
```
Press 'I'
# Neurons stop moving
```

### 5. Inject Input
```
Press '1' (S0 = 200)
Press '2' (S1 = 200)
# For XOR, this is input "11" → should output FALSE
```

### 6. Step Network
```
Press 'N' multiple times
# Watch spikes propagate
# See output neurons respond
```

### 7. Try Different Inputs
```
Press '0' to clear
Press '1' only (input "10" → should output TRUE)
Press 'N' to step
```

---

## Troubleshooting

### No Window Appears
```
ERROR: Failed to open GLFW window
```
**Fix:** 
- Update graphics drivers
- Check OpenGL version: `glxinfo | grep "OpenGL version"`
- Need OpenGL 3.3+

### Neurons Not Visible
```
Black screen or just connections
```
**Fix:**
- Press `P` to toggle particles on
- Check that network loaded (see terminal output)

### Connections Not Visible
```
Just see dots, no lines
```
**Fix:**
- Press `W` to toggle wireframe (connections) on
- May be disabled by default

### Neurons Not Moving (Training Mode)
```
Press T, but nothing happens
```
**Fix:**
- Press `A` to start animation
- Check animation is on (message in terminal)

### Neurons Not Lighting Up (Inference Mode)
```
Press I and inject, but colors don't change
```
**Fix:**
- Make sure animation is on (`A`)
- Step network with `N` key
- Inject with `1` or `2`
- Wait a few steps for propagation

### Performance Issues
```
Low FPS, stuttering
```
**Fix:**
- Press `-` to slow timestep
- Toggle off unnecessary display (F, V)
- Use smaller network
- Update graphics drivers

---

## File Formats

### Network File (.net)
```
# Comments
NEURON <id> <threshold> <leak> <resting>
CONNECTION <from> <to> <weight>
```

Example:
```
# XOR Network
NEURON S0 100.0 1.0 0.0
NEURON S1 100.0 1.0 0.0
NEURON N0 50.0 0.0 0.0
CONNECTION S0 N0 60.0
CONNECTION S0 N1 -120.0
```

---

## Performance Tips

### For Large Networks
1. Start in inference mode (Press `I` first)
2. Don't run training mode physics
3. Toggle off force visualization (`F`)
4. Use larger timestep (`+`)
5. Reduce window size

### For Smooth Animation
1. Use modest network size (<100 neurons)
2. Let training mode settle before inference
3. Don't inject inputs too rapidly
4. Use hardware acceleration (native not WSL)

---

## Common Tasks

### Test XOR Function
```bash
./vis --network xor_network.net
Press: A I 1 N N N N N N N N N N
# S0=1, S1=0 → should see output TRUE (N1 fires)

Press: 0 2 N N N N N N N N N N
# S0=0, S1=1 → should see output TRUE (N1 fires)

Press: 0 1 2 N N N N N N N N N N
# S0=1, S1=1 → should see output FALSE (N2 fires)
```

### Explore Network Structure
```bash
./vis --network network.net
Press: A T
# Let run for 10-20 seconds
# Observe how neurons cluster
# See which neurons are connected
Press: P W B
# Toggle displays to see different aspects
```

### Debug Network Issue
```bash
./vis --network network.net
Press: I A 1
# Inject specific input
Press: N (repeatedly)
# Step through execution
# Watch which neurons fire
# Identify problem area
```

---

## Tips & Tricks

### Better Viewing Angle
- Drag to rotate until you see all layers
- Zoom in to focus on specific neurons
- Pan to center region of interest

### Finding Specific Neuron
- Training mode spreads neurons out
- Connections show relationships
- Color indicates type

### Understanding Information Flow
- Inference mode + step through
- Watch spike propagation
- Green lines = excitatory path
- Red lines = inhibitory path

### Optimizing Layout
- Let training mode run longer
- Restart if layout is tangled (`R`)
- Adjust physics params in code

---

## FAQ

**Q: Can I edit the network while visualizing?**  
A: No, reload with `R` or restart program

**Q: Can I save a screenshot?**  
A: Not yet - use external tool or add feature

**Q: Can I visualize my own network?**  
A: Yes! Create a .net file (see file format above)

**Q: Why are neurons squares not spheres?**  
A: Using GL_POINTS - upgrade to instanced meshes planned

**Q: Can I inject custom values?**  
A: Currently fixed at 200.0 - edit OpenGLCanvas.cpp

**Q: Does it support large networks?**  
A: Up to ~1000 neurons - needs optimization beyond that

**Q: Can I export the layout?**  
A: Not yet - add position export feature

---

## Getting Help

1. **Read the docs:**
   - `docs/RENDERING_USAGE.md` - Detailed usage
   - `docs/BUILD_INSTRUCTIONS.md` - Build help
   - `docs/STATUS_SUMMARY.md` - Technical overview

2. **Check terminal output:**
   - Errors appear in console
   - Mode changes logged
   - Input injections confirmed

3. **Test simple case:**
   - Run `test_network_simple` first
   - Verify core functionality
   - Isolate rendering issues

4. **File an issue:**
   - Include error messages
   - Describe what you expected
   - Share your .net file

---

## Useful Console Output

### Successful Launch
```
-------------------------------------------------------
OpenGL Version: 3.3.0 ...
-------------------------------------------------------
Loading network from: xor_network.net
Network configuration loaded from ...
Network loaded and spatialized successfully
```

### Mode Switches
```
Training mode: ON (neurons move via physics)
Inference mode: ON (neurons show activations)
```

### Input Injection
```
Injected S0 = 200.0
Injected S1 = 200.0
Network step executed
```

---

**Quick Reference Complete!**  
For more details, see full documentation in `docs/` directory.
