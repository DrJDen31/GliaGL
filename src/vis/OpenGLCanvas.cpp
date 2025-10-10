#include <vector>
#include "OpenGLCanvas.h"
#include "OpenGLCamera.h"
#include "OpenGLRenderer.h"
#include "meshdata.h"
#include "argparser.h"
#include "input_sequence.h"
#include "input_control.h"
#include "network_graph.h"

// ========================================================
// static variables of OpenGLCanvas class

ArgParser* OpenGLCanvas::args = NULL;
MeshData* OpenGLCanvas::mesh_data = NULL;
OpenGLCamera* OpenGLCanvas::camera = NULL;
OpenGLRenderer* OpenGLCanvas::renderer = NULL;
GLFWwindow* OpenGLCanvas::window = NULL;

// mouse position
int OpenGLCanvas::mouseX = 0;
int OpenGLCanvas::mouseY = 0;
// which mouse button
bool OpenGLCanvas::leftMousePressed = false;
bool OpenGLCanvas::middleMousePressed = false;
bool OpenGLCanvas::rightMousePressed = false;
// current state of modifier keys
bool OpenGLCanvas::shiftKeyPressed = false;
bool OpenGLCanvas::controlKeyPressed = false;
bool OpenGLCanvas::altKeyPressed = false;
bool OpenGLCanvas::superKeyPressed = false;
// input toggle states (dynamic for any number of sensory neurons)
std::map<std::string, bool> OpenGLCanvas::sensory_input_enabled;
// visual parameter controls
float OpenGLCanvas::neuron_size_scale = 1.0f;

// ========================================================
// Initialize all appropriate OpenGL variables, set
// callback functions, and start the main event loop.
// This function will not return but can be terminated
// by calling 'exit(0)'
// ========================================================

void OpenGLCanvas::initialize(ArgParser *_args, MeshData *_mesh_data, OpenGLRenderer *_renderer) {
  args = _args;
  mesh_data = _mesh_data;
  renderer = _renderer;
  
  glfwSetErrorCallback(error_callback);

  // Initialize GLFW
  if( !glfwInit() ) {
    std::cerr << "ERROR: Failed to initialize GLFW" << std::endl;
    exit(1);
  }
  
  // We will ask it to specifically open an OpenGL 3.2 context
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a GLFW window
  const char* window_title = args->use_network ? "GliaGL - Neural Network Visualization" : "GliaGL - Cloth Simulation";
  window = glfwCreateWindow(mesh_data->width,mesh_data->height, window_title, NULL, NULL);
  if (!window) {
    std::cerr << "ERROR: Failed to open GLFW window" << std::endl;
    glfwTerminate();
    exit(1);
  }
  glfwMakeContextCurrent(window);
  HandleGLError("in glcanvas first");

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    std::cerr << "ERROR: Failed to initialize GLEW" << std::endl;
    glfwTerminate();
    exit(1);
  }

  // there seems to be a "GL_INVALID_ENUM" error in glewInit that is a
  // know issue, but can safely be ignored
  HandleGLError("after glewInit()",true);

  std::cout << "-------------------------------------------------------" << std::endl;
  std::cout << "OpenGL Version: " << (char*)glGetString(GL_VERSION) << '\n';
  std::cout << "-------------------------------------------------------" << std::endl;

  // Initialize callback functions
  glfwSetCursorPosCallback(OpenGLCanvas::window,OpenGLCanvas::mousemotionCB);
  glfwSetMouseButtonCallback(OpenGLCanvas::window,OpenGLCanvas::mousebuttonCB);
  glfwSetKeyCallback(OpenGLCanvas::window,OpenGLCanvas::keyboardCB);

  // initial placement of camera 
  // look at an object scaled & positioned to just fit in the box (-1,-1,-1)->(1,1,1)
  glm::vec3 camera_position = glm::vec3(1,3,8);
  glm::vec3 point_of_interest = glm::vec3(0,0,0);
  glm::vec3 up = glm::vec3(0,1,0);

  if (mesh_data->perspective) {
    float angle = 20.0;
    camera = new PerspectiveOpenGLCamera(camera_position, point_of_interest, up, angle);
  } else {
    float size = 2.5;
    camera = new OrthographicOpenGLCamera(camera_position, point_of_interest, up, size);
  }
  camera->glPlaceCamera();

  
  HandleGLError("finished glcanvas initialize");
}

// ========================================================
// Callback function for mouse click or release
// ========================================================

void OpenGLCanvas::mousebuttonCB(GLFWwindow */*window*/, int which_button, int action, int /*mods*/) {
  // Check for input control interaction first (network mode only)
  extern ArgParser *GLOBAL_args;
  if (which_button == GLFW_MOUSE_BUTTON_1 && GLOBAL_args && GLOBAL_args->use_network && GLOBAL_args->input_controls) {
    if (action == GLFW_PRESS) {
      // Handle click on input controls
      GLOBAL_args->input_controls->handleMouseClick((float)mouseX, (float)mouseY);
    } else if (action == GLFW_RELEASE) {
      // Handle release
      GLOBAL_args->input_controls->handleMouseRelease();
    }
  }
  
  // store the current state of the mouse buttons
  if (which_button == GLFW_MOUSE_BUTTON_1) {
    if (action == GLFW_PRESS) {
      leftMousePressed = true;
    } else {
      assert (action == GLFW_RELEASE);
      leftMousePressed = false;
    }
  } else if (which_button == GLFW_MOUSE_BUTTON_2) {
    if (action == GLFW_PRESS) {
      rightMousePressed = true;
    } else {
      assert (action == GLFW_RELEASE);
      rightMousePressed = false;
    }
  } else if (which_button == GLFW_MOUSE_BUTTON_3) {
    if (action == GLFW_PRESS) {
      middleMousePressed = true;
    } else {
      assert (action == GLFW_RELEASE);
      middleMousePressed = false;
    }
  }
}	

// ========================================================
// Callback function for mouse drag
// ========================================================

void OpenGLCanvas::mousemotionCB(GLFWwindow */*window*/, double x, double y) {
  // Handle input control dragging (network mode only)
  extern ArgParser *GLOBAL_args;
  bool ui_is_active = false;
  
  if (leftMousePressed && GLOBAL_args && GLOBAL_args->use_network && GLOBAL_args->input_controls) {
    GLOBAL_args->input_controls->handleMouseDrag((float)x, (float)y);
    ui_is_active = GLOBAL_args->input_controls->isActivelyDragging();
  }
  
  // Only handle camera controls if we're NOT dragging a UI element
  if (!ui_is_active) {
    // camera controls that work well for a 3 button mouse
    if (!shiftKeyPressed && !controlKeyPressed && !altKeyPressed) {
      if (leftMousePressed) {
        camera->rotateCamera(mouseX-x,mouseY-y);
      } else if (middleMousePressed)  {
        camera->truckCamera(mouseX-x, y-mouseY);
      } else if (rightMousePressed) {
        camera->dollyCamera(mouseY-y);
      }
    }

    if (leftMousePressed || middleMousePressed || rightMousePressed) {
      if (shiftKeyPressed) {
        camera->zoomCamera(mouseY-y);
      }
      // allow reasonable control for a non-3 button mouse
      if (controlKeyPressed) {
        camera->truckCamera(mouseX-x, y-mouseY);    
      }
      if (altKeyPressed) {
        camera->dollyCamera(y-mouseY);    
      }
    }
  }
  
  mouseX = x;
  mouseY = y;
}

// ========================================================
// Callback function for keyboard events
// ========================================================


// NOTE: These functions are also called by the Mac Metal Objective-C
// code, so we need this extern to allow C code to call C++ functions
// (without function name mangling confusion).

extern "C" {
void Animate();
void Step();
void Load();
void PackMesh();
}

void OpenGLCanvas::keyboardCB(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int mods) {
  // store the modifier keys
  shiftKeyPressed = (GLFW_MOD_SHIFT & mods);
  controlKeyPressed = (GLFW_MOD_CONTROL & mods);
  altKeyPressed = (GLFW_MOD_ALT & mods);
  superKeyPressed = (GLFW_MOD_SUPER & mods);
  // non modifier key actions
  if (key == GLFW_KEY_ESCAPE || key == 'q' || key == 'Q') {
    glfwSetWindowShouldClose(OpenGLCanvas::window, GL_TRUE);
    // force quit
    exit(0);
  }
  if (action == GLFW_PRESS && key < 256) {
    switch (key) {
    case 'a': case 'A':
      // start continuous animation
      if (!mesh_data->animate) {
        printf ("animation started, press 'X' to stop\n");
        mesh_data->animate = true;
      }
      break;
    case 'x': case 'X':
      // stop continuous animation
      if (mesh_data->animate) {
        printf ("animation stopped, press 'A' to start\n");
        mesh_data->animate = false;
      }
      break;
    case ' ':
      // a single step of animation
      mesh_data->animate = false;
      printf ("press 'A' to start continuous animation\n");
      Step();
      break; 
    case 'm':  case 'M': 
      mesh_data->particles = !mesh_data->particles;
      break; 
    case 'v':  case 'V': 
      mesh_data->velocity = !mesh_data->velocity;
      break; 
    case 'f':  case 'F': 
      mesh_data->force = !mesh_data->force;
      break; 
    case 's':  case 'S': 
      mesh_data->surface = !mesh_data->surface;
      break; 
    case 'w':  case 'W':
      mesh_data->wireframe = !mesh_data->wireframe;
      break;
    case 'g': case 'G':
      mesh_data->gouraud = !mesh_data->gouraud;
      break;
    case 'b':  case 'B':
      mesh_data->bounding_box = !mesh_data->bounding_box;
      break; 
    // 'r'/'R' handled in network controls section below
    // (to avoid duplicate case)
    
    // NETWORK VISUALIZATION CONTROLS
    case 't':  case 'T':
      // Toggle training mode (physics-based layout)
      if (args->use_network && args->network_graph) {
        args->network_graph->setTrainingMode(true);
        std::cout << "Training mode: ON (neurons move via physics)" << std::endl;
      }
      break;
    case 'i':  case 'I':
      // Toggle inference mode (activity visualization)
      if (args->use_network && args->network_graph) {
        args->network_graph->setTrainingMode(false);
        std::cout << "Inference mode: ON (neurons show activations)" << std::endl;
      }
      break;
    case 'n':  case 'N':
      // Single network step (same as spacebar)
      if (args->use_network) {
        Step();  // Use the same Step() function as spacebar
        std::cout << "Network step executed" << std::endl;
      }
      break;
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      // Load custom test from --tests files
      if (args->use_network) {
        int test_index = key - '1';  // '1' -> 0, '2' -> 1, etc.
        if (test_index < (int)args->loaded_tests.size()) {
          // Load the custom test from file
          delete args->input_sequence;
          args->input_sequence = new InputSequence();
          *args->input_sequence = *args->loaded_tests[test_index];
          mesh_data->current_tick = 0;
          mesh_data->last_tick_time = glfwGetTime();
          std::cout << "Loaded test " << (test_index + 1) << ": " 
                    << args->test_files[test_index] << std::endl;
        } else {
          std::cout << "No test loaded in slot " << (test_index + 1) 
                    << " (use --tests to load .seq files)" << std::endl;
        }
      }
      break;
    case '0':
      // Clear test sequence (return to manual control)
      if (args->use_network) {
        delete args->input_sequence;
        args->input_sequence = nullptr;
        mesh_data->current_tick = 0;
        std::cout << "Test sequence cleared (manual control mode)" << std::endl;
      }
      break;
    case '`':  case '~':
      // Clear all sensory inputs (manual mode)
      if (args->use_network && args->glia) {
        // Clear all sensory inputs
        sensory_input_enabled.clear();
        
        // Sync all UI sliders to 0
        if (args->input_controls) {
          for (int i = 0; i < 10; i++) {  // Support up to S0-S9
            std::string sensory_id = "S" + std::to_string(i);
            args->input_controls->setValueForNeuron(sensory_id, 0.0f);
          }
        }
        std::cout << "All inputs cleared (OFF)" << std::endl;
      }
      break;
    case 'p':  case 'P':
      // Toggle particles (neurons) - reusing 'm' functionality but with P
      mesh_data->particles = !mesh_data->particles;
      std::cout << "Particles (neurons): " << (mesh_data->particles ? "ON" : "OFF") << std::endl;
      break;
    case '[':
      // Decrease neuron size
      neuron_size_scale = std::max(0.1f, neuron_size_scale - 0.1f);
      std::cout << "Neuron size scale: " << neuron_size_scale << std::endl;
      break;
    case ']':
      // Increase neuron size
      neuron_size_scale = std::min(5.0f, neuron_size_scale + 0.1f);
      std::cout << "Neuron size scale: " << neuron_size_scale << std::endl;
      break;
    case ',':  case '<':
      // Slower: multiply seconds_per_tick by 2 (logarithmic)
      if (args->use_network) {
        mesh_data->seconds_per_tick = std::min(5.0f, mesh_data->seconds_per_tick * 2.0f);
        float ticks_per_sec = 1.0f / mesh_data->seconds_per_tick;
        std::cout << "Speed: " << mesh_data->seconds_per_tick << " s/tick (" 
                  << ticks_per_sec << " ticks/s)" << std::endl;
      }
      break;
    case '.':  case '>':
      // Faster: divide seconds_per_tick by 2 (logarithmic)
      if (args->use_network) {
        mesh_data->seconds_per_tick = std::max(0.001f, mesh_data->seconds_per_tick * 0.5f);
        float ticks_per_sec = 1.0f / mesh_data->seconds_per_tick;
        std::cout << "Speed: " << mesh_data->seconds_per_tick << " s/tick (" 
                  << ticks_per_sec << " ticks/s)" << std::endl;
      }
      break;
    case '-':  case '_':
      // Cloth only: halve timestep
      if (!args->use_network) {
        std::cout << "timestep halved:  " << mesh_data->timestep << " -> ";
        mesh_data->timestep /= 2.0; 
        std::cout << mesh_data->timestep << std::endl;
      }
      break;
    case '=':  case '+':
      // Cloth only: double timestep
      if (!args->use_network) {
        std::cout << "timestep doubled:  " << mesh_data->timestep << " -> ";
        mesh_data->timestep *= 2.0; 
        std::cout << mesh_data->timestep << std::endl;
      }
      break;
    case 'c':  case 'C':
      // Toggle tick counter display
      if (args->use_network) {
        mesh_data->show_tick_counter = !mesh_data->show_tick_counter;
        std::cout << "Tick counter: " << (mesh_data->show_tick_counter ? "ON" : "OFF") << std::endl;
      }
      break;
    case 'r':  case 'R': 
      // Reset: tick counter for network, reload for cloth
      if (args->use_network) {
        mesh_data->current_tick = 0;
        if (args->input_sequence) {
          args->input_sequence->reset();
        }
        std::cout << "Tick counter reset to 0" << std::endl;
      } else {
        // Cloth: reset system
        Load();
      }
      break;
    
    case 'q':  case 'Q':
      exit(0);
      break;
    default:
      std::cout << "UNKNOWN KEYBOARD INPUT  '" << (char)key << "'" << std::endl;
    }
    PackMesh();
    renderer->updateVBOs();
  }
}

// ========================================================
// Load the vertex & fragment shaders
// ========================================================

GLuint LoadShaders(const std::string &vertex_file_path,const std::string &fragment_file_path){

  // Create the shaders
  GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
  GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
  
  // Read the Vertex Shader code from the file
  std::string VertexShaderCode;
  std::ifstream VertexShaderStream(vertex_file_path.c_str(), std::ios::in);
  if (VertexShaderStream.is_open()){
    std::string Line = "";
    while(getline(VertexShaderStream, Line))
      VertexShaderCode += "\n" + Line;
    VertexShaderStream.close();
  } else {
    std::cerr << "ERROR: cannot open " << vertex_file_path << std::endl;
    exit(0);
  }
  // Read the Fragment Shader code from the file
  std::string FragmentShaderCode;
  std::ifstream FragmentShaderStream(fragment_file_path.c_str(), std::ios::in);
  if(FragmentShaderStream.is_open()){
    std::string Line = "";
    while(getline(FragmentShaderStream, Line))
      FragmentShaderCode += "\n" + Line;
    FragmentShaderStream.close();
  } else {
    std::cerr << "ERROR: cannot open " << vertex_file_path << std::endl;
    exit(0);
  }
  
  GLint Result = GL_FALSE;
  int InfoLogLength;
  
  // Compile Vertex Shader
  std::cout << "Compiling shader : " << vertex_file_path << std::endl;
  char const * VertexSourcePointer = VertexShaderCode.c_str();
  glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
  glCompileShader(VertexShaderID);
  // Check Vertex Shader
  glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if ( InfoLogLength > 0 ){
    std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    std::cerr << "VERTEX SHADER ERROR: " << &VertexShaderErrorMessage[0] << std::endl;
  }
  
  // Compile Fragment Shader
  std::cout << "Compiling shader : " << fragment_file_path << std::endl;
  char const * FragmentSourcePointer = FragmentShaderCode.c_str();
  glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
  glCompileShader(FragmentShaderID);
  // Check Fragment Shader
  glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if ( InfoLogLength > 0 ){
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    std::cerr << "FRAGMENT SHADER ERROR: " << &FragmentShaderErrorMessage[0] << std::endl;
  }
  
  // Link the program
  std::cout << "Linking program" << std::endl;
  GLuint ProgramID = glCreateProgram();
  glAttachShader(ProgramID, VertexShaderID);
  glAttachShader(ProgramID, FragmentShaderID);
  glLinkProgram(ProgramID);
  // Check the program
  glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
  glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::cout << "Link status: " << (Result == GL_TRUE ? "SUCCESS" : "FAILED") << std::endl;
  std::cout << "Info log length: " << InfoLogLength << std::endl;
  if ( InfoLogLength > 1 ){
    std::vector<char> ProgramErrorMessage(InfoLogLength+1);
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    std::cerr << "SHADER LINKING MESSAGE: " << &ProgramErrorMessage[0] << std::endl;
  }
  
  glDeleteShader(VertexShaderID);
  glDeleteShader(FragmentShaderID);
  
  return ProgramID;
}

// ========================================================
// Functions related to error handling
// ========================================================

void OpenGLCanvas::error_callback(int error, const char* description) {
  std::cerr << "ERROR CALLBACK: " << error << " " << description << std::endl;
}

std::string WhichGLError(GLenum &error) {
  switch (error) {
  case GL_NO_ERROR:
    return "NO_ERROR";
  case GL_INVALID_ENUM:
    return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE:
    return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION:
    return "GL_INVALID_OPERATION";
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    return "GL_INVALID_FRAMEBUFFER_OPERATION";
  case GL_OUT_OF_MEMORY:
    return "GL_OUT_OF_MEMORY";
  case GL_STACK_UNDERFLOW:
    return "GL_STACK_UNDERFLOW";
  case GL_STACK_OVERFLOW:
    return "GL_STACK_OVERFLOW";
  default:
    return "OTHER GL ERROR";
  }
}

int HandleGLError(const std::string &message, bool ignore) {
  GLenum error;
  int i = 0;
  while ((error = glGetError()) != GL_NO_ERROR) {
    if (!ignore) {
      if (message != "") {
	std::cout << "[" << message << "] ";
      }
      std::cout << "GL ERROR(" << i << ") " << WhichGLError(error) << std::endl;
    }
    i++;
  }
  if (i == 0) return 1;
  return 0;
}

// ========================================================
// ========================================================
