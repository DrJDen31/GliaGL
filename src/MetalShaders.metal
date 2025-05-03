// ==================================================================
// Apple Metal Shaders
// ==================================================================

#include <simd/simd.h>
#include <metal_common>

using namespace metal;

// Include header shared between this Metal shader code and C code executing Metal API commands
#include "MetalShaderTypes.h"


// Vertex shader outputs and fragment shader inputs
typedef struct
{
  float4 clipSpacePosition [[position]];
  float3 position_worldspace;
  float3 normal_cameraspace;
  float3 eyedirection_cameraspace;
  float3 lightdirection_cameraspace;
  float4 mycolor;
} RasterizerData;


// ======================================================================
// Vertex function
vertex RasterizerData
vertexShader(uint vertexID [[vertex_id]],
             constant MetalVertex *vertices [[buffer(MetalVertexInputIndexVertices)]],
             constant vector_uint2 *viewportSizePointer [[buffer(MetalVertexInputIndexViewportSize)]],
             constant Uniforms &uniforms [[buffer(2)]] )
{
  RasterizerData out;

  out.clipSpacePosition = uniforms.modelViewProjectionMatrix * vertices[vertexID].position;  

  out.position_worldspace = (uniforms.modelMatrix * vertices[vertexID].position).xyz;      
  
  vector_float3 vertexposition_cameraspace = (uniforms.modelViewMatrix * vertices[vertexID].position).xyz;
  out.eyedirection_cameraspace = vector_float3(0,0,0) - vertexposition_cameraspace;

  out.normal_cameraspace = (uniforms.modelViewMatrix * vertices[vertexID].normal).xyz;

  out.lightdirection_cameraspace = uniforms.lightPosition_worldspace;

  out.mycolor = vertices[vertexID].color;
  return out;
}


// ======================================================================
// Fragment function
fragment float4 fragmentShader(RasterizerData in [[stage_in]],
                               float2 pointCoord [[point_coord]],
                               bool is_front_face [[front_facing]] ){

  // NOTE: This is a work in progress, somethig is fishy about the uniforms -- needs debugging

  // Light emission properties
  // You probably want to put them as uniforms
  // vector_float3 LightColor = vector_float3(1,1,1);
  vector_float3 SpecularLightColor = vector_float3(0.2,0.2,0.2);
  vector_float3 DiffuseLightColor = vector_float3(0.8,0.8,0.8);
  float LightPower = 50.0;

  // Normal of the computed fragment, in camera space
  vector_float3 n = normalize( in.normal_cameraspace );

  // Material properties
  vector_float3 MaterialDiffuseColor;
  MaterialDiffuseColor = in.mycolor.xyz;
    
  // WORK IN PROGRESS

  /*
  int wireframe = 0; //wireframe2;
  if ( (wireframe == 0) ||
       ( in.mycolor.x > 0.1 &&
         in.mycolor.y > 0.1 &&
         in.mycolor.z > 0.1) ) {
    MaterialDiffuseColor = 0.7 * vector_float3(1,1,1);
  } else {
    MaterialDiffuseColor = in.mycolor.xyz;
    */
    if (in.mycolor.x > 0.1 &&
        in.mycolor.y > 0.1 &&
        in.mycolor.z > 0.1) {
      MaterialDiffuseColor = vector_float3(1,1,1); //*= 7.0;
    } else {
     MaterialDiffuseColor = saturate(MaterialDiffuseColor); //; //vector_float3(1,1,1); //*= 7.0;
    }
    /*
  }
  */

  vector_float3 MaterialAmbientColor = vector_float3(0.2,0.2,0.2) * MaterialDiffuseColor;
  vector_float3 MaterialSpecularColor = vector_float3(0.1,0.1,0.1);

  if(! is_front_face ) {
    MaterialDiffuseColor = vector_float3(0.0,0.0,0.6);
    MaterialAmbientColor = vector_float3(0.3,0.3,0.3) * MaterialDiffuseColor;
    MaterialSpecularColor = vector_float3(0.1,0.1,0.3);
    n = -n;
  }

  // Distance to the light
  // NOTE: NOT RIGHT FOR A HEADLAMP...
  float distance = length( in.lightdirection_cameraspace - in.position_worldspace );
  distance *= 1.1;
  
  // Direction of the light (from the fragment to the light)
  // NOTE: INSTEAD MAKING A HEADLAMP
  vector_float3 l = normalize(in.lightdirection_cameraspace);

  // Cosine of the angle between the normal and the light direction,
  // clamped above 0
  //  - light is at the vertical of the triangle -> 1
  //  - light is perpendicular to the triangle -> 0
  //  - light is behind the triangle -> 0
  float cosTheta = clamp( dot( n,l ), 0,1 );

  // Eye vector (towards the camera)
  vector_float3 E = normalize(in.eyedirection_cameraspace);
  // Direction in which the triangle reflects the light
  vector_float3 R = reflect(-l,n);

  // Cosine of the angle between the Eye vector and the Reflect vector,
  // clamped to 0
  //  - Looking into the reflection -> 1
  //  - Looking elsewhere -> < 1
  float cosAlpha = clamp( dot( E,R ), 0,1 );

  vector_float3 mycolor2 =
    // Ambient : simulates indirect lighting
    MaterialAmbientColor +
    // Diffuse : "color" of the object
    MaterialDiffuseColor * DiffuseLightColor * LightPower * cosTheta / (distance*distance) +
    // Specular : reflective highlight, like a mirror
    MaterialSpecularColor * SpecularLightColor * LightPower * pow(cosAlpha,5) / (0.1*distance*distance);

  return vector_float4(mycolor2,1);
}

// ======================================================================
