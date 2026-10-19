// Fields

#include "arrayinteger.h"
#include "linear.h"
/*!
\class Array2I scalarfield.h
\brief A base two-dimensional array of integers.

\ingroup StructureGroup
*/

/*!
\brief Create the field structure.
\param a The array.
\param v Default value of field.
*/
Array2I::Array2I(const Array2& a, int v) :Array2(a)
{
    field.fill(v, nx * ny);
}

/*!
\brief Create the field structure.
\param box The box.
\param x,y Size of the array.
\param v Default value of field.
*/
Array2I::Array2I(const Box2& box, int x, int y, int v) :Array2I(Array2(box, x, y), v)
{}

/*!
\brief Create the field structure.
\param box The box.
\param x,y Size of the array.
\param v Array of scalar values.
*/
Array2I::Array2I(const Box2& box, int x, int y, const QVector<int>& v) :Array2(box, x, y)
{
    field = v;
}

/*!
\brief Create a field structure from an image.
\param box The box.
\param image The image.
\param grayscale Read image as grayscale if set to true, otherwize use color for a better accuracy.
*/
Array2I::Array2I(const Box2& box, const QImage& image, bool grayscale) :Array2(box, image.width(), image.height())
{
    // Set size
    field.resize(nx * ny);

    // Write Heightmap
    for (int i = 0; i < image.width(); i++)
    {
        for (int j = 0; j < image.height(); j++)
        {
            double t;
            QRgb color = image.pixel(i, j);

            // Greyscale
            if (grayscale)
            {
                t = qGray(color);
            }
            // Color 
            else
            {
                t = double(qRed(color) << 16 | qGreen(color) << 8 | qBlue(color));
            }
            field[VertexIndex(i, j)] = t;
        }
    }
}

/*!
\brief Create a field structure from a scalar field.
\param s The scalar field.
\param a,b Integer range, the scalar field will be normalized and scale to interval [a,b].
*/
Array2I::Array2I(const ScalarField2& s, int a, int b) :Array2(s)
{
    double u, v;
    s.GetRange(u, v);
    const int size = nx * ny;
    field.resize(size);

    if (u == v)
    {
        field.fill(a, size);
        return;
    }

    for (int i = 0; i < size; i++)
    {
        field[i] = a + int((b - a) * Linear::Step(s.at(i), u, v));
    }
}

/*!
\brief Create a field structure from a scalar field.
\param s The scalar field.
*/
Array2I::Array2I(const ScalarField2& s) :Array2(s)
{
    const int size = nx * ny;
    for (int i = 0; i < size; i++)
    {
        field[i] = int(s.at(i));
    }
}

/*!
\brief Empty.
*/
Array2I::~Array2I()
{}

/*!
\brief Return the field gradient at a given array vertex.
\param i,j Integer coordinates of the vertex.
\sa at(int,int)
*/
int Array2I::Value(int i, int j) const
{
    return field.at(VertexIndex(i, j));
}


/*
\brief Cropping.
\param a,b Points defining the cropping region.
*/

/*!
\brief Overloaded.
\param s Stream.
\param a The array.
*/
std::ostream& operator<<(std::ostream& s, const Array2I& a)
{
    Array2 array = a;
    s << array;
    for (int j = a.ny - 1; j > -1; j--)
    {
        for (int i = 0; i < a.nx; i++)
        {
            s << a.at(i, j) << ' ';
        }
        s << std::endl;
    }
    return s;
}
