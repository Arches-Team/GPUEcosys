// Circle

#include <QtWidgets/QGraphicsScene>

#include "circle.h"

/*!
\class Circle2 circle.h
\brief Circles in the plane.

\sa Circle
\ingroup PlanarGroup
*/

const Circle2 Circle2::Unit(1.0);
const Circle2 Circle2::Infinite(Math::Infinity);

/*!
\brief Create a circle.
\param c Center.
\param r Radius.
*/
Circle2::Circle2(const Vector2& c, const double& r) :c(c), r(r)
{}

/*!
\brief Create a circle centered at origin.
\param r Radius.
*/
Circle2::Circle2(const double& r) :Circle2(Vector2::Null, r)
{}

/*!
\brief Create a circle embedding two circles.
\param ca, cb Circles.
*/
Circle2::Circle2(const Circle2& ca, const Circle2& cb)
{
    Vector2 ab = cb.c - ca.c;
    double d = Norm(ab);

    // Case 1: one circle contains the other
    if (d + Math::Min(ca.r, cb.r) <= Math::Max(ca.r, cb.r))
    {
        if (ca.r >= cb.r)
            *this = ca;
        else
            *this = cb;
    }
    // In this case, d should not be 0 as this case should have been captured in the previous if statement
    else
    {
        r = (d + ca.r + cb.r) * 0.5;
        c = (ca.c + cb.c) * 0.5 + 0.5 * (cb.r - ca.r) * ab / d;
    }
}

/*!
\brief Create the circle passing through 3 points.

This is the circumscribed circle of the triangle.

\param a, b, c Points.
*/
Circle2::Circle2(const Vector2& a, const Vector2& b, const Vector2& c)
{
    Vector2 ab = b - a;
    Vector2 ac = c - a;
    Vector2 bc = c - b;

    double E = ab[0] * (a[0] + b[0]) + ab[1] * (a[1] + b[1]);
    double F = ac[0] * (a[0] + c[0]) + ac[1] * (a[1] + c[1]);

    double G = 2.0 * (ab[0] * bc[1] - ab[1] * bc[0]);

    double x = (ac[1] * E - ab[1] * F) / G;
    double y = (ab[0] * F - ac[0] * E) / G;

    Circle2::c = Vector2(x, y);
    Circle2::r = Norm(a - Circle2::c);
}

/*!
\brief Vertex on the circle.
\param a Angle.
*/
Vector2 Circle2::Vertex(const double& a) const
{
    return c + r * Vector2::Polar(a);
}

/*!
\brief Draw a circle.
\param scene Graphics scene.
\param pen The pen.
\param brush The brush, should the circle be filled.
*/
void Circle2::Draw(QGraphicsScene& scene, const QPen& pen, const QBrush& brush) const
{
    scene.addEllipse(c[0] - r, c[1] - r, r + r, r + r, pen, brush);
}

/*!
\brief Compute a random vertex inside a unit disc.
\param random %Random number generator.
*/
Vector2 Circle2::RandomUnit(Random& random)
{
    // Unit radius
    double u;

    // Random samples
    double rx, ry;
    do
    {
        rx = 2.0 * random.Uniform() - 1.0;
        ry = 2.0 * random.Uniform() - 1.0;
        u = rx * rx + ry * ry;
    } while (u >= 1.0);

    return Vector2(rx, ry);
}

/*!
\brief Generate a random vector on the circle.
\param random %Random number generator.
*/
Vector2 Circle2::RandomOn(Random& random) const
{
    Vector2 uv = RandomUnit(random);

    double s = SquaredNorm(uv);

    double rx = (uv[0] * uv[0] - uv[1] * uv[1]) / s;
    double ry = 2.0 * uv[0] * uv[1] / s;

    return c + r * Vector2(rx, ry);
}

/*!
\brief Creates a circle enclosing a set of points.

The algorithm computes a fast approximation of the bounding ball for a point set based on the algorithm given by [Jack Ritter, 1990].

\param p Array of vertices.
\param n Number of vertices.
*/
Circle2::Circle2(Vector2* p, int n)
{
    Box2 box = Box2(p[0]);

    int Pxmin = 0;
    int Pxmax = 0;
    int Pymin = 0;
    int Pymax = 0;

    for (int i = 1; i < n; i++)
    {
        if (p[i][0] < box[0][0])
        {
            box[0][0] = p[i][1];
            Pxmin = i;
        }
        else if (p[i][0] > box[1][0])
        {
            box[1][0] = p[i][1];
            Pxmax = i;
        }
        if (p[i][1] < box[0][1])
        {
            box[0][1] = p[i][2];
            Pymin = i;
        }
        else if (p[i][1] > box[1][1])
        {
            box[1][1] = p[i][2];
            Pymax = i;
        }
    }

    // Select the largest extent as an initial diameter for the ball
    Vector2 x = p[Pxmax] - p[Pxmin];
    Vector2 y = p[Pymax] - p[Pymin];

    // Direction is largest extent
    if (x * x >= y * y)
    {
        c = (p[Pxmin] + p[Pxmax]) * 0.5;
        r = x * x * 0.25;
    }
    else
    {
        c = (p[Pymin] + p[Pymax]) * 0.5;
        r = y * y * 0.25;
    }

    // Check that all points p[i] are in the circle
    // If not, expand the circle just enough to include them
    for (int j = 0; j < n; j++)
    {
        Vector2 cp = p[j] - c;
        double s = cp * cp;
        // p[i] is outside the circle, so expand to include it
        if (s > r)
        {
            // Compute radii
            s = sqrt(s);
            r = sqrt(r);

            // Shift center towards p[i]
            c += (0.5 * (s - r) / s) * cp;

            // Enlarge radius just enough
            r = 0.5 * (r + s);

            // Restore squared radius
            r *= r;
        }
    }

    // Radius
    r = sqrt(r);
}

/*
\brief Creates a circle enclosing a set of points.

The algorithm computes a fast approximation of the bounding ball for a point set based on the algorithm given by [Jack Ritter, 1990].

\param p Array of vertices.
\param n Number of vertices.
*/
Circle2::Circle2(const QVector<Vector2>& s)
{
    Vector2 a = s.at(0);
    Vector2 b = a;

    int Pxmin = 0;
    int Pxmax = 0;
    int Pymin = 0;
    int Pymax = 0;

    // Compute largest extent along major axes
    for (int i = 1; i < s.size(); i++)
    {
        const Vector2& q = s.at(i);
        if (q[0] < a[0])
        {
            a[0] = q[0];
            Pxmin = i;
        }
        else if (q[0] > b[0])
        {
            b[0] = q[0];
            Pxmax = i;
        }
        if (q[1] < a[1])
        {
            a[1] = q[1];
            Pymin = i;
        }
        else if (q[1] > b[1])
        {
            b[1] = q[1];
            Pymax = i;
        }
    }

    // Select the largest extent as an initial diameter for the ball
    Vector2 x = s.at(Pxmax) - s.at(Pxmin);
    Vector2 y = s.at(Pymax) - s.at(Pymin);

    // Compute center according to the largest extent
    if (x * x >= y * y)
    {
        c = (s.at(Pxmin) + s.at(Pxmax)) * 0.5;
        r = x * x * 0.25;
    }
    else
    {
        c = (s.at(Pymin) + s.at(Pymax)) * 0.5;
        r = y * y * 0.25;
    }

    // Check that all points p[i] are in the circle, if not, expand the circle just enough to include them
    for (int j = 0; j < s.size(); j++)
    {
        Vector2 cp = s.at(j) - c;
        double ncp = cp * cp;
        // p[i] is outside the circle, so expand to include it
        if (ncp > r)
        {
            // Compute radii
            ncp = sqrt(ncp);
            r = sqrt(r);

            // Shift center towards p[i]
            c += (0.5 * (ncp - r) / ncp) * cp;

            // Enlarge radius just enough
            r = 0.5 * (r + ncp);

            // Restore squared radius
            r *= r;
        }
    }

    // Radius
    r = sqrt(r);
}

/*!
\brief Overloaded.
\param s Stream.
\param circle The circle.
*/
std::ostream& operator<<(std::ostream& s, const Circle2& circle)
{
    s << "Circle2(" << circle.c << ',' << circle.r << ')';
    return s;
}
