// Vector  

// Self include
#include "evector.h"

/*!
\class Vector2 evector.h
\brief Vectors in two dimensions.

This class implements most operators and member functions as for Vector class.

\ingroup MathGroup
*/

const Vector2 Vector2::Null = Vector2(0.0, 0.0);

const Vector2 Vector2::X = Vector2(1.0, 0.0);
const Vector2 Vector2::Y = Vector2(0.0, 1.0);

/*!
\brief Normalize a two dimensional vector.

It computes the inverse of its norm and scaling the components.
This function does not check if the vector is null, which might yield errors.

\param u %Vector.
*/
void Normalize(Vector2& u)
{
    u *= 1.0 / Norm(u);
}

/*!
\brief Returns the sine of two vectors.

Computes the cross product of the vectors and normalizes the result.
\param u, v Vectors.
*/
double Sine(const Vector2& u, const Vector2& v)
{
    return (u[0] * v[1] - u[1] * v[0]) / sqrt((u * u) * (v * v));
}

/*!
\brief Returns the positive cosine of two vectors.

Computes the dot product of the normalized vectors.
\param u, v Vectors.
*/
double Cosine(const Vector2& u, const Vector2& v)
{
    return (u * v) / sqrt((u * u) * (v * v));
}

/*!
\brief Swap two vectors.
\param a, b Vectors.
*/
void Swap(Vector2& a, Vector2& b)
{
    Vector2 t = a;
    a = b;
    b = t;
}

/*!
\brief Overloaded output-stream operator.
\param u %Vector.
\param s Stream.
*/
std::ostream& operator<<(std::ostream& s, const Vector2& u)
{
    s << "Vector2(" << u.c[0] << ',' << u.c[1] << ')';
    return s;
}

#include <QtCore/QStringList>

