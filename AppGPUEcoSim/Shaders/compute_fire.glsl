#version 430 core

#ifdef COMPUTE_SHADER

#include <include/random.glsl>
#include <include/data.glsl>

layout (binding = 0, r16f) readonly uniform image2DArray ressources;
layout (binding = 1, r8ui) uniform uimage2D species;
layout (binding = 2, r32f) uniform image2D age;
layout (binding = 3, r32f) writeonly uniform image2DArray regenLayers;
layout (binding = 4, rg32f) readonly uniform image2D canopy;
layout (binding = 5, r32f) readonly uniform image2D carbonPool;

layout (std430, binding = 0) readonly buffer Buffer2
{
	SpeciesParams sParams[];
};

#include <include/species.glsl>
#include <include/encoding.glsl>

// Grid dimensions (2m cells)
uniform int nx;
uniform int ny;
uniform int year;

uniform float maxH;
uniform float minH;

uniform float windDirX;
uniform float windDirY;
uniform float windSpeed;
uniform int useWind;

float GetTerrainHeight(ivec2 uv) {
    return imageLoad(ressources, ivec3(uv, 10)).r * (maxH - minH) + minH;
}

void GetDensity(ivec2 uv, out float d, out float moist) {
    const float canopyDensity = 0.3; //0.075;
    const float trunkDensity = 0.8; //0.2;
    float ad = 0;
    float wd = 0;
    d = 0;
    for (int i = -4; i <= 4; ++i)
        for (int j = -4; j <= 4; ++j)
        {
            ivec2 offset = ivec2(i, j);
            ivec2 pos = uv + offset;
        
            if (min(pos.x, pos.y) < 0) continue;
			if (pos.x >= nx) continue;
			if (pos.y >= ny) continue;

            // 63 marks an empty cell and sParams has no entry for it: without this guard
            // GetHeight / GetCanopyRadius read past the end of the storage buffer
            uint s = decodeSpecies(pos);
            if (s >= nbSpecies) continue;

            float a = imageLoad(age, pos).r;
            float h = GetHeight(a, s);
            float c = GetCanopyRadius(h, s);

            float dist = length(offset * 2);

            if (i == 0 && j == 0)
                wd = h * trunkDensity;
            else if (dist < c)
                wd += h * canopyDensity;
            d += wd;

            float C = imageLoad(carbonPool, pos).r;
            ad += wd * (0.5 + 0.5 * C);
        }

    if (wd > 0)
        moist = ad / wd;
    else
        moist = 0;
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	if (uv.x >= nx) return;
	if (uv.y >= ny) return;

    uint status = decodeBurnFlag(uv);

    const float p0 = 0.6;
    const float wind_c1 = 0.045;
    const float wind_c2 = 0.191;
    const float height_d = 0.932;
    const float slope_a = 0.063;
    const float moist_a = 3.258;
    const float moist_b = 0.111;

    if (status == NON_FLAMMABLE) return;
    if (status == BURNED) return;

    if (status == BURNING)
    {
        encodeBurnFlag(uv, BURNED);

        encodeSpecies(uv, 63);
        imageStore(age, uv, vec4(0, 0, 0, 0));
        for (int i = 0; i < nbSpecies; ++i)
            imageStore(regenLayers, ivec3(uv, i), vec4(0, 0, 0, 0));

        float h0 = GetTerrainHeight(uv);
        vec2 windDir = vec2(windDirX, windDirY);

        // Depends only on uv, but used to sit inside the neighbour loop below, so this
        // 9x9 gather was evaluated 8 times per burning cell for an identical result
        float den, moist;
        GetDensity(uv, den, moist);

        for (int i = -1; i < 2; ++i)
            for (int j = -1; j < 2; ++j)
            {
                if (i == 0 && j == 0) continue;

                ivec2 offset = ivec2(i, j);
                ivec2 pos = uv + offset;
                if (pos.x >= nx) continue;
                if (pos.y >= ny) continue;
                if (min(pos.x, pos.y) < 0) continue;

                float h1 = GetTerrainHeight(pos);
                float dist = sqrt(i*i*2*2 + j*j*2*2);
                float slope = atan((h1 - h0) / dist) * 180. / 3.14;

                float dx = i / dist;
                float dy = j / dist;
                vec2 d = vec2(dx, dy);

                uint s = decodeBurnFlag(pos);

                float pDen = 1 - 0.4 + den * 0.7;
                float pMoist = moist_a * exp(-moist_b * moist);
                float pSlope = exp(slope_a * slope);
                float pHeight = pow(imageLoad(canopy, uv / 5).r / 50.0, height_d);

                float cosW = dot(d, windDir) / (length(d) * length(windDir));
                float pWind = exp(windSpeed * (wind_c1 + wind_c2 * (cosW - 1)));

                float pBurn = p0 * pHeight * pSlope * pWind; // * pDen * pMoist;

                // The draw has to depend on the neighbour: with a constant 4 the same number
                // was reused for all 8 of them, so spread was perfectly correlated -- every
                // neighbour above the threshold caught fire, or none did. Offsets 8..16 stay
                // clear of the constants used elsewhere (0-3 seeding, 5-7 regeneration).
                float r = Hash01(vec4(uv, year, 8 + (i + 1) * 3 + (j + 1)));
                if (s == FLAMMABLE && r < pBurn)
                    encodeBurnFlag(pos, BURNING);
            }
    }
}

#endif