#include <QtCore/QFile>

#include "abiotics.h"
#include "cpu.h"
#include "gputerrainsimu.h"

/* \brief Proxy for moisture
 *
 * SMI = 0.7 x 1 / DTW + 0.3 x TWI
*/
ScalarField2 Moisture::ProxySMI(const HeightField& hf)
{
    HeightField H = hf;
    H.CompleteBreach();

    ScalarField2 twi = H.WetnessIndex();
    twi.Normalize();

    TerrainDX tdx(hf);
    ScalarField2 dtw = hf.DepthToWater(&tdx);
    dtw.Clamp(1.0, 10000000.0);

    ScalarField2 inv_dtw = 1.0 / dtw;

    ScalarField2 smi = 0.7 * inv_dtw + 0.3 * twi;
    return smi;
}

SunlightSimulator::SunlightSimulator(double lat, Vector north)
    : sunMaps(12), direct(12), sunHours(12), latitude(lat), north(north)
{}

void SunlightSimulator::Simulate(const HeightField& hf, int minuteStep, int samples)
{
    DiffuseSunlight(hf, samples);
    DirectSunlight(hf, minuteStep, sunHours);
}

/*!
 * \brief Calculate the direct sunlight exposure for the middle of each month
 *
 * The self-shadowing of the terrain is taken into account
 *
 * \param hf The terrain
 * \param minuteStep The increment in minutes for the sun position sampling
 * \param sunHours The number of hours of sun per day, for each month
*/
void SunlightSimulator::DirectSunlight(const HeightField& hf, int minuteStep, QVector<double>& sunHours, bool gpu)
{
    for (int m = 0; m < 12; ++m)
    {
        DirectSunlight(hf, m, minuteStep, sunHours[m], gpu);
    }
}

void SunlightSimulator::DirectSunlight(const HeightField& hf, int m, int minuteStep, double& sunHours, bool gpu)
{
    double hourstep = double(minuteStep) / 60.0;
    int nx = hf.GetSizeX();
    int ny = hf.GetSizeY();
    direct[m] = ScalarField2(Box2(hf.GetBox()), nx, ny, 0.);
    sunHours = 0;

    for (int t = 0; t < 1440; t += minuteStep)
    {
        Vector sunPos = GetSunPos(m, t);
        if (sunPos[2] > 0) // Sun above the horizon
        {
            Normalize(sunPos);
            CalculateVisibility(direct[m], hf, sunPos, hourstep, gpu); // Update sunMap
            sunHours += hourstep;
        }
    }
}

/*!
\brief Sample the hemisphere of the sky to get a diffuse sunlight contribution (for cloudy days)

\param hf The terrain
\param samples The number of samples
*/
void SunlightSimulator::DiffuseSunlight(const HeightField& hf, int samples, bool usegpu)
{
    if (usegpu)
    {
        GPUHeightFieldAnalysis gpu;
        diffuse = gpu.Accessibility(hf, 10, samples, 99999);
    }
    else
    {
        diffuse = hf.Accessibility(10, samples);
    }
}

QVector<ScalarField2> SunlightSimulator::TotalSunlight(const QVector<double>& cloudiness)
{
    for (int i = 0; i < 12; ++i)
    {
        sunMaps[i] = direct[i] * (1. - cloudiness[i]) + diffuse * cloudiness[i] * sunHours[i];
    }

    return sunMaps;
}



/*!
* \brief Get the position of the sun
*
* \param m Month (0 = January, 11 = December)
* \param t Time of the day in minutes
*/
Vector SunlightSimulator::GetSunPos(int m, int t) const
{
    // First calculate some orientations we need values we need
    Vector east_orientation = Matrix::RotationZ(M_PI_2) * north;
    Vector true_north_orientation = Matrix::Rotation(east_orientation, Math::DegreeToRadian(-latitude)) * north;

    int sun_trajectory_radius = 500000;
    double max_axis_tilt = GetAxisTiltAngle(m);
    double day_angle = MinutesToAngle(t);
    Vector cp_tn_and_east = Normalized(true_north_orientation / east_orientation);

    // First calculate the sun position at midday during the equinox
    Vector sun_position = sun_trajectory_radius * cp_tn_and_east;

    // Now take into consideration axis tilt based on the month
    sun_position = Matrix::Rotation(east_orientation, -max_axis_tilt) * sun_position; // PITCH

    // Now rotate around true north for the day
    sun_position = Matrix::Rotation(true_north_orientation, day_angle) * sun_position;

    // Now align to the center of the terrain (i.e the center of the terrain is at the latitude specified)
    // sun_position += glm::vec3(center.x, center.y, center.z);

    return sun_position;
}

/*!
 * \brief Axial tilt based on the month of the year
 *
 * \param m The month (0 = January, 11 = December)
 */
double SunlightSimulator::GetAxisTiltAngle(int m) const
{
    return -axisTilt + Math::Abs(5.0 - m) * monthlyAxisTilt;
}

/*!
 * \brief The rotation relative to noon, in radians
 *
 * \param minutes The time of the day, in minutes
*/
double SunlightSimulator::MinutesToAngle(int minutes) const
{
    return (double(halfDayInMinutes - minutes) / halfDayInMinutes) * M_PI;
}

/*!
 * \brief Increments the sunMap by the timestep if the points on the terrain are
 * visible from the sun's perspective
 *
 * \param sunMap The sun exposure values
 * \param hf The HeightField
 * \param sunPos The sun vector
 * \param timestep The timestep (in hours)
 */
void SunlightSimulator::CalculateVisibility(ScalarField2& sunMap, const HeightField& hf, const Vector& sunPos, double timestep, bool gpu)
{
    if (gpu)
    {
        GPUHeightFieldAnalysis gpua;
        ScalarField2 res = gpua.Visibility(hf, sunPos, 99999);
        sunMap += res * timestep;
    }
    else
    {
        Box box = hf.GetBox();
        const double epsilon = 0.1;
        const double length = Norm(box.Diagonal());

        const int nx = hf.GetSizeX();
        const int ny = hf.GetSizeY();
        double k = hf.K();
        for (int i = 0; i < nx; ++i)
        {
            for (int j = 0; j < ny; j++)
            {
                Ray ray(hf.Vertex(i, j) + hf.Normal(i, j) * epsilon, sunPos);
                Vector p;
                double t;
                if (!hf.Intersect(ray, t, p, box, k, length))
                {
                    sunMap(i, j) += timestep;
                }
            }
        }
    }
}


