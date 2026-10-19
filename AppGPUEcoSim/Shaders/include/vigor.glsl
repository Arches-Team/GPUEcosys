float layerResponse(ivec2 p, float h)
{
    float avg = imageLoad(canopy, p / 5).g;
    // >= and not >: an empty 10m block has avg = 0, and seeding probes with h = 0, which
    // used to fall through to the division below and yield 0/0. The NaN then propagated to
    // a vigor of exactly 0, so no seed could ever appear away from a mature tree.
    // The boundary case h == avg already evaluated to 0.3, so this is continuous.
    if (h >= avg) return 0.3;
    if (h < avg * 0.8) return 1;
    return (1 - pow(abs(sin(3.14 * h / (2 * 0.2 * avg))), 3)) * 0.7 + 0.3;
}

float responseFunction(float val, float c, float r, float a)
{
  float ln = log(0.2);
  float d = abs(val - c);
  return (1 + a) * exp(pow(d / r, 4.5) * ln) - a;
}

float tempResponse(ivec2 p, uint sp, int i)
{
  float minT = tempBounds[i * 2];
  float maxT = tempBounds[i * 2 + 1];
  float t = imageLoad(ressources, ivec3(p, 10)).r * (maxT - minT) + minT;
  if (tempChangeEnd > 0)
    t += tempChange * max(0.0, min(1.0, float(year - tempChangeStart) / float(tempChangeEnd - tempChangeStart)));
  return responseFunction(t, sParams[sp].c_t, sParams[sp].r_t, sParams[sp].a);
}
float moistResponse(ivec2 p, uint sp)
{
  float m = imageLoad(ressources, ivec3(p, 9)).r;
  if (moistChangeEnd > 0)
    m *= 1 + (moistChange - 1) * max(0.0, min(1.0, float(year - moistChangeStart) / float(moistChangeEnd - moistChangeStart)));;
  return responseFunction(m, sParams[sp].c_m, sParams[sp].r_m, sParams[sp].a);
}
float slopeResponse(ivec2 p, uint sp)
{
  float sl = imageLoad(ressources, ivec3(p, 11)).r;
  return responseFunction(sl, sParams[sp].c_sl, sParams[sp].r_sl, sParams[sp].a);
}
float sunResponse(ivec2 p, uint sp, int i, float shadow)
{
  float s = imageLoad(ressources, ivec3(p, i)).r * shadow;
  return responseFunction(s, sParams[sp].c_s, sParams[sp].r_s, sParams[sp].a);
}

float windResponse(ivec2 p, uint sp)
{
  if (useWind == 0)
    return 1;
  
  float w = imageLoad(ressources, ivec3(p, 12)).r;
  return responseFunction(w, 0, 0.7, sParams[sp].a);
}

float ComputeVigor(ivec2 p, uint sp, int i, float h, float l)
{
  float f_t = tempResponse(p, sp, i);
  float f_m = moistResponse(p, sp);
  float f_sl = slopeResponse(p, sp);
  float f_w = windResponse(p, sp);

  // float shadow = l; // V3: LIF scale = 1 - V4: LIF scale = h / c
  float shadow = 1 - (1 - l) * layerResponse(p, h); // V2 - V5 with LIF scale = h / c
  // float shadow = l * layerResponse(p, h); // V1
  float f_s = sunResponse(p, sp, i, shadow);

  float b = imageLoad(boost, ivec3(p / 5, sp)).r;
  float B = sign(b) * min(b * b, 1);
  return min(min(f_s, min(f_t, min(f_sl, min(f_m, f_w)))) + B, 1);
}
