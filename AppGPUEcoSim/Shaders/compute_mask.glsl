#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, rgba8) writeonly uniform image2D albedo;
layout (binding = 1, r32f) writeonly uniform image2D debug;
layout (binding = 2, r32f) readonly uniform image2D age;
layout (binding = 3, r8ui) uniform uimage2D species;
layout (binding = 4, r16f) readonly uniform image2D mask;

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
uniform int year;

uniform int sp; // Species to visualize
uniform int radius;
uniform float threshold;
uniform int useThresh;

#include <include/encoding.glsl>
#include <include/species.glsl>

void ComputeMaskCrown(ivec2 uv)
{
    bool present = false;
    for (int i = -4; i <= 4; ++i)
      for (int j = -4; j <= 4; ++j)
      {
        ivec2 offset = ivec2(i, j);
        ivec2 pos = uv + offset;
        
        if (min(pos.x, pos.y) < 0) continue;
		if (pos.x >= nx) continue;
		if (pos.y >= ny) continue;

        uint s = decodeSpecies(pos);
        if (s != sp) continue;

        float a = imageLoad(age, pos).r;
        float h = GetHeight(a, s);
        float c = GetCanopyRadius(h, s) * 3; // inflate crown

        float dist = length(offset * 2);

        // TODO if uv under canopy of tree of species sp located at pos
        if (dist < c)
        {
          present = true;
          break;
        }
      }

      if (present) {
        imageStore(albedo, uv, colors[sp]);
        imageStore(debug, uv, vec4(1, 0, 0, 0));
      }
      else {
        imageStore(albedo, uv, vec4(1, 1, 1, 1));
        imageStore(debug, uv, vec4(0, 0, 0, 0));
      }
}

void ComputeMaskNeighborhood(ivec2 uv)
{
    float m = imageLoad(mask, uv).r;
    if (m != 0) {
        imageStore(albedo, uv, vec4(1, 1, 1, 1));
        imageStore(debug, uv, vec4(0, 0, 0, 0));
        return;
    }

    float n = 0;
    for (int i = -radius; i <= radius; ++i)
        for (int j = -radius; j <= radius; ++j)
        {
            ivec2 offset = ivec2(i, j);
            if (length(offset) > radius) continue;
            ivec2 pos = uv + offset;
            
            if (min(pos.x, pos.y) < 0) continue;
            if (pos.x >= nx) continue;
            if (pos.y >= ny) continue;

            uint s = decodeSpecies(pos);
            if (s != sp) continue;

            float a = imageLoad(age, pos).r;
            float h = GetHeight(a, s);

            n += h / sParams[s].hmax;
        }

    n /= pow(radius * 2 + 1, 2);
    if (useThresh == 0) {
        imageStore(albedo, uv, colors[sp]);
        imageStore(debug, uv, vec4(n, 0, 0, 0));
        return;
    }

    if (n > threshold) {
        imageStore(albedo, uv, colors[sp]);
        imageStore(debug, uv, vec4(1, 0, 0, 0));
    }
    else {
        imageStore(albedo, uv, vec4(1, 1, 1, 1));
        imageStore(debug, uv, vec4(0, 0, 0, 0));
    }
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{	
    ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

    // ComputeMaskCrown(uv);
    ComputeMaskNeighborhood(uv);    
}

#endif
