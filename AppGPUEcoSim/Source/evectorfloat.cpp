#include "evectorfloat.h"
#include "matrix.h"

/*!
\class VectorFloat evectorfloat.h
\brief Vector representation for the GPU.

\ingroup MayaGpuGroup
*/

/*!
\class VectorFloat2 evectorfloat.h
\brief Vector representation for the GPU.

\ingroup MayaGpuGroup
*/

/*!
\class Matrix4Float evectorfloat.h
\brief Matrix representation for the GPU.

\ingroup MayaGpuGroup
*/
Matrix4Float Matrix4Float::Identity = Matrix4Float(1.0f);

/*!
\brief Create a look-at matrix from (eye, at, up) vectors.
\param eye camera eye
\param at camera look-at point
\param up camera up vector
*/
Matrix4Float Matrix4Float::LookAt(const Vector& eye, const Vector& at, const Vector& up)
{
    Vector view = at - eye;
    double norm = Norm(view);
    if (norm > 0.0)
        view /= norm;

    Vector right = view / up;
    norm = Norm(right);
    if (norm > 0.0)
        right /= norm;

    Vector upp = right / view;
    norm = Norm(upp);
    if (norm > 0.0)
        upp /= norm;

    Matrix4Float m = Matrix4Float::Identity;

    m(0, 0) = right[0];
    m(1, 0) = right[1];
    m(2, 0) = right[2];

    m(0, 1) = upp[0];
    m(1, 1) = upp[1];
    m(2, 1) = upp[2];

    m(0, 2) = -view[0];
    m(1, 2) = -view[1];
    m(2, 2) = -view[2];

    Matrix4Float t = Matrix4Float::Identity;
    t(3, 0) = -eye[0];
    t(3, 1) = -eye[1];
    t(3, 2) = -eye[2];

    return m * t;
}

/*!
\brief Create a perspective projection matrix.
\param fovy %Camera vertical fov (in degrees).
\param zNear, zFar Planes.
\param width, height %Camera dimension.
*/
Matrix4Float Matrix4Float::Perspective(float fovy, float zNear, float zFar, float width, float height)
{
    float aspect = width / height;

    float sine, cotangent, deltaZ;
    float radians = Math::DegreeToRadian(fovy / 2.0);
    deltaZ = zFar - zNear;
    sine = sin(radians);
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
        return Matrix4Float::Identity;
    cotangent = cos(radians) / sine;

    Matrix4Float m = Matrix4Float::Identity;
    m(0, 0) = cotangent / aspect;
    m(1, 1) = cotangent;
    m(2, 2) = -(zFar + zNear) / deltaZ;
    m(2, 3) = -1;
    m(3, 2) = -2 * zNear * zFar / deltaZ;
    m(3, 3) = 0;
    return m;
}

/*!
\brief Create an orthographic projection matrix.
\param l coordinate of the left vertical clipping plane
\param r coordinate of the right vertical clipping plane
\param b coordinate of the bottom horizontal clipping plane
\param t coordinate of the top horizontal clipping plane
\param n, f camera near and far plane
*/
Matrix4Float Matrix4Float::Orthographic(float l, float r, float b, float t, float n, float f)
{
    Matrix4Float m;

    m(0, 0) = 2 / (r - l);
    m(0, 1) = 0.0f;
    m(0, 2) = 0.0f;
    m(0, 3) = 0.0f;

    m(1, 0) = 0;
    m(1, 1) = 2 / (t - b);
    m(1, 2) = 0;
    m(1, 3) = 0;

    m(2, 0) = 0;
    m(2, 1) = 0;
    m(2, 2) = -2 / (f - n);
    m(2, 3) = 0;

    m(3, 0) = -(r + l) / (r - l);
    m(3, 1) = -(t + b) / (t - b);
    m(3, 2) = -(f + n) / (f - n);
    m(3, 3) = 1;

    return m;
}

/*!
\brief Compute the inverse.
*/
Matrix4Float Matrix4Float::Inverse() const
{
    Matrix4 tmp = Matrix4(
        (*this)(0, 0), (*this)(0, 1), (*this)(0, 2), (*this)(0, 3),
        (*this)(1, 0), (*this)(1, 1), (*this)(1, 2), (*this)(1, 3),
        (*this)(2, 0), (*this)(2, 1), (*this)(2, 2), (*this)(2, 3),
        (*this)(3, 0), (*this)(3, 1), (*this)(3, 2), (*this)(3, 3));

    Matrix4 invTmp = tmp.Inverse();
    float coef[16];
    for (int i = 0; i < 16; i++)
        coef[i] = float(invTmp[i]);

    return Matrix4Float(coef);
}

/*!
\brief Overloaded.
\param s Stream.
\param matrix The matrix.
*/
std::ostream& operator<<(std::ostream& s, const Matrix4Float& matrix)
{
        s << "Matrix4Float(";
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
