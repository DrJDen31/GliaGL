#ifndef _MESH_DATA_H_
#define _MESH_DATA_H_

// ====================================================================
// ====================================================================

// a homogeneous 3D point or a color with alpha
typedef struct float3 {
  float data[3];
} float3;

// a homogenous 3D point or a color with alpha
typedef struct float4 {
  float data[4];
} float4;

// a vertex with position, normal, and color
typedef struct float12 {
  float data[12];
} float12;

// a 4x4 matrix
typedef struct float16 {
  float data[16];
} float16;


typedef struct MeshData {
  
  // REPRESENTATION
  int width;
  int height;

  // animation control
  double timestep;
  bool animate;
  float3 gravity;
  
  // network simulation control
  int current_tick;           // Current simulation tick
  float seconds_per_tick;     // Time per tick (0.001 to 5.0 seconds)
  bool show_tick_counter;     // Display tick counter on screen
  double last_tick_time;      // Timestamp of last tick (for delay timing)

  // display option toggles 
  // (used by both)
  bool particles;
  bool velocity;
  bool surface;
  bool bounding_box;
  bool gouraud;
  
  // used by cloth
  bool force;
  bool wireframe;

  bool perspective;
  
  int clothTriCount;
  float* clothTriData;
  
  float3 bb_center;
  float bb_max_dim;
  float bb_scale;
  
} MeshData;


void INIT_MeshData(MeshData *mesh_data);
void loadOBJ(MeshData *mesh_data);

// ====================================================================
// ====================================================================

#endif
