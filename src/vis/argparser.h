// ================================================================
// Parse the command line arguments and the input file
// ================================================================

#ifndef __ARG_PARSER_H__
#define __ARG_PARSER_H__

#include <string>
#include <random>

struct MeshData;
class Mesh;
class Cloth;
class BoundingBox;

// ====================================================================
// ====================================================================

class NetworkGraph;  // Forward declaration
class Glia;          // Forward declaration
class InputControlManager;  // Forward declaration
class InputSequence;  // Forward declaration

class ArgParser {

public:
  
  ArgParser(int argc, const char *argv[], MeshData *_mesh_data);

  double rand() {
    static std::random_device rd;    
    static std::mt19937 engine(rd());
    static std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(engine);
  }
  
  // helper functions
  void separatePathAndFile(const std::string &input, std::string &path, std::string &file);

  void Load();
  void LoadNetwork();
  
  // ==============
  // REPRESENTATION
  // all public! (no accessors)

  std::string cloth_file;
  std::string network_file;
  std::string network_file_full_path; // Full path to network file
  std::string path;        // Path to shaders (stays at src/vis/)
  std::vector<std::string> test_files;  // List of test sequence files to load

  Cloth *cloth;
  NetworkGraph *network_graph;
  Glia *glia;
  InputControlManager *input_controls;  // Input controls for sensory neurons
  InputSequence *input_sequence;         // Currently active input sequence
  std::vector<InputSequence*> loaded_tests;  // Pre-loaded test sequences (accessible via 1-9)
  MeshData *mesh_data;
  BoundingBox *bbox;
  
  bool use_network;  // true = render network, false = render cloth
};

extern ArgParser *GLOBAL_args;
void packMesh(MeshData *mesh_data, Cloth *cloth);

#endif
