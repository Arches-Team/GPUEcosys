#pragma once

#include "box.h"
#include "scalarfield.h"
#include "heightfield.h"
#include "gpu-wrapper.h"
#include <QtOpenGLWidgets/QOpenGLWidget>

namespace Moisture
{
    ScalarField2 ProxySMI(const HeightField& hf);
}

class SunlightSimulator : public QOpenGLWidget
{
public:
    SunlightSimulator(double = 48.85, Vector = Vector::X);

    void Simulate(const HeightField& hf, int minuteStep, int samples);

    void DirectSunlight(const HeightField&, int, QVector<double>&, bool = true);
    void DirectSunlight(const HeightField&, int, int, double&, bool = true);
    void DiffuseSunlight(const HeightField&, int, bool = true);

    QVector<ScalarField2> TotalSunlight(const QVector<double>&);

private:
    Vector GetSunPos(int, int) const;
    double GetAxisTiltAngle(int) const;
    double MinutesToAngle(int) const;

    void CalculateVisibility(ScalarField2&, const HeightField&, const Vector&, double, bool = true);

private:
    QVector<ScalarField2> sunMaps;

    QVector<ScalarField2> direct;
    QVector<double> sunHours;
    ScalarField2 diffuse;

    double latitude;
    Vector north;

    Vector currentSunPos;
    HeightField currentHf;

    // Constants:
    static constexpr double axisTilt = 0.408407;
    static constexpr double monthlyAxisTilt = axisTilt / 3.;
    static constexpr int halfDayInMinutes = 720;
};
