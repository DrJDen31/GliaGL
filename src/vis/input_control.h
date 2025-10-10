#ifndef _INPUT_CONTROL_H_
#define _INPUT_CONTROL_H_

#include <string>
#include <vector>
#include "vectors.h"

// =====================================================================================
// InputControl - Visual control for setting sensory neuron input
// =====================================================================================

class InputControl {
public:
    InputControl(const std::string& neuron_id, const Vec3f& screen_pos);
    
    // Accessors
    const std::string& getNeuronID() const { return neuron_id; }
    float getValue() const { return value; }
    void setValue(float v) { value = std::max(0.0f, std::min(1.0f, v)); }
    
    // Get input current to inject (maps 0-1 to 0-max_input)
    float getInputCurrent() const { return value * max_input_current; }
    
    // Screen position (for 2D overlay rendering and mouse picking)
    const Vec3f& getScreenPosition() const { return screen_position; }
    void setScreenPosition(const Vec3f& pos) { screen_position = pos; }
    
    // Check if mouse is over this control (for clicking/dragging)
    bool containsPoint(float x, float y) const;
    
    // Dimensions
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    
private:
    std::string neuron_id;      // Which sensory neuron this controls (e.g., "S0")
    float value;                // Current value (0.0 = off, 1.0 = max)
    Vec3f screen_position;      // Position in screen space (pixels from top-left)
    
    // Visual properties
    float width;                // Control width in pixels
    float height;               // Control height in pixels
    
    // Input mapping
    static constexpr float max_input_current = 200.0f;  // Max input to inject
};

// =====================================================================================
// InputControlManager - Manages all input controls
// =====================================================================================

class InputControlManager {
public:
    InputControlManager();
    
    // Setup controls for sensory neurons
    void initializeControls(const std::vector<std::string>& sensory_ids, int window_width, int window_height);
    
    // Mouse interaction
    void handleMouseClick(float x, float y);
    void handleMouseDrag(float x, float y);
    void handleMouseRelease();
    
    // Get input current for a specific neuron ID
    float getInputForNeuron(const std::string& neuron_id) const;
    
    // Set value for a specific neuron (for keyboard control sync)
    void setValueForNeuron(const std::string& neuron_id, float value);
    
    // Rendering (returns geometry for overlay rendering)
    const std::vector<InputControl>& getControls() const { return controls; }
    
    // Check if actively dragging a control (to prevent camera movement)
    bool isActivelyDragging() const { return active_control != nullptr; }
    
    // Update layout when window resizes
    void updateLayout(int window_width, int window_height);
    
private:
    std::vector<InputControl> controls;
    InputControl* active_control;  // Currently being dragged
    int window_width;
    int window_height;
};

#endif // _INPUT_CONTROL_H_
