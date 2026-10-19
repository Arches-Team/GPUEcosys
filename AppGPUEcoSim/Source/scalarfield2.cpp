// Fields
#include <algorithm>

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <immintrin.h>

#include "scalarfield.h"
#include "vectorfield.h"
#include "arrayinteger.h"
#include "linear.h"
#include "cpu.h"

/*!
\class ScalarField2 scalarfield.h
\brief A base two-dimensional field of real values.

\ingroup StructureGroup
*/

/*!
\brief Create the field structure.
\param a Array representing the grid domain.
\param v Constant value of field.
*/
ScalarField2::ScalarField2(const Array2& a, const double& v) :Array2(a)
{
    field.fill(v, nx * ny);
}

/*!
\brief Create the field structure.
\param box The box.
\param x,y Size of the array.
\param v Constant value of field.
*/
ScalarField2::ScalarField2(const Box2& box, int x, int y, const double& v) :ScalarField2(Array2(box, x, y), v)
{}

/*!
\brief Create the field structure.
\param box The box.
\param x,y Size of the array.
\param v Array of scalar values.
*/
ScalarField2::ScalarField2(const Box2& box, int x, int y, const QVector<double>& v) :Array2(box, x, y), field(v)
{}

/*!
\brief Create a field structure from an image.
\param image The image.
\param v Range of values to scale the image, values will vary between 0.0 and v.
\param d Distance between samples.
\param grayscale Read image as grayscale if set to true, otherwize use color for a better accuracy.
*/
ScalarField2::ScalarField2(const QImage& image, const double& v, const double& d, bool grayscale) :ScalarField2(Box2(image.size()).Scaled(d), image, 0.0, v, grayscale)
{}

/*!
\brief Create a field structure from an image.
\param box The box.
\param image The image.
\param a,b Range of values to scale the image.
\param grayscale Read image as grayscale if set to true, otherwize use color for a better accuracy.
*/
ScalarField2::ScalarField2(const Box2& box, const QImage& image, const double& a, const double& b, bool grayscale) :Array2(box, image.width(), image.height())
{
    // Set size
    field.resize(nx * ny);

    // Write Heightmap
    for (int i = 0; i < image.width(); i++)
    {
        for (int j = 0; j < image.height(); j++)
        {
            double t = 0.0;

            // Grayscale
            if (grayscale)
            {
                // Grayscale 16 bits
                if (image.format() == QImage::Format_Grayscale16 || image.format() == QImage::Format_RGBA64)
                {
                    QColor thecolor = image.pixelColor(i, j);
                    t = thecolor.blueF();
                }
                // Grayscale 8 bits
                else
                {
                    QRgb color = image.pixel(i, j);
                    t = double(qGray(color)) / 255.0;
                }
            }
            // Color 
            else
            {
                QRgb color = image.pixel(i, j);
                // Maximum value is 256^3-1
                int u = qRed(color) << 16 | qGreen(color) << 8 | qBlue(color);

                t = double(u) / (16777216.0 - 1.0);
            }
            field[VertexIndex(i, j)] = Math::Lerp(a, b, t);
        }
    }
}

/*!
\brief Create a field structure from an image.

This is a convenience constructor which is the same as:
\code
ScalarField2 s(Box2(0.5), QImage(name),0.0,1.0,true);
\endcode

\param name Name of the image.
\param a,b Range of values to scale the image.
*/
ScalarField2::ScalarField2(const QString& name, double a, double b) :ScalarField2(Box2(0.5), QImage(name), a, b, true)
{}

/*!
\brief Create a ScalarField2 (double) from an Integer Array (int).

\param a Integer array.
*/
ScalarField2::ScalarField2(const Array2I& a) : ScalarField2(static_cast<const Array2&>(a))
{
    const int n = nx * ny;

    for (int i = 0; i < n; i++)
    {
        field[i] = a.at(i);
    }
}

/*!
\brief Create a ScalarField2 (double) from an Integer Array (int) with a lookup table for the double values.
The lookup table is a vector of double values where the index corresponds to the integer value in the array.
This allows for mapping integer values to specific double values in the scalar field.

\param int_array Integer array.
\param v Vector of double values used as a lookup table for mapping integer values to double values in the scalar field.
*/
ScalarField2::ScalarField2(const Array2I& a, const std::vector<double>& v) : ScalarField2(static_cast<const Array2&>(a))
{
    const int n = nx * ny;
    for (int i = 0; i < n; i++)
    {
        field[i] = v[a.at(i)];
    }
}

/*!
\brief Empty.
*/
ScalarField2::~ScalarField2()
{}

/*!
\brief Return the array representing the grid domain.
*/
Array2 ScalarField2::GetArray() const
{
    return Array2(Box2(a, b), nx, ny);
}

/*!
\brief Get the range of the field.

\avx

\param a,b Returned minimum and maximum.
*/
void ScalarField2::GetRange(double& a, double& b) const
{
    const int size = nx * ny;
    // Escape
    if (size == 0)
    {
        a = b = 0.0;
        return;
    }

#ifdef _MSC_VER
    if (System::Avx() && field.size() > 8)
    {
        const double* p = field.data();

        __m256d avxa = _mm256_load_pd(p);
        __m256d avxb = avxa;

        const int offset = size % 4;
        for (int i = 1; i < size / 4; i++)
        {
            __m256d avxi = _mm256_load_pd(p + i * 4);
            avxa = _mm256_min_pd(avxa, avxi);
            avxb = _mm256_max_pd(avxb, avxi);
        }
        double e[4];
        _mm256_store_pd(e, avxa);
        a = Math::Min(e[0], e[1], e[2], e[3]);

        _mm256_store_pd(e, avxb);
        b = Math::Max(e[0], e[1], e[2], e[3]);

        for (int i = size - offset; i < size; i++)
        {
            double x = field.at(i);
            if (x < a)
            {
                a = x;
            }
            else if (x > b)
            {
                b = x;
            }
        }
    }
    else
#endif
    {
        a = field.at(0);
        b = a;

        for (int i = 1; i < field.size(); i++)
        {
            double x = field.at(i);
            if (x < a)
            {
                a = x;
            }
            else if (x > b)
            {
                b = x;
            }
        }
    }
}

#include "histogram.h"

/*!
\brief Compute the histogram of the scalar field.
\param n Discretization.
*/
Histogram ScalarField2::GetHistogram(int n) const
{
    return Histogram(n, field);
}

/*!
\brief Compute the histogram of the scalar field.
\param a,b Interval.
\param n Discretization.
*/
Histogram ScalarField2::GetHistogram(int n, double a, double b) const
{
    return Histogram(n, a, b, field);
}

/*!
\brief Create an image from the field.
\param grayscale Export as grayscale if set to true, and to color otherwise (provides a better precision).
*/
QImage ScalarField2::CreateImage(bool grayscale) const
{
    double a, b;
    GetRange(a, b);

    // Check if the scalarfield is constant, in that case modify the range to avoid division by 0 errors
    if (a == b)
    {
        b = a + 1.0;
    }
    return CreateImage(a, b, grayscale);
}

/*!
\brief Create an image from the field using a palette.
\param palette The palette.
\param symmetric If set to true, adapt the range interval so that shading should be performed with median alignment.
\param transparent If set to true, the lowest values of the scalar field will be transparent.
*/
QImage ScalarField2::CreateImage(const AnalyticPalette& palette, bool symmetric, bool transparent) const
{
    double a, b;
    GetRange(a, b);
    // Check if the scalarfield is constant, in that case modify the range to avoid division by 0 errors
    if (a == b)
    {
        b = a + 1.0;
    }
    if (symmetric)
    {
        double m = Math::Max(fabs(a), fabs(b));
        a = -m;
        b = m;
    }

    return CreateImage(a, b, palette, transparent);
}

/*!
\brief Create an image from the field using a palette.
\param a,b Range of elevation that will be mapped to image scale.
\param palette The palette.
\param transparent If set to true, the lowest values of the scalar field will be transparent.
*/
QImage ScalarField2::CreateImage(const double& a, const double& b, const AnalyticPalette& palette, bool transparent) const
{
    // Define image
    QImage image(nx, ny, QImage::Format_ARGB32);

    // Write Heightmap
    for (int i = 0; i < image.width(); i++)
    {
        for (int j = 0; j < image.height(); j++)
        {
            double x = field.at(VertexIndex(i, j));
            double y = Linear::Step(x, a, b);

            QColor color = palette.GetColor(y).GetQt();

            // Transparency
            if (transparent)
            {
                if (y < 0.001)
                {
                    color.setAlpha(0);
                }
            }
            image.setPixel(i, j, color.rgba());
        }
    }
    return image;
}

/*!
\brief Create an image from the field.
\param a,b Range of elevation that will be mapped to image scale.
\param grayscale Export as grayscale if set to true, color otherwise.
*/
QImage ScalarField2::CreateImage(const double& a, const double& b, bool grayscale) const
{
    // Write Heightmap
    if (grayscale)
    {
        QImage image(nx, ny, QImage::Format_Grayscale16);
        for (int j = 0; j < image.height(); j++)
        {
            quint16* dst = reinterpret_cast<quint16*>(image.bits() + j * image.bytesPerLine());
            for (int i = 0; i < image.width(); i++)
            {
                double x = field.at(VertexIndex(i, j));
                double y = Linear::Step(x, a, b);

                unsigned short pixelval = ushort(y * 65535.0);

                dst[i] = pixelval;
            }
        }
        return image;
    }
    else
    {
        // Define image
        QImage image(nx, ny, QImage::Format_ARGB32);

        // Write Heightmap
        for (int i = 0; i < image.width(); i++)
        {
            for (int j = 0; j < image.height(); j++)
            {
                double x = field.at(VertexIndex(i, j));
                double y = Linear::Step(x, a, b);

                QColor color;

                int c = int(y * (256.0 * 256.0 * 256.0 - 1.0));
                int cr = (c >> 16) & 255;
                int cv = (c >> 8) & 255;
                int cb = c & 255;

                color = QColor(cr, cv, cb);

                image.setPixel(i, j, color.rgb());
            }
        }
        return image;
    }

}

/*!
\brief Get the field value at a given point.

This function computes a bi-linear interpolation of the values.

\sa Math::Bilinear
\param p Point (should be strictly inside the box domain).
*/
double ScalarField2::Value(const Vector2& p) const
{
    double u, v;
    int i, j;
    CellInteger(p, i, j, u, v);

    // Test position
    if (!InsideCellIndex(i, j))
    {
        return 0.0;
    }

    return Math::Bilinear(at(i, j), at(i + 1, j), at(i + 1, j + 1), at(i, j + 1), u, v);
}

/*!
\brief Sets the entire field with a constant value.
\param s Scalar.
*/
void ScalarField2::Fill(const double& s)
{
    field.fill(s, nx * ny);
}

/*!
\brief Change the resolution of the scalar field.

Note that because the box should be strictly inside the original domain, this
function is not the same as:
\code
ScalarField field(Box2(Vector2(-2.0),Vector(3.0),5,5); // Original field
ScalarField s=Sample(field.GetBox(),12,12);
\endcode
\param x,y Sampling size.
*/
ScalarField2 ScalarField2::Resized(int x, int y) const
{
    // Sampled scalar field
    ScalarField2 sampled(Box2(a, b), x, y);

    // Corners
    sampled(0, 0) = at(0, 0);
    sampled(0, y - 1) = at(0, ny - 1);
    sampled(x - 1, 0) = at(nx - 1, 0);
    sampled(x - 1, y - 1) = at(nx - 1, ny - 1);

    // Borders (use linear interpolation)
    for (int i = 1; i < x - 1; i++)
    {
            double tx = (nx - 1) * (i / double(x - 1));
            int x0 = int(floor(tx));
            int x1 = int(ceil(tx));
  
            sampled(i, 0) = Math::Lerp(at(x0, 0), at(x1, 0), tx - x0);
            sampled(i, y - 1) = Math::Lerp(at(x0, ny - 1), at(x1, ny - 1), tx - x0);
    }
  
    for (int j = 1; j < y - 1; j++)
    {
            double ty = (ny - 1) * (j / double(y - 1));
            int y0 = int(floor(ty));
            int y1 = int(ceil(ty));
  
            sampled(0, j) = Math::Lerp(at(0, y0), at(0, y1), ty - y0);
            sampled(x - 1, j) = Math::Lerp(at(nx - 1, y0), at(nx - 1, y1), ty - y0);
    }
  
    // Interior
    for (int i = 1; i < x - 1; i++)
    {
            for (int j = 1; j < y - 1; j++)
            {
            sampled(i, j) = Value(sampled.ArrayVertex(i, j));
            }
    }
    return sampled;
}

/*!
\brief Resample a rectangular region in the scalar field.
\param box Rectangular region, should be strictly inside the domain.
\param x,y Sampling size.
*/
ScalarField2 ScalarField2::Sample(const Box2& box, int x, int y) const
{
    // Sampled scalar field
    ScalarField2 sampled(box, x, y);

    // Sample the domain
    for (int i = 0; i < x; i++)
    {
        for (int j = 0; j < y; j++)
        {
            sampled(i, j) = Value(sampled.ArrayVertex(i, j));
        }
    }
    return sampled;
}


/*!
\brief Crops a rectangular region in the scalar field.

Note that the corners of the region of the scalar field are (0,0) and (nx-1,ny-1).

\param a,b Rectangular region.
*/
ScalarField2 ScalarField2::Crop(const QPoint& a, const QPoint& b) const
{
    // Vertices of the domain
    Vector2 va = ArrayVertex(a.x(), a.y());
    Vector2 vb = ArrayVertex(b.x(), b.y());

    // Size
    int x = b.x() - a.x() + 1;
    int y = b.y() - a.y() + 1;

    // Sampled scalar field
    ScalarField2 sampled(Box2(va, vb), x, y);

    // Copy
    for (int i = 0; i < x; i++)
    {
        for (int j = 0; j < y; j++)
        {
            sampled(i, j) = at(a.x() + i, a.y() + j);
        }
    }
    return sampled;
}

/*!
\brief Lower the resolution of the scalarfield.
\param f Lowering factor.
*/
ScalarField2 ScalarField2::DownSample(int f) const
{
    ScalarField2 res(Box2(a, b), nx / f, ny / f);

    for (int i = 0; i < nx / f; i++)
    {
        for (int j = 0; j < ny / f; j++)
        {
            double sij = 0.0;
            for (int ti = i * f; ti < i * f + f; ti++)
            {
                for (int tj = j * f; tj < j * f + f; tj++)
                {
                    sij += at(ti, tj);
                }
            }
            res(i, j) = sij / (f * f);
        }
    }

    return res;
}

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the array vertex.
\sa at(int,int)
*/
double ScalarField2::Value(int i, int j) const
{
    return field.at(VertexIndex(i, j));
}

/*!
\brief Compute the gradient at a given array vertex.

\param i,j Integer coordinates of the array vertex.
*/
Vector2 ScalarField2::Gradient(int i, int j) const
{
    Vector2 n;

    // Gradient along x axis
    if (i == 0)
    {
        n[0] = (at(i + 1, j) - at(i, j)) * inversecelldiagonal[0];
    }
    else if (i == nx - 1)
    {
        n[0] = (at(i, j) - at(i - 1, j)) * inversecelldiagonal[0];
    }
    else
    {
        n[0] = (at(i + 1, j) - at(i - 1, j)) * 0.5 * inversecelldiagonal[0];
    }

    // Gradient along y axis
    if (j == 0)
    {
        n[1] = (at(i, j + 1) - at(i, j)) * inversecelldiagonal[1];
    }
    else if (j == ny - 1)
    {
        n[1] = (at(i, j) - at(i, j - 1)) * inversecelldiagonal[1];
    }
    else
    {
        n[1] = (at(i, j + 1) - at(i, j - 1)) * 0.5 * inversecelldiagonal[1];
    }

    return n;
}

/*!
\brief Compute the %Laplacian at a given sample.

Returns a discrete approximation of Laplace's differential operator applied to the field.
The implementation is equivalent to the 4*del2(U) Matlab function, except on the edges of the field.

\sa Laplacian()
\author Mathieu Gaillard
\param i,j Integer coordinates of the sample.
*/
double ScalarField2::Laplacian(int i, int j) const
{
    double laplacian = 0.0;

    // Divergence along x axis
    if (i == 0)
    {
        laplacian += (at(i, j) - 2.0 * at(i + 1, j) + at(i + 2, j)) / (celldiagonal[0] * celldiagonal[0]);
    }
    else if (i == nx - 1)
    {
        laplacian += (at(i, j) - 2.0 * at(i - 1, j) + at(i - 2, j)) / (celldiagonal[0] * celldiagonal[0]);
    }
    else
    {
        laplacian += (at(i + 1, j) - 2.0 * at(i, j) + at(i - 1, j)) / (celldiagonal[0] * celldiagonal[0]);
    }

    // Divergence along y axis
    if (j == 0)
    {
        laplacian += (at(i, j) - 2.0 * at(i, j + 1) + at(i, j + 2)) / (celldiagonal[1] * celldiagonal[1]);
    }
    else if (j == ny - 1)
    {
        laplacian += (at(i, j) - 2.0 * at(i, j - 1) + at(i, j - 2)) / (celldiagonal[1] * celldiagonal[1]);
    }
    else
    {
        laplacian += (at(i, j + 1) - 2.0 * at(i, j) + at(i, j - 1)) / (celldiagonal[1] * celldiagonal[1]);
    }

    return laplacian;
}

/*!
\brief Compute the %Laplacian field.

Returns a discrete approximation of Laplace's differential operator applied to the field.
The implementation is equivalent to the 4*del2(U) Matlab function, except on the edges of the field.

\sa Laplacian(int, int)
\author Mathieu Gaillard
*/
ScalarField2 ScalarField2::Laplacian() const
{
    ScalarField2 L(GetBox(), nx, ny);

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < ny; j++)
        {
            L(i, j) = Laplacian(i, j);
        }
    }

    return L;
}

/*!
\brief Compute the gradient norm scalar field.
*/
ScalarField2 ScalarField2::GradientNorm() const
{
    // Scalar field of the same size
    ScalarField2 n(GetBox(), nx, ny);

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < ny; j++)
        {
            n(i, j) = Norm(Gradient(i, j));
        }
    }
    return n;
}

/*!
\brief Compute the Lipschitz constant of the elevation function.

Note that this is equivalent to the following code, with the
difference that this function does not store the norm of the gradient in memory.
\code
ScalarField2 n=f.GradientNorm();
double k=0.0;
for (int i=0;i<f.VertexSize();i++)
{
k=Math::Max(k,f.at(i));
}
\endcode
*/
double ScalarField2::K() const
{
    double k = 0.0;

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < ny; j++)
        {
            k = Math::Max(k, Norm(Gradient(i, j)));
        }
    }
    return k;
}

/*!
\brief Normalize the values of a scalar field to unit interval.

Compute range and apply the affine transformation to map values to [0,1].

\sa Unitize()
\avx
*/
void ScalarField2::Normalize()
{
    const int size = nx * ny;
    const int offset = size % 4;

    // Escape
    if (size == 0)
        return;

    double a, b;
    GetRange(a, b);
    if (a == b)
    {
        field.fill(1.0);
        return;
    }
#ifdef _MSC_VER
    if (System::Avx())
    {
        double* p = field.data();

        __m256d avxiba = _mm256_set1_pd(1.0 / (b - a));
        __m256d avxa = _mm256_set1_pd(a);

        for (int i = 0; i < size / 4; i++)
        {
            __m256d avx = _mm256_load_pd(p + i * 4);
            // Could use _mm256_fmadd_ps
            avx = _mm256_sub_pd(avx, avxa);
            avx = _mm256_mul_pd(avx, avxiba);
            _mm256_store_pd(p + i * 4, avx);
        }

        for (int i = size - offset; i < size; i++)
        {
            field[i] = (field[i] - a) / (b - a);
        }
    }
    else
#endif
    {
        for (int i = 0; i < field.size(); i++)
        {
            field[i] = (field[i] - a) / (b - a);
        }
    }
}

/*!
\brief Linearly scale the values of a scalar field to unit interval.

This function differs from ScalarField2::Normalize() which maps values to [0,1]. Instead,
we divide scalard field values by the maximum of the absolute value of the scalar field,
which maps values to [-1,1] and preserves 0.

\sa Normalize()

\avx
*/
void ScalarField2::Unitize()
{
    double a, b;
    GetRange(a, b);
    if (a == b)
    {
        field.fill(1.0);
        return;
    }

    const double ab = 1.0 / Math::Max(fabs(a), fabs(b));

#ifdef _MSC_VER
    if (System::Avx())
    {
        double* p = field.data();

        const int size = nx * ny;
        const int offset = size % 4;

        __m256d avxiba = _mm256_set1_pd(ab);

        for (int i = 0; i < size / 4; i++)
        {
            __m256d avx = _mm256_load_pd(p + i * 4);
            // Could use _mm256_fmadd_ps
            avx = _mm256_mul_pd(avx, avxiba);
            _mm256_store_pd(p + i * 4, avx);
        }

        for (int i = size - offset; i < size; i++)
        {
            field[i] = field[i] * ab;
        }
    }
    else
#endif
    {
        for (int i = 0; i < field.size(); i++)
        {
            field[i] = field[i] * ab;
        }
    }
}

/*!
\brief Clamp the values of a scalar field.
\param a,b Interval.
*/
void ScalarField2::Clamp(const double& a, const double& b)
{
    const int size = nx * ny;

    for (int i = 0; i < size; i++)
    {
        field[i] = Math::Clamp(field.at(i), a, b);
    }
}

#include <math.h>

/*!
\brief Compute the arctangent of the values of a scalar field, may be usefull for converting slope to angles.

\avx
*/
void ScalarField2::Atan()
{
    // not supported by gcc
#ifdef _MSC_VER
    if (System::Avx())
    {
        double* p = field.data();

        const int size = nx * ny;
        const int offset = size % 4;

        for (int i = 0; i < size / 4; i++)
        {
            __m256d avx = _mm256_load_pd(p + i * 4);
            // Could use _mm256_fmadd_ps
            avx = _mm256_atan_pd(avx);
            _mm256_store_pd(p + i * 4, avx);
        }

        for (int i = size - offset; i < size; i++)
        {
            field[i] = atan(field[i]);
        }
    }
    else
#endif
    {

        for (int i = 0; i < field.size(); i++)
        {
            field[i] = atan(field.at(i));
        }
    }
}

/*!
\brief Add a constant to the values the scalar field.
\param c Constant.
*/
ScalarField2& ScalarField2::operator+=(const double& c)
{
    const int size = nx * ny;

    for (int i = 0; i < size; i++)
    {
        field[i] += c;
    }
    return *this;
}

/*!
\brief Scale the values of a scalar field.

\param s Scaling factor.
*/
ScalarField2& ScalarField2::operator*=(const double& s)
{
    const int size = nx * ny;

    for (int i = 0; i < size; i++)
    {
        field[i] *= s;
    }
    return *this;
}

/*!
\brief Addition.
\param s Scalar field.
\avx
*/
ScalarField2& ScalarField2::operator+=(const ScalarField2& s)
{
#ifdef _MSC_VER
    if (System::Avx())
    {
        const double* sp = s.field.data();
        double* p = field.data();

        const int size = nx * ny;
        const int offset = size % 4;
        for (int i = 0; i < size / 4; i++)
        {
            __m256d avx = _mm256_load_pd(p + i * 4);
            __m256d savx = _mm256_load_pd(sp + i * 4);
            __m256d addavx = _mm256_add_pd(avx, savx);
            _mm256_store_pd(p + i * 4, addavx);
        }

        for (int i = size - offset; i < size; i++)
        {
            field[i] += s.field.at(i);
        }
    }
    else
#endif
    {
        int n = field.size();
        double* fieldp = field.data();
        const double* sfieldp = s.field.data();

        for (int i = 0; i < n; i++)
        {
            fieldp[i] += sfieldp[i];
        }
    }
    return *this;
}

/*!
\brief Inverse.

\param a Scalar field.
\param x Real.
*/
ScalarField2 operator/(const double& x, const ScalarField2& a)
{
    ScalarField2 r = a;

    const int size = a.field.size();

    for (int i = 0; i < size; i++)
    {
        r[i] = x / r[i];
    }
    return r;
}

/*!
\brief Perform a linear step over the values of a scalar field.

The function Linear::Step() is applied to all the values of the field.
\sa Linear::Step()
\sa ScalarField2::Normalize()

\param a,b Interval.
*/
void ScalarField2::Step(const double& a, const double& b)
{
    // The step could use _mm256_fmadd_pd with field.at(i), 1.0/(b-a) and -a/(b-a)
    // or _mm256_fadd_pd with -a and then _mm256_fmull_pd with 1.0/(b-a)
    // Finally, clamp to [0,1] by using _mm256_max_pd and _mm256_min_pd

    const int size = nx * ny;

    for (int i = 0; i < size; i++)
    {
        field[i] = Linear::Step(field.at(i), a, b);
    }
}

/*!
\brief Compute the absolute value of a scalar field.
*/
void ScalarField2::Abs()
{
    const int size = nx * ny;

    for (int i = 0; i < size; i++)
    {
        field[i] = fabs(field[i]);
    }
}

/*!
\brief Translate the domain of the scalar field.

\param t Translation vector.
*/
void ScalarField2::Translate(const Vector2& t)
{
    Box2::Translate(t);
}

/*!
\brief Scale the domain of the scalar field.

\param s Scaling factor.
*/
void ScalarField2::Scale(const Vector2& s)
{
    Array2::Scale(s);
}

/*!
\brief Scale the scalar field.

This function also scales the values, it is different from ScalarField2::Scale(const Vector2&);

\param s Scaling factor.
*/
void ScalarField2::Scale(const double& s)
{
    Array2::Scale(s);

    const int size = nx * ny;

    for (int i = 0; i < size; i++)
    {
        field[i] *= s;
    }
}

/*!
\brief Create the set of scalar points.
*/
QVector<ScalarPoint2> ScalarField2::GetScalarPoints() const
{
    // Create array
    QVector<ScalarPoint2> e(nx * ny);

    int k = 0;
    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < ny; j++)
        {
            e[k++] = ScalarPoint2(QPoint(i, j), at(i, j));
        }
    }
    return e;
}

/*!
\brief Compute and return an array of 4 byte floats.
*/
FloatArray ScalarField2::GetAsFloats() const
{
    FloatArray a(nx * ny);
    for (int i = 0; i < nx * ny; i++)
    {
        a[i] = at(i);
    }
    return a;
}
