// Matrix

#include "matrix.h"

/*!
\class Matrix4 matrix.h
\brief This class implements \f( 4^2 \f)  matrix.

Components are stored in a single dimension array, starting from
element \f( a_{00} \f) and sorting components by column.

The diagonal elements of a matrix A are \fn( A_{00}=A[0] \f), \fn( A_{11}=A[5] \f), \fn( A_{22}=A[10] \f), \fn( A_{33}=A[15] \f).
The terms corresponding to the translation vector are A[12], A[13], A[14].
\ingroup MathGroup
*/

const double Matrix4::epsilon = 1.0e-18;

const Matrix4 Matrix4::Null(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

const Matrix4 Matrix4::Identity(1.0);

const Matrix4 Matrix4::Hermite(
    2.0, -3.0, 0.0, 1.0,
    -2.0, 3.0, 0.0, 0.0,
    1.0, -2.0, 1.0, 0.0,
    1.0, -1.0, 0.0, 0.0);

/*!
\brief Create an scaling matrix with the same diagonal terms.

The last term of the matrix is set to 1.0.
\param a Value of diagonal entries.
*/
Matrix4::Matrix4(const double& a) :r{ a,0.0,0.0,0.0,0.0,a,0.0,0.0,0.0,0.0,a,0.0,0.0,0.0,0.0,1.0 }
{
}

/*!
\brief Create a homogeneous diagonal matrix with diagonal terms
set to the vector entries.

Last diagonal entry is set to 1.0.
\param a Vector of diagonal entries.
*/
Matrix4::Matrix4(const Vector& a) :r{ a[0],0.0,0.0,0.0,0.0,a[1],0.0,0.0,0.0,0.0,a[2],0.0,0.0,0.0,0.0,1.0 }
{
}

/*!
\brief Create an homogeneous matrix from a simple Matrix.

The translation and shear coefficients are set to 0.0.
\param a Matrix.
*/
Matrix4::Matrix4(const Matrix& a)
{
    // Rotation and scale
    r[0] = a[0];
    r[1] = a[1];
    r[2] = a[2];
    r[4] = a[3];
    r[5] = a[4];
    r[6] = a[5];
    r[8] = a[6];
    r[9] = a[7];
    r[10] = a[8];

    // Translation
    r[3] = r[7] = r[11] = 0.0;

    // Shear
    r[12] = r[13] = r[14] = 0.0;

    // Scale
    r[15] = 1.0;
}

/*!
\brief Create an homogeneous matrix from a simple Matrix and a translation vector

The shear coefficients are set to 0.0.
\param a Matrix.
\param t Translation vector.
*/
Matrix4::Matrix4(const Matrix& a, const Vector& t) :Matrix4(a, t, Vector::Null)
{
}

/*!
\brief Constructor from a Matrix, a translation and a shear vector.
\param a Matrix.
\param t Translation vector.
\param s Shear vector.
*/
Matrix4::Matrix4(const Matrix& a, const Vector& t, const Vector& s)
{
    // Rotation and scale
    r[0] = a[0];
    r[1] = a[1];
    r[2] = a[2];
    r[4] = a[3];
    r[5] = a[4];
    r[6] = a[5];
    r[8] = a[6];
    r[9] = a[7];
    r[10] = a[8];

    // Translation
    r[12] = t[0];
    r[13] = t[1];
    r[14] = t[2];

    // Shear
    r[3] = s[0];
    r[7] = s[1];
    r[11] = s[2];

    // Scale
    r[15] = 1.0;
}

/*!
\brief Convert to a generic 4&times;4 float matrix.
\param a Returned matrix.
*/
void Matrix4::Float(float a[16]) const
{
    for (int i = 0; i < 16; i++)
    {
        a[i] = float(r[i]);
    }
}

/*!
\brief Returns the opposite of a matrix -A.
*/
Matrix4 Matrix4::operator-() const
{
    Matrix4 n;
    for (int i = 0; i < 16; i++)
    {
        n.r[i] = -r[i];
    }
    return n;
}

/*!
\brief Multiplication.
\param u,v Argument matrix.
*/
Matrix4 operator*(const Matrix4& u, const Matrix4& v)
{
    Matrix4 a;
    for (int i = 0; i < 4; i++)
    {
        int k = i << 2;
        for (int j = 0; j < 4; j++)
        {
            a.r[k + j] = u.r[j] * v.r[k] + u.r[4 + j] * v.r[k + 1] + u.r[8 + j] * v.r[k + 2] + u.r[12 + j] * v.r[k + 3];
        }
    }
    return a;
}

/*!
\brief Destructive multiplication.
\param v %Matrix.
*/
Matrix4& Matrix4::operator*=(const Matrix4& v)
{
    Matrix4 r;

    r = (*this) * v;
    *this = r;
    return *this;
}

/*!
\brief Right multiply by a vector.
\param v %Vector.
*/
Vector Matrix4::operator*(const Vector& v) const
{
    double w = 1.0 / (v[0] * r[3] + v[1] * r[7] + v[2] * r[11] + r[15]);

    Vector res;
    for (int i = 0; i < 3; i++)
    {
        res[i] = (v[0] * r[i] + v[1] * r[4 + i] + v[2] * r[8 + i] + r[12 + i]) * w;
    }
    return res;
}

/*!
\brief Computes the inverse of a matrix \f( A^{-1} \f).

This function returns the null matrix if A cannot be inverted.
The threshold value involved in the singular matrix detection is
set to \f( 10^{-18} \f).
*/
Matrix4 Matrix4::Inverse() const
{
    double d00 = r[5] * r[10] * r[15] + r[9] * r[14] * r[7] + r[13] * r[6] * r[11] - r[7] * r[10] * r[13] - r[11] * r[14] * r[5] - r[15] * r[6] * r[9];
    double d01 = r[1] * r[10] * r[15] + r[9] * r[14] * r[3] + r[13] * r[2] * r[11] - r[3] * r[10] * r[13] - r[11] * r[14] * r[1] - r[15] * r[2] * r[9];
    double d02 = r[1] * r[6] * r[15] + r[5] * r[14] * r[3] + r[13] * r[2] * r[7] - r[3] * r[6] * r[13] - r[7] * r[14] * r[1] - r[15] * r[2] * r[5];
    double d03 = r[1] * r[6] * r[11] + r[5] * r[10] * r[3] + r[9] * r[2] * r[7] - r[3] * r[6] * r[9] - r[7] * r[10] * r[1] - r[11] * r[2] * r[5];

    // Test if singular matrix
    double d = r[0] * d00 - r[4] * d01 + r[8] * d02 - r[12] * d03;

    if (fabs(d) < Matrix4::epsilon)
    {
        return Matrix4::Null;
    }

    double d10 = r[4] * r[10] * r[15] + r[8] * r[14] * r[7] + r[12] * r[6] * r[11] - r[7] * r[10] * r[12] - r[11] * r[14] * r[4] - r[15] * r[6] * r[8];
    double d11 = r[0] * r[10] * r[15] + r[8] * r[14] * r[3] + r[12] * r[2] * r[11] - r[3] * r[10] * r[12] - r[11] * r[14] * r[0] - r[15] * r[2] * r[8];
    double d12 = r[0] * r[6] * r[15] + r[4] * r[14] * r[3] + r[12] * r[2] * r[7] - r[3] * r[6] * r[12] - r[7] * r[14] * r[0] - r[15] * r[2] * r[4];
    double d13 = r[0] * r[6] * r[11] + r[4] * r[10] * r[3] + r[8] * r[2] * r[7] - r[3] * r[6] * r[8] - r[7] * r[10] * r[0] - r[11] * r[2] * r[4];

    double d20 = r[4] * r[9] * r[15] + r[8] * r[13] * r[7] + r[12] * r[5] * r[11] - r[7] * r[9] * r[12] - r[11] * r[13] * r[4] - r[15] * r[5] * r[8];
    double d21 = r[0] * r[9] * r[15] + r[8] * r[13] * r[3] + r[12] * r[1] * r[11] - r[3] * r[9] * r[12] - r[11] * r[13] * r[0] - r[15] * r[1] * r[8];
    double d22 = r[0] * r[5] * r[15] + r[4] * r[13] * r[3] + r[12] * r[1] * r[7] - r[3] * r[5] * r[12] - r[7] * r[13] * r[0] - r[15] * r[1] * r[4];
    double d23 = r[0] * r[5] * r[11] + r[4] * r[9] * r[3] + r[8] * r[1] * r[7] - r[3] * r[5] * r[8] - r[7] * r[9] * r[0] - r[11] * r[1] * r[4];

    double d30 = r[4] * r[9] * r[14] + r[8] * r[13] * r[6] + r[12] * r[5] * r[10] - r[6] * r[9] * r[12] - r[10] * r[13] * r[4] - r[14] * r[5] * r[8];
    double d31 = r[0] * r[9] * r[14] + r[8] * r[13] * r[2] + r[12] * r[1] * r[10] - r[2] * r[9] * r[12] - r[10] * r[13] * r[0] - r[14] * r[1] * r[8];
    double d32 = r[0] * r[5] * r[14] + r[4] * r[13] * r[2] + r[12] * r[1] * r[6] - r[2] * r[5] * r[12] - r[6] * r[13] * r[0] - r[14] * r[1] * r[4];
    double d33 = r[0] * r[5] * r[10] + r[4] * r[9] * r[2] + r[8] * r[1] * r[6] - r[2] * r[5] * r[8] - r[6] * r[9] * r[0] - r[10] * r[1] * r[4];

    // Inverse of determinant
    d = 1.0 / d;

    // Create inverse
    Matrix4 r;

    r[0] = d00 * d;  r[4] = -d10 * d; r[8] = d20 * d; r[12] = -d30 * d;
    r[1] = -d01 * d; r[5] = d11 * d; r[9] = -d21 * d; r[13] = d31 * d;
    r[2] = d02 * d; r[6] = -d12 * d; r[10] = d22 * d; r[14] = -d32 * d;
    r[3] = -d03 * d; r[7] = d13 * d; r[11] = -d23 * d; r[15] = d33 * d;
    return r;
}

/*!
\brief Destructive addition.
*/
Matrix4& Matrix4::operator+= (const Matrix4& u)
{
    for (int i = 0; i < 16; i++)
    {
        r[i] += u.r[i];
    }
    return *this;
}

/*!
\brief Destructive subtraction.
*/
Matrix4& Matrix4::operator-= (const Matrix4& u)
{
    for (int i = 0; i < 16; i++)
    {
        r[i] -= u.r[i];
    }
    return *this;
}

/*!
\brief Destructive scalar multiply.
*/
Matrix4& Matrix4::operator*= (double a)
{
    for (int i = 0; i < 16; i++)
    {
        r[i] *= a;
    }
    return *this;
}

/*!
\brief Destructive scalar divide.
*/
Matrix4& Matrix4::operator/= (double a)
{
    const double ia = 1.0 / a;
    for (int i = 0; i < 16; i++)
    {
        r[i] *= ia;
    }
    return *this;
}

/*!
\brief Overloaded.
\param u,v Argument matrixes.
*/
Matrix4 operator+ (const Matrix4& u, const Matrix4& v)
{
    return Matrix4(
        u[0] + v[0], u[1] + v[1], u[2] + v[2], u[3] + v[3],
        u[4] + v[4], u[5] + v[5], u[6] + v[6], u[7] + v[7],
        u[8] + v[8], u[9] + v[9], u[10] + v[10], u[11] + v[11],
        u[12] + v[12], u[13] + v[13], u[14] + v[14], u[15] + v[15]);
}

/*!
\brief Overloaded.
\param u,v Argument matrixes.
*/
Matrix4 operator- (const Matrix4& u, const Matrix4& v)
{
    return Matrix4(
        u[0] - v[0], u[1] - v[1], u[2] - v[2], u[3] - v[3],
        u[4] - v[4], u[5] - v[5], u[6] - v[6], u[7] - v[7],
        u[8] - v[8], u[9] - v[9], u[10] - v[10], u[11] - v[11],
        u[12] - v[12], u[13] - v[13], u[14] - v[14], u[15] - v[15]);
}

/*!
\brief Overloaded.
\param s Stream.
\param matrix The matrix.
*/
std::ostream& operator<<(std::ostream& s, const Matrix4& matrix)
{
    s << "Matrix4(";
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            s << matrix(i, j);
            if (i * 4 + j != 15)
            {
                s << ',';
            }
        }
    }
    s << ')';
    return s;
}
