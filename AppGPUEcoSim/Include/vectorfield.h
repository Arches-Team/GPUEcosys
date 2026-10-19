// Fundamentals

#pragma once

#include <QtGui/QImage>

#include "array.h"
#include "scalarfield.h"

class ScalarField2;

class VectorField2 :public Array2
{
protected:
    QVector<Vector2> field; //!< Field samples.
public:
    VectorField2() {}
    explicit VectorField2(const Box2&, int, int, const Vector2 & = Vector2::Null);
    ~VectorField2() {}

    // Access to elements
    Vector2 at(int, int) const;
    Vector2 at(const QPoint&) const;
    Vector2& operator()(int, int);
    Vector2& operator()(const QPoint&);

    Vector2 at(int) const;
    Vector2& operator[](int);

    VectorField2& operator+= (const VectorField2&);
    VectorField2& operator-= (const VectorField2&);

    VectorField2& operator*=(const ScalarField2&);

    virtual Vector2 Value(const Vector2&) const;
    virtual Vector2 Value(int, int) const;

    // Divergence
    virtual ScalarField2 Divergence() const;
    virtual double Divergence(int, int) const;

    // Curl
    virtual ScalarField2 Curl() const;
    virtual double Curl(int, int) const;
};

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline Vector2 VectorField2::at(int i, int j) const
{
    return field.at(VertexIndex(i, j));
}

/*!
\brief Return the field value at a given array vertex.
\param p Point with its coordinates.
*/
inline Vector2 VectorField2::at(const QPoint& p) const
{
    return field.at(VertexIndex(p.x(), p.y()));
}

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline Vector2& VectorField2::operator()(int i, int j)
{
    return field[VertexIndex(i, j)];
}

/*!
\brief Return the field value at a given array vertex.
\param p Point.
*/
inline Vector2& VectorField2::operator()(const QPoint& p)
{
    return field[VertexIndex(p.x(), p.y())];
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline Vector2 VectorField2::at(int c) const
{
    return field.at(c);
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline Vector2& VectorField2::operator[](int c)
{
    return field[c];
}

class AnalyticVectorField
{
protected:
    static const double epsilon; //!< Epsilon value for partial derivatives
};


class VectorField :public Array
{
protected:
    QVector<Vector> field; //!< Field samples.
public:
    VectorField() {}
    ~VectorField() {}

    // Access to elements
    Vector at(int, int, int) const;
    Vector& operator()(int, int, int);

    Vector at(int) const;
    Vector& operator[](int);
};

/*!
\brief Return the field value at a given array vertex.
\param i,j,k Integer coordinates of the vertex.
*/
inline Vector VectorField::at(int i, int j, int k) const
{
    return field.at(VertexIndex(i, j, k));
}

/*!
\brief Return the field value at a given array vertex.
\param i,j,k Integer coordinates of the vertex.
*/
inline Vector& VectorField::operator()(int i, int j, int k)
{
    return field[VertexIndex(i, j, k)];
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline Vector VectorField::at(int c) const
{
    return field.at(c);
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline Vector& VectorField::operator[](int c)
{
    return field[c];
}
