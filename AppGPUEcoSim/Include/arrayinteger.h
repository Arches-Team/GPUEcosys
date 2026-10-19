// Fundamentals

#pragma once

#include "array.h"
#include "scalarfield.h"

class Array2I :public Array2
{
protected:
    QVector<int> field; //!< Field samples.
public:
    //! Empty.
    Array2I() {}
    explicit Array2I(const Array2&, int = 0);
    explicit Array2I(const Box2&, int, int, int = 0);
    explicit Array2I(const Box2&, int, int, const QVector<int>&);
    explicit Array2I(const Box2&, const QImage&, bool = true);
    explicit Array2I(const ScalarField2&, int, int);
    explicit Array2I(const ScalarField2&);

    //! Empty
    ~Array2I();

    virtual int Value(int, int) const;

    // Access to elements
    int at(int, int) const;
    int at(const QPoint&) const;
    int& operator()(int, int);
    int& operator()(const QPoint&);

    int at(int) const;
    int& operator[](int);

    friend std::ostream& operator<<(std::ostream&, const Array2I&);
};

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline int Array2I::at(int i, int j) const
{
    return field.at(VertexIndex(i, j));
}

/*!
\brief Return the field value at a given array vertex.
\param q Point.
*/
inline int Array2I::at(const QPoint& q) const
{
    return field.at(VertexIndex(q.x(), q.y()));
}

/*!
\brief Return the field value at a given array vertex.
\param q Point.
*/
inline int& Array2I::operator()(const QPoint& q)
{
    return field[VertexIndex(q.x(), q.y())];
}

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline int& Array2I::operator()(int i, int j)
{
    return field[VertexIndex(i, j)];
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline int Array2I::at(int c) const
{
    return field.at(c);
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline int& Array2I::operator[](int c)
{
    return field[c];
}


