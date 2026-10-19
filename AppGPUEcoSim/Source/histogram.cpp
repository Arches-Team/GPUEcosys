#include "histogram.h"
#include "mathematics.h"

/*!
\class Histogram histogram.h
\brief A simple histogram.

\sa CumulatedHistogram
*/

/*!
\brief Create a histogram.
\param a,b Range.
\param size Number of bins.
*/
Histogram::Histogram(int size, double a, double b) :a(a), b(b), v(size, 0), e(size / (b - a))
{}

/*!
\brief Create a histogram from a set.
\param values Set of values.
\param a,b Range.
\param size Number of bins.
*/
Histogram::Histogram(int size, double a, double b, const QVector<double>& values) :Histogram(size, a, b)
{
    for (const double& x : values)
    {
        v[Index(x)]++;
    }
}

/*!
\brief Create a histogram.
\param size Number of bins.
\param values Set of values.
*/
Histogram::Histogram(int size, const QVector<double>& values) :Histogram(size, 0.0, 0.0)
{
    if (size == 0) return;

    a = values.at(0);
    b = values.at(0);

    for (const double& x : values)
    {
        Math::SetMinMax(x, a, b);
    }

    e = size / (b - a);

    for (const double& x : values)
    {
        v[Index(x)]++;
    }
}

/*!
\brief Overloaded.
\param s Stream.
\param h The histogram.
*/
std::ostream& operator<<(std::ostream& s, const Histogram& h)
{
    s << "Histogram(" << h.v.size() << ", " << h.a << ", " << h.b;

    for (int i : h.v)
    {
        s << ", " << std::endl << i;
    }
    s << ")" << std::endl;
    return s;
}
