#ifndef MESH_H
#define MESH_H

#include <cstdlib>
#include <vector>
#include <string>
#include "hash.h"
#include "boundingbox.h"
#include "argparser.h"

class Vertex;
class Edge;
class Triangle;
struct MeshData;

// ======================================================================
// ======================================================================
// Stores and renders all the vertices, triangles, and edges for a 3D model

class Mesh {

public:

  // ========================
  // CONSTRUCTOR & DESTRUCTOR
  Mesh(ArgParser *a) { args = a; }
  ~Mesh();
  void Load(const std::string &input_file);
  
  // ========
  // VERTICES
  int numVertices() const { return vertices.size(); }
  Vertex* addVertex(const Vec3f &pos);
  // look up vertex by index from original .obj file
  Vertex* getVertex(int i) const {
    assert (i >= 0 && i < numVertices());
    Vertex *v = vertices[i];
    assert (v != NULL);
    return v; }

  // ==================================================
  // PARENT VERTEX RELATIONSHIPS (used for subdivision)
  // this creates a relationship between 3 vertices (2 parents, 1 child)
  void setParentsChild(Vertex *p1, Vertex *p2, Vertex *child);
  // this accessor will find a child vertex (if it exists) when given
  // two parent vertices
  Vertex* getChildVertex(Vertex *p1, Vertex *p2) const;

  // =====
  // EDGES
  int numEdges() const { return edges.size(); }
  // this efficiently looks for an edge with the given vertices, using a hash table
  Edge* getMeshEdge(Vertex *a, Vertex *b) const;

  // =========
  // TRIANGLES
  int numTriangles() const { return triangles.size(); }
  void addTriangle(Vertex *a, Vertex *b, Vertex *c);
  void removeTriangle(Triangle *t);

  // ===============
  // OTHER ACCESSORS
  const BoundingBox& getBoundingBox() const { return bbox; }

  // =========
  // RENDERING
  void packMesh(MeshData *mesh_data);

  // ==========================
  // MESH PROCESSING OPERATIONS
  void LoopSubdivision();
  void Simplification(int target_tri_count);

private:

  // don't use these constructors
  Mesh(const Mesh &/*m*/) { assert(0); exit(0); }
  const Mesh& operator=(const Mesh &/*m*/) { assert(0); exit(0); }


  // =====================================
  // ASSIGNMENT: add private helper functions as needed
  // =====================================


  // ==============
  // REPRESENTATION
  ArgParser *args;
  std::vector<Vertex*> vertices;
  edgeshashtype edges;
  triangleshashtype triangles;
  BoundingBox bbox;
  vphashtype vertex_parents;
  int num_mini_triangles;

};

// ======================================================================
// ======================================================================

#endif




