#pragma once
#include <QtCore/qstring.h>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <color.h>
#include <scalarfield.h>
#include <QtCore/QDebug>
#include <QtCore/QDir>

enum class MapSpecies
{
    NONE,
    SEED_PROBS,
    REGEN_LAYERS,
    TREES,
    BOOST,
    LIF
};

struct LIP
{
    ScalarField2 lip;
    double height;
};

// Ressource: https://www.wur.nl/en/research-results/chair-groups/environmental-sciences/forest-ecology-and-forest-management-group/education/tree-database/temperate-species.htm
struct SpeciesParam
{
    QString species;

    QString name;
    Color col; // Display color

    QString model;
    double hModel;

    QString allometryFile;

    double hmax; // Max attainable tree height
    double q; // Growth function parameter

    double k1, k2, k3; // Seed dispersal params

    // response to sunlight
    double c_s;
    double r_s;

    // response to moisture
    double c_m;
    double r_m;

    // response to temperature
    double c_t;
    double r_t;

    // response to slope (in radians)
    double c_sl;
    double r_sl;

    double gr; // growth rate

    double pa_max; // Percentage of individuals that reach max age
    double a_max; // Max Age

    double a; // Maximum stress value

    double ct; // Crown transparency
    double p_b; // Browsing probability

    bool longGrowthSeason = false;
    bool enabled = true;

    QVector<LIP> lips;

    double GrowthFunction(double age) const;
};

struct SpeciesData
{
    QImage seedProbs; // Seed probabilities
    ScalarField2 sf_seedProbs = ScalarField2(); // Seed probabilities

    QImage regenLayers; // Regeneration layers
    ScalarField2 sf_regenLayers = ScalarField2(); // Regeneration layers

    QImage trees;    // Individual Trees
    ScalarField2 sf_trees = ScalarField2(); // Individual Trees

    QImage boost;    // Individual Trees
    ScalarField2 sf_boost = ScalarField2(); // Individual Trees

    ScalarField2 sf_carbon = ScalarField2(); // Individual Trees
};

class Allometry
{
public:
    QVector<double> h;    //< tree height, all units in metres
    QVector<double> rad;     //< canopy radius
    QVector<double> dbh;     //< diameter at breast height
    QVector<double> branchLen;   //< average length of primary branches
    QVector<int> nbBranches;   //< number of primary branches

    // For extrapolation
    double qVal = 0;
    int lifespan = 0;
    double sdi = 0; // Ref: https://oakmissouri.org/nrbiometrics/topics/maxsdi.html

    // Max values
    double hMax = 0;

    double radMax = 0;
    double radSlope = 0;

    double dbhMax = 0;
    double dbhSlope = 0;

    double blenMax = 0;
    double numbMax = 0;

public:
    double GrowthFn(double t) const;
    double InvGrowthFn(double n) const;

    double GetAllomAge(double height) const;

    double GetCanopyRadiusOld(double n) const;
    double GetCanopyRadius(double h) const;
    double GetDBH(double n) const;

    double GetNumTreesPer4m2(double dbh) const;
};

class PlantData
{
protected:
    bool isParamEdited = false;
    SpeciesParam plantParam; // Plant parameters to initialize the species

    bool isDataEdited = false;
    SpeciesData plantData; // Plant data to display the species

    Allometry allometry;

public:
    PlantData() = default;
    PlantData(const SpeciesParam& param) : plantParam(param) {}

    void SetParam(const SpeciesParam& param) { plantParam = param; isParamEdited = true; }
    const SpeciesParam& GetParam() const { return plantParam; }
    bool IsParamEdited() const { return isParamEdited; }

    void SetData(const SpeciesData& data) { plantData = data; isDataEdited = true; }
    const SpeciesData& GetData() const { return plantData; }
    SpeciesData& GetData() { return plantData; }
    bool IsDataEdited() const { return isDataEdited; }

    const Allometry& GetAllom() const { return allometry; }
    Allometry& GetAllom() { return allometry; }

    void Delete(const ScalarField2& mask);

    void SyncImages()
    {
        std::cout << "SyncImages called\n";
        {
            plantData.seedProbs = plantData.sf_seedProbs.CreateImage(0.,2);
            plantData.regenLayers = plantData.sf_regenLayers.CreateImage(0.,4.);
            plantData.trees = plantData.sf_trees.CreateImage(0., plantParam.hmax);
            plantData.boost = plantData.sf_boost.CreateImage(-1, 1); // Attention parametres

            isDataEdited = false;
        }
    }

    void SyncScalarFields()
    {
        std::cout << "SyncImages called\n";
        {
            plantData.sf_seedProbs = ScalarField2(plantData.sf_seedProbs.GetBox(), plantData.seedProbs, 0, 2);

            plantData.sf_regenLayers = ScalarField2(plantData.sf_regenLayers.GetBox(), plantData.regenLayers, 0., 4.);
            
            plantData.sf_trees = ScalarField2(plantData.sf_trees.GetBox(), plantData.trees, 0.,  plantParam.hmax);

            plantData.sf_boost = ScalarField2(plantData.sf_boost.GetBox(), plantData.boost, -1, 1); // Attention parametres

            isDataEdited = false;
        }
    }


    QJsonObject SaveSpeciesParam(const QString&)const;
    void LoadSpeciesParam(QJsonObject obj, const QString&);

    void LoadAllometry();
};

class EcosystemData
{
protected:
    QVector<PlantData> plants;
    ScalarField2 sf_species = ScalarField2(); // Mature Tree Species
    ScalarField2 sf_age = ScalarField2(); // Mature Tree Age

    ScalarField2 sf_lif = ScalarField2(); // Light Interference Field
    QImage lif; // Light Interference Field
public:

    EcosystemData() = default;
    EcosystemData(const QVector<SpeciesParam>& params);

    void SetSpeciesParam(int i, const SpeciesParam& param);
    const SpeciesParam& GetSpeciesParam(int i) const;

    const Allometry& GetAllometry(int i) const;
    Allometry& GetAllometry(int i);

    void SetData(int i, const SpeciesData& data);
    const SpeciesData& GetData(int i) const;
    SpeciesData& GetData(int i);

    void SetLIF(const ScalarField2& sf);
    const ScalarField2& GetSFLIF() const { return sf_lif; }

    void SyncMatureTrees();
    const ScalarField2& GetSFSpecies() const { return sf_species; }
    const ScalarField2& GetSFAge() const { return sf_age; }

    void Delete(const ScalarField2& mask);

    int size() const;
    void clear();

    QPixmap GetPixmap(int i, MapSpecies mtype) const;
    const QImage& GetImage(MapSpecies mtype, int i) const;
    QImage& GetImage(MapSpecies mtype, int i);
    ScalarField2& GetScalarField(MapSpecies mtype, int i);

    void GetMinMax(int i, MapSpecies mtype, double& min, double& max) const;

    void SyncScalarFields();
    void SyncImages();

    void SaveSpeciesParam();
    void SaveSpeciesParam(const QString& filename);
    void LoadSpeciesParam();
    void LoadSpeciesParam(const QString& filename);
    void LoadLIPs(const QDir& dir);
    QVector<QString> getListSpecies();

};
