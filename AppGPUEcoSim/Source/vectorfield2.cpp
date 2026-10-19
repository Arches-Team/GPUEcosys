// Vector fields

#include "vectorfield.h"
#include "triangle.h"

#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsScene>
#include <immintrin.h>


/*!
\class VectorField2 vectorfield.h
\brief A base two-dimensional field of Vector2 values.

\ingroup StructureGroup
*/

/*!
\brief Create the field structure.
\param box The box.
\param x,y Size of the array.
\param v Default value of field.
*/
VectorField2::VectorField2(const Box2& box, int x, int y, const Vector2& v) :Array2(box, x, y)
{
    field.fill(v, nx * ny);
}

/*!
\brief Multiply a vector field by a scalar field.
\param s Scalar field; should have the same resolution as the vector field.
*/
VectorField2& VectorField2::operator*=(const ScalarField2& s)
{
    const int n = nx * ny;
    for (int i = 0; i < n; i++)
    {
        field[i] *= s.at(i);
    }
    return *this;
}


#include "cpu.h"
#include "sampling.h"

#include "sampling.h"

/*!
\brief Return the field vector at a given array vertex.
\param i,j Integer coordinates of the vertex.
\sa at(int,int)
*/
Vector2 VectorField2::Value(int i, int j) const
{
    return field.at(VertexIndex(i, j));
}

/*!
\brief Compute the divergence field.
\sa Divergence(int, int)
\author Mathieu Gaillard
*/
ScalarField2 VectorField2::Divergence() const
{
    ScalarField2 d(GetBox(), nx, ny);

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < ny; j++)
        {
            d(i, j) = Divergence(i, j);
        }
    }

    return d;
}

/*!
\brief Compute the divergence at a given sample.
\sa Divergence()
\author Mathieu Gaillard
\param i,j Integer coordinates of the sample.
*/
double VectorField2::Divergence(int i, int j) const
{
    double div = 0.0;

    // Divergence along x axis
    if (i == 0)
    {
        div += (at(i + 1, j)[0] - at(i, j)[0]) * inversecelldiagonal[0];
    }
    else if (i == nx - 1)
    {
        div += (at(i, j)[0] - at(i - 1, j)[0]) * inversecelldiagonal[0];
    }
    else
    {
        div += (at(i + 1, j)[0] - at(i - 1, j)[0]) * 0.5 * inversecelldiagonal[0];
    }

    // Divergence along y axis
    if (j == 0)
    {
        div += (at(i, j + 1)[1] - at(i, j)[1]) * inversecelldiagonal[1];
    }
    else if (j == ny - 1)
    {
        div += (at(i, j)[1] - at(i, j - 1)[1]) * inversecelldiagonal[1];
    }
    else
    {
        div += (at(i, j + 1)[1] - at(i, j - 1)[1]) * 0.5 * inversecelldiagonal[1];
    }

    return div;
}

/*!
\brief Compute the curl of the vector field.
\sa Curl(int, int)
*/
ScalarField2 VectorField2::Curl() const
{
    ScalarField2 c(GetBox(), nx, ny);

    for (int i = 0; i < nx; i++)
    {
        for (int j = 0; j < ny; j++)
        {
            c(i, j) = Curl(i, j);
        }
    }

    return c;
}

/*!
\brief Compute the curl at a given sample.
\sa Curl()
\param i,j Integer coordinates of the sample.
*/
double VectorField2::Curl(int i, int j) const
{
    double c = 0.0;

    // Divergence along x axis
    if (i == 0)
    {
        c += (at(i + 1, j)[1] - at(i, j)[1]) * inversecelldiagonal[0];
    }
    else if (i == nx - 1)
    {
        c += (at(i, j)[1] - at(i - 1, j)[1]) * inversecelldiagonal[0];
    }
    else
    {
        c += 0.5 * (at(i + 1, j)[1] - at(i - 1, j)[1]) * inversecelldiagonal[0];
    }

    // Divergence along y axis
    if (j == 0)
    {
        c -= (at(i, j + 1)[0] - at(i, j)[0]) * inversecelldiagonal[1];
    }
    else if (j == ny - 1)
    {
        c -= (at(i, j)[0] - at(i, j - 1)[0]) * inversecelldiagonal[1];
    }
    else
    {
        c -= 0.5 * (at(i, j + 1)[0] - at(i, j - 1)[0]) * inversecelldiagonal[1];
    }

    return c;
}

/*!
\brief Get the field value at a given point.

This function relies on a bi-linear interpolation of the vectors.

\sa Math::Bilinear
\param p Point.
*/
Vector2 VectorField2::Value(const Vector2& p) const
{
    double u, v;
    int i, j;
    CellInteger(p, i, j, u, v);

    // Test position
    if (!InsideCellIndex(i, j))
        return Vector2::Null;

    return Vector2::Bilinear(at(i, j), at(i + 1, j), at(i + 1, j + 1), at(i, j + 1), u, v);
}

/*!
\brief Destructive sum of two vector fields.
\param f %Vector field.
*/
VectorField2& VectorField2::operator += (const VectorField2& f)
{
    for (int i = 0; i < field.size(); i++)
    {
        field[i] += f.at(i);
    }
    return *this;
}

/*!
\brief Destructive difference of two vector fields.
\param f %Vector field.
*/
VectorField2& VectorField2::operator -= (const VectorField2& f)
{
    for (int i = 0; i < field.size(); i++)
    {
        field[i] -= f.at(i);
    }
    return *this;
}

