// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

// Hash 0 - 1
// https://stackoverflow.com/a/17479300
float Hash01(uvec3 v)
{
//   return floatConstruct(hash(floatBitsToUint(v)));
  return floatConstruct(hash(v));
}
float Hash01(vec4 v)
{
  return floatConstruct(hash(floatBitsToUint(v)));
}

// Normal distribution using the Box-Muller transform
// r1 and r2 are uniform random number in [0, 1]
float NormalDist(float r1, float r2, float mean, float stddev)
{
    float n = sqrt(-2.0 * log(r1)) * cos(2 * 3.14 * r2);
    return mean + stddev * n;
}