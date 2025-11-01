# Network Visualization Implementation Plan

## Overview

Repurpose the cloth simulation physics engine to create an interactive 3D visualization of the spiking neural network, where:
- **Neurons** = particles (positions in 3D space)
- **Connections** = springs (visual edges between neurons)
- **Spring physics** = used to spatially organize the network
- **Two modes**: Training (dynamic topology) and Inference (static, visualize activations)

---

## Spatial Organization

### Coordinate System Layout

```
        ┌─────────────────────────────────────────┐
        │                                         │
    Z   │   Sensory    Interneurons    Output    │
    ↑   │   Plane      Volume           Plane    │
    │   │     ║       ░░░░░░░░░         ║       │
    │   │  S0 ║    ░░░░░░░░░░░░░        ║ O0    │
    │   │     ║   ░░░░░░░░░░░░░░░       ║       │
    │   │  S1 ║  ░░░░░░░░░░░░░░░░░      ║ O1    │
    │   │     ║   ░░░░░░░░░░░░░░░       ║       │
    │   │  S2 ║    ░░░░░░░░░░░░░        ║ O2    │
    │   │     ║       ░░░░░░░░░         ║       │
    │   └─────║───────────────────────

──║───┘
        ←─────────────────────────────────────────→
                        X axis
```

### Neuron Placement Rules

1. **Sensory Neurons** (Input plane)
   - Fixed X coordinate: `X_LEFT` (e.g., -5.0)
   - Spread vertically (Y) and in depth (Z)
   - **Anchored** - do not move during simulation
   - Spacing: Evenly distributed based on count

2. **Output Neurons** (Output plane)
   - Fixed X coordinate: `X_RIGHT` (e.g., +5.0)
   - Spread vertically (Y) and in depth (Z)
   - **Anchored** - do not move during simulation
   - Spacing: Evenly distributed based on count

3. **Interneurons** (Central volume)
   - Initial X range: `X_LEFT + margin` to `X_RIGHT - margin`
   - Spread in all three dimensions (X, Y, Z)
   - **Free to move** - spring physics determines final positions
   - Initial positions: Random or layer-based

---

## Physics Mapping

### From Cloth to Network

| Cloth Concept | Network Equivalent | Purpose |
|---------------|-------------------|---------|
| ClothParticle | NeuronParticle | Represents a neuron in 3D space |
| Spring connection | Synaptic connection | Visual and physical link between neurons |
| Spring constant (k) | Connection weight magnitude | Stronger weights = stronger pull |
| Rest length (L0) | Desired spacing | Target distance between connected neurons |
| Fixed particles | Input/output neurons | Anchored planes that don't move |
| Free particles | Interneurons | Move based on spring forces |

### Spring Force Formula

```
F(neuron_i, neuron_j) = k * (L0 - ||pi - pj||) * (pi - pj) / ||pi - pj||

Where:
- k = spring constant (derived from connection weight)
- L0 = rest length (desired distance)
- pi, pj = 3D positions of neurons i and j
```

### Spring Constants from Weights

```cpp
k = base_spring_constant * abs(weight)

Examples:
- weight = 60.0  → k = 1.0 * 60 = 60.0 (strong pull)
- weight = -120.0 → k = 1.0 * 120 = 120.0 (strong pull, regardless of sign)
- weight = 10.0  → k = 1.0 * 10 = 10.0 (weak pull)
```

**Note**: Spring constant uses `abs(weight)` - the sign (excitatory/inhibitory) doesn't affect spatial attraction, only neural dynamics.

---

## Data Structures

### NeuronParticle Class

```cpp
class NeuronParticle {
public:
    // Identification
    std::string neuron_id;        // e.g., "S0", "N5", "O1"
    Neuron* neuron_ptr;           // Pointer to actual Neuron object
    
    // Spatial properties (from ClothParticle)
    Vec3f position;
    Vec3f velocity;
    Vec3f acceleration;
    double mass;
    bool fixed;                   // true for input/output, false for interneurons
    
    // Visual properties
    Vec3f base_color;             // Color when inactive
    Vec3f active_color;           // Color when firing
    float size;                   // Rendering size
    
    // Connectivity (for spring physics)
    std::vector<std::pair<NeuronParticle*, double>> connections;  // (target, weight)
    
    // Activation state (for visualization)
    bool is_firing;               // Current firing state
    float activation_level;       // EMA of recent firing (for smooth color transitions)
};
```

### NetworkGraph Class

Replaces `Cloth` class:

```cpp
class NetworkGraph {
public:
    NetworkGraph(Glia* network, ArgParser* args);
    ~NetworkGraph();
    
    // Initialization
    void buildFromGlia(Glia* network);
    void initializeSpatialLayout();
    
    // Physics simulation (training mode)
    void animatePhysics();        // Spring-based position updates
    void applyProvotCorrection(); // Prevent over-stretching
    
    // Visualization updates (inference mode)
    void updateActivationStates();
    void updateColors();
    
    // Rendering
    void packMesh();
    void packNeurons(float* &current);
    void packConnections(float* &current);
    
    // Mode control
    void setTrainingMode(bool training);
    bool isTrainingMode() const { return training_mode; }
    
private:
    Glia* glia;                              // Pointer to neural network
    ArgParser* args;
    
    std::vector<NeuronParticle*> particles;  // All neurons as particles
    std::map<std::string, NeuronParticle*> particle_map;  // ID -> particle lookup
    
    bool training_mode;                      // Training vs inference mode
    
    // Physics parameters
    double k_connection;                     // Base spring constant
    double damping;
    double provot_correction_threshold;
    
    // Spatial layout parameters
    float x_left, x_right;                   // Boundaries
    float y_span, z_span;                    // Spread dimensions
    float rest_length;                       // Desired spacing
};
```

---

## Implementation Phases

### Phase 1: Core Data Structures ✓

**Files to create:**
- `src/vis/neuron_particle.h` - NeuronParticle class
- `src/vis/network_graph.h/cpp` - NetworkGraph class

**Tasks:**
1. Define NeuronParticle with spatial and visual properties
2. Create NetworkGraph class structure
3. Implement `buildFromGlia()` to convert Glia → NetworkGraph
4. Implement `initializeSpatialLayout()` for initial positioning

**Key Functions:**
```cpp
void NetworkGraph::buildFromGlia(Glia* network) {
    // For each sensory neuron
    for (auto* neuron : network->sensory_neurons) {
        NeuronParticle* p = new NeuronParticle();
        p->neuron_id = neuron->getId();
        p->neuron_ptr = neuron;
        p->fixed = true;  // Sensory neurons are anchored
        p->position = computeSensoryPosition(index);  // Left plane
        particles.push_back(p);
        particle_map[p->neuron_id] = p;
    }
    
    // For each interneuron
    for (auto* neuron : network->neurons) {
        NeuronParticle* p = new NeuronParticle();
        p->neuron_id = neuron->getId();
        p->neuron_ptr = neuron;
        p->fixed = false;  // Interneurons can move
        p->position = computeInterneuronPosition(index);  // Central volume
        particles.push_back(p);
        particle_map[p->neuron_id] = p;
    }
    
    // Identify output neurons (those with no outgoing connections)
    identifyOutputNeurons();
    
    // Build connection springs
    buildConnectionSprings();
}
```

### Phase 2: Spring Physics (Training Mode)

**Tasks:**
1. Port spring force calculation from `cloth.cpp`
2. Adapt for network topology (not grid-based)
3. Implement Euler integration for position updates
4. Add damping to prevent oscillation
5. Implement Provot correction for stability

**Key Functions:**
```cpp
void NetworkGraph::animatePhysics() {
    double h = timestep;
    
    // Compute forces on each free particle
    for (auto* p : particles) {
        if (p->fixed) continue;
        
        Vec3f force(0, 0, 0);
        
        // Spring forces from all connections
        for (auto& conn : p->connections) {
            NeuronParticle* neighbor = conn.first;
            double weight = conn.second;
            
            // F = k * (L0 - L) * direction
            double k = k_connection * abs(weight);
            Vec3f diff = p->position - neighbor->position;
            double length = diff.Length();
            Vec3f direction = diff / length;
            Vec3f spring_force = k * (rest_length - length) * direction;
            
            force += spring_force;
        }
        
        // Damping: F_damp = -damping * velocity
        force += -damping * p->velocity;
        
        // Euler integration
        Vec3f acceleration = force / p->mass;
        p->velocity += acceleration * h;
        p->position += p->velocity * h;
    }
    
    // Provot correction (prevent over-stretching)
    applyProvotCorrection();
}
```

### Phase 3: Activation Visualization (Inference Mode)

**Tasks:**
1. Query neuron firing states from Glia
2. Update particle colors based on activation
3. Implement smooth color transitions (EMA)
4. Add glow effect for active neurons
5. Pulse connections when spikes propagate

**Key Functions:**
```cpp
void NetworkGraph::updateActivationStates() {
    for (auto* p : particles) {
        // Query neuron's current firing state
        bool is_firing = p->neuron_ptr->didFire();
        p->is_firing = is_firing;
        
        // Smooth activation level with EMA
        float alpha = 0.2f;  // Decay rate
        if (is_firing) {
            p->activation_level = 1.0f;  // Instant jump on fire
        } else {
            p->activation_level *= (1.0f - alpha);  // Smooth decay
        }
    }
}

void NetworkGraph::updateColors() {
    for (auto* p : particles) {
        // Interpolate between base and active colors
        float t = p->activation_level;
        p->current_color = (1-t) * p->base_color + t * p->active_color;
        
        // Example:
        // base_color = Vec3f(0.3, 0.3, 0.3)  // gray
        // active_color = Vec3f(1.0, 0.8, 0.0)  // yellow-orange
    }
}
```

### Phase 4: Rendering Integration

**Tasks:**
1. Update `OpenGLRenderer` to handle NetworkGraph
2. Implement neuron rendering (spheres with colors)
3. Implement connection rendering (lines with thickness based on weight)
4. Add shader support for glow effects
5. Integrate with existing camera controls

**Key Functions:**
```cpp
void NetworkGraph::packNeurons(float* &current) {
    for (auto* p : particles) {
        Vec3f pos = p->position;
        Vec3f color = p->current_color;
        float size = p->size;
        
        // Pack vertex data: position (3) + color (3) + size (1)
        *current++ = pos.x();
        *current++ = pos.y();
        *current++ = pos.z();
        *current++ = color.x();
        *current++ = color.y();
        *current++ = color.z();
        *current++ = size;
    }
}

void NetworkGraph::packConnections(float* &current) {
    for (auto* p : particles) {
        for (auto& conn : p->connections) {
            NeuronParticle* target = conn.first;
            double weight = conn.second;
            
            // Line from p to target
            Vec3f start = p->position;
            Vec3f end = target->position;
            
            // Color based on weight (red=inhibitory, green=excitatory)
            Vec3f color = (weight > 0) ? Vec3f(0, 1, 0) : Vec3f(1, 0, 0);
            float alpha = abs(weight) / 120.0;  // Opacity based on magnitude
            
            // Pack line data
            *current++ = start.x();
            *current++ = start.y();
            *current++ = start.z();
            *current++ = end.x();
            *current++ = end.y();
            *current++ = end.z();
            *current++ = color.x();
            *current++ = color.y();
            *current++ = color.z();
            *current++ = alpha;
        }
    }
}
```

### Phase 5: Mode Switching & Controls

**Tasks:**
1. Add keyboard shortcuts to toggle modes
2. Add GUI controls (if applicable)
3. Implement training simulation loop
4. Implement inference simulation loop
5. Add pause/resume functionality

**Key Bindings:**
- `T`: Toggle training mode
- `I`: Toggle inference mode
- `Space`: Pause/resume
- `R`: Reset positions
- `S`: Step one frame
- `N`: Run network step (tick)

---

## Rendering Details

### Neuron Visual Design

**Sensory Neurons:**
- Color: Blue (0.2, 0.5, 1.0)
- Size: Medium (0.1 units)
- Shape: Sphere
- Position: Fixed left plane

**Interneurons:**
- Base color: Gray (0.3, 0.3, 0.3)
- Active color: Yellow-orange (1.0, 0.8, 0.0)
- Size: Small (0.08 units)
- Shape: Sphere
- Position: Dynamic (physics-based)

**Output Neurons:**
- Base color: Purple (0.8, 0.2, 0.8)
- Active color: Bright magenta (1.0, 0.3, 1.0)
- Size: Medium (0.1 units)
- Shape: Sphere
- Position: Fixed right plane

### Connection Visual Design

**Excitatory Connections (positive weight):**
- Color: Green (0, 1, 0)
- Thickness: Proportional to weight magnitude
- Style: Solid line

**Inhibitory Connections (negative weight):**
- Color: Red (1, 0, 0)
- Thickness: Proportional to weight magnitude
- Style: Solid line or dashed

**Spike Propagation:**
- When a neuron fires, pulse the connections
- Animate a "traveling dot" along the connection line
- Delay matches the 1-tick synaptic delay

---

## Integration with Existing Code

### Files to Modify

1. **src/vis/main.cpp**
   - Add network loading
   - Initialize NetworkGraph instead of Cloth
   
2. **src/vis/OpenGLRenderer.h/cpp**
   - Add NetworkGraph support
   - Update VBO setup for neurons and connections
   
3. **src/vis/meshdata.h**
   - Add network-specific data fields
   - Add mode toggle flags
   
4. **src/vis/CMakeLists.txt**
   - Link arch/ files (glia.cpp, neuron.cpp)
   - Add new neuron_particle.cpp, network_graph.cpp

### New Files to Create

1. **src/vis/neuron_particle.h**
   - NeuronParticle class definition
   
2. **src/vis/network_graph.h**
   - NetworkGraph class definition
   
3. **src/vis/network_graph.cpp**
   - NetworkGraph implementation
   
4. **src/vis/network_render.cpp**
   - Rendering functions for network visualization

---

## Example Usage

### Training Mode Example

```cpp
// Load network
Glia network(2, 3);
network.configureNetworkFromFile("xor_network.net");

// Create visualization
NetworkGraph graph(&network, args);
graph.setTrainingMode(true);

// Training loop
while (training) {
    // Update network topology (add/remove connections)
    // ... learning algorithm ...
    
    // Update spatial layout via physics
    graph.animatePhysics();
    
    // Render
    graph.packMesh();
    renderer.drawVBOs();
}
```

### Inference Mode Example

```cpp
// Load trained network
Glia network(2, 3);
network.configureNetworkFromFile("xor_network.net");

// Create visualization
NetworkGraph graph(&network, args);
graph.setTrainingMode(false);  // Inference mode

// Inference loop
while (running) {
    // Inject input
    network.injectSensory("S0", input_value);
    
    // Step network
    network.step();
    
    // Update visualization
    graph.updateActivationStates();
    graph.updateColors();
    
    // Render
    graph.packMesh();
    renderer.drawVBOs();
}
```

---

## Questions & Decisions

### 1. Output Neuron Detection

**Question**: How do we identify which neurons are "output" neurons?

**Options**:
a) **Explicit labeling**: Add a flag to Neuron class (`is_output`)
b) **Heuristic**: Neurons with no outgoing connections
c) **Configuration**: Specify in .net file (new `OUTPUT N1 N2` command)

**Recommendation**: Option (c) - explicit configuration
- Add to .net format: `OUTPUT N1 N2`
- Most flexible, allows multi-output scenarios
- Clear user intent

### 2. Initial Interneuron Placement

**Question**: How to initialize interneuron positions?

**Options**:
a) **Random**: Uniform distribution in central volume
b) **Layered**: Arrange in implicit layers based on connectivity depth
c) **Gridded**: Start in a grid, let physics sort it out

**Recommendation**: Option (b) - layered
- Compute "depth" from inputs (shortest path distance)
- Place neurons in X based on their depth
- Spread Y/Z randomly within layer
- Provides better initial layout, faster convergence

### 3. Rest Length Strategy

**Question**: What should the spring rest length be?

**Options**:
a) **Fixed**: Same for all connections (e.g., 1.0)
b) **Weight-based**: Shorter for stronger weights
c) **Adaptive**: Adjust based on local density

**Recommendation**: Option (a) - fixed rest length
- Simplest to implement
- Provides uniform spacing
- Weight affects spring stiffness (k), not rest length

### 4. Spring Constant Scaling

**Question**: How to map connection weights to spring constants?

**Options**:
a) **Linear**: `k = base * abs(weight)`
b) **Normalized**: `k = base * (abs(weight) / max_weight)`
c) **Logarithmic**: `k = base * log(abs(weight))`

**Recommendation**: Option (b) - normalized
- Prevents extremely strong weights from dominating
- Provides more balanced spatial distribution
- Easier to tune base constant

### 5. Color Scheme

**Question**: How to color connections?

**Current Plan**:
- Excitatory (positive weight): Green
- Inhibitory (negative weight): Red

**Alternative**: Use heatmap based on weight magnitude
- Cold (blue) = weak
- Warm (yellow/red) = strong
- Sign indicated by solid vs dashed line

---

## Performance Considerations

### Optimization Strategies

1. **Spatial Partitioning**
   - Use octree or grid for neighbor queries
   - Only compute forces for connected neurons (not all pairs)
   
2. **LOD (Level of Detail)**
   - Reduce geometry complexity for distant neurons
   - Cull connections below visibility threshold
   
3. **Adaptive Timestep**
   - Larger timesteps when system is stable
   - Smaller timesteps during rapid changes
   
4. **GPU Acceleration**
   - Move spring force computation to GPU (compute shaders)
   - Particle physics well-suited for parallelization

### Expected Performance

- **Small networks** (< 100 neurons): 60 FPS easily
- **Medium networks** (100-1000 neurons): 30-60 FPS with optimization
- **Large networks** (> 1000 neurons): May need LOD and GPU acceleration

---

## Testing Plan

### Unit Tests

1. **NeuronParticle**
   - Position/velocity updates
   - Color interpolation
   - Activation state tracking

2. **NetworkGraph**
   - Build from Glia
   - Spring force calculation
   - Provot correction

### Integration Tests

1. **XOR Network Visualization**
   - Load XOR network
   - Verify 2 sensory + 3 interneurons + 2 outputs
   - Check spatial layout (left-center-right)

2. **Training Mode**
   - Add/remove connections dynamically
   - Verify positions update correctly
   - Check for stability (no explosions)

3. **Inference Mode**
   - Run XOR inputs through network
   - Verify correct neurons light up
   - Check output neuron activation

### Visual Tests

1. **Static Layout**
   - Verify sensory neurons on left plane
   - Verify output neurons on right plane
   - Verify interneurons in central volume

2. **Physics Convergence**
   - Start with random positions
   - Run physics simulation
   - Check that system reaches equilibrium
   - Measure convergence time

3. **Activation Visualization**
   - Input 00, 01, 10, 11 to XOR network
   - Verify correct activation patterns
   - Check color transitions are smooth

---

## Future Enhancements

### Short Term
- [ ] Add connection weight labels (on hover)
- [ ] Add neuron ID labels
- [ ] Add firing rate display
- [ ] Add performance metrics (FPS, physics steps/sec)

### Medium Term
- [ ] Support for neuron groups/populations
- [ ] Layer-based automatic layout
- [ ] Export network layout to file
- [ ] Replay mode (record and playback activations)

### Long Term
- [ ] VR support for immersive exploration
- [ ] Real-time training visualization
- [ ] Comparative view (multiple networks side-by-side)
- [ ] Graph analytics overlay (centrality, modularity)

---

## Summary

This plan provides a comprehensive roadmap for transforming the cloth simulation into a neural network visualizer. The key insight is that neurons map naturally to particles, and connections map to springs, allowing us to repurpose the existing physics engine with minimal changes.

**Next Steps:**
1. Create NeuronParticle class
2. Implement NetworkGraph::buildFromGlia()
3. Test with XOR network
4. Iterate on spatial layout
5. Add activation visualization
6. Polish rendering and add controls
