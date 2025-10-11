#include <string.h>
#include <string.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <random>

#include "matrix.h"
#include "meshdata.h"
#include "argparser.h"
#include "cloth.h"
#include "network_graph.h"
#include "glia.h"
#include "input_control.h"
#include "../arch/input_sequence.h"
#include "OpenGLCanvas.h"

// ====================================================================


// default values for the MeshData variables
void INIT_MeshData(MeshData *mesh_data) {
  mesh_data->width = 400;
  mesh_data->height = 400;
  mesh_data->timestep = 0.01;
  mesh_data->animate = false;
  
  // Network simulation control
  mesh_data->current_tick = 0;
  mesh_data->seconds_per_tick = 1.0f;  // Default: 1 tick per second
  mesh_data->show_tick_counter = true;  // Show tick counter by default
  mesh_data->last_tick_time = 0.0;

  mesh_data->particles = true;
  mesh_data->surface = true;
  mesh_data->velocity = false;
  mesh_data->force = false;
  mesh_data->wireframe = true;
  mesh_data->bounding_box = true;
  mesh_data->gouraud = false;
  mesh_data->perspective = true;

  mesh_data->clothTriCount = 0;
  mesh_data->clothTriData = NULL;

  mesh_data->gravity.data[0] = 0;
  mesh_data->gravity.data[1] = -9.8;
  mesh_data->gravity.data[2] = 0;
}


// ====================================================================
// ====================================================================

// NOTE: These functions are called by the Objective-C code, so we
// need this extern to allow C code to call C++ functions (without
// function name mangling confusion).

// Also, they use global variables...  

extern "C" {

  void PackMesh() {
    if (GLOBAL_args->use_network && GLOBAL_args->network_graph) {
      GLOBAL_args->network_graph->packMesh();
    } else if (GLOBAL_args->cloth) {
      packMesh(GLOBAL_args->mesh_data, GLOBAL_args->cloth);
    }
  }

  void Step() {
    if (GLOBAL_args->use_network) {
      // Network simulation step
      if (GLOBAL_args->glia && GLOBAL_args->network_graph) {
        // If in training mode, update physics
        if (GLOBAL_args->network_graph->isTrainingMode()) {
          GLOBAL_args->network_graph->animatePhysics();
        } else {
          // Check if enough time has passed for next tick
          double current_time = glfwGetTime();
          double time_since_last_tick = current_time - GLOBAL_args->mesh_data->last_tick_time;
          
          if (time_since_last_tick < GLOBAL_args->mesh_data->seconds_per_tick) {
            // Not ready for next tick yet - just update visualization
            GLOBAL_args->network_graph->updateActivationStates();
            GLOBAL_args->network_graph->updateColors();
            GLOBAL_args->network_graph->packMesh();
            return;
          }
          
          // Calculate how many ticks to run (for fast speeds, multiple ticks per frame)
          int ticks_to_run = 1;
          if (time_since_last_tick >= GLOBAL_args->mesh_data->seconds_per_tick * 2.0) {
            // Running behind, catch up (capped at 100 ticks)
            ticks_to_run = std::min(100, (int)(time_since_last_tick / GLOBAL_args->mesh_data->seconds_per_tick));
          }
          
          // Update tick time
          GLOBAL_args->mesh_data->last_tick_time = current_time;
          
          // Run ticks
          for (int t = 0; t < ticks_to_run; t++) {
            // Apply inputs in priority order:
            // 1. Input sequence (if active) - highest priority
            // 2. Keyboard toggles
            // 3. Mouse sliders
            
            if (GLOBAL_args->input_sequence && !GLOBAL_args->input_sequence->isEmpty()) {
              // Check if sequence is complete
              if (GLOBAL_args->input_sequence->getCurrentTick() > GLOBAL_args->input_sequence->getMaxTick()) {
                // Test complete - stop animation and reset for next run
                GLOBAL_args->mesh_data->animate = false;
                GLOBAL_args->input_sequence->reset();
                std::cout << "\nTest sequence completed. Press 'A' to run again." << std::endl;
                break;  // Exit tick loop
              }
              
              // Apply pre-programmed input sequence
              auto inputs = GLOBAL_args->input_sequence->getCurrentInputs();
              for (const auto& kv : inputs) {
                GLOBAL_args->glia->injectSensory(kv.first, kv.second);
              }
              GLOBAL_args->input_sequence->advance();
            } else {
              // Manual control mode
              // Keyboard toggle inputs
              for (const auto& kv : OpenGLCanvas::sensory_input_enabled) {
                if (kv.second) {
                  GLOBAL_args->glia->injectSensory(kv.first, 200.0f);
                }
              }
              
              // Mouse slider inputs
              if (GLOBAL_args->input_controls) {
                for (int i = 0; i < 100; i++) {
                  std::string neuron_id = "S" + std::to_string(i);
                  float input = GLOBAL_args->input_controls->getInputForNeuron(neuron_id);
                  if (input > 0.0f) {
                    GLOBAL_args->glia->injectSensory(neuron_id, input);
                  }
                }
              }
            }
            
            // Step the network
            GLOBAL_args->glia->step();
            GLOBAL_args->mesh_data->current_tick++;
          }
          
          // Update visualization (once per frame, not per tick)
          GLOBAL_args->network_graph->updateActivationStates();
          GLOBAL_args->network_graph->updateColors();
          GLOBAL_args->network_graph->packMesh();
        }
      }
    } else {
      // Cloth simulation step
      if (GLOBAL_args->cloth) {
        GLOBAL_args->cloth->Animate();
      }
    }
  }

  void Animate() {
    if (GLOBAL_args->mesh_data->animate) {
      for (int i = 0; i < 10; i++) {
        Step();
      }
      PackMesh();
    }
  }
  
  void Load() {
    if (GLOBAL_args->use_network) {
      GLOBAL_args->LoadNetwork();
    } else {
      GLOBAL_args->Load();
    }
  }
  
}
