#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r8ui) uniform uimage2D species;
layout (binding = 1, r32f) readonly uniform image2D age;
layout (binding = 2, rg32f) readonly uniform image2D canopy;
layout (binding = 3, rg32f) writeonly uniform image2D stamp;

layout (std430, binding = 0) readonly buffer BufferParams
{
	SpeciesParams sParams[];
};

// Grid dimensions (2m cells)
uniform int nx;
uniform int ny;

#include <include/species.glsl>
#include <include/encoding.glsl>

/*
 * Per 2m cell, everything compute_lif.glsl needs about the tree standing there:
 *   R = scale = min(h / c, 1), or 0 when the cell contributes nothing
 *   G = LIP height bucket
 *
 * compute_lif.glsl used to recompute this for every one of the 1681 neighbours it
 * gathers, which meant up to 1681 GetHeight() calls -- so 1681 exp() and divisions --
 * per cell, for a value that only depends on the neighbour itself.
 *
 * A scale of exactly 0 means "multiply the LIF by 1", which is what the gather did for
 * an empty cell, a disabled species, or a tree below 4m (getLIP() returns 0 there).
 */
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

	float scale = 0;
	float bucket = 0;

	uint s = decodeSpecies(uv);
	if (s < maxSpecies && sParams[s].enabled != 0)
	{
		float t = imageLoad(age, uv).r;
		float h = GetHeight(t, s);

		// getLIP() returns 0 below 4m, so the neighbour contributes a factor of 1
		if (h >= 4)
		{
			float c = imageLoad(canopy, uv / 5).r;
			scale = min(h / c, 1);

			// LIP discretized every 5m, same expression as getLIP()
			bucket = float(int(min(h, 49.9) / 5.));
		}
	}

	imageStore(stamp, uv, vec4(scale, bucket, 0, 0));
}

#endif
