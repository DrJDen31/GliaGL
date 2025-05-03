// ================================================================
// Parse the command line arguments and the input file
// ================================================================

#ifndef __ARG_PARSER_H__
#define __ARG_PARSER_H__

#include <string>

struct MeshData;
class Mesh;

class Glia;

// ====================================================================
// ====================================================================

class ArgParser {

public:
  
  ArgParser(int argc, const char *argv[], MeshData *_mesh_data);

  // helper functions
  void separatePathAndFile(const std::string &input, std::string &path, std::string &file);
  
  // ==============
  // REPRESENTATION
  // all public! (no accessors)

  std::string input_file;
  std::string path;

  Mesh *mesh;
  MeshData *mesh_data;

  Glia* glia;
};

extern ArgParser *GLOBAL_args;

#endif
