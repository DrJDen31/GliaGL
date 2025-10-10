#include "input_control.h"
#include <algorithm>
#include <iostream>

// =====================================================================================
// InputControl Implementation
// =====================================================================================

InputControl::InputControl(const std::string& neuron_id, const Vec3f& screen_pos)
    : neuron_id(neuron_id), screen_position(screen_pos), value(0.0f)
{
    // Default dimensions (horizontal slider)
    width = 150.0f;
    height = 30.0f;
}

bool InputControl::containsPoint(float x, float y) const {
    float left = screen_position.x();
    float top = screen_position.y();
    float right = left + width;
    float bottom = top + height;
    
    return (x >= left && x <= right && y >= top && y <= bottom);
}

// =====================================================================================
// InputControlManager Implementation
// =====================================================================================

InputControlManager::InputControlManager() 
    : active_control(nullptr), window_width(800), window_height(600)
{
}

void InputControlManager::initializeControls(const std::vector<std::string>& sensory_ids, 
                                             int win_width, int win_height) {
    window_width = win_width;
    window_height = win_height;
    controls.clear();
    
    // Create a control for each sensory neuron
    // Position them vertically on the left side of the screen
    float start_y = 50.0f;  // Pixels from top
    float spacing = 60.0f;  // Vertical spacing between controls
    float left_margin = 20.0f;
    
    for (size_t i = 0; i < sensory_ids.size(); i++) {
        Vec3f pos(left_margin, start_y + i * spacing, 0);
        controls.push_back(InputControl(sensory_ids[i], pos));
        std::cout << "Created input control for " << sensory_ids[i] 
                  << " at (" << pos.x() << ", " << pos.y() << ")" << std::endl;
    }
}

void InputControlManager::handleMouseClick(float x, float y) {
    // Check if click is on any control
    for (auto& control : controls) {
        if (control.containsPoint(x, y)) {
            active_control = &control;
            
            // Set value based on horizontal position within control
            float local_x = x - control.getScreenPosition().x();
            float normalized = local_x / control.getWidth();
            control.setValue(normalized);
            
            std::cout << control.getNeuronID() << " input set to " 
                      << (normalized * 100.0f) << "%" << std::endl;
            return;
        }
    }
    
    active_control = nullptr;
}

void InputControlManager::handleMouseDrag(float x, float y) {
    if (active_control) {
        // Update value based on horizontal position
        float local_x = x - active_control->getScreenPosition().x();
        float normalized = local_x / active_control->getWidth();
        active_control->setValue(normalized);
    }
}

void InputControlManager::handleMouseRelease() {
    active_control = nullptr;
}

float InputControlManager::getInputForNeuron(const std::string& neuron_id) const {
    for (const auto& control : controls) {
        if (control.getNeuronID() == neuron_id) {
            return control.getInputCurrent();
        }
    }
    return 0.0f;  // No control for this neuron
}

void InputControlManager::setValueForNeuron(const std::string& neuron_id, float value) {
    for (auto& control : controls) {
        if (control.getNeuronID() == neuron_id) {
            control.setValue(value);
            return;
        }
    }
}

void InputControlManager::updateLayout(int win_width, int win_height) {
    window_width = win_width;
    window_height = win_height;
    
    // Reposition controls if needed (currently fixed position, but could scale)
}
