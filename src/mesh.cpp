#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

#include "mesh.h"
#include "edge.h"
#include "vertex.h"
#include "triangle.h"
#include "meshdata.h"

// to give a unique id number to each triangles
int Triangle::next_triangle_id = 0;


// =======================================================================
// MESH DESTRUCTOR 
// =======================================================================

Mesh::~Mesh() {
  // delete all the triangles
  std::vector<Triangle*> todo;
  for (triangleshashtype::iterator iter = triangles.begin();
       iter != triangles.end(); iter++) {
    Triangle *t = iter->second;
    todo.push_back(t);
  }
  int num_triangles = todo.size();
  for (int i = 0; i < num_triangles; i++) {
    removeTriangle(todo[i]);
  }
  // delete all the vertices
  int num_vertices = numVertices();
  for (int i = 0; i < num_vertices; i++) {
    delete vertices[i];
  }
}

// =======================================================================
// MODIFIERS:   ADD & REMOVE
// =======================================================================

Vertex* Mesh::addVertex(const Vec3f &position) {
  int index = numVertices();
  Vertex *v = new Vertex(index, position);
  vertices.push_back(v);
  if (numVertices() == 1)
    bbox = BoundingBox(position,position);
  else 
    bbox.Extend(position);
  return v;
}


void Mesh::addTriangle(Vertex *a, Vertex *b, Vertex *c) {
  // create the triangle
  Triangle *t = new Triangle();
  // create the edges
  Edge *ea = new Edge(a,b,t);
  Edge *eb = new Edge(b,c,t);
  Edge *ec = new Edge(c,a,t);
  // point the triangle to one of its edges
  t->setEdge(ea);
  // connect the edges to each other
  ea->setNext(eb);
  eb->setNext(ec);
  ec->setNext(ea);
  // verify these edges aren't already in the mesh 
  // (which would be a bug, or a non-manifold mesh)
  assert (edges.find(std::make_pair(a,b)) == edges.end());
  assert (edges.find(std::make_pair(b,c)) == edges.end());
  assert (edges.find(std::make_pair(c,a)) == edges.end());
  // add the edges to the master list
  edges[std::make_pair(a,b)] = ea;
  edges[std::make_pair(b,c)] = eb;
  edges[std::make_pair(c,a)] = ec;
  // connect up with opposite edges (if they exist)
  edgeshashtype::iterator ea_op = edges.find(std::make_pair(b,a)); 
  edgeshashtype::iterator eb_op = edges.find(std::make_pair(c,b)); 
  edgeshashtype::iterator ec_op = edges.find(std::make_pair(a,c)); 
  if (ea_op != edges.end()) { ea_op->second->setOpposite(ea); }
  if (eb_op != edges.end()) { eb_op->second->setOpposite(eb); }
  if (ec_op != edges.end()) { ec_op->second->setOpposite(ec); }
  // add the triangle to the master list
  assert (triangles.find(t->getID()) == triangles.end());
  triangles[t->getID()] = t;
}


void Mesh::removeTriangle(Triangle *t) {
  Edge *ea = t->getEdge();
  Edge *eb = ea->getNext();
  Edge *ec = eb->getNext();
  Vertex *a = ea->getStartVertex();
  Vertex *b = eb->getStartVertex();
  Vertex *c = ec->getStartVertex();
  // remove these elements from master lists
  edges.erase(std::make_pair(a,b)); 
  edges.erase(std::make_pair(b,c)); 
  edges.erase(std::make_pair(c,a)); 
  triangles.erase(t->getID());
  // clean up memory
  delete ea;
  delete eb;
  delete ec;
  delete t;
}


// =======================================================================
// Helper functions for accessing data in the hash table
// =======================================================================

Edge* Mesh::getMeshEdge(Vertex *a, Vertex *b) const {
  edgeshashtype::const_iterator iter = edges.find(std::make_pair(a,b));
  if (iter == edges.end()) return NULL;
  return iter->second;
}

Vertex* Mesh::getChildVertex(Vertex *p1, Vertex *p2) const {
  vphashtype::const_iterator iter = vertex_parents.find(std::make_pair(p1,p2)); 
  if (iter == vertex_parents.end()) return NULL;
  return iter->second; 
}

void Mesh::setParentsChild(Vertex *p1, Vertex *p2, Vertex *child) {
  assert (vertex_parents.find(std::make_pair(p1,p2)) == vertex_parents.end());
  vertex_parents[std::make_pair(p1,p2)] = child; 
}


// =======================================================================
// the load function parses very simple .obj files
// the basic format has been extended to allow the specification 
// of crease weights on the edges.
// =======================================================================

#define MAX_CHAR_PER_LINE 200

void Mesh::Load(const std::string &input_file) {

  std::ifstream istr(input_file.c_str());
  if (!istr) {
    std::cout << "ERROR! CANNOT OPEN: " << input_file << std::endl;
    return;
  }

  char line[MAX_CHAR_PER_LINE];
  std::string token, token2;
  float x,y,z;
  int a,b,c;
  int index = 0;
  int vert_count = 0;
  int vert_index = 1;

  // read in each line of the file
  while (istr.getline(line,MAX_CHAR_PER_LINE)) { 
    // put the line into a stringstream for parsing
    std::stringstream ss;
    ss << line;

    // check for blank line
    token = "";   
    ss >> token;
    if (token == "") continue;

    if (token == std::string("usemtl") ||
	token == std::string("g")) {
      vert_index = 1; 
      index++;
    } else if (token == std::string("v")) {
      vert_count++;
      ss >> x >> y >> z;
      addVertex(Vec3f(x,y,z));
    } else if (token == std::string("f")) {
      a = b = c = -1;
      ss >> a >> b;
      // handle faces with > 3 vertices
      // assume the face can be triangulated with a triangle fan
      while (ss >> c) {
        int a_ = a-vert_index;
        int b_ = b-vert_index;
        int c_ = c-vert_index;
        assert (a_ >= 0 && a_ < numVertices());
        assert (b_ >= 0 && b_ < numVertices());
        assert (c_ >= 0 && c_ < numVertices());
        addTriangle(getVertex(a_),getVertex(b_),getVertex(c_));
        b = c;
      }
    } else if (token == std::string("e")) {
      a = b = -1;
      ss >> a >> b >> token2;
      // whoops: inconsistent file format, don't subtract 1
      assert (a >= 0 && a <= numVertices());
      assert (b >= 0 && b <= numVertices());
      if (token2 == std::string("inf")) x = 1000000; // this is close to infinity...
      x = atof(token2.c_str());
      Vertex *va = getVertex(a);
      Vertex *vb = getVertex(b);
      Edge *ab = getMeshEdge(va,vb);
      Edge *ba = getMeshEdge(vb,va);
      assert (ab != NULL);
      assert (ba != NULL);
      ab->setCrease(x);
      ba->setCrease(x);
    } else if (token == std::string("vt")) {
    } else if (token == std::string("vn")) {
    } else if (token[0] == '#') {
    } else {
      printf ("LINE: '%s'",line);
    }
  }

  std::cout << "Loaded " << numTriangles() << " triangles." << std::endl;

  assert (numTriangles() > 0);
  num_mini_triangles = 0;
}


// =======================================================================
// DRAWING
// =======================================================================

Vec3f ComputeNormal(const Vec3f &p1, const Vec3f &p2, const Vec3f &p3) {
  Vec3f v12 = p2;
  v12 -= p1;
  Vec3f v23 = p3;
  v23 -= p2;
  Vec3f normal;
  Vec3f::Cross3(normal,v12,v23);
  normal.Normalize();
  return normal;
}


// boundary edges are red, crease edges are yellow
Vec3f EdgeColor(Edge *e) {
  if (GLOBAL_args->mesh_data->wireframe) {
    if (e->getOpposite() == NULL) {
      return Vec3f(1,0,0); 
    } else if (e->getCrease() > 0) {
      return Vec3f(1,1,0);
    } else {
      return Vec3f(0,0,0.0);
    }
  } else {
    return Vec3f(1,1,1);
  }
}


// =================================================================
// SUBDIVISION
// =================================================================


void Mesh::LoopSubdivision() {
    printf("Subdivide the mesh!\n");

    // =====================================
    // ASSIGNMENT: complete this functionality
    // =====================================

    std::unordered_map<int, Vec3f> old_positions;

    // put a vertex at the midpoint of each edge (ensuring no duplicates)
    for (edgeshashtype::iterator iter = GLOBAL_args->mesh->edges.begin(); iter != GLOBAL_args->mesh->edges.end(); iter++) {
        Vertex* v1 = iter->second->getStartVertex();
        Vertex* v2 = iter->second->getEndVertex();

        // don't already have child, create one
        if (!getChildVertex(v1, v2)) {
            Vec3f new_pos;
            if (iter->second->getOpposite()) {
                // no boundary, use surrounding vertices in weighted average
                Vertex* v3 = iter->second->getOpposite()->getNext()->getEndVertex();
                Vertex* v4 = iter->second->getNext()->getEndVertex();
                Vec3f new_p(((v1->x() + v2->x()) * 0.375f) + ((v3->x() + v4->x()) * 0.125f),
                            ((v1->y() + v2->y()) * 0.375f) + ((v3->y() + v4->y()) * 0.125f),
                            ((v1->z() + v2->z()) * 0.375f) + ((v3->z() + v4->z()) * 0.125f));
                new_pos = new_p;
            } else {
                // boundary, so just use immediately adjacent vertices
                Vec3f new_p(((v1->x() + v2->x()) * 0.5f), ((v1->y() + v2->y()) * 0.5f), ((v1->z() + v2->z()) * 0.5f));
                new_pos = new_p;
            }
            
            // update mesh
            Vertex* new_v = this->addVertex(new_pos);
            this->setParentsChild(v1, v2, new_v);

            // store for calculating new position later
            old_positions[v1->getIndex()] = v1->getPos();
            old_positions[v2->getIndex()] = v2->getPos();
            old_positions[new_v->getIndex()] = new_v->getPos();
        }
    }

    // for each triangle (existing/parent vertices)
    for (triangleshashtype::iterator iter = GLOBAL_args->mesh->triangles.begin(); iter != GLOBAL_args->mesh->triangles.end(); iter++) {

        Triangle* t = iter->second;
        Edge* cur_edge = t->getEdge();

        // for each vertex
        for (int i = 0; i < 3; i++) {
            Vertex* cur_v = cur_edge->getEndVertex();


            // if already calculated, skip
            if (cur_v->getPos().x() != old_positions[cur_v->getIndex()].x() ||
                cur_v->getPos().y() != old_positions[cur_v->getIndex()].y() ||
                cur_v->getPos().z() != old_positions[cur_v->getIndex()].z())
            {
                cur_edge = cur_edge->getNext();
                continue;
            }

            // track positions of all impactful vertices
            std::vector<Vec3f> positions;
            std::vector<Vec3f> boundary_positions;

            // flags for boundaries
            bool boundary = false;
            bool reset = false;

            int num_v = 1;

            Edge* g_edge = cur_edge->getNext();

            // checking for open shapes
            if (cur_edge->getOpposite() == NULL) {
                // open shape found, add correct vertex
                if (cur_edge->getStartVertex()->getIndex() == cur_v->getIndex()) {
                    boundary_positions.push_back(old_positions[cur_edge->getEndVertex()->getIndex()]);
                }
                else {
                    boundary_positions.push_back(old_positions[cur_edge->getStartVertex()->getIndex()]);
                }

                reset = true;
                g_edge = g_edge->getOpposite();

            } else if (g_edge->getOpposite() == NULL && g_edge != cur_edge) {
                // open shape found, add correct vertex
                if (g_edge->getStartVertex()->getIndex() == cur_v->getIndex()) {
                    boundary_positions.push_back(old_positions[g_edge->getEndVertex()->getIndex()]);
                } else {
                    boundary_positions.push_back(old_positions[g_edge->getStartVertex()->getIndex()]);
                }
                
                // set g_edge to proper/usable edge
                boundary = true;
                reset = true;
                g_edge = cur_edge->getOpposite();
                // (loop around triangle backwards)
                Edge* around = g_edge;
                while (g_edge->getNext() != around) {
                    g_edge = g_edge->getNext();
                }
            } else {
                // no open shape, add this vertex and move on
                g_edge = g_edge->getOpposite();
                positions.push_back(old_positions[g_edge->getStartVertex()->getIndex()]);
            }
           
            //std::cout << "starting loop" << std::endl;

            Triangle* g_face = g_edge->getTriangle();

            // while not checked all faces
            while (g_face != t || (boundary && !reset)) {
                num_v += 1;

                // found a boundary but haven't reset, so reset
                // NOTE: resetting goes back to the start edge and switches to navigating the vertex in the opposite direction
                //          until reaching the start or another boundary
                if (boundary && !reset) {
                    if (cur_edge->getOpposite() == NULL) {
                        if (g_edge->getStartVertex()->getIndex() == cur_v->getIndex()) {
                            boundary_positions.push_back(old_positions[g_edge->getEndVertex()->getIndex()]);
                        }
                        else {
                            boundary_positions.push_back(old_positions[g_edge->getStartVertex()->getIndex()]);
                        }
                        reset = true;
                        break;
                    }
                    g_edge = cur_edge->getOpposite();
                    Edge* around = g_edge;
                    while (g_edge->getNext() != around) {
                        g_edge = g_edge->getNext();
                    }
                    reset = true;
                } else if (!boundary) {
                    // else, move to next edge
                    g_edge = g_edge->getNext();
                }


                // checking for open shapes, similar to before while
                if (g_edge->getOpposite() == NULL && g_edge != cur_edge) {

                    if (g_edge->getStartVertex()->getIndex() == cur_v->getIndex()) {
                        boundary_positions.push_back(old_positions[g_edge->getEndVertex()->getIndex()]);
                    } else {
                        boundary_positions.push_back(old_positions[g_edge->getStartVertex()->getIndex()]);
                    }

                    if (boundary) {
                        // found both boundaries
                        g_face = t;
                        continue;
                    } else {
                        // only found one so far, reset
                        boundary = true;
                        continue;
                    }
                }

                // no boundary found, so add vertex
                if (g_edge->getStartVertex()->getIndex() == cur_v->getIndex()) {
                    positions.push_back(old_positions[g_edge->getEndVertex()->getIndex()]);
                } else {
                    positions.push_back(old_positions[g_edge->getStartVertex()->getIndex()]);
                }

                // reached beginning
                if (g_edge == cur_edge || g_edge->getOpposite() == cur_edge) {
                    g_face = t;
                } else {
                    // advance
                    g_edge = g_edge->getOpposite();

                    if (boundary) {
                        Edge* around = g_edge;
                        while (g_edge->getNext() != around) {
                            g_edge = g_edge->getNext();
                        }
                    }
                    g_face = g_edge->getTriangle();
                }

            }

            if (boundary || reset) {
                // new position rules for boundary edge
                Vec3f new_pos = (0.75f * cur_v->getPos()) + (0.125f * boundary_positions[0]) + (0.125f * boundary_positions[1]);
                cur_v->setPos(new_pos);
            } else {
                // new position rules for interior edge
                float beta = (3.f / (8.f * (num_v - 1)));
                int n = num_v - 1;
              
                float new_x = (old_positions[cur_v->getIndex()].x() * (1 - (num_v * beta)));
                float new_y = (old_positions[cur_v->getIndex()].y() * (1 - (num_v * beta)));
                float new_z = (old_positions[cur_v->getIndex()].z() * (1 - (num_v * beta)));

                for (std::vector<Vec3f>::iterator v_itr = positions.begin(); v_itr != positions.end(); v_itr++) {
                    new_x += (beta * (*v_itr).x());
                    new_y += (beta * (*v_itr).y());
                    new_z += (beta * (*v_itr).z());
                }
                cur_v->setPos(Vec3f(new_x, new_y, new_z));
            }

            // move to next edge (vertex) in triangle
            cur_edge = cur_edge->getNext();
        }
    }

    // create new triangles given new vertices/updated vertex positions
    bool still_adding = true;
    while (still_adding) {
        bool found = false;
        triangleshashtype::iterator itr;
        for (triangleshashtype::iterator iter = GLOBAL_args->mesh->triangles.begin(); iter != GLOBAL_args->mesh->triangles.end(); iter++) {
            // found an "old" triangle
            if (getChildVertex(iter->second->getEdge()->getStartVertex(), iter->second->getEdge()->getNext()->getStartVertex())) {
                itr = iter;
                found = true;
                break;
            }
        }

        // no more "old" triangles
        if (!found) {
            still_adding = false;
            break;
        }

        // get "old" triangle's information
        Triangle* t = itr->second;
        Vertex* v1 = t->getEdge()->getStartVertex();
        Vertex* v2 = t->getEdge()->getNext()->getStartVertex();
        Vertex* v3 = t->getEdge()->getNext()->getNext()->getStartVertex();

        // get new vertices (children of "old" triangle's vertices)
        Vertex* v12 = getChildVertex(v1, v2);
        Vertex* v23 = getChildVertex(v2, v3);
        Vertex* v31 = getChildVertex(v3, v1);

        // if all children exist, remove "old" triangle and create the 4 new ones
        if (v12 && v23 && v31) {
            this->removeTriangle(t);

            this->addTriangle(v1, v12, v31);
            this->addTriangle(v2, v23, v12);
            this->addTriangle(v3, v31, v23);
            this->addTriangle(v12, v23, v31);
        }
    }
}


// =================================================================
// SIMPLIFICATION
// =================================================================

void Mesh::Simplification(int target_tri_count) {
  // clear out any previous relationships between vertices
  vertex_parents.clear();

  printf ("Simplify the mesh! %d -> %d\n", numTriangles(), target_tri_count);

  // =====================================
  // ASSIGNMENT: complete this functionality
  // =====================================

  // store edges that have been determined invalid to remove so they don't need to be rechecked again
  std::vector<Edge*> invalid_edges;

  while (numTriangles() > target_tri_count) {
      // get shortest edge not in invalid edges
      Edge* e = GLOBAL_args->mesh->edges.begin()->second;
      float len = e->Length();
      for (edgeshashtype::iterator iter = GLOBAL_args->mesh->edges.begin(); iter != GLOBAL_args->mesh->edges.end(); iter++) {
          if (iter->second->Length() < len) {
              bool check = false;
              for (std::vector<Edge*>::iterator itr = invalid_edges.begin(); itr != invalid_edges.end(); itr++) {
                  if (iter->second == *itr) {
                      check = true;
                      break;
                  }
              }
              if (!check) {
                  len = iter->second->Length();
                  e = iter->second;
              }
          }
      }

      // gather triangles to remove (2 vertices on edge)
      Triangle* remove_1 = e->getTriangle();
      Triangle* remove_2 = e->getOpposite()->getTriangle();

      // gather 2 vertices that make up the edge
      Vertex* vertex_1 = e->getStartVertex();
      Vertex* vertex_2 = e->getEndVertex();

      // vector of triangles with 1 vertex on edge
      std::vector<Triangle*> to_adjust;

      // loop around first vertex, gather triangles using it
      Edge* cur_e = e->getNext()->getOpposite();
      Triangle* connected = cur_e->getTriangle();
      while (connected != remove_2) {
          to_adjust.push_back(connected);
          cur_e = cur_e->getNext()->getOpposite();
          connected = cur_e->getTriangle();
      }

      // loop around second vertex, gather triangles using it
      cur_e = e->getOpposite()->getNext()->getOpposite();
      connected = cur_e->getTriangle();
      while (connected != remove_1) {
          to_adjust.push_back(connected);
          cur_e = cur_e->getNext()->getOpposite();
          connected = cur_e->getTriangle();
      }

      // for each triangle, gather its edges
      bool valid = true;
      for (std::vector<Triangle*>::iterator itr = to_adjust.begin(); itr != to_adjust.end(); itr++) {
          Triangle* cur = *itr;
          std::vector<Edge*> t_edges;
          std::vector<Edge*> safe_edges;
          t_edges.push_back(cur->getEdge());
          t_edges.push_back(cur->getEdge()->getNext());
          t_edges.push_back(cur->getEdge()->getNext()->getNext());

          // for each edge (representing a vertex), add it to safe edges
          for (std::vector<Edge*>::iterator t_e_itr = t_edges.begin(); t_e_itr != t_edges.end(); t_e_itr++) {
              safe_edges.push_back(*t_e_itr);
              Edge* cur_edge = (*t_e_itr)->getNext();
              bool next = false;
              while (cur_edge != *t_e_itr) {
                  safe_edges.push_back(cur_edge);
                  if (next) {
                      cur_edge = cur_edge->getNext();
                  }
                  else {
                      cur_edge = cur_edge->getOpposite();
                  }
                  next = !next;
              }
          }

          // for all other triangle edges
          for (std::vector<Triangle*>::iterator itr2 = to_adjust.begin(); itr2 != to_adjust.end(); itr2++) {
              if (*itr2 == *itr) continue;

              // for each edge (vertex)
              Edge* edge_to_check = (*itr2)->getEdge();
              for (int i = 0; i < 3; i++) {
                  
                  // if not in "safe_edges"
                  bool safe_edge = false;
                  for (std::vector<Edge*>::iterator s_itr = safe_edges.begin(); s_itr != safe_edges.end(); s_itr++) {
                      if (*s_itr == edge_to_check) {
                          safe_edge = true;
                      }
                  }
                  if (safe_edge) continue;

                  // ensure the vertices don't match t_edges
                  if (edge_to_check->getStartVertex() == t_edges[0]->getStartVertex() ||
                      edge_to_check->getEndVertex() == t_edges[0]->getStartVertex() ||
                      edge_to_check->getStartVertex() == t_edges[1]->getStartVertex() ||
                      edge_to_check->getEndVertex() == t_edges[1]->getStartVertex() ||
                      edge_to_check->getStartVertex() == t_edges[2]->getStartVertex() ||
                      edge_to_check->getEndVertex() == t_edges[2]->getStartVertex()) {
                      valid = false;
                      break;
                  }

                  edge_to_check = edge_to_check->getNext();
              }
              if (!valid) break;
          }
          if (!valid) break;
      }
      
      // edge determined to be invalid, add to invalid_edges
      if (!valid) {
          invalid_edges.push_back(e);
          continue;
      }


      // calculate new vertex position
      double x = (vertex_1->x() + vertex_2->x()) * 0.5;
      double y = (vertex_1->y() + vertex_2->y()) * 0.5;
      double z = (vertex_1->z() + vertex_2->z()) * 0.5;
      Vec3f position(x, y, z);
      Vertex* replacement_v = this->addVertex(position);

      // remove triangles using edge
      this->removeTriangle(remove_1);
      this->removeTriangle(remove_2);

      // gather all vertices making up the triangles to adjust
      std::vector<Vertex*> copies;
      for (std::vector<Triangle*>::iterator itr = to_adjust.begin(); itr != to_adjust.end(); itr++) {
          Triangle* t = *itr;
          Edge* e = t->getEdge();
          Vertex* v1 = e->getStartVertex();
          Vertex* v2 = e->getNext()->getStartVertex();
          Vertex* v3 = e->getNext()->getNext()->getStartVertex();

          copies.push_back(v1);
          copies.push_back(v2);
          copies.push_back(v3);

          this->removeTriangle(*itr);

      }
      
      // adjust triangles
      for (std::vector<Vertex*>::iterator itr = copies.begin(); itr != copies.end(); itr++) {
          // make new triangle with two common vertices and one new vertex
          Vertex* v1 = (*itr);
          itr++;
          Vertex* v2 = (*itr);
          itr++;
          Vertex* v3 = (*itr);

          if (v1 == vertex_1 || v1 == vertex_2) v1 = replacement_v;
          if (v2 == vertex_1 || v2 == vertex_2) v2 = replacement_v;
          if (v3 == vertex_1 || v3 == vertex_2) v3 = replacement_v;


          // create the new triangle
          this->addTriangle(v1, v2, v3);
      }
  }

  std::cout << "After simplification: " << numTriangles() << std::endl;
}


// =================================================================
// PACK MESH -- PREPARE FOR RENDERING
// =================================================================

void Mesh::packMesh(MeshData *mesh_data) {

  // clean up the old data (if any)
  delete [] mesh_data->triData;
  
  /*
  // To create a wireframe rendering...
  // Each mesh triangle is actually rendered as 3 small triangles
  //           b
  //          /|\
  //         / | \
  //        /  |  \
  //       /   |   \   
  //      /    |    \  
  //     /    .'.    \   
  //    /  .'     '.  \  
  //   /.'           '.\ 
  //  a-----------------c
  //
  */

  // we triple the number of triangles if wireframe is active
  mesh_data->triCount = GLOBAL_args->mesh->numTriangles();
  if (GLOBAL_args->mesh_data->wireframe) { mesh_data->triCount *= 3; }

  // allocate space for the new data
  mesh_data->triData = new float[12*3* mesh_data->triCount];


  // Loop over all of the triangles
  float* current = mesh_data->triData;
  int which = 0;

  for (triangleshashtype::iterator iter = GLOBAL_args->mesh->triangles.begin();
       iter != GLOBAL_args->mesh->triangles.end(); iter++) {

    Triangle *t = iter->second;

    Vec3f a = (*t)[0]->getPos();
    Vec3f b = (*t)[1]->getPos();
    Vec3f c = (*t)[2]->getPos();
    // use simple averaging to find centroid & average normal
    Vec3f centroid = 1.0f / 3.0f * (a+b+c);

    // for flat shading
    Vec3f normal = ComputeNormal(a,b,c);
    Vec3f na = normal;
    Vec3f nb = normal;
    Vec3f nc = normal;
    
    // for smooth gouraud shading
    if (GLOBAL_args->mesh_data->gouraud) {


      // =====================================
      // ASSIGNMENT: complete this functionality
      // =====================================

      // non-gouraud normal values
      Vec3f normals[3] = { normal, normal, normal };
      Edge* cur_edge = t->getEdge();

      // for each vertex
      for (int i = 0; i < 3; i++) {
          bool boundary = false;
          bool reset = false;
          
          Vec3f g_norm = normal;
          Edge* g_edge = cur_edge->getNext();

          // checking for open shapes
          if (g_edge->getOpposite() == NULL && g_edge != cur_edge) {
              boundary = true;
              reset = true;
          } else {
              g_edge = g_edge->getOpposite();
          }          

          Triangle* g_face = g_edge->getTriangle();
          
          int num_faces = 1;

          // while not checked all faces
          while (g_face != t) {
              Vec3f g_a = (*g_face)[0]->getPos();
              Vec3f g_b = (*g_face)[1]->getPos();
              Vec3f g_c = (*g_face)[2]->getPos();

              // compute face's normal
              g_norm += ComputeNormal(g_a, g_b, g_c);
              num_faces += 1;

              // if detected boundary and haven't reset, reset
              if (boundary && !reset){
                  g_edge = cur_edge->getNext();
                  reset = true;
              } else {
                  // else, proceed to next edge
                  g_edge = g_edge->getNext();
              }

              // checking for open shapes
              if (g_edge->getOpposite() == NULL && g_edge != cur_edge) {
                  if (boundary) {
                      g_face = t;
                      continue;
                  }
                  else {
                      boundary = true;
                      continue;
                  }
              }

              // check if all faces have been traversed
              if (g_edge == cur_edge || g_edge->getOpposite() == cur_edge) {
                  g_face = t;
              } else {
                  // proceed to next edge
                  g_edge = g_edge->getOpposite();

                  if (boundary) {
                      Edge* around = g_edge;
                      while (g_edge->getNext() != around) {
                          g_edge = g_edge->getNext();
                      }
                  }
                  g_face = g_edge->getTriangle();
              }
          }

          // calculate gouraud normal
          g_norm /= num_faces;
          normals[i] = g_norm;

          cur_edge = cur_edge->getNext();
      }

      // populate vertex normals with gouraud values
      na = normals[2];
      nb = normals[0];
      nc = normals[1];
    }
    
    // determine edge colors (when wireframe is enabled)
    // the center is white, the colors of the two vertices depend on
    // whether the edge is a boundary edge (red) or crease edge (yellow)
    Vec3f edgecolor_ab = EdgeColor(t->getEdge());
    Vec3f edgecolor_bc = EdgeColor(t->getEdge()->getNext());
    Vec3f edgecolor_ca = EdgeColor(t->getEdge()->getNext()->getNext());
  
    if (!GLOBAL_args->mesh_data->wireframe) {
      
      // Just draw the one triangle if wireframe is not active
      float12 ta = { float(a.x()),float(a.y()),float(a.z()),1, float(na.x()),float(na.y()),float(na.z()),0, 1.0,1.0,1.0,1 };
      float12 tb = { float(b.x()),float(b.y()),float(b.z()),1, float(nb.x()),float(nb.y()),float(nb.z()),0, 1.0,1.0,1.0,1 };
      float12 tc = { float(c.x()),float(c.y()),float(c.z()),1, float(nc.x()),float(nc.y()),float(nc.z()),0, 1.0,1.0,1.0,1 };
      memcpy(current, &ta, sizeof(float)*12); current += 12; which++;
      memcpy(current, &tb, sizeof(float)*12); current += 12; which++;
      memcpy(current, &tc, sizeof(float)*12); current += 12; which++;
      
    } else {

      // Draw 3 triangles if wireframe is active
      float12 ta = { float(a.x()),float(a.y()),float(a.z()),1, float(na.x()),float(na.y()),float(na.z()),0, float(edgecolor_ab.r()),float(edgecolor_ab.g()),float(edgecolor_ab.b()),1 };
      float12 tb = { float(b.x()),float(b.y()),float(b.z()),1, float(nb.x()),float(nb.y()),float(nb.z()),0, float(edgecolor_ab.r()),float(edgecolor_ab.g()),float(edgecolor_ab.b()),1 };
      float12 tc = { float(centroid.x()),float(centroid.y()),float(centroid.z()),1, float(normal.x()),float(normal.y()),float(normal.z()),0, 1.0,1.0,1.0,1 };
      memcpy(current, &ta, sizeof(float)*12); current += 12; which++;
      memcpy(current, &tb, sizeof(float)*12); current += 12; which++;
      memcpy(current, &tc, sizeof(float)*12); current += 12; which++;
      
      ta = { float(b.x()),float(b.y()),float(b.z()),1, float(nb.x()),float(nb.y()),float(nb.z()),0, float(edgecolor_bc.r()),float(edgecolor_bc.g()),float(edgecolor_bc.b()),1 };
      tb = { float(c.x()),float(c.y()),float(c.z()),1, float(nc.x()),float(nc.y()),float(nc.z()),0, float(edgecolor_bc.r()),float(edgecolor_bc.g()),float(edgecolor_bc.b()),1 };
      tc = { float(centroid.x()),float(centroid.y()),float(centroid.z()),1, float(normal.x()),float(normal.y()),float(normal.z()),0, 1.0,1.0,1.0,1 };
      memcpy(current, &ta, sizeof(float)*12); current += 12; which++;
      memcpy(current, &tb, sizeof(float)*12); current += 12; which++;
      memcpy(current, &tc, sizeof(float)*12); current += 12; which++;
      
      ta = { float(c.x()),float(c.y()),float(c.z()),1, float(nc.x()),float(nc.y()),float(nc.z()),0, float(edgecolor_ca.r()),float(edgecolor_ca.g()),float(edgecolor_ca.b()),1 };
      tb = { float(a.x()),float(a.y()),float(a.z()),1, float(na.x()),float(na.y()),float(na.z()),0, float(edgecolor_ca.r()),float(edgecolor_ca.g()),float(edgecolor_ca.b()),1 };
      tc = { float(centroid.x()),float(centroid.y()),float(centroid.z()),1, float(normal.x()),float(normal.y()),float(normal.z()),0, 1.0,1.0,1.0,1 };
      memcpy(current, &ta, sizeof(float)*12); current += 12; which++;
      memcpy(current, &tb, sizeof(float)*12); current += 12; which++;
      memcpy(current, &tc, sizeof(float)*12); current += 12; which++;
    }
  }

  // the boundingbox center and size will be used to adjust the camera
  Vec3f center;
  bbox.getCenter(center);
  
  mesh_data->bb_center.data[0] = center.x();
  mesh_data->bb_center.data[1] = center.y();
  mesh_data->bb_center.data[2] = center.z();
  
  mesh_data->bb_scale = 1.8 / float(bbox.maxDim());
}


// =================================================================
