#include "ui_renderer.h"
#include "OpenGLCanvas.h"
#include "argparser.h"
#include "meshdata.h"
#include <iostream>
#include <vector>

// External function to load shaders
extern GLuint LoadShaders(const std::string &vertex_file_path, const std::string &fragment_file_path);

// =====================================================================================
// Static member initialization
// =====================================================================================

GLuint UIRenderer::ui_program_id = 0;
GLuint UIRenderer::ui_vao = 0;
GLuint UIRenderer::ui_vbo = 0;
GLuint UIRenderer::screen_size_uniform = 0;
bool UIRenderer::initialized = false;
std::vector<float> UIRenderer::vertex_buffer;

// =====================================================================================
// Initialization and Cleanup
// =====================================================================================

void UIRenderer::initialize(const std::string& shader_path) {
    if (initialized) return;
    
    // Load UI shaders
    ui_program_id = LoadShaders(shader_path + "/UI.vertexshader", 
                                 shader_path + "/UI.fragmentshader");
    
    // Get uniform locations
    screen_size_uniform = glGetUniformLocation(ui_program_id, "screenSize");
    
    // Create VAO and VBO
    glGenVertexArrays(1, &ui_vao);
    glGenBuffers(1, &ui_vbo);
    
    glBindVertexArray(ui_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
    
    // Set up vertex attributes
    // Position (2 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    
    // Color (4 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
    
    initialized = true;
    std::cout << "UIRenderer initialized with modern OpenGL" << std::endl;
}

void UIRenderer::cleanup() {
    if (!initialized) return;
    
    glDeleteBuffers(1, &ui_vbo);
    glDeleteVertexArrays(1, &ui_vao);
    glDeleteProgram(ui_program_id);
    
    initialized = false;
}

// =====================================================================================
// Helper Functions
// =====================================================================================

void UIRenderer::drawRect(float x, float y, float width, float height, 
                         float r, float g, float b, float alpha) {
    // Add two triangles (6 vertices) to the batch buffer
    // Triangle 1: top-left, top-right, bottom-left
    // Triangle 2: bottom-left, top-right, bottom-right
    
    float vertices[] = {
        // Triangle 1
        x, y, r, g, b, alpha,                           // top-left
        x + width, y, r, g, b, alpha,                   // top-right
        x, y + height, r, g, b, alpha,                  // bottom-left
        
        // Triangle 2
        x, y + height, r, g, b, alpha,                  // bottom-left
        x + width, y, r, g, b, alpha,                   // top-right
        x + width, y + height, r, g, b, alpha           // bottom-right
    };
    
    vertex_buffer.insert(vertex_buffer.end(), vertices, vertices + 36);
}

void UIRenderer::flushRects() {
    if (vertex_buffer.empty()) return;
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_buffer.size() * sizeof(float), 
                 vertex_buffer.data(), GL_DYNAMIC_DRAW);
    
    // Draw
    glBindVertexArray(ui_vao);
    glDrawArrays(GL_TRIANGLES, 0, vertex_buffer.size() / 6);
    glBindVertexArray(0);
    
    // Clear buffer for next frame
    vertex_buffer.clear();
}

// =====================================================================================
// Rendering Functions
// =====================================================================================

void UIRenderer::renderInputControls(InputControlManager* controls, int window_width, int window_height) {
    if (!initialized || !controls) return;
    
    // Use UI shader
    glUseProgram(ui_program_id);
    glUniform2f(screen_size_uniform, (float)window_width, (float)window_height);
    
    // Disable depth testing for 2D overlay
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render each control
    for (const auto& control : controls->getControls()) {
        Vec3f pos = control.getScreenPosition();
        float width = control.getWidth();
        float height = control.getHeight();
        float value = control.getValue();
        
        // Draw label indicator (colored square)
        std::string neuron_id = control.getNeuronID();
        float r = 0.3f, g = 0.5f, b = 1.0f; // Default blue
        if (neuron_id == "S1") {
            r = 0.6f; g = 0.4f; b = 1.0f;  // Purple
        } else if (neuron_id == "S2") {
            r = 1.0f; g = 0.4f; b = 0.6f;  // Pink
        }
        drawRect(pos.x() - 25, pos.y() + 5, 15, 15, r, g, b, 1.0f);
        
        // Draw background track (dark gray)
        drawRect(pos.x(), pos.y(), width, height, 0.2f, 0.2f, 0.2f, 0.8f);
        
        // Draw filled portion (cyan for active)
        if (value > 0.0f) {
            drawRect(pos.x(), pos.y(), width * value, height, 0.3f, 0.7f, 1.0f, 0.9f);
        }
        
        // Draw border
        drawRect(pos.x() - 1, pos.y() - 1, width + 2, 1, 0.5f, 0.5f, 0.5f, 1.0f); // Top
        drawRect(pos.x() - 1, pos.y() + height, width + 2, 1, 0.5f, 0.5f, 0.5f, 1.0f); // Bottom
        drawRect(pos.x() - 1, pos.y(), 1, height, 0.5f, 0.5f, 0.5f, 1.0f); // Left
        drawRect(pos.x() + width, pos.y(), 1, height, 0.5f, 0.5f, 0.5f, 1.0f); // Right
    }
    
    flushRects();
    
    // Restore OpenGL state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void UIRenderer::renderWinnerIndicator(const std::string& winner_id,
                                      int window_width, int window_height) {
    if (!initialized) return;
    
    // Use UI shader
    glUseProgram(ui_program_id);
    glUniform2f(screen_size_uniform, (float)window_width, (float)window_height);
    
    // Disable depth testing for 2D overlay
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw winner indicator in top-right corner
    float x_pos = window_width - 180;
    float y_pos = 20;
    float width = 160;
    float height = 60;
    
    if (!winner_id.empty()) {
        // Draw background panel
        drawRect(x_pos, y_pos, width, height, 0.15f, 0.15f, 0.15f, 0.9f);
        
        // Draw bright indicator box (magenta/purple for winner)
        drawRect(x_pos + 10, y_pos + 10, 40, 40, 1.0f, 0.3f, 1.0f, 1.0f);
        
        // Draw simple character representation (O0, O1, O2, etc.)
        // Extract number from ID (e.g., "O1" -> 1)
        if (winner_id.length() >= 2) {
            char digit = winner_id[1];
            float digit_x = x_pos + 60;
            float digit_y = y_pos + 10;
            float digit_size = 40;  // Even larger
            
            // Draw "O" character (circle-ish)
            float o_thickness = 5;
            drawRect(digit_x, digit_y, digit_size, o_thickness, 1.0f, 1.0f, 1.0f, 1.0f);  // Top
            drawRect(digit_x, digit_y + digit_size - o_thickness, digit_size, o_thickness, 1.0f, 1.0f, 1.0f, 1.0f);  // Bottom
            drawRect(digit_x, digit_y, o_thickness, digit_size, 1.0f, 1.0f, 1.0f, 1.0f);  // Left
            drawRect(digit_x + digit_size - o_thickness, digit_y, o_thickness, digit_size, 1.0f, 1.0f, 1.0f, 1.0f);  // Right
            
            // Draw digit using 7-segment-like pattern
            digit_x += 50;
            float seg_w = 25;  // Even wider segments
            float seg_h = 5;   // Even thicker segments
            
            switch (digit) {
                case '0':
                    drawRect(digit_x, digit_y, seg_w, seg_h, 1.0f, 1.0f, 1.0f, 1.0f);  // Top
                    drawRect(digit_x, digit_y + digit_size - seg_h, seg_w, seg_h, 1.0f, 1.0f, 1.0f, 1.0f);  // Bottom
                    drawRect(digit_x, digit_y, seg_h, digit_size / 2, 1.0f, 1.0f, 1.0f, 1.0f);  // Top-left
                    drawRect(digit_x + seg_w - seg_h, digit_y, seg_h, digit_size / 2, 1.0f, 1.0f, 1.0f, 1.0f);  // Top-right
                    drawRect(digit_x, digit_y + digit_size / 2, seg_h, digit_size / 2, 1.0f, 1.0f, 1.0f, 1.0f);  // Bottom-left
                    drawRect(digit_x + seg_w - seg_h, digit_y + digit_size / 2, seg_h, digit_size / 2, 1.0f, 1.0f, 1.0f, 1.0f);  // Bottom-right
                    break;
                case '1':
                    drawRect(digit_x + seg_w - seg_h, digit_y, seg_h, digit_size, 1.0f, 1.0f, 1.0f, 1.0f);  // Right side only
                    break;
                case '2':
                    drawRect(digit_x, digit_y, seg_w, seg_h, 1.0f, 1.0f, 1.0f, 1.0f);  // Top
                    drawRect(digit_x, digit_y + digit_size / 2 - seg_h / 2, seg_w, seg_h, 1.0f, 1.0f, 1.0f, 1.0f);  // Middle
                    drawRect(digit_x, digit_y + digit_size - seg_h, seg_w, seg_h, 1.0f, 1.0f, 1.0f, 1.0f);  // Bottom
                    drawRect(digit_x + seg_w - seg_h, digit_y, seg_h, digit_size / 2, 1.0f, 1.0f, 1.0f, 1.0f);  // Top-right
                    drawRect(digit_x, digit_y + digit_size / 2, seg_h, digit_size / 2, 1.0f, 1.0f, 1.0f, 1.0f);  // Bottom-left
                    break;
                case '3':
                    drawRect(digit_x, digit_y, seg_w, seg_h, 1.0f, 1.0f, 1.0f, 1.0f);  // Top
                    drawRect(digit_x, digit_y + digit_size / 2 - seg_h / 2, seg_w, seg_h, 1.0f, 1.0f, 1.0f, 1.0f);  // Middle
                    drawRect(digit_x, digit_y + digit_size - seg_h, seg_w, seg_h, 1.0f, 1.0f, 1.0f, 1.0f);  // Bottom
                    drawRect(digit_x + seg_w - seg_h, digit_y, seg_h, digit_size, 1.0f, 1.0f, 1.0f, 1.0f);  // Right side
                    break;
                // Add more digits as needed
                default:
                    // Unknown - draw a box
                    drawRect(digit_x, digit_y, seg_w, digit_size, 0.8f, 0.8f, 0.8f, 1.0f);
                    break;
            }
        }
        
        // Border
        drawRect(x_pos - 1, y_pos - 1, width + 2, 1, 0.7f, 0.7f, 0.7f, 1.0f);
        drawRect(x_pos - 1, y_pos + height, width + 2, 1, 0.7f, 0.7f, 0.7f, 1.0f);
        drawRect(x_pos - 1, y_pos, 1, height, 0.7f, 0.7f, 0.7f, 1.0f);
        drawRect(x_pos + width, y_pos, 1, height, 0.7f, 0.7f, 0.7f, 1.0f);
    }
    
    flushRects();
    
    // Restore OpenGL state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void UIRenderer::renderTickClock(int current_tick,
                                int window_width, int window_height) {
    if (!initialized) return;
    
    // Use UI shader
    glUseProgram(ui_program_id);
    glUniform2f(screen_size_uniform, (float)window_width, (float)window_height);
    
    // Disable depth testing for 2D overlay
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw tick clock in top-right corner (below winner indicator)
    float x_pos = window_width - 180;
    float y_pos = 90;  // Below larger winner indicator
    float size = 40;  // Larger clock
    
    // Determine if animating and calculate tick rate
    extern ArgParser *GLOBAL_args;
    bool is_animating = false;
    float seconds_per_tick = 1.0f;
    if (GLOBAL_args && GLOBAL_args->mesh_data) {
        is_animating = GLOBAL_args->mesh_data->animate;
        seconds_per_tick = GLOBAL_args->mesh_data->seconds_per_tick;
    }
    
    // Calculate approximate ticks per second
    float ticks_per_second = (seconds_per_tick > 0.0f) ? (1.0f / seconds_per_tick) : 0.0f;
    
    // Flash for slow speeds (<32 tps), solid for fast speeds (>=32 tps)
    bool should_flash = (ticks_per_second < 32.0f);
    bool is_on = (current_tick % 2) == 0;  // Alternate each tick
    
    // Draw circle indicator
    if (!is_animating) {
        // Dim gray when paused
        drawRect(x_pos + 60, y_pos, size, size, 0.3f, 0.3f, 0.3f, 0.6f);
    } else if (should_flash) {
        // Flash green/dim at slow speeds
        if (is_on) {
            drawRect(x_pos + 60, y_pos, size, size, 0.2f, 1.0f, 0.3f, 1.0f);  // Bright green
        } else {
            drawRect(x_pos + 60, y_pos, size, size, 0.1f, 0.4f, 0.15f, 0.8f);  // Dim green
        }
    } else {
        // Solid green at fast speeds
        drawRect(x_pos + 60, y_pos, size, size, 0.2f, 1.0f, 0.3f, 1.0f);
    }
    
    // Border
    drawRect(x_pos + 60 - 1, y_pos - 1, size + 2, 1, 0.5f, 0.5f, 0.5f, 1.0f);
    drawRect(x_pos + 60 - 1, y_pos + size, size + 2, 1, 0.5f, 0.5f, 0.5f, 1.0f);
    drawRect(x_pos + 60 - 1, y_pos, 1, size, 0.5f, 0.5f, 0.5f, 1.0f);
    drawRect(x_pos + 60 + size, y_pos, 1, size, 0.5f, 0.5f, 0.5f, 1.0f);
    
    flushRects();
    
    // Restore OpenGL state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
