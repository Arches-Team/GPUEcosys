// Arrays

#include <QtGui/QPainter>

#include "array.h"
#include "triangle.h"

/*!
\class Array2 array.h
\brief A base two-dimensional array structure.

\image html array.png

An array is a two-dimensional rectangle virtually decomposed into cells.

\ingroup StructureGroup
*/

const QPoint Array2::next[8] = { QPoint(1, 0), QPoint(1, 1), QPoint(0, 1), QPoint(-1, 1), QPoint(-1, 0), QPoint(-1, -1), QPoint(0, -1), QPoint(1, -1) };
const double Array2::length[8] = { 1.0, sqrt(2.0), 1.0, sqrt(2.0), 1.0, sqrt(2.0), 1.0, sqrt(2.0) };
const double Array2::inverselength[8] = { 1.0, 1.0 / sqrt(2.0), 1.0, 1.0 / sqrt(2.0), 1.0, 1.0 / sqrt(2.0), 1.0, 1.0 / sqrt(2.0) };

/*!
\brief Empty array, with empty box.
*/
Array2::Array2() :Box2(Box2::Null), nx(0), ny(0), celldiagonal(Vector2::Null), inversecelldiagonal(Vector2::Null)
{
}

/*!
\brief Create the lattice structure.
\param box The box.
\param x,y Size of the lattice.
*/
Array2::Array2(const Box2& box, int x, int y) :Box2(box), nx(x), ny(y), celldiagonal((b[0] - a[0]) / (nx - 1), (b[1] - a[1]) / (ny - 1))
{
    inversecelldiagonal = celldiagonal.Inverse();
}

/*!
\brief Create the lattice structure.
\param box The box.
\param n Size of the lattice.
*/
Array2::Array2(const Box2& box, int n) :Array2(box, n, n)
{
}

/*!
\brief Create the lattice structure.
\param a The lattice.
*/
Array2::Array2(const Array& a) :Array2(Box2(a.GetBox()), a.GetSizeX(), a.GetSizeY())
{
}

/*!
\brief Scales the lattice structure.

\param s Scaling vector.
*/
void Array2::Scale(const Vector2& s)
{
    Box2::Scale(s);
    celldiagonal = Vector2((b[0] - a[0]) / (nx - 1), (b[1] - a[1]) / (ny - 1));
    inversecelldiagonal = celldiagonal.Inverse();
}

/*!
\brief Scales the array structure.

\param s Scaling factor.
*/
void Array2::Scale(const double& s)
{
    Box2::Scale(Vector2(s));
    celldiagonal = Vector2((b[0] - a[0]) / (nx - 1), (b[1] - a[1]) / (ny - 1));
    inversecelldiagonal = celldiagonal.Inverse();
}


/*!
\brief Compute the k-th point on the boundary of the domain.

\image html borders.png

\param k Integer index.
*/
QPoint Array2::VertexBorder(int k) const
{
    int x;
    int y;
    if (k < 2 * nx)
    {
        if (k < nx)
        {
            x = k;
            y = 0;
        }
        else
        {
            x = k - nx;
            y = ny - 1;
        }
    }
    else
    {
        k -= 2 * nx;
        if (k < ny - 2)
        {
            x = 0;
            y = k + 1;
        }
        else
        {
            x = nx - 1;
            y = k + 1 - (ny - 2);
        }
    }

    return QPoint(x, y);
}


/*!
\brief Compute the index of a point on the boundary of the domain.
\param i,j Coordinates.
*/
int Array2::VertexBorderIndex(int i, int j) const
{
    if (j == 0)
    {
        return i;
    }
    else if (j == ny - 1)
    {
        return i + nx;
    }
    else
    {
        // Already processed corners in previous if statements
        if (i == 0)
        {
            return nx * 2 + j - 1;
        }
        else
        {
            return nx * 2 + ny - 2 + j - 1;
        }
    }
}

/*!
\brief Return the geometry of the cell.
\param c Integer cell index.
*/
Box2 Array2::Cell(int c) const
{
    int i, j;
    InverseCellIndex(c, i, j);
    return Cell(i, j);
}

/*!
\brief Return the geometry of the cell.
\param i,j Integer coordinates of the cell.
*/
Box2 Array2::Cell(int i, int j) const
{
    Vector2 e = ArrayVertex(i, j);
    return Box2(e, e + celldiagonal);
}

/*!
\brief Return the center of the cell.
\param i,j Integer coordinates of the cell.
*/
Vector2 Array2::CellCenter(int i, int j) const
{
    return  a + celldiagonal.Scaled(Vector2(i, j)) + 0.5 * celldiagonal;
}

/*!
\brief Overloaded.
\param s Stream.
\param a The array.
*/
std::ostream& operator<<(std::ostream& s, const Array2& a)
{
    s << "Array2(" << a.a << ',' << a.b << "," << a.nx << ',' << a.ny << ')';
    return s;
}


/*!
\brief Return the cell diagonal vector.
*/
Vector2 Array2::CellDiagonal() const
{
    return celldiagonal;
}

/*!
\brief Return the area of a cell.

This is convenience function, which is the same as the following code, but more efficient:
\code
Array2 array;
double a=array.UnitCell().Area();
\endcode
*/
double Array2::CellArea() const
{
    return Box2::Area() / ((nx - 1) * (ny - 1));
}

/*!
\brief Compute the integer coordinates of the vertices embedding a cell with prescribed integer radius.

The coordinates are clamped to the size of the array.
\param i,j Cell.
\param r %Radius.
*/
QRect Array2::VertexIntegerArea(int i, int j, int r) const
{
    QRect area(i - r, j - r, 2 * r + 1, 2 * r + 1);

    // Limit to domain
    QRect mask(0, 0, nx - 1, ny - 1);
    return area.intersected(mask);
}

/*!
\brief Compute the integer coordinates of the vertices embedding a box.

The coordinates are clamped to the size of the array.
\param box %Box.
*/
QRect Array2::VertexIntegerArea(const Box2& box) const
{
    QPoint pa = VertexInteger(box[0]);
    QPoint pb = VertexInteger(box[1]);

    // Rectangle
    QRect area(pa.x(), pa.y(), pb.x() - pa.x() + 1, pb.y() - pa.y() + 1);

    // Limit to domain
    QRect mask(0, 0, nx - 1, ny - 1);
    return area.intersected(mask);
}

/*!
\brief Compute the integer coordinates of the vertices embedding a circle.

The coordinates are clamped to the size of the array.
\param circle %Circle.
*/
QRect Array2::VertexIntegerArea(const Circle2& circle) const
{
    return VertexIntegerArea(circle.GetBox());
}

/*!
\brief Compute the integer coordinates span of the vertices embedding a rectangle.

\param rect %Rectangle.
*/
QRect Array2::VertexIntegerArea(const QRect& rect) const
{
    QRect mask = AreaInteger();
    return rect.intersected(mask);
}

/*!
\brief Convert an 8 bit integer code direction into a direction vector.
\param c Direction code.
*/
QPoint Array2::CodeToDir(int c)
{
    int a = 0;
    int b = 0;

    if (c == 1) { a = 1; b = 0; }
    if (c == 2) { a = 1; b = -1; }
    if (c == 4) { a = 0; b = -1; }
    if (c == 8) { a = -1; b = -1; }
    if (c == 16) { a = -1; b = 0; }
    if (c == 32) { a = -1; b = 1; }
    if (c == 64) { a = 0; b = 1; }
    if (c == 128) { a = 1; b = 1; }
    return QPoint(a, b);
}
