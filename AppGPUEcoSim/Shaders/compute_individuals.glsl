#version 430 core

#ifdef COMPUTE_SHADER

#include <include/data.glsl>

layout (binding = 0, r16f) readonly uniform image2DArray ressources;
layout (binding = 1, r32f) readonly uniform image2D lif;
layout (binding = 2, r32f) readonly uniform image2DArray lips;
layout (binding = 3, r8ui) uniform uimage2D species;
layout (binding = 4, r32f) uniform image2D age;
layout (binding = 5, r32f) uniform image2D carbonPool;
//layout (binding = 5, r32f) uniform image2D debug;
layout (binding = 6, r16f) readonly uniform image2DArray boost;
layout (binding = 7, rg32f) readonly uniform image2D canopy;

layout (std430, binding = 0) readonly buffer Buffer1
{
	float tempBounds[];
};
layout (std430, binding = 1) readonly buffer Buffer2
{
	SpeciesParams sParams[];
};

uniform int nbLips;
uniform int N;
uniform int monthIndex;

// Grid dimensions
uniform int nx;
uniform int ny;
uniform int year;

uniform int start_lgc;
uniform int end_lgc;
uniform int start_sgc;
uniform int end_sgc;

uniform int checkDeath;

uniform float tempChange;
uniform int tempChangeStart;
uniform int tempChangeEnd;

uniform float moistChange;
uniform int moistChangeStart;
uniform int moistChangeEnd;
uniform int useWind;

#include <include/random.glsl>
#include <include/species.glsl>
#include <include/vigor.glsl>
#include <include/lip.glsl>
#include <include/encoding.glsl>

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    // Grid coords 2m
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

    uint sp = decodeSpecies(uv);

    if (sp < maxSpecies && sParams[sp].enabled == 1)
    {
        int i = monthIndex;
        int sLength;
        int lastMonth;
        if (sParams[sp].longGrowthSeason != 0)
        {
            sLength = 9;
            lastMonth = 8;
        }
        else
        {
            if (monthIndex >= 6) return;
            i += 1;
            sLength = 6;
            lastMonth = 6;
        }

        float t = imageLoad(age, uv).r;
        float h = GetHeight(t, sp);


        // Get LIF value
        float cr = GetCanopyRadius(h, sp);
        int r = int(floor(cr / 2));
        int nb = 0;
        float f_light = 0;

        for (int i = -r; i <= r; ++i)
            for (int j = -r; j <= r; ++j)
            {
                ivec2 offset = ivec2(i, j);
                ivec2 coords = uv + offset;
                
                // cr is in meters and a cell is 2m wide: comparing it to the raw cell offset
                // made the test unreachable, so the neighbourhood was a square, not a disc
                if (length(vec2(offset) * 2) > cr) continue;

                if (min(coords.x, coords.y) < 0) continue;
                if (coords.x >= nx) continue;
                if (coords.y >= ny) continue;
                
                nb++;
                float l = max(0, imageLoad(lif, coords).r);
                // Remove this tree's own shadow from the field it is reading
                float contrib = GetLIFContribution(sp, h, uv, offset);
                if (contrib == 0)
                    f_light += 1;
                else
                    f_light += l / contrib;
            }

        if (nb > 0)
            f_light /= nb;

        // The LIF is a product of factors in [0,1], so the light available to the tree is
        // at most 1: clamping upwards instead would make shadow >= 1 and neutralise the
        // light competition of every mature tree.
        f_light = min(f_light, 1);



        float vigor = ComputeVigor(uv, sp, i, h, f_light);

        float C = imageLoad(carbonPool, uv).r;

        C += vigor;
        if (C > 1)
        {
            float vitality = max(0, C - 1);
            C = 1;

            float growth = max(0, vitality);
            t += growth / sLength;
        }

        // Test death only at the end of the growth season
        if (checkDeath == 1 && i == lastMonth)
        {
            float pDeath_age = 1 - pow(sParams[sp].pa_max, 1. / sParams[sp].a_max);
            float pDeath_stress = max(-C, 0);
            float pDeath = min(pDeath_age + pDeath_stress, 1);
            float r = Hash01(uvec3(gl_GlobalInvocationID.xy, uint(year)));
            if (r <= pDeath)
            {
                t = 0;
                encodeSpecies(uv, 63);
            }
        }

        imageStore(age, uv, vec4(t, 0, 0, 0));
        imageStore(carbonPool, uv, vec4(C, 0, 0, 0));
        //imageStore(debug, uv, vec4(vigor, 0, 0, 0));
    }
}

#endif