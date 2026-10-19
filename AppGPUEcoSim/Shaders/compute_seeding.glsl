#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r16f) readonly uniform image2DArray ressources;
layout (binding = 1, r32f) readonly uniform image2D lif;
layout (binding = 2, r16f) readonly uniform image2D mask;
layout (binding = 3, r32f) readonly uniform image2DArray sProbs;
layout (binding = 4, r32f) uniform image2DArray regenLayers;
layout (binding = 5, r16f) readonly uniform image2DArray boost;
layout (binding = 6, rg32f) readonly uniform image2D canopy;
layout (binding = 7, r8ui) uniform uimage2D species;

layout (std430, binding = 0) readonly buffer Buffer1
{
	float tempBounds[];
};
layout (std430, binding = 1) readonly buffer Buffer
{
	SpeciesParams sParams[];
};

// Grid dimensions
uniform int nx;
uniform int ny;

uniform int year;
uniform int seedMonth;

uniform float tempChange;
uniform int tempChangeStart;
uniform int tempChangeEnd;

uniform float moistChange;
uniform int moistChangeStart;
uniform int moistChangeEnd;
uniform int useWind;

#include <include/random.glsl>
#include <include/vigor.glsl>
#include <include/encoding.glsl>

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
  // Grid coords 2m
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

  float m = imageLoad(mask, uv).r;
  if (m != 0)
    return;

  // Grid coords 20m
  ivec2 UV = uv / 10;

  float maxV = 0.; // Max vigor for a single species
  float totalV = 0.; // Sum of all the vigors
  float cumulatedV[nbSpecies]; // Cumulated vigors

  for (int s = 0; s < nbSpecies; ++s)
  {
    if (sParams[s].enabled == 0) continue;

    float sProb = imageLoad(sProbs, ivec3(UV, s)).r;
  // For ablation study
    // sProb = 1;
    float lif = imageLoad(lif, uv).r;
    float vigor = max(0.0, sProb * ComputeVigor(uv, s, seedMonth, 0, lif));
		maxV = max(maxV, vigor);

    // Adjust for maxAge
    vigor /= pow(sParams[s].a_max, 1.2f);
    totalV += vigor;
    cumulatedV[s] = totalV;
  }

	float r = Hash01(vec4(uv, year, 0));
  if (r < maxV) // If try to plant a seed
  {
    float r = Hash01(vec4(uv, year, 1)) * totalV;
    int s = 0;
    while (true) // Find the selected species
    {
      if (s >= nbSpecies)
        return;

      if (sParams[s].enabled == 0) 
      {
        s++;
        continue;
      }

      if (r <= cumulatedV[s])
      {
        float rl = imageLoad(regenLayers, ivec3(uv, s)).r;
        if (rl <= 0.0)
        {
          float r1 = Hash01(vec4(uv, year, 2));
          float r2 = Hash01(vec4(uv, year, 3));
          imageStore(regenLayers, ivec3(uv, s), vec4(NormalDist(r1, r2, 2, 1.5), 0, 0, 0));
          encodeBurnFlag(uv, FLAMMABLE);
        }
        return;
      }
      s++;
    }
  }
}

#endif
