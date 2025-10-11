// ================================================================
// Parse the command line arguments and the input file
// ================================================================

#include <iostream>
#include <fstream>
#include <sstream>

#include "cloth.h"
#include "argparser.h"
#include "meshdata.h"
#include "network_graph.h"
#include "glia.h"
#include "input_control.h"
#include "../arch/input_sequence.h"

#if __APPLE__
#include "matrix.h"
#else
#include <glm/gtc/type_ptr.hpp>
#endif


ArgParser *GLOBAL_args;

// The command line arguments
ArgParser::ArgParser(int argc, const char *argv[], MeshData *_mesh_data) {

  // set some default values
  mesh_data = _mesh_data;
  path = ".";
  cloth = NULL;
  network_graph = NULL;
  glia = NULL;
  input_controls = NULL;
  input_sequence = NULL;  // No sequence by default
  use_network = false;
  
  // parse the command line arguments
  for (int i = 1; i < argc; i++) {
    if (argv[i] == std::string("--cloth")) {
      i++; assert (i < argc); 
      separatePathAndFile(argv[i],path,cloth_file);
      use_network = false;
    } else if (argv[i] == std::string("--network")) {
      i++; assert (i < argc);
      // Store the full path as-is for network file
      network_file_full_path = argv[i];
      // Extract just filename for display
      std::string temp_path;
      separatePathAndFile(argv[i], temp_path, network_file);
      use_network = true;
      // For network mode, shaders are in src/vis/
      path = "../src/vis";
    } else if (argv[i] == std::string("--tests")) {
      // Load multiple test sequence files
      // Format: --tests test1.seq test2.seq test3.seq
      i++;
      while (i < argc && argv[i][0] != '-') {
        test_files.push_back(argv[i]);
        i++;
      }
      i--; // Back up one since loop will increment
    } else if (argv[i] == std::string("--size")) {
      i++; assert (i < argc); 
      mesh_data->width = atoi(argv[i]);
      i++; assert (i < argc); 
      mesh_data->height = atoi(argv[i]);
    } else {
      std::cout << "ERROR: unknown command line argument " 
                << i << ": '" << argv[i] << "'" << std::endl;
      exit(1);
    }
  }

  if (use_network) {
    LoadNetwork();
  } else {
    Load();
  }
  
  GLOBAL_args = this;
  
  if (use_network && network_graph) {
    network_graph->packMesh();
    packMesh(mesh_data, nullptr);  // Set up camera bounding box for network
  } else if (cloth) {
    packMesh(mesh_data,cloth);
  }
}

void ArgParser::Load() {
  delete cloth;
  if (cloth_file != "") {
    cloth = new Cloth(this);
  } else {
    cloth = NULL;
  }
}

void ArgParser::LoadNetwork() {
  delete network_graph;
  delete glia;
  
  if (network_file_full_path != "") {
    std::cout << "Loading network from: " << network_file << std::endl;
    
    // Create empty network - all neurons will be created dynamically from config
    std::cout << "Creating empty network (neurons defined in config file)..." << std::endl;
    glia = new Glia();  // Empty constructor
    
    // Load network configuration using full path directly
    glia->configureNetworkFromFile(network_file_full_path);
    
    // Create NetworkGraph for visualization
    network_graph = new NetworkGraph(glia, this);
    
    // Create input controls for sensory neurons (dynamically discovered)
    std::vector<std::string> sensory_ids = glia->getSensoryNeuronIDs();
    input_controls = new InputControlManager();
    input_controls->initializeControls(sensory_ids, mesh_data->width, mesh_data->height);
    
    // Default output handling removed; output selection handled by detector
    
    std::cout << "Network loaded and spatialized successfully" << std::endl;
    
    // Load test sequence files if specified
    if (!test_files.empty()) {
      std::cout << "\nLoading " << test_files.size() << " test sequence file(s)..." << std::endl;
      for (size_t i = 0; i < test_files.size() && i < 9; i++) {  // Limit to 9 tests (keys 1-9)
        InputSequence* seq = new InputSequence();
        if (seq->loadFromFile(test_files[i])) {
          loaded_tests.push_back(seq);
          std::cout << "  [" << (i+1) << "] Loaded: " << test_files[i] 
                    << " (" << seq->getMaxTick() << " ticks, " 
                    << (seq->isLooping() ? "looping" : "one-shot") << ")" << std::endl;
        } else {
          std::cerr << "  [" << (i+1) << "] Failed to load: " << test_files[i] << std::endl;
          delete seq;
        }
      }
      if (test_files.size() > 9) {
        std::cout << "  Note: Only first 9 tests loaded (keys 1-9)" << std::endl;
      }
      std::cout << "\nPress [1-9] to load a test, [0] to clear" << std::endl;
    }
  } else {
    network_graph = NULL;
    glia = NULL;
    input_controls = NULL;
  }
}

void ArgParser::separatePathAndFile(const std::string &input, std::string &path, std::string &file) {
  // we need to separate the filename from the path
  // (we assume the vertex & fragment shaders are in the same directory)
  // first, locate the last '/' in the filename
  size_t last = std::string::npos;  
  while (1) {
    int next = input.find('/',last+1);
    if (next != (int)std::string::npos) { 
      last = next;
      continue;
    }
    next = input.find('\\',last+1);
    if (next != (int)std::string::npos) { 
      last = next;
      continue;
    }
    break;
  }
  if (last == std::string::npos) {
    // if there is no directory in the filename
    file = input;
    path = ".";
  } else {
    // separate filename & path
    file = input.substr(last+1,input.size()-last-1);
    path = input.substr(0,last);
  }
}



void packMesh(MeshData *mesh_data, Cloth *cloth) {
  if (cloth != NULL)
    cloth->PackMesh();

  BoundingBox bbox;
  if (cloth != NULL) {
    bbox = cloth->getBoundingBox();
  } else if (GLOBAL_args && GLOBAL_args->use_network && GLOBAL_args->network_graph) {
    // For network mode, get bounding box from NetworkGraph
    bbox = GLOBAL_args->network_graph->getBoundingBox();
  }

  // the boundingbox center and size will be used to adjust the camera
  Vec3f center;
  bbox.getCenter(center);
  mesh_data->bb_center.data[0] = center.x();
  mesh_data->bb_center.data[1] = center.y();
  mesh_data->bb_center.data[2] = center.z();
  mesh_data->bb_max_dim = bbox.maxDim();
  mesh_data->bb_scale = 1.8 / float(bbox.maxDim());
}
