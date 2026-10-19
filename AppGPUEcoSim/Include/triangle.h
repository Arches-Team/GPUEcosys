// Triangle

#pragma once

#include <QtCore/QVector>

#include "box.h"

// Triangle
class Triangle
{
protected:
    Vector p[3]; //!< %Array of vertices.
public:
    Triangle() {}
    explicit Triangle(const Vector&, const Vector&, const Vector&);
    ~Triangle() {}

    Vector operator[] (int) const;
    Vector& operator[] (int);

    // Geometry
    Vector Normal() const;
    Vector AreaNormal() const;

    Vector Normal(const Vector&) const;

    double Area() const;

    Box GetBox() const;

    // Stream
    friend std::ostream& operator<<(std::ostream&, const Triangle&);

protected:
    static constexpr const double epsilon = 1.0e-7; //!< Internal epsilon constant.
};

/*!
\brief Return the i-th vertex.
\param i Index.
*/
inline Vector Triangle::operator[] (int i) const
{
    return p[i];
}

/*!
\brief Return a reference to the i-th vertex.
\param i Index.
*/
inline Vector& Triangle::operator[] (int i)
{
    return p[i];
}

//! Compute the area of the triangle.
inline double Triangle::Area() const
{
    return 0.5 * Norm((p[0] - p[1]) / (p[2] - p[0]));
}

/*!
\brief Create a triangle.
\param a,b,c Vertices of the triangle.
*/
inline Triangle::Triangle(const Vector& a, const Vector& b, const Vector& c) :p{ a,b,c }
{
}
