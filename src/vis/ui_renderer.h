#ifndef _UI_RENDERER_H_
#define _UI_RENDERER_H_

#include <GL/glew.h>
#include "input_control.h"

// =====================================================================================
// UIRenderer - Renders 2D UI overlay elements using modern OpenGL
// =====================================================================================

class UIRenderer {
public:
    static void initialize(const std::string& shader_path);
    static void cleanup();
    
    static void renderInputControls(InputControlManager* controls, int window_width, int window_height);
    static void renderWinnerIndicator(const std::string& winner_id, 
                                      int window_width, int window_height);
    static void renderTickClock(int current_tick, 
                               int window_width, int window_height);
    
private:
    // Shader and VBO resources
    static GLuint ui_program_id;
    static GLuint ui_vao;
    static GLuint ui_vbo;
    static GLuint screen_size_uniform;
    static bool initialized;
    
    // Helper to batch render rectangles
    static void drawRect(float x, float y, float width, float height, 
                        float r, float g, float b, float alpha = 1.0f);
    static void flushRects();
    
    // Vertex data buffer
    static std::vector<float> vertex_buffer;
};

#endif // _UI_RENDERER_H_
