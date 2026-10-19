// Circle

#pragma once

#include <QtCore/QVector> 

#include "box.h"

class Circle2
{
protected:
    Vector2 c = Vector2::Null; //!< Center.
    double r = 1.0; //!< Radius.
public:
    //! Empty
    Circle2() {}
    explicit Circle2(const double&);
    explicit Circle2(const Vector2&, const double&);
    explicit Circle2(const Vector2&, const Vector2&, const Vector2&);
    explicit Circle2(Vector2*, int);
    explicit Circle2(const QVector<Vector2>&);
    explicit Circle2(const Circle2&, const Circle2&);

    Vector2 Center() const;
    Vector2 Vertex(const double&) const;
    double Radius() const;

    Box2 GetBox() const;

    Circle2 Translated(const Vector2&) const;
    Circle2 Scaled(const double&) const;

    void Translate(const Vector2&);
    void Scale(const double&);

    double Length() const;

    Vector2 RandomOn(Random & = Random::R239) const;

    static Vector2 RandomUnit(Random & = Random::R239);

    // Stream
    friend std::ostream& operator<<(std::ostream&, const Circle2&);

    // Drawing
    void Draw(QGraphicsScene&, const QPen & = QPen(), const QBrush & = QBrush()) const;
public:
    static const Circle2 Unit; //!< Unit circle.
    static const Circle2 Infinite; //!< Infinite circle.
protected:
    static constexpr const double Epsilon = 1.e-6; //!< %Epslion for intersection tests
};

//! Center of the circle.
inline Vector2 Circle2::Center() const
{
    return c;
}

//! Radius of the circle.
inline double Circle2::Radius() const
{
    return r;
}

/*!
\brief Perimeter of the circle.
*/
inline double Circle2::Length() const
{
    return 2.0 * Math::Pi * r;
}

/*!
\brief Compute the axis aligned bounding box of a circle.
*/
inline Box2 Circle2::GetBox() const
{
    return Box2(c, r);
}

/*!
\brief Translate a circle.
\param t Translation vector.
*/
inline Circle2 Circle2::Translated(const Vector2& t) const
{
    return Circle2(c + t, r);
}

/*!
\brief Return a uniformly scaled circle.
\param s Scaling factor.
*/
inline Circle2 Circle2::Scaled(const double& s) const
{
    return Circle2(c * s, r * s);
}

/*!
\brief Translate a circle.

\param t Translation vector.
*/
inline void Circle2::Translate(const Vector2& t)
{
    c += t;
}

/*!
\brief Scales a circle.

\param s Scaling factor.
*/
inline void Circle2::Scale(const double& s)
{
    c *= s;
    r *= s;
}




