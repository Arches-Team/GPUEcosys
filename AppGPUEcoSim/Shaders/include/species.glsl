// Get height from nominal age t
float GetHeight(float t, uint sp)
{
  return ((2. / (1 + exp((t / sParams[sp].a_max) * sParams[sp].q))) - 1) * sParams[sp].hmax;
}

float GetCanopyRadius(float h, uint sp)
{
  return h * sParams[sp].crSlope;
}
