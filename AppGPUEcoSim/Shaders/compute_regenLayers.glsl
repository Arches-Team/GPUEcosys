#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r16f) readonly uniform image2DArray ressources;
layout (binding = 1, r32f) readonly uniform image2D lif;
layout (binding = 2, r8ui) uniform uimage2D species;
layout (binding = 3, r32f) writeonly uniform image2D age;
layout (binding = 4, r32f) writeonly uniform image2D carbonPool;
layout (binding = 5, r32f) uniform image2DArray regenLayers;
layout (binding = 6, r16f) readonly uniform image2DArray boost;
layout (binding = 7, rg32f) readonly uniform image2D canopy;

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

uniform int start_lgc;
uniform int end_lgc;
uniform int start_sgc;
uniform int end_sgc;

uniform int year;
uniform float tempChange;
uniform int tempChangeStart;
uniform int tempChangeEnd;

uniform float moistChange;
uniform int moistChangeStart;
uniform int moistChangeEnd;

uniform float limit;
uniform int useWind;

#include <include/random.glsl>
#include <include/species.glsl>
#include <include/vigor.glsl>
#include <include/encoding.glsl>

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    // Grid coords 2m
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

    for (int s = 0; s < nbSpecies; ++s)
    {
        if (sParams[s].enabled == 0) continue;

        float rl = imageLoad(regenLayers, ivec3(uv, s)).r;
        float h = GetHeight(rl, s);

        // For 100m ablation study: need death
        if (limit > 100.0)
        {
            float pDeath_age = 1 - pow(sParams[s].pa_max, 1. / sParams[s].a_max);
            float rand = Hash01(uvec3(gl_GlobalInvocationID.xy, uint(year)));
            if (rand < pDeath_age) {
                imageStore(regenLayers, ivec3(uv, s), vec4(0, 0, 0, 0));
                continue;
            }
        }

        if (rl <= 0.0 || h >= limit) continue;

        float lif = imageLoad(lif, uv).r;
        float vigor = 0;

        int start = 1;
        int end = 7;
        int sLength = 6;
        if (sParams[s].longGrowthSeason == 1) {
            start = 0;
            end = 9;
            sLength = 9;
        }

        // start / end are month *layer* indices, matching how gpu_resources was filled:
        // layer i holds the sunlight of month i + start_lgc, and tempBounds has 9 pairs.
        // This loop used to run over start_lgc..end_lgc, i.e. 2..10, so it read the wrong
        // sunlight layers, ran past the end of both arrays for m = 9 and 10 (hitting the
        // moisture and temperature layers), and summed 9 months while dividing by sLength.
        for (int m = start; m < end; ++m)
            vigor += ComputeVigor(uv, s, m, rl, lif);

        vigor /= sLength;

        // For 0m ablation study
        if (limit < 1)
		{
            uint sp = decodeSpecies(uv);
            if (sp >= maxSpecies)
            {
                float r1 = Hash01(vec4(uv, year, 6));
                float r2 = Hash01(vec4(uv, year, 7));
                float r3 = Hash01(vec4(uv, year, 8));
                int sigma = int(rl * 0.3);
                float a = NormalDist(r1, r2, rl, sigma);

                if (r3 >= vigor) continue;
                
                // Instantiation
                imageStore(age, uv, vec4(a, 0, 0, 0));
                imageStore(carbonPool, uv, vec4(1, 0, 0, 0));
                encodeSpecies(uv, s);
                for (int i = 0; i < nbSpecies; ++i)
                    imageStore(regenLayers, ivec3(uv, i), vec4(0, 0, 0, 0));
            }
            continue;
        }
        
        float r = Hash01(vec4(uv, year, 5));
        if (r < sParams[s].p_b)
                continue;

        if (vigor > 0)
        {
		    rl += vigor;
            float h = GetHeight(rl, s);

			if (h > limit)
			{
                uint sp = decodeSpecies(uv);
                if (sp >= maxSpecies)
                {
                    float r1 = Hash01(vec4(uv, year, 6));
                    float r2 = Hash01(vec4(uv, year, 7));
                    int sigma = int(rl * 0.3);
                    float a = NormalDist(r1, r2, rl, sigma);

                    // Instantiation
                    imageStore(age, uv, vec4(a, 0, 0, 0));
                    imageStore(carbonPool, uv, vec4(1, 0, 0, 0));
                    encodeSpecies(uv, s);
                    for (int i = 0; i < nbSpecies; ++i)
                        imageStore(regenLayers, ivec3(uv, i), vec4(0, 0, 0, 0));
                }
			}
            else 
                imageStore(regenLayers, ivec3(uv, s), vec4(rl, 0, 0, 0));
        }
        else
            imageStore(regenLayers, ivec3(uv, s), vec4(0, 0, 0, 0));
    }
}

#endif