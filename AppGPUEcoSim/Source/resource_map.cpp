#include "resource_map.h"
#include "abiotics.h"
#include <QtGui/QPainter>
#include <QtCore/QDebug>

ResourceMapMonthly::ResourceMapMonthly(const Box2& box, const QVector<QImage>& imgs, const QVector<double>& minV, const QVector<double>& maxV, const QVector<double>& simParams)
    : box(box), imgs(imgs), minV(minV), maxV(maxV), simParams(simParams)
{
    imgEdited.resize(12,true);
    sfEdited.resize(12, false);
    sfs.resize(12);

    SyncScalarFields();
}

void ResourceMapMonthly::SyncScalarFields()
{
    for (int i = 0; i < 12; ++i)
        if (imgEdited[i])
        {
            sfs[i] = ScalarField2(box, imgs[i], minV[i], maxV[i]);
            imgEdited[i] = false;
        }
}

void ResourceMapMonthly::SyncImages()
{
    for (int i = 0; i < 12; ++i)
        if (sfEdited[i])
        {
            sfs[i].GetRange(minV[i], maxV[i]);
            imgs[i] = sfs[i].CreateImage(minV[i], maxV[i]);
            sfEdited[i] = false;
        }
}

const QImage& ResourceMapMonthly::GetImage(int i) const
{
    assert(i >= 0 && i < 12);
    return imgs[i];
}

QImage& ResourceMapMonthly::GetImage(int i)
{
    assert(i >= 0 && i < 12);
    imgEdited[i] = true;
    return imgs[i];
}

QPixmap ResourceMapMonthly::GetPixmap(int i) const
{
    assert(i >= 0 && i < 12);
    return QPixmap::fromImage(imgs[i].scaled(141, 141).flipped(Qt::Orientation::Horizontal));
}

const ScalarField2& ResourceMapMonthly::GetScalarField(int i) const
{
    assert(i >= 0 && i < 12);
    return sfs[i];
}

ScalarField2& ResourceMapMonthly::GetScalarField(int i)
{
    assert(i >= 0 && i < 12);
    sfEdited[i] = true;
    return sfs[i];
}

QVector<double>& ResourceMapMonthly::GetSimParams()
{
    return simParams;
}

void ResourceMapMonthly::SetSimParams(const QVector<double>& params)
{
    simParams = params;
}


void ResourceMapMonthly::GetMinMax(int month, double& min, double& max) const
{
    assert(month >= 0 && month < 12);
    min = minV[month];
    max = maxV[month];
}

ResourceMapYearly::ResourceMapYearly(const Box2& box, const QImage& img, double minV, double maxV)
    : box(box), img(img), minV(minV), maxV(maxV), imgEdited(true), sfEdited(false)
{
    SyncScalarField();
}

void ResourceMapYearly::GetMinMax(double& min, double& max) const
{
    min = minV;
    max = maxV;
}

void ResourceMapYearly::SyncScalarField()
{
    if (imgEdited)
    {
        sf = ScalarField2(box, img, minV, maxV);
        imgEdited = false;
    }
}

void ResourceMapYearly::SyncImage()
{
    if (sfEdited)
    {
        img = sf.CreateImage(minV, maxV);
        sfEdited = false;
    }
}

const QImage& ResourceMapYearly::GetImage() const
{
    return img;
}

QImage& ResourceMapYearly::GetImage()
{
    imgEdited = true;
    return img;
}

QPixmap ResourceMapYearly::GetPixmap() const
{
    return QPixmap::fromImage(img.scaled(141, 141).flipped(Qt::Orientation::Horizontal));
}

const ScalarField2& ResourceMapYearly::GetScalarField() const
{
    return sf;
}

ScalarField2& ResourceMapYearly::GetScalarField()
{
    sfEdited = true;
    return sf;
}


SunlightMap::SunlightMap(const Box2& box, const QVector<QImage>& imgs, const QVector<double>& minV, const QVector<double>& maxV, const QVector<double>& simParams)
    : ResourceMapMonthly(box, imgs, minV, maxV, simParams)
{
}

void SunlightMap::Regenerate(const HeightField& heightField)
{
    SunlightSimulator sun;
    sun.Simulate(heightField, 100, 1024);
    sfs = sun.TotalSunlight(simParams);
}

MoistureMap::MoistureMap(const Box2& box, const QImage& imgs, double minV, double maxV, const QVector<double>& precipitations)
    : ResourceMapYearly(box, imgs, minV, maxV), precipitations(precipitations)
{
}

void MoistureMap::Regenerate(const HeightField& heightField)
{
    sf = Moisture::ProxySMI(heightField);
}

QVector<double>& MoistureMap::GetPrecipitations()
{
    return precipitations;
}

void MoistureMap::SetPrecipitations(const QVector<double>& p)
{
    precipitations = p;
}

TemperatureMap::TemperatureMap(const Box2& box, const QImage& img, const QVector<double>& minT, const QVector<double>& maxT)
    : ResourceMapYearly(box, img, 0, 1), minV(minT), maxV(maxT)
{
}

const QVector<double>& TemperatureMap::GetMinTemps() const
{
    return minV;
}

const QVector<double>& TemperatureMap::GetMaxTemps() const
{
    return maxV;
}

ResourceData::ResourceData(const HeightField& heightField, const SunlightMap& sunMap, const MoistureMap& moistMap, const TemperatureMap& tempMap, const MaskMap& maskMap)
    : heightField(heightField), sunMap(sunMap), moistMap(moistMap), tempMap(tempMap), maskMap(maskMap)
{
}
HeightField* ResourceData::GetPtrHeightField()
{
    return &heightField;
}

const HeightField& ResourceData::GetHeightField() 
{
    return heightField;
}
const ScalarField2& ResourceData::GetSlope() const
{
    return slope;
}
SunlightMap& ResourceData::GetSunMap() 
{
    return sunMap;
}
MoistureMap& ResourceData::GetMoistMap() 
{
    return moistMap;
}
TemperatureMap& ResourceData::GetTempMap() 
{
    return tempMap;
}

WindMap& ResourceData::GetWindMap()
{
    return windMap;
}

MaskMap& ResourceData::GetMaskMap()
{
    return maskMap;
}



ScalarField2& ResourceData::GetScalarField(MapType type, int m)
{
    switch (type)
    {
    default:
    case MapType::TERRAIN:
        return heightField;
    case MapType::SUNLIGHT:
        return sunMap.GetScalarField(m);
    case MapType::MOISTURE:
        return moistMap.GetScalarField();
    case MapType::TEMPERATURE:
        return tempMap.GetScalarField();
    case MapType::MASK:
        return maskMap.GetScalarField();
    }
}


void ResourceData::GetMinMax(MapType type, int month, double& min, double& max) const
{
    switch (type)
    {
    case MapType::TERRAIN:
        heightField.GetRange(min, max);
        break;
    case MapType::SUNLIGHT:
        sunMap.GetMinMax(month, min, max);
        break;
    case MapType::MOISTURE:
        moistMap.GetMinMax(min, max);
        break;
    case MapType::TEMPERATURE:
        tempMap.GetMinMax(min, max);
        break;
    case MapType::MASK:
        maskMap.GetMinMax(min, max);
        break;
    default:
        min = 0;
        max = 1;
    }
}

void ResourceData::GetMinMaxTool(MapType type, int month, double& min, double& max) const
{
    switch (type)
    {
    case MapType::TERRAIN:
        min = 0.;
        max = 300.;
        break;
    case MapType::SUNLIGHT:
        sunMap.GetMinMax(month, min, max);
        min = 0.;
        break;
    case MapType::MOISTURE:
        moistMap.GetMinMax(min, max);
        min = 0.;
        break;
    case MapType::TEMPERATURE:
        tempMap.GetMinMax(min, max);
        break;
    case MapType::MASK:
        maskMap.GetMinMax(min, max);
        break;
    default:
        min = 0.;
        max = 1.;
    }
}



/*!
 * \brief ResourceData::SetHeightField
 * \param hf
 * \details Set the height field and regenerate the maps
 */
void ResourceData::SetHeightField(const HeightField& hf)
{
    heightField = hf;
    heightFieldImg = heightField.CreateImage(1000., 2000.,true);
}

/*!
 * \brief ResourceData::SetSlope
 * \param s
 * \details Set the slope field
 */
void ResourceData::SetSlope(const ScalarField2& s)
{
    slope = s;
}

void ResourceData::SetWindMap(const WindMap& wm)
{
    windMap = wm;
}

/*!
 * \brief ResourceData::GetImage
 * \param type
 * \param month
 * \return QImage for the specified map type and month
 */
const QImage& ResourceData::GetImage(MapType type, int month) 
{
    switch (type)
    {
    default:
    case MapType::TERRAIN:
        return heightFieldImg;
    case MapType::SUNLIGHT:
        return sunMap.GetImage(month);
    case MapType::MOISTURE:
        return moistMap.GetImage();
    case MapType::TEMPERATURE:
        return tempMap.GetImage();
    case MapType::MASK:
        return maskMap.GetImage();
    }
}

/*!
 * \brief ResourceData::GetPixmap
 * \param type
 * \param month
 * \return QPixmap for the specified map type and month
 */
QPixmap ResourceData::GetPixmap(MapType type, int month)
{
    static const QPixmap emptyPixmap; // Objet QPixmap statique pour les cas invalides

    switch (type)
    {
    case MapType::TERRAIN:
        return QPixmap::fromImage(heightFieldImg.scaled(141, 141).flipped(Qt::Orientation::Horizontal));
    case MapType::SUNLIGHT:
        return sunMap.GetPixmap(month);
    case MapType::MOISTURE:
        return moistMap.GetPixmap();
    case MapType::TEMPERATURE:
        return tempMap.GetPixmap();
    case MapType::MASK:
        return maskMap.GetPixmap();
    default:
        return emptyPixmap;
    }
}


void ResourceData::SyncImages()
{
    heightFieldImg = heightField.CreateImage(1000., 2000., true);
    sunMap.SyncImages();
    moistMap.SyncImage();
    tempMap.SyncImage();
    windMap.SyncImage();
    maskMap.SyncImage();
}


MaskMap::MaskMap(const Box2& box, const QImage& img)
    : ResourceMapYearly(box, img, 0, 1)
{
}

WindMap::WindMap(const Box2& box, const QImage& img, double m, double M)
    : ResourceMapYearly(box, img, m, M),
    isSet(!img.isNull())
{
}
