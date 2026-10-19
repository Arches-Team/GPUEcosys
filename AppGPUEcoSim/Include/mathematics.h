// Fundamentals

#pragma once

#include <math.h>

#include <iostream>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

/*!
\brief Minimum of two integers.
*/
inline int min(int a, int b)
{
    return (a < b ? a : b);
}

/*!
\brief Minimum of two unsigned integers.
*/
inline unsigned int min(unsigned int a, unsigned int b)
{
    return (a < b ? a : b);
}

/*!
\brief Maximum of two integers.
*/
inline int max(int a, int b)
{
    return (a > b ? a : b);
}

/*!
\brief Minimum of two doubles.
*/
inline double min(const double& a, const double& b)
{
    return (a < b ? a : b);
}

/*!
\brief Maximum of two doubles.
*/
inline double max(const double& a, const double& b)
{
    return (a > b ? a : b);
}

/*!
\brief Minimum of two doubles.
*/
inline float min(const float& a, const float& b)
{
    return (a < b ? a : b);
}

/*!
\brief Maximum of two doubles.
*/
inline float max(const float& a, const float& b)
{
    return (a > b ? a : b);
}

/*!
\brief Minimum of three integers.
*/
inline int min(int a, int b, int c)
{
    return min(min(a, b), c);
}

/*!
\brief Maximum of three integers.
*/
inline int max(int a, int b, int c)
{
    return max(max(a, b), c);
}

/*!
\brief Minimum of four integers.
*/
inline int min(int a, int b, int c, int d)
{
    return min(min(a, b), min(c, d));
}

/*!
\brief Minimum of four unsigned integers.
*/
inline unsigned int min(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    return min(min(a, b), min(c, d));
}

/*!
\brief Maximum of four integers.
*/
inline int max(int a, int b, int c, int d)
{
    return max(max(a, b), max(c, d));
}

//! Clamps an integer value between two bounds.
inline int Clamp(int x, int a, int b)
{
    return (x < a ? a : (x > b ? b : x));
}

/*!
\brief Compute the next integer as \f( 2^k >= n \f).
\param n Integer.

Set all bits on the right-hand side of the most significant set bit to 1 and then increment the value by 1 to &#8220;rollover&#8221; to two&#8217;s nearest power.
*/
inline int NextPower2(int n)
{
    // Decrement n to handle the case when n itself is a power of 2
    n--;

    // Set all bits after the last set bit
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    // Increment n and return
    return ++n;
}

/*!
\brief Compute the logarithm of n in base 2.
\param n Integer.
*/
inline int Log2(int n)
{
    int l = 0;
    while (n >>= 1) { l++; }
    return l;
}

//! Clamps a float value between two bounds.
inline float Clamp(float x, float a, float b)
{
    return (x < a ? a : (x > b ? b : x));
}

//! Clamps a double value between two bounds.
inline double Clamp(double x, double a, double b)
{
    return (x < a ? a : (x > b ? b : x));
}

// Math class for constants
class Math
{
public:
    static constexpr double Pi = 3.14159265358979323846; //!< \f( \pi \f).
    static constexpr double TwoPi = 6.28318530717958647693;; //!< \f( 2\pi \f).
    static constexpr double HalfPi = Math::Pi / 2.0;; //!< \f( \pi/2 \f).
    static constexpr double E = 2.7182818284590452354;; //!< \f( e \f).
    static constexpr double TwoPiOverThree = 2.0943951023931954923084; //!< \f( 2/3 \pi \f).
    static constexpr double FourPiOverThree = 4.1887902047863909846168;; //!< \f( 4/3 \pi \f).
    static constexpr double Sqrt5 = 2.23606797749978969640917366873127623; //!< Constant \f( \sqrt{5} \f).
    static constexpr double Sqrt3 = 1.73205080756887729352744634150587236; //!< Constant \f( \sqrt{3} \f).
    static constexpr double Sqrt2 = 1.41421356237309504880168872420969807; //!< Constant \f( \sqrt{2} \f).
    static constexpr double Sqrt12 = 1.0 / Math::Sqrt2; //!< Constant \f(  1/\sqrt{2} \f).
    static constexpr double Golden = (Math::Sqrt5 + 1.0) / 2.0;; //!< Constant \f(  (\sqrt{5}+1)/2 \f).
    static constexpr double Large = 1.0e20;; //!< Large constant \f( 10^{20} \f).
    static constexpr double Infinity = 1.7976931348623158e+308; //!< \f( \infty \f).
    static constexpr double InfinityEpsilon = 2.2250738585072014e-308; //!< Smallest positive  \f( \epsilon \f).

    static constexpr int binomials[16][16] = {
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, 6, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 5, 10, 10, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 6, 15, 20, 15, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, 21, 35, 35, 21, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 8, 28, 56, 70, 56, 28, 8, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 9, 36, 84, 126, 126, 84, 36, 9, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1, 0, 0, 0, 0, 0 },
    { 1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1, 0, 0, 0, 0 },
    { 1, 12, 66, 220, 495, 792, 924, 792, 495, 220, 66, 12, 1, 0, 0, 0 },
    { 1, 13, 78, 286, 715, 1287, 1716, 1716, 1287, 715, 286, 78, 13, 1, 0, 0 },
    { 1, 14, 91, 364, 1001, 2002, 3003, 3432, 3003, 2002, 1001, 364, 91, 14, 1, 0 },
    { 1, 15, 105, 455, 1365, 3003, 5005, 6435, 6435, 5005, 3003, 1365, 455, 105, 15, 1 }
    };; //!< %Array of binomial coefficients.

public:
    static double Clamp(const double&, const double& = 0.0, const double& = 1.0);
    static int Clamp(int, int = 0, int = 255);

    // Squares
    static constexpr double Sqr(const double&);
    static constexpr double Cube(const double&);
    static constexpr double Sqr4(const double&);
    static double SymmetricSqr(const double&);
    static double Pow(const double&, const double&);
    static double Abs(const double&);
    static double Sqrt32(const double&);
    static double Sqrt4(const double&);


    static double Floor(const double&);
    static double Fract(const double&);
    static double FractFloor(const double&, double&);

    // Minimum and maximum
    static double Min(const double&, const double&);
    static double Max(const double&, const double&);
    static double Min(const double&, const double&, const double&);
    static double Max(const double&, const double&, const double&);
    static double Min(const double&, const double&, const double&, const double&);
    static double Max(const double&, const double&, const double&, const double&);

    // Angles
    static constexpr double DegreeToRadian(const double&);
    static constexpr double RadianToDegree(const double&);

    static constexpr double Angle(int, int);
    static double AngleSigned(const double&, const double&);
    static double AngleLerp(const double&, const double&, const double&);

    static double ArcTan(const double&, const double&);
    static double SinCard(double, bool = false);

    static double Step(double, double);

    // Linear interpolation
    static double Lerp(const double&, const double&, const double&);
    static double Bilinear(const double&, const double&, const double&, const double&, const double&, const double&);
    static double Trilinear(const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&);

    static double BiCubic(const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double& = 0.0, const double& = 0.0, const double& = 0.0, const double& = 0.0);
    static double BiCubic(const double&, const double&, const double&, const double&, const double&, const double&);

    // Sigmoid
    static double Sigmoid(const double&, const double& = 1.0);
    static double SigmoidQuadric(const double&);

    // Impulse
    static double Impulse(const double&, const double& = 1.0);

    // Gain warping
    static double Warp(const double&);
    static double Warp(const double&, const double&);

    static bool InRange(const double&, const double&, const double&, const double&);
    static void SetMinMax(const double&, double&, double&);

    static int Integer(const double&);

    static void Swap(int&, int&);
    static void Swap(double&, double&);

    // Signs
    static int IntegerSign(const double&, const double& = 0.0);
    static bool SameSign(const double&, const double&);
    static bool SameSign(const double&, const double&, const double&);
    static double CopySign(const double&, const double&);

    static constexpr double Unit(int, int);

    static double Gaussian(const double&, const double&);
    static double GaussianProbability(const double&, const double&);
    static double CompactGaussian(const double&, const double&);
    static double Fract(const double&, double&);
    static double Fract(const double&, const double&, double&);
    static double Fract(const double&, const double&, const double&, double&);

    static void Swap(double*&, double*&);
    static void Swap(int*&, int*&);

    static double SmoothingCubic(double, double, double);
    static double SmoothingQuadric(double, double, double);
};

/*!
\brief Check if a values lies within a prescribed range.
\param x Value.
\param a, b Interval.
\param epsilon Epsilon tolerance.
*/
inline bool Math::InRange(const double& x, const double& a, const double& b, const double& epsilon)
{
    return ((x + epsilon) >= a) && ((x - epsilon) <= b);
}

/*!
\brief Bi-linear interpolation between four values.

The values are given in trigonometric order.

\image html bilinear.png
\param a00, a10, a11, a01 Interpolated values.
\param u,v Interpolation coefficients.
*/
inline double Math::Bilinear(const double& a00, const double& a10, const double& a11, const double& a01, const double& u, const double& v)
{
    return (1.0 - u) * (1.0 - v) * a00 + (1.0 - u) * (v)*a01 + (u) * (1.0 - v) * a10 + (u) * (v)*a11;
}

/*!
\brief Trilinear interpolation between eight values.

\sa Math::Bilinear()
\param a,b,c,d,e,f,g,h Interpolated values.
\param u,v,w Interpolation coefficients.
*/
inline double Math::Trilinear(const double& a, const double& b, const double& c, const double& d, const double& e, const double& f, const double& g, const double& h, const double& u, const double& v, const double& w)
{
    return (1.0 - w) * Math::Bilinear(a, b, c, d, u, v) + w * Math::Bilinear(e, f, g, h, u, v);
}

/*!
\brief Squares a double value.
\sa Math::SymmetricSqr
\param x Real value.
*/
inline constexpr double Math::Sqr(const double& x)
{
    return x * x;
}

/*!
\brief Cubes a double value.
\param x Real value.
*/
inline constexpr double Math::Cube(const double& x)
{
    return x * x * x;
}

/*!
\brief Compute \f( x^{3/2} \f) as the square root of \f( x^3 \f).

This function is more efficient than pow() which is extremely slow.
\code
double y=Math::Sqrt32(7.0); // Equivalent but faster than y=pow(7.0,1.5);
\endcode
\param x Real value.
*/
inline double Math::Sqrt32(const double& x)
{
    return sqrt(x * x * x);
}

/*!
\brief Compute \f( \sqrt[4]{x} \f).

This function is more efficient than pow() which is extremely slow.
\code
double y=Math::Sqrt4(7.0); // Equivalent to y=sqrt(sqrt(7.0));
\endcode
\param x Real value.
*/
inline double Math::Sqrt4(const double& x)
{
    return sqrt(sqrt(x));
}

/*!
\brief Fourth power \f( x^4 \f).
\param x Real value.
*/
inline constexpr double Math::Sqr4(const double& x)
{
    double y = x * x;
    return y * y;
}

/*!
\brief Symmetric of square function over unit interval.

Simply compute \f( 1-(1-x)^2 \f).
\image html mathquadrics.png
\sa Math::Sqr
\param x Real value.
*/
inline double Math::SymmetricSqr(const double& x)
{
    return 1.0 - Math::Sqr(1.0 - x);
}

/*!
\brief Linear interpolation.

Returns (1-t)a+tb.

\param a,b Interpolated values.
\param t Interpolant.
*/
inline double Math::Lerp(const double& a, const double& b, const double& t)
{
    return a + t * (b - a);
}

/*!
\brief Convert degrees to randians.
\param a Angle in degrees.
*/
inline constexpr double Math::DegreeToRadian(const double& a)
{
    return a * Math::Pi / 180.0;
}

/*!
\brief Convert radian to degrees.
\param a Angle in radian.
*/
inline constexpr double Math::RadianToDegree(const double& a)
{
    return a * 180.0 / Math::Pi;
}

/*!
\brief Compute \f( 2 k \pi / n \f).
\param k, n Integers.
*/
inline constexpr double Math::Angle(int k, int n)
{
    return (Math::TwoPi * k) / n;
}

/*!
\brief Swap two reals.
\sa Sort(double&,double&)
\param a, b Arguments.
*/
inline void Math::Swap(double& a, double& b)
{
    double t = a;
    a = b;
    b = t;
}

/*!
\brief Swap two integers.
\sa Sort(int&,int&)
\param a, b Arguments.
*/
inline void Math::Swap(int& a, int& b)
{
    int t = a;
    a = b;
    b = t;
}

/*!
\brief Swap two integers.
\sa Swap(double&,double&)
\param a, b Arguments.
*/
inline void Swap(int& a, int& b)
{
    int t = a;
    a = b;
    b = t;
}

/*!
\brief Sort two integers.
\sa Swap(int&,int&)
\param a, b Arguments.
*/
inline void Sort(int& a, int& b)
{
    if (a > b)
    {
        Swap(a, b);
    }
}

/*!
\brief Compute the integer part of a real.

This function handles negative values differently by subtracting 1 from the result.
\param x %Real.
*/
inline int Math::Integer(const double& x)
{
    return (x > 0.0 ? int(x) : int(x) - 1);
}

/*!
\brief Compute the interpolation between two angles, using the smallest angle rotation.
\sa Math::AngleSigned
\param a, b %Angles.
\param t %Interpolant.
*/
inline double Math::AngleLerp(const double& a, const double& b, const double& t)
{
    double d = AngleSigned(a, b);
    return a + t * d; // Instead of standard linear interpolation
}

/*!
\brief Clamp a double value between two bounds.
\param x Input value.
\param a, b Lower and upper bounds.
*/
inline double Math::Clamp(const double& x, const double& a, const double& b)
{
    return (x < a ? a : (x > b ? b : x));
}

/*!
\brief Clamp an integer value between two bounds.
\param x Input value.
\param a, b Lower and upper bounds.
*/
inline int Math::Clamp(int x, int a, int b)
{
    return (x < a ? a : (x > b ? b : x));
}

/*!
\brief Update the minimum and maximum values given a double value.
\param x Input value.
\param a, b Lower and upper bounds that will be updated according to x.
*/
inline void Math::SetMinMax(const double& x, double& a, double& b)
{
    a = Math::Min(a, x);
    b = Math::Max(b, x);
}

/*!
\brief Minimum of two reals.
\param a, b Real values.
*/
inline double Math::Min(const double& a, const double& b)
{
    return (a < b ? a : b);
}

/*!
\brief Maximum of two reals.
\param a, b Real values.
*/
inline double Math::Max(const double& a, const double& b)
{
    return (a > b ? a : b);
}

/*!
\brief Maximum of three reals.
\param a, b, c Real values.
*/
inline double Math::Max(const double& a, const double& b, const double& c)
{
    return Math::Max(Math::Max(a, b), c);
}

/*!
\brief Minimum of three reals.
\param a, b, c Real values.
*/
inline double Math::Min(const double& a, const double& b, const double& c)
{
    return Math::Min(Math::Min(a, b), c);
}

/*!
\brief Maximum of four reals.
\param a, b, c, d Real values.
*/
inline double Math::Max(const double& a, const double& b, const double& c, const double& d)
{
    return Math::Max(Math::Max(a, b), Math::Max(c, d));
}

/*!
\brief Minimum of four reals.
\param a, b, c, d Real values.
*/
inline double Math::Min(const double& a, const double& b, const double& c, const double& d)
{
    return Math::Min(Math::Min(a, b), Math::Min(c, d));
}

/*!
\brief Fractional part of a real.

Implemented as:
\code
double y=x-Math::Floor(x);
\endcode
\param x Real.
*/
inline double Math::Fract(const double& x)
{
    return x - floor(x);
}

/*!
\brief Fractional part of a real.

Implemented as:
\code
double y = x - Math::Floor(x);
\endcode
\param x Real.
\param y Return floor.
*/
inline double Math::FractFloor(const double& x, double& y)
{
    y = floor(x);
    return x - y;
}

/*!
\brief %Floor function.

While it is easier to use the C++ function, this has been implemented to be consistent with Fract().
\param x Real.
*/
inline double Math::Floor(const double& x)
{
    return floor(x);
}

/*!
\brief Power.

Returns base x raised to the power exponent e if x>0, and -x raised to the power exponent e otherwise.

\param x Base.
\param ee Exponent.
*/
inline double Math::Pow(const double& x, const double& ee)
{
    if (x == 0.0)
    {
        return 0.0;
    }
    else if (x > 0.0)
    {
        return pow(x, ee);
    }
    else
    {
        return -pow(-x, ee);
    }
}

/*!
\brief Absolute value.

\param x Real.
*/
inline double Math::Abs(const double& x)
{
    return fabs(x);
}

/*!
\brief Unit real value in [0,1] from two integers.

Usefull for loops, same as:
\code
for (int i=0;i<n;i++)
{
double t=double(i)/double(n-1); // t=Math::Unit(i,n);
}
\endcode
\param i, n Integers.
*/
inline constexpr double Math::Unit(int i, int n)
{
    return double(i) / double(n - 1);
}

/*!
\brief Sigmoid-like function.

This function is more efficient than the real sigmoid function which requires the computation of the exponential.

\param s Sigma.
\param x Real.
*/
inline double Math::Sigmoid(const double& x, const double& s)
{
    return x / sqrt(s * s + x * x);
}

/*!
\brief Impulse function.

An impulse function that doesn't use exponentials, k controls the
falloff of the function.

Derivative is 2*sqrt(k)*(1-k*x*x)/(k*x*x+1), maximum is reached x = sqrt(1/k), value at maximum is 1.

\param k Control parameter.
\param x Real.
*/
inline double Math::Impulse(const double& x, const double& k)
{
    return 2.0 * sqrt(k) * x / (1.0 + k * x * x);
}

/*!
\brief Compactly supported sigmoid-like function implemented using a \f( C^1 \f) piecewise symmetric quadric.

The compact support is [-2,2] and the range [-1,1].

The quadric was obtaied by solving the Hermite Cubic contraints:
\code
Cubic c=2.0*Cubic::Compose(Cubic::Hermite(0, 0.5, 1.0, 0.0), Linear(0.5, 0.0)) <<std::endl;
\endcode

\sa Math::Sigmoid

\param x Real.
*/
inline double Math::SigmoidQuadric(const double& x)
{
    if (x > 0.0)
    {
        if (x > 2.0)
        {
            return 1.0;
        }
        else
        {
            return x * (1.0 - 0.25 * x);
        }
    }
    else
    {
        if (x < -2.0)
        {
            return -1.0;
        }
        else
        {
            return x * (1.0 + 0.25 * x);
        }
    }
}

/*!
\copydoc Quadric::Warp(const double&)
*/
inline double Math::Warp(const double& x)
{
    const double y = 2.0 * x;
    if (x < 0.5)
    {
        return 0.5 * sqrt(y);
    }
    else
    {
        return 1.0 - 0.5 * sqrt(2.0 - y);
    }
}

/*!
\brief Unit interval gain function.

Remaps the unit interval into the unit interval by expanding the sides and compressing the center, keeping 1/2 mapped to 1/2.

This function uses calls to pow and is computationally intensive.

\sa Quadric::Warp(const double&)
\param x Real in [0,1].
\param k Exponent.
*/
inline double Math::Warp(const double& x, const double& k)
{
    const double a = 0.5 * pow(2.0 * ((x < 0.5) ? x : 1.0 - x), k);
    return (x < 0.5) ? a : 1.0 - a;
}

/*!
\brief Check if two reals have the same signs.
\param a,b Reals.
*/
inline bool Math::SameSign(const double& a, const double& b)
{
    return signbit(a) == signbit(b);
}

/*!
\brief Check if three reals have the same signs.
\param a,b,c Reals.
*/
inline bool Math::SameSign(const double& a, const double& b, const double& c)
{
    return (signbit(a) == signbit(b)) && (signbit(a) == signbit(c));
}

/*!
\brief Composes a real with the magnitude of x and the sign of y.
\param x Real.
\param y Real whose sign will be copied.
*/
inline double Math::CopySign(const double& x, const double& y)
{
    return copysign(x, y);
}

/*!
\brief Compute the integer sign of a real.
\param x Real.
\param t Threshold.
*/
inline int Math::IntegerSign(const double& x, const double& t)
{
    if (x < -t)
    {
        return -1;
    }
    else if (x > t)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*!
\brief Compute integer and fractionnal parts.
\param x Real.
\param i Returned integer part.
\return Fractional part.
*/
inline double Math::Fract(const double& x, double& i)
{
    i = floor(x);
    return (x - i);
}

/*!
\brief Compute integer and fractionnal parts.
\param x Real.
\param a Modulus.
\param i Returned integer part.
\return Fractional part.
*/
inline double Math::Fract(const double& x, const double& a, double& i)
{
    double y = x / a;
    i = floor(y);
    return (y - i) * a;
}

/*!
\brief Compute integer and fractionnal parts given an interval
\param x Real.
\param a,b Interval.
\param i Returned integer part.
\return Fractional part.
*/
inline double Math::Fract(const double& x, const double& a, const double& b, double& i)
{
    return Math::Fract(x - a, b - a, i);
}

/*!
\brief Swap two reals.
\param a,b Arguments.
*/
inline void Math::Swap(double*& a, double*& b)
{
    double* t = a;
    a = b;
    b = t;
}

/*!
\brief Swap two integers.
\param a,b Arguments.
*/
inline void Math::Swap(int*& a, int*& b)
{
    int* t = a;
    a = b;
    b = t;
}

/*!
\brief %Gaussian.
\param x Real.
\param s Sigma.
*/
inline double Math::Gaussian(const double& x, const double& s)
{
    return exp(-0.5 * (x * x) / (s * s));
}

/*!
\brief %Gaussian probability density function.
\param x Real.
\param s Sigma.
*/
inline double Math::GaussianProbability(const double& x, const double& s)
{
    return exp(-0.5 * (x * x) / (s * s)) / (s * sqrt(Math::TwoPi));
}

/*!
\brief Compact support Gaussian approximation.

The compact support has a radius r = 3 sigma.


\param x Real.
\param s Sigma.
*/
inline double Math::CompactGaussian(const double& x, const double& s)
{
    double y = x / (3.0 * s);
    if (y > 1.0) return 0.0;

    y = 1.0 - y * y;

    // Raise to power 8
    y *= y;
    y *= y;
    y *= y;
    return y;
}

/*!
\brief Step function.
\param e Step position.
\param x Variable.
*/
inline double Math::Step(double e, double x)
{
    return (x < e) ? 0.0 : 1.0;
}

/*!
\brief Generalized C1 polynomial smoothing function between two distances.

This is a general version of the union, intersection and difference function of Media Molecule
using in the Dreams video game.

The function is not associative.

\param a, b Distances.
\param sr Smoothing radius.
*/
inline double Math::SmoothingQuadric(double a, double b, double sr)
{
    double h = Math::Max(sr - Math::Abs(a - b), 0.0) / sr;
    return Math::Min(a, b) - h * h * sr * 0.25;
}

/*!
\brief Generalized C2 polynomial smoothing function between two distances.

The function is not associative.

\param a, b Distances.
\param sr Smoothing radius.
*/
inline double Math::SmoothingCubic(double a, double b, double sr)
{
    double h = Math::Max(sr - Math::Abs(a - b), 0.0) / sr;
    return h * h * h * sr * (1.0 / 6.0);
}

/*!
\brief Cardinal sine.
\param x Real.
\param normalized If true, then the function is normalized to the interval \f([-1,1]\f) instead of \f([-\pi,\pi]\f).

Use the first order Taylor expansion \f( sin(x)/x \approx 1 - x^2/6 + x^4/120 + o(x^4)\f) for small values to avoid numerical issues.
*/
inline double Math::SinCard(double x, bool normalized)
{
    static constexpr double epsilon = 1e-8;

    if (normalized == false)
    {
        if (fabs(x) < epsilon)
        {
            // Taylor expansion:
            const double xx = x * x;
            return 1.0 - xx / 6.0 + xx * xx / 120.0;
        }

        return sin(x) / x;
    }
    else
    {

        const double t = Math::Pi * x;
        if (fabs(x) < epsilon)
        {
            const double tt = t * t;
            return 1.0 - tt / 6.0 + tt * tt / 120.0;
        }

        return sin(t) / (t);
    }
}
