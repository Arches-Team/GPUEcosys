// Matrix 

#include "matrix.h"
#include "random.h"

/*!
\class Matrix matrix.h
\brief This class implements \f( 3^2 \f)  matrix.

Operators have been overloaded so as to behave as expected
with the Vector class.

\image html matrix3.png

Many functions have been coded inline out of efficiency.

The constructors of this class are used to create matrices
in the general case.
See the static member functions Matrix::Rotation() to create
different kinds of rotation matrices. Canonical symmetry matrices should
be created as follows:
\code
Matrix planar=Matrix(Vector(1.0,-1.0,1.0)); // Symmetry, plane y=0
Matrix origin=Matrix(Vector(-1.0,-1.0,-1.0)); // Symmetry around origin
\endcode
More general symmetries can be created using static member functions:
\code
Matrix planar=Matrix::Symmetry(Vector(2.0,-3.0,1.0)); // Symmetry, plane with normal vector as argument
\endcode

Components are stored in a single dimension array, sorting components
by column.

\ingroup MathGroup
*/

double Matrix::epsilon = 1.0e-8;

/*!
\brief Null matrix.
*/
const Matrix Matrix::Null(0.0);

/*!
\brief Identity matrix.
*/
const Matrix Matrix::Identity(1.0);

const Matrix Matrix::RotationHalfPiZ(0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0);

/*!
\brief Compute the inverse of a matrix \f( A^{-1} \f).

Recall that \f( A^{-1} \f) can be defined as the transposed adjoint matrix divided by the determinant.

This function returns the null matrix if A cannot be inversed.

The threshold value involved in the singular matrix detection
is set to \f( 10^{-18} \f).
*/
Matrix Matrix::Inverse() const
{
    double e = Determinant();

    if (fabs(e) < 1.0e-18)
    {
        return Matrix::Null;
    }

    return Adjoint().T() / e;
}

/*!
\brief Multiplication.

\param u, v Input matrices.
*/
Matrix operator*(const Matrix& u, const Matrix& v)
{
    return Matrix(u[0] * v[0] + u[3] * v[1] + u[6] * v[2], u[1] * v[0] + u[4] * v[1] + u[7] * v[2], u[2] * v[0] + u[5] * v[1] + u[8] * v[2],
        u[0] * v[3] + u[3] * v[4] + u[6] * v[5], u[1] * v[3] + u[4] * v[4] + u[7] * v[5], u[2] * v[3] + u[5] * v[4] + u[8] * v[5],
        u[0] * v[6] + u[3] * v[7] + u[6] * v[8], u[1] * v[6] + u[4] * v[7] + u[7] * v[8], u[2] * v[6] + u[5] * v[7] + u[8] * v[8]);
}

/*!
\brief Destructive multiplication.
*/
Matrix& Matrix::operator*=(const Matrix& M)
{
    Matrix r = (*this) * M;
    *this = r;
    return *this;
}

/*!
\brief Computes the inverse matrix and scales it by a double.

This function calls Matrix::Inverse().
\param x %Real.
\param a %Matrix.
*/
Matrix operator/(const double& x, const Matrix& a)
{
    return x * a.Inverse();
}

/*!
\brief Create a rotation matrix given a
vector of angles that specifies the rotation
around each world coordinate axis.

Rotations are concatenated, starting by rotating
around the z axis, then y and eventually x.

\param v Vector of angles in radian.
*/
Matrix Matrix::Rotation(const Vector& v)
{
    return Matrix::RotationX(v[0]) * Matrix::RotationY(v[1]) * Matrix::RotationZ(v[2]);
}

/*!
\brief Create a rotation matrix around the x-axis.

This static member function is provided out of efficiency as
it is much faster than any other general Matrix::Rotation() member.

\param a Angle (in radian).
*/
Matrix Matrix::RotationX(const double& a)
{
    const double c = cos(a);
    const double s = sin(a);
    return Matrix(1.0, 0.0, 0.0, 0.0, c, s, 0.0, -s, c);
}

/*!
\brief Create a rotation matrix around the y-axis.

\sa Matrix::RotationX(const double&)

\param a Angle (in radian).
*/
Matrix Matrix::RotationY(const double& a)
{
    const double c = cos(a);
    const double s = sin(a);
    return Matrix(c, 0.0, -s, 0.0, 1.0, 0.0, s, 0.0, c);
}

/*!
\brief Create a rotation matrix around the z-axis.

\sa Matrix::RotationX(const double&)

\param a Angle (in radian).
*/
Matrix Matrix::RotationZ(const double& a)
{
    const double c = cos(a);
    const double s = sin(a);
    return Matrix(c, s, 0.0, -s, c, 0.0, 0.0, 0.0, 1.0);
}

/*!
\brief Create a rotation matrix that rotates a normalized
vector into another one.

\param a, b Initial and final vector (should be normalized).
*/
Matrix Matrix::Rotation(const Vector& a, const Vector& b)
{
    Matrix matrix;

    Vector v = a / b;
    double e = a * b;

    // Almost identical vectors
    if (e > 1.0 - epsilon)
    {
        return Matrix::Identity;
    }
    // Almost opposite vectors
    else if (e < epsilon - 1.0)
    {
        Vector left(0.0, a[2], -a[1]);
        if (left * left < epsilon)
        {
            left[0] = -a[2]; left[1] = 0.0; left[2] = a[0];
        }

        Normalize(left);
        Vector up = left / a;
        // We have a coordinate system, i.e. a basis
        // M = (a, up, left), and we want to rotate to:    
        // N = (-a, up, -left). This is done with the matrix:
        // N*M^T where M^T is the transpose of M          
        double fxx = -a[0] * a[0]; double fyy = -a[1] * a[1]; double fzz = -a[2] * a[2];
        double fxy = -a[0] * a[1]; double fxz = -a[0] * a[2]; double fyz = -a[1] * a[2];

        double uxx = up[0] * up[0]; double uyy = up[1] * up[1]; double uzz = up[2] * up[2];
        double uxy = up[0] * up[1]; double uxz = up[0] * up[2]; double uyz = up[1] * up[2];

        double lxx = -left[0] * left[0]; double lyy = -left[1] * left[1]; double lzz = -left[2] * left[2];
        double lxy = -left[0] * left[1]; double lxz = -left[0] * left[2]; double lyz = -left[1] * left[2];

        // Symmetric matrix
        matrix(0, 0) = fxx + uxx + lxx; matrix(0, 1) = fxy + uxy + lxy; matrix(0, 2) = fxz + uxz + lxz;
        matrix(1, 0) = matrix(0, 1); matrix(1, 1) = fyy + uyy + lyy; matrix(1, 2) = fyz + uyz + lyz;
        matrix(2, 0) = matrix(0, 2); matrix(2, 1) = matrix(1, 2); matrix(2, 2) = fzz + uzz + lzz;
    }
    else
    {
        double h = (1.0 - e) / (v * v);
        double hvx = h * v[0];
        double hvz = h * v[2];
        double hvxy = hvx * v[1];
        double hvxz = hvx * v[2];
        double hvyz = hvz * v[1];
        matrix(0, 0) = e + hvx * v[0]; matrix(0, 1) = hvxy - v[2];    matrix(0, 2) = hvxz + v[1];
        matrix(1, 0) = hvxy + v[2];  matrix(1, 1) = e + h * v[1] * v[1]; matrix(1, 2) = hvyz - v[0];
        matrix(2, 0) = hvxz - v[1];  matrix(2, 1) = hvyz + v[0];    matrix(2, 2) = e + hvz * v[2];
    }
    return matrix;
}

/*!
\brief Create a rotation matrix about an arbitrary axis.

\code
Matrix R=Matrix::Rotation(Normalized(Vector(1,2,-1)),Math::DegreeToRadian(30.0));
\endcode

\param u Rotation axis, which should be unit.
\param a %Angle, in radian.
*/
Matrix Matrix::Rotation(const Vector& u, const double& a)
{
    double c = cos(a);
    double s = sin(a);
    double ux = u[0];
    double uy = u[1];
    double uz = u[2];

    double ic = 1.0 - c;
    return Matrix(c + ux * ux * ic,
        uy * ux * ic + uz * s,
        uz * ux * ic - uy * s,
        ux * uy * ic - uz * s,
        c + uy * uy * ic,
        uz * uy * ic + ux * s,
        ux * uz * ic + uy * s,
        uy * uz * ic - ux * s,
        c + uz * uz * ic);
}

/*!
\brief Destructive addition operator.
*/
Matrix& Matrix::operator+=(const Matrix& u)
{
    for (int i = 0; i < 9; i++)
    {
        r[i] += u.r[i];
    }
    return *this;
}

/*!
\brief Destructive subtraction operator.
*/
Matrix& Matrix::operator-=(const Matrix& u)
{
    for (int i = 0; i < 9; i++)
    {
        r[i] -= u.r[i];
    }
    return *this;
}

/*!
\brief Destructive multiplication operator.
*/
Matrix& Matrix::operator*=(double a)
{
    for (int i = 0; i < 9; i++)
    {
        r[i] *= a;
    }
    return *this;
}

/*!
\brief Destructive division operator.
*/
Matrix& Matrix::operator/=(double a)
{
    const double ia = 1.0 / a;
    for (int i = 0; i < 9; i++)
    {
        r[i] *= ia;
    }
    return *this;
}

/*!
\brief Compute the adjoint, i.e, the comatrix of the matrix.

\sa Inverse()
*/
Matrix Matrix::Adjoint() const
{
    return Matrix(
        r[4] * r[8] - r[7] * r[5],
        -(r[3] * r[8] - r[6] * r[5]),
        r[3] * r[7] - r[6] * r[4],

        -(r[1] * r[8] - r[7] * r[2]),
        r[0] * r[8] - r[6] * r[2],
        -(r[0] * r[7] - r[6] * r[1]),

        r[1] * r[5] - r[4] * r[2],
        -(r[0] * r[5] - r[3] * r[2]),
        r[0] * r[4] - r[3] * r[1]);
}

/*!
\brief Overloaded.
\param s Stream.
\param matrix The matrix.
*/
std::ostream& operator<<(std::ostream& s, const Matrix& matrix)
{
    s << "Matrix(";
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            s << matrix(i, j);
            if (i * 3 + j != 8)
            {
                s << ',';
            }
        }
    }
    s << ')';
    return s;
}

/*!
\brief Compute the angles of the rotation matrix.
*/
Vector Matrix::GetRotationAngles() const
{
    // Angles
    double x, y, z;

    const Matrix& m = *this;

    if (m(0, 2) < +1.0)
    {
        if (m(0, 2) > -1.0)
        {
            y = asin(m(0, 2));
            x = atan2(-m(1, 2), m(2, 2));
            z = atan2(-m(0, 1), m(0, 0));
        }
        else // r02 = -1
        {
            // Not a unique solution: thetaZ - thetaX = atan2(r10,r11)
            y = -Math::HalfPi;
            x = -atan2(m(1, 0), m(1, 1));
            z = 0.0;
        }
    }
    else // r02 = +1
    {
        // Not a unique solution: thetaZ + thetaX = atan2(r10,r11)
        y = +Math::HalfPi;
        x = atan2(m(1, 0), m(1, 1));
        z = 0.0;
    }

    return Vector(x, y, z);
}


/*!
\brief Generate uniformly distributed random rotation matrix.

See Fast Random Rotation Matrices, J. Arvo. <I>Graphics Gems III</I>, 1992.

\param ra Fast random number generator.
*/
Matrix Matrix::Rotation(RandomFast& ra)
{
    double theta = ra.Uniform() * Math::TwoPi; // Rotation about the pole (Z).
    double phi = ra.Uniform() * Math::TwoPi; // For direction of pole deflection.
    double z = ra.Uniform() * 2.0;                 // For magnitude of pole deflection.

    // Compute a vector V used for distributing points over the sphere  via the reflection I - V t(V). 
    // This formulation of V will guarantee that if x[1] and x[2] are uniformly distributed, the reflected points will be uniform on the sphere.  
    // Note that V  has length sqrt(2) to eliminate the 2 in the Householder matrix. 
    double r = sqrt(z);
    double vx = sin(phi) * r;
    double vy = cos(phi) * r;
    double vz = sqrt(2.0 - z);

    // Compute the row vector S = t(V) * R, where R is a simple rotation by theta about the z-axis.  
    // No need to compute Sz since it's just Vz.                                                    
    double st = sin(theta);
    double ct = cos(theta);
    double sx = vx * ct - vy * st;
    double sy = vx * st + vy * ct;

    // Construct the rotation matrix : ( V t(V) - I ) R, 
    // which is equivalent to V S - R (last element == Vz * Vz - 1)
    return Matrix(
        vx * sx - ct, vx * sy - st, vx * vz,
        vy * sx + st, vy * sy - ct, vy * vz,
        vz * sx, vz * sy, 1.0 - z);
}
