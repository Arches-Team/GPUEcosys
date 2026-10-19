// Fundamentals
#pragma once

#include <functional>

#include <QtGui/QImage>

#include "array.h"

class ScalarPoint2
{
protected:
    QPoint p; //!< Point.
    double z; //!< Elevation
public:
    ScalarPoint2() :p(0, 0), z(0.0) {}
    explicit ScalarPoint2(const QPoint&, const double&);
    friend bool operator<(const ScalarPoint2& a, const ScalarPoint2& b) { return a.z < b.z; }

    // Access to members
    QPoint Point() const;
};

/*!
\class ScalarPoint2
\brief Internal class for sorting points.
*/

/*!
\brief Create a scalar point.
\param p Point.
\param s Elevation.
*/
inline ScalarPoint2::ScalarPoint2(const QPoint& p, const double& s) :p(p), z(s)
{}

/*!
\brief Get point.
*/
inline QPoint ScalarPoint2::Point() const
{
    return p;
}

#include "palette.h"

class FloatArray
{
protected:
    QVector<float> a; //!< %Array.
public:
    FloatArray(int);
    float& operator[](int);
};

inline FloatArray::FloatArray(int n)
{
    a.resize(n);
}

/*!
\brief Access to element.
*/
inline float& FloatArray::operator[](int i)
{
    return a[i];
}

class Histogram;
class Array2I;

class ScalarField2 :public Array2
{
protected:
    QVector<double> field; //!< Field samples.
public:
    //! Empty.
    ScalarField2() {}
    ScalarField2(const Array2&, const double& = 0.0);
    explicit ScalarField2(const Box2&, int, int, const double& = 0.0);
    explicit ScalarField2(const Box2&, int, int, const QVector<double>&);
    explicit ScalarField2(const Box2&, const QImage&, const double& = 0.0, const double& = 256.0 * 256.0 - 1.0, bool = true);
    explicit ScalarField2(const QImage&, const double& = 1.0, const double& = 1.0, bool = true);
    explicit ScalarField2(const QString&, double = 0.0, double = 1.0);
    explicit ScalarField2(const Array2I&);
    explicit ScalarField2(const Array2I&, const std::vector<double>&);
    virtual ~ScalarField2();

    Array2 GetArray() const;

    void GetRange(double&, double&) const;

    Histogram GetHistogram(int) const;
    Histogram GetHistogram(int, double, double) const;

    virtual Vector2 Gradient(int, int) const;
    virtual double Value(int, int) const;
    virtual double K() const;

    // Access to elements
    double at(int, int) const;
    double at(const QPoint&) const;
    double& operator()(int, int);
    double& operator()(const QPoint&);

    double at(int) const;
    double& operator[](int);

    // Create image from the scalar field
    QImage CreateImage(bool = true) const;

    QImage CreateImage(const AnalyticPalette&, bool = false, bool = false) const;
    QImage CreateImage(const double&, const double&, const AnalyticPalette&, bool = false) const;

    QImage CreateImage(const double&, const double&, bool = true) const;

    ScalarField2 Resized(int, int) const;
    ScalarField2 Sample(const Box2&, int, int) const;

    ScalarField2 Crop(const QPoint&, const QPoint&) const;

    ScalarField2 DownSample(int) const;

    virtual double Value(const Vector2&) const;

    void Fill(const double&);

    void Translate(const Vector2&);
    void Scale(const Vector2&);
    void Scale(const double&);

    // Functions
    void Normalize();
    void Unitize();

    void Clamp(const double&, const double&);

    ScalarField2& operator*= (const double&);

    ScalarField2& operator+=(const ScalarField2&);
    ScalarField2& operator+=(const double&);

    friend ScalarField2 operator+(const ScalarField2&, const ScalarField2&);

    friend ScalarField2 operator*(const ScalarField2&, const double&);
    friend ScalarField2 operator*(const double&, const ScalarField2&);

    friend ScalarField2 operator/(const double&, const ScalarField2&);

    void Atan();
    void Step(const double&, const double&);
    void Abs();

    // Gradient
    ScalarField2 GradientNorm() const;

    // Laplacian
    virtual ScalarField2 Laplacian() const;
    virtual double Laplacian(int, int) const;

    QVector<ScalarPoint2> GetScalarPoints() const;
    FloatArray GetAsFloats() const;
};

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline double ScalarField2::at(int i, int j) const
{
    return field.at(VertexIndex(i, j));
}

/*!
\brief Return the field value at a given array vertex.
\param q Point.
*/
inline double ScalarField2::at(const QPoint& q) const
{
    return field.at(VertexIndex(q.x(), q.y()));
}

/*!
\brief Return the field value at a given array vertex.
\param q Point.
*/
inline double& ScalarField2::operator()(const QPoint& q)
{
    return field[VertexIndex(q.x(), q.y())];
}

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline double& ScalarField2::operator()(int i, int j)
{
    return field[VertexIndex(i, j)];
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline double ScalarField2::at(int c) const
{
    return field.at(c);
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline double& ScalarField2::operator[](int c)
{
    return field[c];
}

/*!
\brief Addition.

\param a,b Scalar fields.
*/
inline ScalarField2 operator+(const ScalarField2& a, const ScalarField2& b)
{
    ScalarField2 r = a;
    r += b;
    return r;
}

/*!
\brief Multiplication.

\param a Scalar field.
\param x Real.
*/
inline ScalarField2 operator*(const ScalarField2& a, const double& x)
{
    ScalarField2 r = a;
    r *= x;
    return r;
}

/*!
\brief Multiplication.

\param a Scalar field.
\param x Real.
*/
inline ScalarField2 operator*(const double& x, const ScalarField2& a)
{
    ScalarField2 r = a;
    r *= x;
    return r;
}
