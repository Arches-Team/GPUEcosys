// Core class

#pragma once
#include "evector.h"

// Random number generator
class Random {
protected:
    int vec[48];
    int feed, tap, borrow;
    int V[571];
    int Y;
public:
    Random(int = 239);
    ~Random();

    void Seed(int);

    // Uniform distributions
    double Uniform();
    double Uniform(const double&);
    double Uniform(const double&, const double&);

    Vector Sphere();

public:
    static Random R239;
protected:
    int Int31();
    int SimpleInt31();
};

// Fast random number generator
class RandomFast
{
private:
    unsigned int x; //!< Variable for computing random values
public:
    RandomFast(unsigned int = 239);
    RandomFast(int);
    //! Empty
    ~RandomFast() {}

    void Seed(unsigned int);
    void Seed(int);

    unsigned int Integer();
    unsigned int Integer(int);
    double Uniform();
    double Uniform(const double&);
    double Uniform(const double&, const double&);

    int Pick(const double*, int);
};
