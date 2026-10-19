#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r8ui) uniform uimage2D species;
layout (binding = 1, r32f) readonly uniform image2D age;
layout (binding = 2, rg32f) writeonly uniform image2D canopy;

layout (std430, binding = 0) readonly buffer Buffer1
{
	SpeciesParams sParams[];
};

// Grid dimensions (2m cells)
uniform int nx;
uniform int ny;

// Canopy grid dimensions (10m cells). Passed in rather than derived as nx / 5: the texture
// is rounded up so that the uv / 5 conversions of the other shaders stay in bounds when the
// terrain is not a multiple of 20m, and that last cell has to be written too.
uniform int cx;
uniform int cy;

#include <include/species.glsl>
#include <include/encoding.glsl>

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	// Coords in sProb grid (10m cells)
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= cx) return;
	if (uv.y >= cy) return;

    float h_max = 0;
    float avg = 0;
    int nb = 0;

    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j)
        {
            ivec2 offset = ivec2(i, j);
            ivec2 pos = 5 * uv + offset;
            if (pos.x >= nx) continue;
            if (pos.y >= ny) continue;

            uint sp = decodeSpecies(pos);
            if (sp < maxSpecies)
            {
                float t = imageLoad(age, pos).r;
                float h = GetHeight(t, sp);
                h_max = max(h, h_max);

                avg += h;
                nb++;
            }
        }

    if (nb > 0)
        avg = avg / nb;
    imageStore(canopy, uv, vec4(h_max, avg, 0, 0));
}

#endif