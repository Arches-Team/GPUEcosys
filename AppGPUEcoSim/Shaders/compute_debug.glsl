#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r16f) readonly uniform image2DArray ressources;
layout (binding = 1, rgba8) writeonly uniform image2D albedo;
layout (binding = 2, r32f) writeonly uniform image2D debug;
layout (binding = 3, r16f) readonly uniform image2DArray boost;
layout (binding = 4, rg32f) readonly uniform image2D canopy;
layout (binding = 5, r32f) readonly uniform image2D lif;
layout (binding = 6, r32f) readonly uniform image2DArray lips;

layout (std430, binding = 0) readonly buffer Buffer1
{
	float tempBounds[];
};
layout (std430, binding = 1) readonly buffer Buffer
{
	vec4 colors[];
};
layout (std430, binding = 2) readonly buffer Buffer2
{
	SpeciesParams sParams[];
};

// Grid dimensions
uniform int nx;
uniform int ny;
uniform int N;

uniform int sp; // Species to visualize
uniform int m; // Month
uniform float h; // Height

uniform int year;
uniform float tempChange;
uniform int tempChangeStart;
uniform int tempChangeEnd;

uniform float moistChange;
uniform int moistChangeStart;
uniform int moistChangeEnd;

uniform int useSun;
uniform int useShadow;
uniform int useMoist;
uniform int useTemp;
uniform int useSlope;
uniform int useBoost;
uniform int removeTreeContrib;
uniform int useWind;

uniform int showDominant;

#include <include/lip.glsl>
#include <include/vigor.glsl>

float ComputeDebugVigor(ivec2 uv, int s)
{
  float ln = log(0.2);
  float v = 1;

  if (useSun > 0) {
    float shadow = 1;
    if (useShadow > 0)
    {
      float l = imageLoad(lif, uv).r;
      float f_light = l;
      if (removeTreeContrib > 0) 
      {
        // The probe tree and the sampled cell are the same one here, hence the null offset
        float contrib = GetLIFContribution(sp, h, uv, ivec2(0, 0));
        if (contrib == 0)
            f_light = 1;
        else
            f_light = ( l) / contrib;

        if (f_light > 1)
            f_light = l;
      }

      // shadow = f_light; // V3 ou V4
      // shadow = f_light * layerResponse(uv, h); // V1
      shadow = 1 - (1 - f_light) * layerResponse(uv, h); // V2 ou V5
    }

    float f_s = sunResponse(uv, s, m, shadow);
    v = min(v, f_s);
  }
  if (useMoist > 0) {
    float f_m = moistResponse(uv, s);
    v = min(v, f_m);
  }
  if (useTemp > 0) {
    float f_t = tempResponse(uv, s, m);
    v = min(v, f_t);
  }
  if (useSlope > 0) {
    float f_sl = slopeResponse(uv, s);
    v = min(v, f_sl);
  }
  if (useBoost > 0) {
    float b = imageLoad(boost, ivec3(uv / 5, s)).r;
    float B = sign(b) * min(b * b, 1);
    v = min(v + B, 1);
  }

  return v;
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

  if (showDominant == 0)
  {
    float v = ComputeDebugVigor(uv, sp);
    float c = (v + 0.2) / 1.2;
    imageStore(albedo, uv, vec4(c, c, c, 1));
    imageStore(debug, uv, vec4(v, 0, 0, 0));
  }
  else
  {
    int maxS = 0;
    float maxV = -1;
    for (int i = 0; i < 7; ++i)
    {
      if (sParams[i].enabled == 0) continue;

      float v = ComputeDebugVigor(uv, i);
      if (v > maxV)
      {
        maxS = i;
        maxV = v;
      }
    }

    imageStore(albedo, uv, colors[maxS]);
  }

}

#endif

