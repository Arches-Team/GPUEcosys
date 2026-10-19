// Triangle

#include "triangle.h"

// Used for drawing
#include <QtGui/QPolygonF>

/*!
\brief Compute the normal vector of the triangle.

This function is expensive as it requires normalizing the cross product of the edge vectors.
\sa TriangleEdge
*/
Vector Triangle::Normal() const
{
    return Normalized((p[1] - p[0]) / (p[2] - p[0]));
}

/*!
\brief Compute the normal vector of the triangle, and scale the normal using its area.

This function is less expensive than Triangle::Normal() as it does not require normalizing the cross product of the edge vectors.

IIt is useful for computing the vertex normals of a triangle mesh by averaging the face normals of the triangles and weighting
the influence of the different triangles with their area.
\sa Triangle::Normal()
*/
Vector Triangle::AreaNormal() const
{
    return 0.5 * ((p[1] - p[0]) / (p[2] - p[0]));
}

/*!
\brief Overloaded.
\param t %Triangle.
\param s Stream.
*/
std::ostream& operator<<(std::ostream& s, const Triangle& t)
{
    s << "Triangle(" << t.p[0] << ',' << t.p[1] << ',' << t.p[2] << ')';
    return s;
}

/*!
\brief Computes the normal vector to a triangle.
\param q Point.
*/
Vector Triangle::Normal(const Vector& q) const
{
    // Those parameters are constant for a given triangle, and could be preprocessed.
    Vector ab = p[1] - p[0];
    Vector ac = p[2] - p[0];
    double abab = ab * ab;
    double abac = ab * ac;
    double acac = ac * ac;
    double delta = fabs(abab * acac - abac * abac);

    Vector pa = p[0] - q;
    double paab = pa * ab;
    double paac = pa * ac;
    double fS = abac * paac - acac * paab;
    double fT = abac * paab - abab * paac;

    if (fS + fT <= delta)
    {
        if (fS < 0.0)
        {
            if (fT < 0.0)  // region 4
            {
                if (paab < 0.0)
                {
                    fT = 0.0;
                    if (-paab >= abab)
                    {
                        fS = 1.0;
                    }
                    else
                    {
                        fS = -paab / abab;
                    }
                }
                else
                {
                    fS = 0.0;
                    if (paac >= 0.0)
                    {
                        fT = 0.0;
                    }
                    else if (-paac >= acac)
                    {
                        fT = 1.0;
                    }
                    else
                    {
                        fT = -paac / acac;
                    }
                }
            }
            else  // region 3
            {
                fS = 0.0;
                if (paac >= 0.0)
                {
                    fT = 0.0;
                }
                else if (-paac >= acac)
                {
                    fT = 1.0;
                }
                else
                {
                    fT = -paac / acac;
                }
            }
        }
        else if (fT < 0.0)  // region 5
        {
            fT = 0.0;
            if (paab >= 0.0)
            {
                fS = 0.0;
            }
            else if (-paab >= abab)
            {
                fS = 1.0;
            }
            else
            {
                fS = -paab / abab;
            }
        }
        else  // region 0
        {
            // minimum at interior point
            double fInvDet = 1.0 / delta;
            fS *= fInvDet;
            fT *= fInvDet;
        }
    }
    else
    {
        double fTmp0, fTmp1, nu, de;

        if (fS < 0.0)  // region 2
        {
            fTmp0 = abac + paab;
            fTmp1 = acac + paac;
            if (fTmp1 > fTmp0)
            {
                nu = fTmp1 - fTmp0;
                de = abab - 2.0 * abac + acac;
                if (nu >= de)
                {
                    fS = 1.0;
                    fT = 0.0;
                }
                else
                {
                    fS = nu / de;
                    fT = 1.0 - fS;
                }
            }
            else
            {
                fS = 0.0;
                if (fTmp1 <= 0.0)
                {
                    fT = 1.0;
                }
                else if (paac >= 0.0)
                {
                    fT = 0.0;
                }
                else
                {
                    fT = -paac / acac;
                }
            }
        }
        else if (fT < 0.0)  // region 6
        {
            fTmp0 = abac + paac;
            fTmp1 = abab + paab;
            if (fTmp1 > fTmp0)
            {
                nu = fTmp1 - fTmp0;
                de = abab - 2.0 * abac + acac;
                if (nu >= de)
                {
                    fT = 1.0;
                    fS = 0.0;
                }
                else
                {
                    fT = nu / de;
                    fS = 1.0 - fT;
                }
            }
            else
            {
                fT = 0.0;
                if (fTmp1 <= 0.0)
                {
                    fS = 1.0;
                }
                else if (paab >= 0.0)
                {
                    fS = 0.0;
                }
                else
                {
                    fS = -paab / abab;
                }
            }
        }
        else  // region 1
        {
            nu = acac + paac - abac - paab;
            if (nu <= 0.0)
            {
                fS = 0.0;
                fT = 1.0;
            }
            else
            {
                de = abab - 2.0 * abac + acac;
                if (nu >= de)
                {
                    fS = 1.0;
                    fT = 0.0;
                }
                else
                {
                    fS = nu / de;
                    fT = 1.0 - fS;
                }
            }
        }
        }
        return q - (p[0] + fS * ab + fT * ac);
}
