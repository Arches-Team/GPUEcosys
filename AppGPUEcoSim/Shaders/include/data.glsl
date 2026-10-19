struct SpeciesParams
{
  float q;
  float k1;
  float k2;
  float k3;

  float c_s, r_s;
  float c_m, r_m;

  float c_t, r_t;
  float c_sl, r_sl;

  float a_max;
  float a;
  int longGrowthSeason;
  float hmax;

  float pa_max;
  int enabled;
  float ct;
  float crSlope;

  float p_b;
  float pad_1, pad_2, pad_3;
};

#define maxSpecies 63