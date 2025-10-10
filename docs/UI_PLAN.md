# GliaGL UI/UX Enhancement Plan

## Immediate Fixes ✓
- [x] Lighter background color (0.45 gray-blue)
- [x] Output neurons fixed in physics simulation

## Phase 1: Interactive Input Control

### Input Mechanism Options
**Option A: Direct Neuron Click**
- Click sensory neuron → toggles input ON/OFF
- Visual feedback: neuron glows when input is active
- Pros: Simple, direct
- Cons: May conflict with camera rotation, input tied to neuron firing

**Option B: Input Switch Widgets (RECOMMENDED)**
- Render small toggle switches to the LEFT of sensory neurons
- Click switch → injects current to sensory neuron
- Visual: Green=ON, Gray=OFF
- Pros: Separates UI from neuron state, clearer UX
- Implementation:
  - Add `InputSwitch` class with position, state, associated neuron ID
  - Render as small circles/rectangles in 2D overlay or 3D space
  - Mouse picking to detect clicks

### Implementation Steps
1. Add mouse click handler in `OpenGLCanvas.cpp`
2. Implement screen-to-world ray casting for 3D picking
3. Create `InputSwitch` class (position, state, neuronID)
4. Render switches as billboarded quads or 2D overlay
5. Toggle input injection on click

## Phase 2: Output Indicator

### Output Visualization
- **Winner-Takes-All indicator**: Highlight the output neuron with highest activation
- Visual options:
  - Gold ring/halo around winning output
  - Vertical bar graph to RIGHT of output neurons
  - Label text showing "OUTPUT: N1" 
  
### Implementation
- Track max activation among output neurons each frame
- Render indicator geometry (ring/bar) at appropriate position
- Update in `NetworkGraph::updateActivationStates()`

## Phase 3: Visual Parameter Controls

### Parameters to Expose
1. **Neuron Size** (default: 0.08-0.12)
2. **Connection Line Thickness** (default: 2.0)
3. **Spring Rest Length** (default: 2.0)
4. **Neuron Spacing** (x_left, x_right, y_span, z_span)
5. **Point Size Multiplier** (shader: currently 30.0)
6. **Connection Opacity Base** (currently 0.5)

### UI Options

**Option A: ImGui Integration (RECOMMENDED)**
- Add Dear ImGui library
- Floating control panel with sliders
- Real-time updates
- Easy to implement, professional look
- CMake: `find_package(imgui)` or include as submodule

**Option B: Keyboard Shortcuts**
- `[` / `]` : Decrease/increase neuron size
- `-` / `=` : Decrease/increase connection thickness
- `,` / `.` : Decrease/increase spacing
- Simple but less discoverable

**Option C: On-Screen Text Menu**
- Render text overlay with current values
- Number keys to select parameter, +/- to adjust
- No external dependencies
- More work to implement text rendering

### Recommended: ImGui Implementation
```cpp
// In OpenGLCanvas.cpp render loop
ImGui::Begin("Network Controls");

ImGui::SliderFloat("Neuron Size", &neuron_size_mult, 0.5f, 3.0f);
ImGui::SliderFloat("Line Thickness", &line_thickness, 0.5f, 5.0f);
ImGui::SliderFloat("Spring Rest Length", &spring_rest, 0.5f, 5.0f);
ImGui::SliderFloat("Spacing Scale", &spacing_scale, 0.5f, 3.0f);

if (ImGui::Button("Reset to Defaults")) {
    // Reset all values
}

ImGui::End();
```

## Phase 4: Additional UX Improvements

### Camera Controls
- Add "Focus on Network" button (reset camera to framed view)
- Camera rotation speed adjustment
- Toggle orthographic/perspective view

### Network State Display
- Current timestep counter
- Firing neuron list (console or on-screen)
- Connection weight visualization (thickness proportional to |weight|)

### Interaction Modes
- **Inference Mode** (current): Manual input control, observe outputs
- **Training Mode**: Physics simulation active, watch network organize
- **Inspection Mode**: Click neuron to see properties (ID, threshold, leak, connections)

## Implementation Priority

### High Priority (Now)
1. Mouse click → Input switch toggle
2. ImGui slider panel for visual parameters
3. Output winner indicator

### Medium Priority (Next)
4. Connection weight → line thickness mapping
5. Neuron inspection on click
6. Camera focus button

### Low Priority (Future)
7. Save/load network state from UI
8. Real-time connection editing (drag to create)
9. Multi-network comparison view

## File Structure

New files to create:
- `src/vis/ui_manager.h/cpp` - Overall UI state management
- `src/vis/input_switch.h/cpp` - Input toggle widgets
- `src/vis/output_indicator.h/cpp` - Output visualization
- `src/vis/mouse_picker.h/cpp` - 3D picking utilities

Modified files:
- `src/vis/OpenGLCanvas.cpp` - Mouse callbacks, UI rendering
- `src/vis/network_graph.h` - Expose parameters as public members
- `src/vis/OpenGLRenderer.cpp` - Render UI elements

## Testing Plan

1. **Input Control**: Click switches, verify correct neuron receives input
2. **Output Indicator**: Inject various inputs, verify correct output is highlighted
3. **Parameter Sliders**: Adjust each parameter, verify visual changes
4. **Performance**: Ensure UI doesn't impact frame rate
5. **Usability**: Test with non-technical user

---

**Next Steps**: 
1. Rebuild with current fixes (background + output physics)
2. Choose input mechanism (recommend Option B: switches)
3. Integrate ImGui or implement keyboard controls
4. Add output indicator
