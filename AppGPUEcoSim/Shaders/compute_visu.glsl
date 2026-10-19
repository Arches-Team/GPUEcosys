#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r8ui) uniform uimage2D species;
layout (binding = 1, r32f) readonly uniform image2D age;
layout (binding = 2, r32f) readonly uniform image2D terrainShaded;
layout (binding = 3, rgba8) writeonly uniform image2D albedo;
layout (binding = 4, r32f) readonly uniform image2DArray regenLayers;

layout (std430, binding = 0) readonly buffer Buffer
{
	vec4 colors[];
};
layout (std430, binding = 1) readonly buffer Buffer2
{
	SpeciesParams sParams[];
};

// Grid dimensions
uniform int nx;
uniform int ny;
uniform int mode;
uniform float limit;

#include <include/species.glsl>
#include <include/encoding.glsl>

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

    float bg = imageLoad(terrainShaded, uv).r * 1.25;

	uint s = decodeSpecies(uv);
	float t = imageLoad(age, uv).r;
	// sParams has no entry for the empty marker (63), so GetHeight must not be called on it
	float h = (s < nbSpecies) ? GetHeight(t, s) : 0;
	float a = min((h - 4) / 50, 1);

	const float o = 40. / 255;
	const vec4 offset = vec4(o, o, o, 0);

	// vec4 color = vec4(bg, bg, bg, 1);
	// if (s >= 0) color.r = 1;
	// if (h > 1) color.g = 1;
	// if (h == -2) color.g = 1;
	// if (s == -2) color.r = 1;
	// if (h == -2 && s > -2) color.b = 1;
	// if (s >= 0 && h <= 0.0001) color.r = 1;
	// imageStore(albedo, uv, color);
	// return;

	vec4 cool = vec4(0.38, 0.5, 2.91, 1.0);
	vec4 white = vec4(0.87, 0.87, 0.87, 1.0);
	vec4 warm = vec4(0.86, 0.36, 0.29, 1.0);

	vec4 col;
	if (mode == 0)
	{
		if (s < maxSpecies)
			imageStore(albedo, uv, mix(colors[s] + offset, colors[s] - offset, a));
		else
			imageStore(albedo, uv, vec4(bg, bg, bg, 1));
	}
	else if (mode == 1) // Display heights with cool / warm color palette
	{
		if (s >= maxSpecies)
			col = cool;
		else
		{
			float hMax = sParams[s].hmax;
			float u = h / hMax;
			if (u < 0.5)
				col = mix(cool, white, u / 0.5);
			else
				col = mix(white, warm, (u - 0.5) / 0.5);
		}

		imageStore(albedo, uv, col);
	}
	else if (mode == 2) // Saplings
	{
		float hMax = 0;
		for (int s = 0; s < nbSpecies; ++s)
		{
			float rl = imageLoad(regenLayers, ivec3(uv, s)).r;
			if (rl < 0) continue;
        	float h = GetHeight(rl, s);
			if (h > hMax) hMax = h;
		}

		float u = hMax / limit;
		if (u < 0.5)
			col = mix(cool, white, u / 0.5);
		else
			col = mix(white, warm, (u - 0.5) / 0.5);
		
		imageStore(albedo, uv, col);
	}
	else if (mode == 3) // Fire
	{
		// Wildfire
		vec4 nf = vec4(0.35, 0.35, 0.38, 1.0);
		vec4 f = vec4(0.0, 0.6, 0.1, 1.0);
		vec4 burning = vec4(1.0, 0.25, 0.05, 1.0);
		vec4 burned = vec4(0.08, 0.08, 0.08, 1.0);

		uint bs = decodeBurnFlag(uv);
		switch (bs)
		{
			case NON_FLAMMABLE:
				col = nf;
				break;
			case FLAMMABLE:
				col = f;
				break;
			case BURNING:
				col = burning;
				break;
			case BURNED:
				col = burned;
		}

		
		imageStore(albedo, uv, col);
	}
	else if (mode == 4) // bg only
	{
		imageStore(albedo, uv, vec4(bg, bg, bg, 1));
	}
}

#endif
