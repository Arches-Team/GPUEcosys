const int NON_FLAMMABLE = 0;
const int FLAMMABLE = 1;
const int BURNING = 2;
const int BURNED = 3; 

uint decodeBurnFlag(ivec2 uv) 
{
    uint s = imageLoad(species, uv).r;
    return (s >> 6) & 0x3u;
}

void encodeBurnFlag(ivec2 uv, uint val)
{
    val &= 0x3u;  // ensure only 2 bits

    uint s = imageLoad(species, uv).r;

    // Clear top 2 bits
    s &= 0x3Fu;   // 00111111

    // Set new top 2 bits
    s |= (val << 6);

    imageStore(species, uv, uvec4(s, 0, 0, 0));
}

uint decodeSpecies(ivec2 uv)
{
    uint s = imageLoad(species, uv).r;
    return s & 0x3Fu;
}

void encodeSpecies(ivec2 uv, uint val)
{
    val &= 0x3Fu;   // ensure only 6 bits

    uint s = imageLoad(species, uv).r;

    // Clear lower 6 bits
    s &= 0xC0u;     // 11000000 → keeps top 2 bits

    // Set new lower 6 bits
    s |= val;

    imageStore(species, uv, uvec4(s, 0, 0, 0));
}