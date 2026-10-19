#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r8ui) uniform uimage2D species;
layout (binding = 1, r32f) readonly uniform image2D age;
layout (binding = 2, r32f) writeonly uniform image2D lif;
layout (binding = 3, r32f) readonly uniform image2DArray lips;
layout (binding = 4, rg32f) readonly uniform image2D canopy;
layout (binding = 5, rg32f) readonly uniform image2D stamp;
layout (std430, binding = 0) readonly buffer BufferParams
{
	SpeciesParams sParams[];
};
layout (std430, binding = 1) readonly buffer BufferLipRadius
{
	// Lip radius (offset from center), without padding
	int lipRadius[];
};

uniform int nbLips;

// Grid dimensions
uniform int nx;
uniform int ny;

// Patch dimensions
uniform int N;

// Largest offset (from center) at which any LIP is non-zero (<= N / 2).
uniform int R; // = 20 (41 x 41 kernel)

#include <include/lip.glsl>
#include <include/common.glsl>
#include <include/species.glsl>
#include <include/encoding.glsl>

/*
 * The data used by the workgroup is gathered in shared memory. The window is includes a halo
 * around the WG to account for the reads outside the WG. It avoids doing 8 * 8 = 64 reads in
 * global memory for the same data, when it can be cached.
 * The size of the window is (8 + 2R) x (8 + 2R), staged one column strip at a time rather than 
 * as a single square tile to reduce occupancy
 */
#define TILE_W 8
#define TILE_H 48 // 8 + 2 * R

shared float tileScale[TILE_W * TILE_H];
shared uint tileBits[TILE_W * TILE_H]; // species | bucket << 8 | LIP radius << 16

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);

	// No early return: every invocation has to reach the barriers below
	bool valid = (uv.x < nx) && (uv.y < ny);

	int wgBaseX = int(gl_WorkGroupID.x) * TILE_W;
	int wgBaseY = int(gl_WorkGroupID.y) * TILE_W;

	int tileY0 = wgBaseY - R;
	int nbCells = TILE_W * (TILE_W + 2 * R);

	// Using integer arithmetic: 
	// 	 nbStrips = ceil( (TILE_W + 2 * R) / TILE_W )
	int nbStrips = (TILE_W + 2 * R + TILE_W - 1) / TILE_W;

	int lid = int(gl_LocalInvocationIndex);
	int nbThreads = TILE_W * TILE_W;

	float result = 1;

	for (int strip = 0; strip < nbStrips; ++strip)
	{
		int stripX0 = wgBaseX - R + strip * TILE_W;

		// Wait for the previous strip to be consumed before overwriting it
		barrier();

		// Load data into shared memory
		for (int k = lid; k < nbCells; k += nbThreads)
		{
			int lx = k % TILE_W;
			int ly = k / TILE_W;
			ivec2 coords = ivec2(stripX0 + lx, tileY0 + ly);

			float scale = 0;
			uint bits = 0u;
			if (coords.x >= 0 && coords.y >= 0 && coords.x < nx && coords.y < ny)
			{
				// Resolving global information here avoids 8 * 8 global reads if done below...
				vec2 st = imageLoad(stamp, coords).rg;
				scale = st.r;

				int bucket = min(int(st.g), nbLips - 1);
				bits = decodeSpecies(coords)
					| (uint(st.g) << 8)
					| (uint(lipRadius[bucket]) << 16);
			}

			tileScale[k] = scale;
			tileBits[k] = bits;
		}

		barrier();

		// Guarded as a block because of barriers (can't do early return)
		if (valid)
		{
			// Accumulate result for pixel, for the data currently in shared memory
			for (int lx = 0; lx < TILE_W; ++lx)
			{
				int i = stripX0 + lx - uv.x;
				if (i < -R || i > R) continue; // outside this invocation's own window

				for (int j = -R; j <= R; ++j)
				{
					int idx = (uv.y + j - tileY0) * TILE_W + lx;

					float scale = tileScale[idx];
					if (scale == 0) continue; // the factor would be exactly 1

					uint bits = tileBits[idx];

					// That tree's LIP is zero past its own radius, so it would contribute a factor of exactly 1
					int radius = int(bits >> 16);
					if (abs(i) > radius || abs(j) > radius) continue;

					int sp = int(bits & 0x3Fu);
					int bucket = int((bits >> 8) & 0xFu);

					ivec2 lipCoords = ivec2(N / 2) - ivec2(i, j);
					lipCoords.y += bucket * N;

					// result = result * (scale * (sParams[sp].ct) * (1 - imageLoad(lips, ivec3(lipCoords, sp)).r));
					result = result * (1 - scale * imageLoad(lips, ivec3(lipCoords, sp)).r);
				}
			}
		}
	}

	if (valid)
		imageStore(lif, uv, vec4(result, 0, 0, 0));
}

#endif
