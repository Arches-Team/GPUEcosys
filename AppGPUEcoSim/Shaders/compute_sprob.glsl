#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r32f) readonly uniform image2DArray seedCount;
layout (binding = 1, r32f) writeonly uniform image2DArray sProbs;
layout (binding = 2, r32f) readonly uniform image2DArray seedHeight;

layout (std430, binding = 0) readonly buffer Buffer2
{
	SpeciesParams sParams[];
};

// Grid dimensions (20m cells)
uniform int nx;
uniform int ny;

// Patch dimensions
uniform int N;

#include <include/common.glsl>
#include <include/species.glsl>

float getSeedProb(int s, float dist)
{
	float k1 = sParams[s].k1;
	float k2 = sParams[s].k2;
	float k3 = sParams[s].k3;
	return (1 - k3) * exp(-dist / k1) / k1 + k3 * exp(-dist / k2) / k2;
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	// Coords in sProb grid (20m cells)
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

	// Center of Seed Dispersion Kernels (20m cells)
	const int center = N / 2;

	const float pSeedMin = 0.0015;

	float results[nbSpecies];
	for (int s = 0; s < nbSpecies; ++s)
		results[s] = 0;

	// Seed dispersion kernel (20m cells)
	for (int i = -center; i <= center; ++i)
		for (int j = -center; j <= center; ++j)
		{
			ivec2 offset = ivec2(i, j);
			ivec2 p = uv + offset;

			if (min(p.x, p.y) < 0) continue;
			if (p.x >= nx) continue;
			if (p.y >= ny) continue;

			// Distance between the two cell centers, in meters
			float dist = length(vec2(20 * offset));

			// Species histogram of cell p, precomputed by compute_seedsource.glsl
			for (int s = 0; s < nbSpecies; ++s)
			{
				float count = imageLoad(seedCount, ivec3(p, s)).r;
				results[s] += count * getSeedProb(s, dist);

				// For ablation study
				// float avgHeight = imageLoad(seedHeight, ivec3(p, s)).r;
				// if (count > 0)
				//     avgHeight /= count;
				// float scale = avgHeight / sParams[s].hmax;
				// results[s] += count * scale * getSeedProb(s, dist);
			}
		}

	float minV = 0.5;
	float maxV = 2;
	for (int s = 0; s < nbSpecies; ++s)
	{
		results[s] = clamp(results[s], minV, maxV);
		imageStore(sProbs, ivec3(uv, s), vec4(max(results[s], pSeedMin), 0, 0, 0));
	}
}

#endif
