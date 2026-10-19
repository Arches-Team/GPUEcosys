#pragma once

#include <QtGui/QImage>
#include <QtCore/QDebug>
#include "scalarfield.h"
#include "heightfield.h"


enum class MapType
{
    NONE,
    TERRAIN,
    SUNLIGHT,
    MOISTURE,
    TEMPERATURE,
    MASK
};

class ResourceMapYearly
{
public:
    ResourceMapYearly() = default;
    ResourceMapYearly(const Box2&, const QImage&, double, double);

    void SyncScalarField();
    void SyncImage();

    const QImage& GetImage() const;
    QImage& GetImage();
    QPixmap GetPixmap() const;
    const ScalarField2& GetScalarField() const;
    ScalarField2& GetScalarField();

    void GetMinMax(double& min, double& max) const;

    virtual void Regenerate(const HeightField&) { qDebug() << "Not implemented..."; }

protected:
    QImage img;
    ScalarField2 sf;

    Box2 box;
    double minV;
    double maxV;

    bool imgEdited;
    bool sfEdited;
};

class ResourceMapMonthly
{
public:
    ResourceMapMonthly() = default;
    ResourceMapMonthly(const Box2&, const QVector<QImage>&, const QVector<double>&, const QVector<double>&, const QVector<double>&);

    void SyncScalarFields();
    void SyncImages();

    const QImage& GetImage(int) const;
    QImage& GetImage(int);
    QPixmap GetPixmap(int) const;
    const ScalarField2& GetScalarField(int) const;
    ScalarField2& GetScalarField(int);

    void GetMinMax(int month, double& min, double& max) const;

    QVector<double>& GetSimParams();
    void SetSimParams(const QVector<double>& params);

    virtual void Regenerate(const HeightField&) = 0;

protected:
    QVector<QImage> imgs;
    QVector<ScalarField2> sfs;

    QVector<double> simParams;

    Box2 box;
    QVector<double> minV;
    QVector<double> maxV;

    QVector<bool> imgEdited;
    QVector<bool> sfEdited;
};

class SunlightMap : public ResourceMapMonthly
{
public:
    SunlightMap() = default;
    SunlightMap(const Box2&, const QVector<QImage>&, const QVector<double>&, const QVector<double>&, const QVector<double>&);

    void Regenerate(const HeightField&) override;
};

class MoistureMap : public ResourceMapYearly
{
public:
    MoistureMap() = default;
    MoistureMap(const Box2&, const QImage&, double, double, const QVector<double>&);

    void Regenerate(const HeightField&) override;

    QVector<double>& GetPrecipitations();
    void SetPrecipitations(const QVector<double>&);

private:
    QVector<double> precipitations;
};

class TemperatureMap : public ResourceMapYearly
{
public:
    TemperatureMap() = default;
    TemperatureMap(const Box2&, const QImage&, const QVector<double>&, const QVector<double>&);

    const QVector<double>& GetMinTemps() const;
    const QVector<double>& GetMaxTemps() const;

private:
    QVector<double> minV; // Min temps
    QVector<double> maxV; // Max temps
};

class MaskMap : public ResourceMapYearly
{
public:
    MaskMap() = default;
    MaskMap(const Box2&, const QImage&);
};

class WindMap : public ResourceMapYearly
{
public:
    WindMap() = default;
    WindMap(const Box2&, const QImage&, double, double);

    bool IsSet() { return isSet; }
    
private:
    bool isSet = false;
};

class ResourceData
{
protected:
    HeightField heightField;
    QImage heightFieldImg;

    ScalarField2 slope;

    SunlightMap sunMap;
    MoistureMap moistMap;
    TemperatureMap tempMap;
    WindMap windMap;
    MaskMap maskMap;

public:
    ResourceData() = default;
    ResourceData(const HeightField&, const SunlightMap&, const MoistureMap&, const TemperatureMap&, const MaskMap&);

    HeightField* GetPtrHeightField();
    const HeightField& GetHeightField();
    const ScalarField2& GetSlope() const;
    SunlightMap& GetSunMap();
    MoistureMap& GetMoistMap();
    TemperatureMap& GetTempMap();
    WindMap& GetWindMap();
    MaskMap& GetMaskMap();

    ScalarField2& GetScalarField(MapType,int);

    void SetHeightField(const HeightField&);
    void SetSlope(const ScalarField2&);
    void SetWindMap(const WindMap&);

    void GetMinMax(MapType, int, double& min, double& max) const;
    void GetMinMaxTool(MapType, int, double& min, double& max) const;

    const QImage& GetImage(MapType, int);
    QPixmap GetPixmap(MapType, int);

    void SyncImages();
};


