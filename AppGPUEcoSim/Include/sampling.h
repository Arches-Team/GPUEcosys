// Sampling  

#pragma once

#include <QtCore/QVector>

#include "box.h"
#include "circle.h"
#include "random.h"


class DiscTile
{
protected:
    double s = 0.0; //!< Size.
    double r = 0.0; //!< Radius.
    double e = 0.0; //!< Squared radius with small epsilon tolerance.
    QVector<Vector2> p; //!< Set of samples.
public:
    //! Empty
    explicit DiscTile();
    explicit DiscTile(const double&, const double&, const QVector<Vector2>&);
    explicit DiscTile(const double&, const double&, int, Random & = Random::R239);
    explicit DiscTile(const double&, const double&, int, const QVector<Vector2>&, Random & = Random::R239);

    //! Empty
    ~DiscTile() {}

    Box2 GetBox() const;

    Vector2 Vertex(int) const;
    Vector2 Vertex(int, int, int) const;
    Vector2 Vertex(const QPoint&, int) const;

    const QVector<Vector2> GetSet() const;

    int Size() const;

    friend std::ostream& operator<<(std::ostream&, const DiscTile&);

public:
    static const DiscTile T231, T619; //!< Reference unit box tiles with 231 and 619 discs.

protected:
    void Generate(int, Random & = Random::R239);

public:
    // Provide range-based for loops
    auto begin() { return p.begin(); }
    auto end() { return p.end(); }
    auto cbegin() const { return p.begin(); }
    auto cend() const { return p.end(); }
    auto begin() const { return p.begin(); }
    auto end() const { return p.end(); }
};

/*!
\brief Getter on the i-th sample.
\param i Sample index.
*/
inline Vector2 DiscTile::Vertex(int i) const
{
    return p.at(i);
}

/*!
\brief Compute the i-th sample for a displaced tile.
\param x,y Integer coordinates of the tile.
\param i Sample index.
*/
inline Vector2 DiscTile::Vertex(int x, int y, int i) const
{
    return GetBox().Tile(x, y)[0] + p.at(i);
}

/*!
\brief Compute the i-th sample for a displaced tile.
\param t Integer coordinates of the tile.
\param i Sample index.
*/
inline Vector2 DiscTile::Vertex(const QPoint& t, int i) const
{
    return GetBox().Tile(t.x(), t.y())[0] + p.at(i);
}

/*!
\brief Return the number of samples.
*/
inline int DiscTile::Size() const
{
    return p.size();
}

/*!
\brief Return the vector containing the samples.
*/
inline const QVector<Vector2> DiscTile::GetSet() const
{
    return p;
}
