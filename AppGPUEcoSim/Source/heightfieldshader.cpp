// Heightfield

#include "gputerrainsimu.h"
#include "heightfieldshader.h"
#include "vectorfield.h"
#include "linear.h"
#include "cpu.h"

/*!
\class HeightFieldShader heightfieldshader.h
\brief Relief shading class.

The texture image representing the shaded terrain for Hugo's 2024 paper:
 \code
 return LaplacianShading(Palette::HugoShading, 1.6, 0.6);
\endcode

*/

/*!
\brief Create a shader.
\param field Reference to a heightfield.
*/
HeightFieldShader::HeightFieldShader(const HeightField& field) :heightfield(field)
{}

/*!
\brief Empty.
*/
HeightFieldShader::~HeightFieldShader()
{
}

#include "cpu.h"

#include "gputerrainsimu.h"

/*!
\brief Diffuse lighting with enhanced curvature shaded relief.

Create a texture representing the shaded terrain.
*/
QImage HeightFieldShader::ShadedRelief() const
{
    double ra, rb;

    // Get elevation range
    heightfield.GetRange(ra, rb);

    // Modify range by a a few centimeters
    ra -= 0.05;
    rb += 0.05;

    // Shading
    QImage shading = QImage(heightfield.GetQtSize(), QImage::Format_ARGB32);

    // Light
    ScalarField2 light = heightfield.Light(Normalized(Vector(-1.0, 0.5, 2.5)));

    ScalarField2 fractional = GPUHeightFieldAnalysis().FractionalLaplacian(heightfield, 2.0, 0.75);
    fractional.Unitize();

    double u, v;
    fractional.GetRange(u, v);
    ScalarField2 accessibility = GPUHeightFieldAnalysis().Accessibility(heightfield, 25.0 * Norm(heightfield.CellDiagonal()), 64);

    // Laplacian for details
    ScalarField2 laplacian = heightfield.Laplacian();
    laplacian.Step(-0.20, 0.20);

    // Height field
    for (int i = 0; i < heightfield.GetSizeX(); i++)
    {
        for (int j = 0; j < heightfield.GetSizeY(); j++)
        {
            // Normalized height
            double z = Linear::Step(heightfield.at(i, j), ra, rb);

            // Height shading: color is a linear interpolation of height colors
            Color cz = Color::Lerp(z, Color(0.75, 0.725, 0.70), Color(0.95, 0.925, 0.90));

            double s = light(i, j);
            s *= s;
            s *= s;

            Color c = cz * (1.0 + s) / 2.0;
            double la = 1.0 - 2.0 * laplacian(i, j);

            la *= fabs(la);


            c *= 0.70 + 0.30 * ((1.0 + fractional(i, j)) / 2.0) + 0.05 * accessibility(i, j) + 0.05 * la;

            shading.setPixel(i, j, c.GetQt().rgb());
        }
    }
    return shading;
}

/*!
\brief Diffuse lighting with enhanced curvature shaded relief.

Create a texture representing the shaded terrain.
*/
QImage HeightFieldShader::Eric() const
{
    double ra, rb;

    // Get elevation range
    heightfield.GetRange(ra, rb);

    // Modify range by a a few centimeters
    ra -= 0.05;
    rb += 0.05;

    // Shading
    QImage shading = QImage(heightfield.GetQtSize(), QImage::Format_ARGB32);

    // Laplacian
    ScalarField2 fractional = GPUHeightFieldAnalysis().FractionalLaplacian(heightfield, 50, 0.75);
    fractional.Unitize();

    VectorField2 gradient;
    ScalarField2 slope;
    GPUHeightFieldAnalysis().FractionalGradient(heightfield, 0.25, 50, gradient, slope);
    slope.Clamp(0.0, 2.0);
    slope.Unitize();

    // Height field
    for (int i = 0; i < heightfield.GetSizeX(); i++)
    {
        for (int j = 0; j < heightfield.GetSizeY(); j++)
        {
            // Normalized height, laplacian, slope, accessibility
            double z = Linear::Step(heightfield.at(i, j), ra, rb);
            double l = fractional.at(i, j);
            double s = slope.at(i, j);

            Color c = Color::Lerp(0.80 + 0.05 * z + 0.1 * l + 0.05 * s);

            shading.setPixel(i, j, c.GetQt().rgb());
        }
    }
    return shading;
}
