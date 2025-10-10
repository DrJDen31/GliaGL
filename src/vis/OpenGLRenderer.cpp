// ==================================================================
// OpenGL Rendering of the MeshData data
// ==================================================================

#include "OpenGLRenderer.h"
#include "OpenGLCamera.h"
#include "OpenGLCanvas.h"
#include "meshdata.h"
#include "matrix.h"
#include "boundingbox.h"
#include "argparser.h"
#include "network_graph.h"
#include "ui_renderer.h"
#include <iostream>

// NOTE: These functions are also called by the Mac Metal Objective-C
// code, so we need this extern to allow C code to call C++ functions
// (without function name mangling confusion).

extern "C" {
void Animate();
}

// ====================================================================
OpenGLRenderer::OpenGLRenderer(MeshData *_mesh_data, ArgParser *args) {
  mesh_data = _mesh_data;

  OpenGLCanvas::initialize(args,mesh_data,this);

  // Initialize the MeshData
  setupVBOs();
  
  glClearColor(0.45f, 0.45f, 0.48f, 1.0f);  // Light gray-blue background
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); 
  glDisable(GL_CULL_FACE);
  glEnable(GL_PROGRAM_POINT_SIZE);  // Allow shader to control point size
  glEnable(GL_POINT_SMOOTH);        // Smooth/round points (if supported)

  // Create and compile our GLSL program from the shaders
  GLuint programID = LoadShaders( args->path+"/OpenGL.vertexshader", args->path+"/OpenGL.fragmentshader" );
  
  // Get handles for our uniforms
  MatrixID = glGetUniformLocation(programID, "MVP");
  LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
  ViewMatrixID = glGetUniformLocation(programID, "V");
  ModelMatrixID = glGetUniformLocation(programID, "M");
  wireframeID = glGetUniformLocation(programID, "wireframe");
  neuronSizeScaleID = glGetUniformLocation(programID, "neuronSizeScale");
  
  // Initialize UI renderer for 2D overlay
  UIRenderer::initialize(args->path);

  while (!glfwWindowShouldClose(OpenGLCanvas::window))  {
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(programID);

    OpenGLCanvas::camera->glPlaceCamera();

    // transform the object as necessary to fit in the
    // (-1,-1,-1)->(1,1,1) box
    glm::vec3 bb_center(-mesh_data->bb_center.data[0],-mesh_data->bb_center.data[1],-mesh_data->bb_center.data[2]);
    glm::vec3 bb_scale(mesh_data->bb_scale,mesh_data->bb_scale,mesh_data->bb_scale);
    glm::mat4 ModelMatrix(1.0);
    ModelMatrix = glm::scale<GLfloat>(ModelMatrix,bb_scale);
    ModelMatrix = glm::translate<GLfloat>(ModelMatrix,bb_center);

    // Build the matrix to position the camera based on keyboard and mouse input
    glm::mat4 ProjectionMatrix = OpenGLCanvas::camera->getProjectionMatrix();
    glm::mat4 ViewMatrix = OpenGLCanvas::camera->getViewMatrix();
    glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

    Animate();
    updateVBOs();
    
    // pass the matrix to the draw routines (for further editing)
    drawVBOs(MVP,ModelMatrix,ViewMatrix);
    
    // Render 2D UI overlay (input controls and output indicators)
    extern ArgParser *GLOBAL_args;
    if (GLOBAL_args && GLOBAL_args->use_network) {
      // Render input sliders
      if (GLOBAL_args->input_controls) {
        UIRenderer::renderInputControls(GLOBAL_args->input_controls, mesh_data->width, mesh_data->height);
      }
      
      // Render winner indicator
      if (GLOBAL_args->network_graph) {
        std::string winner = GLOBAL_args->network_graph->getCurrentWinner();
        UIRenderer::renderWinnerIndicator(winner, mesh_data->width, mesh_data->height);
      }
      
      // Render tick clock (flashing indicator)
      if (GLOBAL_args->mesh_data->show_tick_counter) {
        UIRenderer::renderTickClock(GLOBAL_args->mesh_data->current_tick, mesh_data->width, mesh_data->height);
      }
    }

    // Swap buffers
    glfwSwapBuffers(OpenGLCanvas::window);
    glfwPollEvents();  
  }
  
  cleanupVBOs();
  glDeleteProgram(programID);
  
  // Close OpenGL window and terminate GLFW
  glfwDestroyWindow(OpenGLCanvas::window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

// ====================================================================

void OpenGLRenderer::setupVBOs() {
  glGenVertexArrays(1, &cloth_VaoId);
  glGenBuffers(1, &cloth_tris_VBO);
}

void OpenGLRenderer::drawVBOs(const glm::mat4 &mvp,const glm::mat4 &m,const glm::mat4 &v) {
  // Note: programID should already be active from main loop
  Vec3f lightPos = Vec3f(4,4,4);
  glUniform3f(LightID, lightPos.x(), lightPos.y(), lightPos.z());
  glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
  glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &m[0][0]);
  glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &v[0][0]);
  glUniform1i(wireframeID, mesh_data->wireframe);
  glUniform1f(neuronSizeScaleID, OpenGLCanvas::neuron_size_scale);
  drawMesh();
}

void OpenGLRenderer::cleanupVBOs() {
  cleanupMesh();
}

// ====================================================================


void OpenGLRenderer::updateVBOs() {
  glBindVertexArray(cloth_VaoId);
  glBindBuffer(GL_ARRAY_BUFFER, cloth_tris_VBO);
  
  // Both network and cloth use the same VBO structure
  // Network data is packed into mesh_data->clothTriData by NetworkGraph::packMesh()
  int sizeOfVertices = 3*sizeof(glm::vec4) * mesh_data->clothTriCount * 3;
  glBufferData(GL_ARRAY_BUFFER, sizeOfVertices, mesh_data->clothTriData, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 3*sizeof(glm::vec4), 0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 3*sizeof(glm::vec4), (void*)sizeof(glm::vec4));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 3*sizeof(glm::vec4), (void*)(sizeof(glm::vec4)*2));
}

void OpenGLRenderer::drawMesh() const {
  glBindVertexArray(cloth_VaoId);
  glBindBuffer(GL_ARRAY_BUFFER, cloth_tris_VBO);
  
  // Check if we're rendering a network or cloth
  extern ArgParser *GLOBAL_args;
  if (GLOBAL_args && GLOBAL_args->use_network && GLOBAL_args->network_graph) {
    int conn_count = GLOBAL_args->network_graph->getConnectionVertexCount();
    int neuron_count = GLOBAL_args->network_graph->getNeuronVertexCount();
    
    // Network rendering: connections (lines), then neurons (points)
    
    // Draw connections as lines (if enabled)
    if (mesh_data->wireframe && conn_count > 0) {
      glLineWidth(3.0f);  // Thicker lines for better visibility
      glDrawArrays(GL_LINES, 0, conn_count);
    }
    
    // Draw neurons as points (if enabled)
    // Point size now controlled by shader (world-space sizing)
    if (mesh_data->particles && neuron_count > 0) {
      glDrawArrays(GL_POINTS, conn_count, neuron_count);  // Start after connections
    }
  } else {
    // Cloth rendering: draw as triangles
    glDrawArrays(GL_TRIANGLES, 0, 3 * mesh_data->clothTriCount);
  }
}

void OpenGLRenderer::cleanupMesh() {
  glDeleteBuffers(1, &cloth_tris_VBO);
  glDeleteBuffers(1, &cloth_VaoId);
}

// ====================================================================
