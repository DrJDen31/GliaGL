# Network Visualization Usage Guide

## Building

### Linux / Mac / WSL
```bash
cd src/vis
mkdir build && cd build
cmake ..
make
```

### Windows (Visual Studio)
```bash
cd src/vis
mkdir build && cd build
cmake ..
cmake --build .
```

## Running

### Visualize XOR Network
```bash
cd src/vis/build
./vis --network ../../../testing/xor/xor_network.net
```

### Visualize Cloth (Original)
```bash
cd src/vis/build
./vis --cloth ../cloth_files/tablecloth.txt
```

## Keyboard Controls

### Network Visualization

**Mode Control:**
- `A`: Toggle animation on/off (Space bar may also work)
- `T`: Toggle training mode (neurons move via physics)
- `I`: Toggle inference mode (neurons activate on firing)

**Display Options:**
- `P`: Toggle particles (neurons)
- `W`: Toggle wireframe (connections)
- `B`: Toggle bounding box

**Simulation:**
- Left/Right arrow: Step forward/backward
- Up/Down arrow: Adjust timestep

**Camera:**
- Left mouse drag: Rotate camera
- Right mouse drag: Pan camera
- Scroll wheel: Zoom in/out

## What You'll See

### Spatial Layout
```
Left Plane          Central Volume         Right Plane
(Sensory)           (Interneurons)         (Output)
   ║                  ░░░░░░░░░                ║
S0 ║  ──springs──> ░░░░░░░░░░░░ ──springs──> ║ O0
   ║               ░░░░░░░░░░░░░              ║
S1 ║                ░░░░░░░░░░                ║ O1
   ║                                          ║
 BLUE               GRAY→YELLOW             PURPLE
 Fixed              Moving                  Fixed
```

### Colors

**Neurons:**
- **Blue**: Sensory neurons (input)
- **Gray → Yellow-Orange**: Interneurons when firing
- **Purple → Bright Magenta**: Output neurons when firing

**Connections:**
- **Green**: Excitatory (positive weight)
- **Red**: Inhibitory (negative weight)
- **Brightness**: Indicates weight magnitude
- **Pulse**: Shows spike propagation

### Two Modes

**Training Mode:**
- Neurons move based on spring physics
- Connected neurons attract each other
- Stronger weights = stronger attraction
- System settles into spatial equilibrium
- Visualizes network topology

**Inference Mode:**
- Neurons are stationary
- Colors change based on firing
- Connections pulse when spikes propagate
- Shows network activity in real-time

## Example Session

### XOR Network Visualization

1. **Launch visualizer:**
   ```bash
   ./vis --network ../../../testing/xor/xor_network.net
   ```

2. **Initial view:**
   - 2 blue neurons on left (S0, S1)
   - 3 gray/purple neurons in center/right (N0, N1, N2)
   - Green and red lines connecting them

3. **Toggle to training mode (press T):**
   - Neurons start moving
   - System converges to stable layout
   - Interneurons position between inputs and outputs

4. **Toggle to inference mode (press I):**
   - Neurons stop moving
   - Turn on animation (press A)
   - Watch neurons light up as they fire
   - Observe XOR computation in action

5. **Inject input** (future feature):
   - Currently runs with no input
   - Will add UI controls for sensory injection

## Troubleshooting

### No Window Appears
- Check OpenGL version: requires OpenGL 3.3+
- Try updating graphics drivers
- Check terminal for error messages

### Network File Not Found
- Ensure path is correct relative to build directory
- Use absolute path if needed:
  ```bash
  ./vis --network /full/path/to/xor_network.net
  ```

### Neurons Not Moving (Training Mode)
- Press `A` to toggle animation on
- Check that you're in training mode (press `T`)
- Verify physics parameters in network_graph.cpp

### Neurons Not Lighting Up (Inference Mode)
- Press `I` to ensure inference mode is active
- Press `A` to enable animation
- Check that network is actually running (input injection needed)

### Rendering Issues
- Black screen: Check lighting in shader
- Missing connections: Toggle wireframe with `W`
- Missing neurons: Toggle particles with `P`

## Advanced Usage

### Performance Tuning

Edit `network_graph.cpp` to adjust:
```cpp
// Physics timestep
timestep = 0.01;  // Smaller = more stable, slower

// Spring constant
k_connection = 10.0;  // Higher = stronger forces

// Damping
damping = 0.5;  // Higher = less oscillation

// Rest length
rest_length = 1.0;  // Target spacing
```

### Custom Network Files

Create `.net` files following this format:
```
# Comments start with #

# Neurons: ID, threshold, leak, resting
NEURON S0 100.0 1.0 0.0
NEURON N0 50.0 0.0 0.0

# Connections: from_id, to_id, weight
CONNECTION S0 N0 60.0
CONNECTION N0 O0 -120.0

# Future: explicit output declaration
OUTPUT O0 O1
```

### Multiple Networks

Compare different networks side-by-side:
```bash
# Terminal 1
./vis --network network1.net

# Terminal 2  
./vis --network network2.net
```

## Known Limitations

### Current Implementation
- No input injection UI (runs with no input)
- No neuron selection/inspection
- No labels (can't see IDs)
- Fixed physics parameters
- Single network at a time

### Performance
- Large networks (>1000 neurons) may lag
- Need spatial partitioning for optimization
- GPU physics would help

### Rendering
- Points rendered as squares, not spheres
- No anti-aliasing on lines
- No bloom/glow effects
- Basic lighting only

## Future Enhancements

### Short Term
- [ ] Input injection controls
- [ ] Neuron ID labels
- [ ] Info panel (firing rates, weights)
- [ ] Screenshot/video recording

### Medium Term
- [ ] Spike propagation animation
- [ ] Better sphere rendering (instanced meshes)
- [ ] Anti-aliased lines
- [ ] Glow effects on active neurons

### Long Term
- [ ] VR support
- [ ] Real-time training visualization
- [ ] Graph analytics overlay
- [ ] Multi-network comparison view

## Support

For issues or questions:
1. Check this documentation
2. Review implementation in `src/vis/`
3. Check `docs/PHASE1_COMPLETE.md` for technical details
4. Create an issue with error messages and steps to reproduce

---

**Enjoy visualizing your neural networks!** 🎨🧠
