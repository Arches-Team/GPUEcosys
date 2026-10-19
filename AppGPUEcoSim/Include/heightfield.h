#pragma once

#include <QtCore/QString>

#include "scalarfield.h"
#include "arrayinteger.h"
#include "random.h"
#include "ray.h"


class FlowStruct
{
protected:
    int mask = 0; //!< Integer mask of downstream flow topology.
public:
    int i[8] = { -1 }; //!< Indexes to neighboring cells.
    QPoint q[8] = { QPoint(0,0) }; //!< Flow points.
    double h[8] = { 0.0 }; //!< Heights.
    double s[8] = { 0.0 }; //!< Slopes.
    double sn[8] = { 0.0 }; //!< Normalized slopes.
    int num = 0; //GUNDY ADD NUMBER OF DIRECTIONS IN IT
protected:
    int steepest = -1; //!< Steepest slope index, i.e., index in the arrays storing the steepest slope data.
public:
    explicit FlowStruct() {}
    ~FlowStruct() {}

    QPoint Steepest() const;
    double SteepestSlope() const;
    int Topology() const;
    int Next(int) const;
    QPoint NextPoint(int) const;

    int SelectSlope(int) const;

    friend class HeightField;
    friend class HeightFieldSediment;
protected:
    static RandomFast fast;
};


/*!
\brief Select direction according to slope distribution.
\param n Number of directions.
*/
inline int FlowStruct::SelectSlope(int n) const
{
    return fast.Pick(s, n);
}

/*!
\brief Get k-th downward point.
\param k Index.
*/
inline QPoint FlowStruct::NextPoint(int k) const
{
    return q[k];
}

/*!
\brief Steepest downward point.
*/
inline QPoint FlowStruct::Steepest() const
{
    return q[steepest];
}

/*!
\brief Steepest downward point.
*/
inline double FlowStruct::SteepestSlope() const
{
    return s[steepest];
}

/*!
\brief %Topology mask.
*/
inline int FlowStruct::Topology() const
{
    return mask;
}

/*!
\brief Get the index of the k-th downward point.
\param k Index.
*/
inline int FlowStruct::Next(int k) const
{
    return i[k];
}

class TerrainDX;

class HeightField :public ScalarField2
{
protected:
public:
    //! Empty
    HeightField() {}

    HeightField(const ScalarField2&);
    explicit HeightField(const Box2&, const QImage&, const double& = 0.0, const double& = 256.0 * 256.0 - 1.0, bool = true);
    explicit HeightField(const Box2&, int, int, const double& = 0.0);
    explicit HeightField(const Box2&, int, int, const QVector<double>&);

    //! Empty
    ~HeightField() {}

    double GetHeight(const Vector2&) const;

    // Vertices
    Vector Vertex(const Vector2&, bool = true) const;
    Vector Vertex(int, int) const;

    // Normals and slope
    Vector Normal(const Vector2&, bool = true) const;
    Vector Normal(int, int) const;
    double Slope(int, int) const;
    double Slope(const QPoint&, const QPoint&) const;
    double AverageSlope(int, int) const;

    double K() const;

    // Stream area
    ScalarField2 StreamArea(const double& = 1.0) const;

    // Slope
    ScalarField2 AverageSlope() const;
    ScalarField2 Slope(bool = false) const;

    // Wetness index, stream power and others derived from previously defined functions
    ScalarField2 WetnessIndex(const double& = 1.0e-3, bool = false) const;

    ScalarField2 Accessibility(const double&, int = 16) const;
    ScalarField2 Light(const Vector&, bool = false) const;

    void CompleteBreach(const double&  = -9999.);
    ScalarField2 DepthToWater(const TerrainDX* terrainDX) const;

    // Ray intersection
    bool Intersect(const Ray&, double&, Vector&) const;
    bool Intersect(const Ray&, double&, Vector&, const Box&, const double&, const double& = 1.0e8, const double& = 1.0e-4) const;
    bool Intersect(const Ray&, QVector<double>&, QVector<Vector>&, const Box&, const double&) const;

    // Box
    Box GetBox() const;

    // Flow
    int CheckFlowSlope(const QPoint&, FlowStruct&, const double& = 1.0) const;

public:
    static const double flat; //!< Small negative epsilon value used in breaching and flow algorithms.
};


/*!
\brief Compute the height at a given position.
\param p Point.
*/
inline double HeightField::GetHeight(const Vector2& p) const
{
    return Value(p);
}

class TerrainDX :public Array2
{
protected:
    HeightField terrain; //!< Reference to elevation.
    Array2I dirx; //!< Flow Direction.
    ScalarField2 accumulaton; //!< Drainage area. 
public:
    explicit TerrainDX(const HeightField&);
    //! Empty.
    virtual ~TerrainDX() {}

    // Get Local Type
    bool isRiver(int, int) const;

    int getRiverOutput(int i, int j) const;

};

/*!
\brief
*/
inline bool TerrainDX::isRiver(int i, int j) const
{
    if (!accumulaton.InsideVertexIndex(i, j)) return false;
    return accumulaton.Value(i, j) > 200;
}
