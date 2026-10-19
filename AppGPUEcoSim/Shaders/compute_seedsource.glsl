#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r8ui) uniform uimage2D species;
layout (binding = 1, r32f) readonly uniform image2D age;
layout (binding = 2, r32f) writeonly uniform image2DArray seedCount;
layout (binding = 3, r32f) writeonly uniform image2DArray seedHeight;

layout (std430, binding = 0) readonly buffer Buffer1
{
	SpeciesParams sParams[];
};

// Grid dimensions (20m cells)
uniform int nx;
uniform int ny;

// Individual trees grid dimensions (2m cells)
uniform int gx;
uniform int gy;

#include <include/species.glsl>
#include <include/encoding.glsl>

/*
 * Computing the number of trees and the sum of their heights, per 20m cell and per species
 * Pre-computing is more efficient than computing independently for a cell that needs it (up to 500m away):
 * less and more localized texture reads
 */
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	// Coords in the sProb grid (20m cells)
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

	int count[nbSpecies];
	float totalHeights[nbSpecies];
	for (int s = 0; s < nbSpecies; ++s)
	{
		count[s] = 0;
		totalHeights[s] = 0;
	}

	// Individuals grid (2m cells): a 20m cell covers 10 x 10 of them
	for (int x = 0; x < 10; ++x)
		for (int y = 0; y < 10; ++y)
		{
			ivec2 pos = 10 * uv + ivec2(x, y);
			if (pos.x >= gx) continue;
			if (pos.y >= gy) continue;

			uint s = decodeSpecies(pos);
			if (s >= nbSpecies) continue;

			float t = imageLoad(age, pos).r;
			totalHeights[s] += GetHeight(t, s);
			count[s]++;
		}

	for (int s = 0; s < nbSpecies; ++s)
	{
		imageStore(seedCount, ivec3(uv, s), vec4(float(count[s]), 0, 0, 0));
		imageStore(seedHeight, ivec3(uv, s), vec4(totalHeights[s], 0, 0, 0));
	}
}

#endif
