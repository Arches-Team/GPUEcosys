// Box

#include <QtWidgets/QGraphicsScene>

#include "box.h"


/*!
\class Box2 box.h
\brief %Axis aligned box in the plane.

\sa Box
\ingroup PlanarGroup
*/

const double Box2::epsilon = 1.0e-5; //!< Epsilon value used to check intersections and some round off errors.

const Box2 Box2::Infinity(Vector2(-Math::Infinity), Vector2(Math::Infinity)); //!< Huge bounding box, which should enclose any other 

const Box2 Box2::Null(0.0); //!< Null box, equivalent to \code Box2(Vector2(0.0)); \endcode

const Box2 Box2::Unit(Vector2(0.0), Vector2(1.0)); //!< Unit box, defined as \code Box2(Vector2(0.0),Vector2(1.0)); \endcode

const int Box2::edge[8] =
{
    0,1,2,3,0,2,1,3
};


/*!
\brief Compute the bounding box of a set of points.
\param p Set of points (should not be empty).
*/
Box2::Box2(const QVector<Vector2>& p)
{
    a = p.at(0);
    b = p.at(0);
    for (int i = 1; i < p.size(); i++)
    {
        a = Vector2::Min(a, p.at(i));
        b = Vector2::Max(b, p.at(i));
    }
}

/*!
\brief Create a box embedding two boxes.
\param x,y Argument boxes.
*/
Box2::Box2(const Box2& x, const Box2& y)
{
    a = Vector2::Min(x.a, y.a);
    b = Vector2::Max(x.b, y.b);
}

/*!
\brief Create a box from a Qt size.
\param size Size.
*/
Box2::Box2(const QSize& size)
{
    a = Vector2::Null;
    b = Vector2(size.width(), size.height());
}

/*!
\brief Extend the limits of the box by a given distance.

Note that this is the same as performing the Minkowski sum with a cubic box of size r.
\param r Range.
*/
void Box2::Extend(const double& r)
{
    a -= Vector2(r);
    b += Vector2(r);
}

/*!
\brief Extend the limits of the box given a point.

If the point lies inside the box, the vertices of the box are unchanged.
\param p Point.
*/
void Box2::Extend(const Vector2& p)
{
    a = Vector2::Min(a, p);
    b = Vector2::Max(b, p);
}

/*!
\brief Test if a point is inside the box.
\param p Point.
*/
bool Box2::Inside(const Vector2& p) const
{
    if ((p[0] < a[0]) || (p[0] > b[0]) || (p[1] < a[1]) || (p[1] > b[1]))
        return false;
    else
        return true;
}

/*!
\brief Computes the intersection between two boxes.

Note that if the intersection is empty, the resulting box is invalid.

\param x Argument box.
*/
Box2 Box2::Intersection(const Box2& x) const
{
    return Box2(Vector2::Max(a, x.a), Vector2::Min(b, x.b));
}

/*!
\brief Test if a point lies withing a given range of the box.

The following two pieces of code are almost equivalent:
\code
bool a=Box2(1.0).Inside(Vector(1.5),1.0); // Do not modify the box and test range directly
\endcode
In this case we modify the geometry:
\code
Box2 box(1.0);
box.Extend(1.0); // We change the box geometry
// Note that we could have used a Minkowski sum: box=box+Box(1.0);
bool a=box.Inside(Vector(1.5));
\endcode

\param p Point.
\param r Range.
*/
bool Box2::Inside(const Vector2& p, const double& r) const
{
    if (p[0]<a[0] - r || p[0]>b[0] + r || p[1]<a[1] - r || p[1]>b[1] + r)
        return false;
    else
        return true;
}

/*!
\brief Computes the intersection between a box and a ray.

\param ray The ray.
*/
bool Box2::Intersect(const Ray2& ray) const
{
    double ta, tb;

    return Intersect(ray, ta, tb);
}

/*!
\brief Computes the intersection between a box and a ray.

\param ray The ray.
\param tmin, tmax Intersection depths
*/
bool Box2::Intersect(const Ray2& ray, double& tmin, double& tmax) const
{
    tmin = -1e16;
    tmax = 1e16;

    Vector2 p = ray.Origin();
    Vector2 d = ray.Direction();

    double t;
    // Ox
    if (d[0] < -epsilon)
    {
        t = (a[0] - p[0]) / d[0];
        if (t < tmin)
            return false;
        if (t <= tmax)
            tmax = t;
        t = (b[0] - p[0]) / d[0];
        if (t >= tmin)
        {
            if (t > tmax)
                return false;
            tmin = t;
        }
    }
    else if (d[0] > epsilon)
    {
        t = (b[0] - p[0]) / d[0];
        if (t < tmin)
            return false;
        if (t <= tmax)
            tmax = t;
        t = (a[0] - p[0]) / d[0];
        if (t >= tmin)
        {
            if (t > tmax)
                return false;
            tmin = t;
        }
    }
    else if (p[0]<a[0] || p[0]>b[0])
        return false;

    // Oy
    if (d[1] < -epsilon)
    {
        t = (a[1] - p[1]) / d[1];
        if (t < tmin)
            return false;
        if (t <= tmax)
            tmax = t;
        t = (b[1] - p[1]) / d[1];
        if (t >= tmin)
        {
            if (t > tmax)
                return false;
            tmin = t;
        }
    }
    else if (d[1] > epsilon)
    {
        t = (b[1] - p[1]) / d[1];
        if (t < tmin)
            return false;
        if (t <= tmax)
            tmax = t;
        t = (a[1] - p[1]) / d[1];
        if (t >= tmin)
        {
            if (t > tmax)
                return false;
            tmin = t;
        }
    }
    else if (p[1]<a[1] || p[1]>b[1])
        return false;

    return true;
}


/*!
\brief Convert a planar Box2 to a Box.
\param a,b Lower and upper coordinates (note that a should be smaller than b).
*/
Box Box2::ToBox(const double& a, const double& b) const
{
    return Box(Box2::a.ToVector(a), Box2::b.ToVector(b));
}

/*!
\brief Overloaded.
\param s Stream.
\param box The box.
*/
std::ostream& operator<<(std::ostream& s, const Box2& box)
{
    s << "Box2(" << box.a << ',' << box.b << ")";
    return s;
}

/*!
\brief Compute the coordinates of a grid aligned point.

This function computes the coordinates of a point inside the box as if the box was decomposed into a regular grid.

\param i,j Integer coordinates.
\param x,y Virtual grid size.
*/
Vector2 Box2::Vertex(int i, int j, int x, int y) const
{
    return Vector2(a[0] + i * (b[0] - a[0]) / (x - 1), a[1] + j * (b[1] - a[1]) / (y - 1));
}

/*!
\brief Translates a box.

\param t Translation vector.
*/
void Box2::Translate(const Vector2& t)
{
    a += t;
    b += t;
}

/*!
\brief Scales a box.

Note that this function handles negative coefficients in
the scaling vector.
\param s Scaling vector.
*/
void Box2::Scale(const Vector2& s)
{
    a *= s;
    b *= s;
    // Swap coordinates for negative coefficients 
    for (int i = 0; i < 2; i++)
    {
        if (s[i] < 0.0)
        {
            Math::Swap(a[i], b[i]);
        }
    }
}

/*!
\brief Scales a box.

Note that this function handles negative coefficients.
\param s Scaling factor.
*/
void Box2::Scale(const double& s)
{
    Box2::Scale(Vector2(s));
}

/*!
\brief Scales a box and return the scaled box.

\param s Scaling factor.
\sa Box2::Scaled(const Vector2&)
*/
Box2 Box2::Scaled(const double& s) const
{
    if (s > 0.0)
    {
        return Box2(a * s, b * s);
    }
    else
    {
        return Box2(b * s, a * s);
    }
}

/*!
\brief Scales a box and return the scaled box.

\param s Scaling vector.
\sa Box2::Scale
*/
Box2 Box2::Scaled(const Vector2& s) const
{
    Box2 box(a.Scaled(s), b.Scaled(s));

    // Swap coordinates for negative coefficients 
    for (int i = 0; i < 2; i++)
    {
        if (s[i] < 0.0)
        {
            Math::Swap(box[0][i], box[1][i]);
        }
    }
    return box;
}

/*!
\brief Scales a box according to a Qt size.

This function preserves the width of the box, and scales its height according to the ratio.
\param s Size.
\sa Box2::Scaled
*/
Box2 Box2::Scaled(const QSize& s) const
{
    double r = s.height() / double(s.width());
    return Box2(Vector2(a[0], r * a[1]), Vector2(b[0], r * b[1]));
}

/*!
\brief Draw a rectangle.
\param scene Graphics scene.
\param pen The pen.
\param brush The brush, should the box be filled.
*/
void Box2::Draw(QGraphicsScene& scene, const QPen& pen, const QBrush& brush) const
{
    scene.addRect(a[0], a[1], b[0] - a[0], b[1] - a[1], pen, brush);
}
