#include <string.h>
#include <string.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <random>

#include "matrix.h"
#include "meshdata.h"
#include "argparser.h"
#include "mesh.h"
#include "triangle.h"

// ====================================================================


// default values for the MeshData variables
void INIT_MeshData(MeshData *mesh_data) {
  mesh_data->width = 400;
  mesh_data->height = 400;
  mesh_data->perspective = true;
  mesh_data->wireframe = 0;
  mesh_data->gouraud = false;
  mesh_data->triData = NULL;
  mesh_data->triCount = 0;
}


// ====================================================================
// ====================================================================

// NOTE: These functions are called by the Objective-C code, so we
// need this extern to allow C code to call C++ functions (without
// function name mangling confusion).

// Also, they use global variables...  

extern "C" {

void Simplification() {
  printf ("in simplification");
  int tri_count = GLOBAL_args->mesh->numTriangles() * 0.9;
  GLOBAL_args->mesh->Simplification(tri_count);
  GLOBAL_args->mesh->packMesh(GLOBAL_args->mesh_data);
}

void LoopSubdivision() {
  printf ("in loop subdivision");
  GLOBAL_args->mesh->LoopSubdivision();
  GLOBAL_args->mesh->packMesh(GLOBAL_args->mesh_data);
}

void PackMesh() {
  printf ("Pack Mesh\n");
  GLOBAL_args->mesh->packMesh(GLOBAL_args->mesh_data);
}

}
