#include "ecosystem-data.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtWidgets/qfiledialog.h>

EcosystemData::EcosystemData(const QVector<SpeciesParam>& params) : plants(params.size())
{
    for (int i = 0; i < params.size(); ++i)
        plants[i].SetParam(params[i]);
}

void EcosystemData::SetSpeciesParam(int i, const SpeciesParam& param) 
{ 
    plants[i].SetParam(param); 
}
const SpeciesParam& EcosystemData::GetSpeciesParam(int i) const 
{ 
    return plants[i].GetParam(); 
}

const Allometry& EcosystemData::GetAllometry(int i) const
{
    return plants[i].GetAllom();
}

Allometry& EcosystemData::GetAllometry(int i)
{
    return plants[i].GetAllom();
}

void EcosystemData::SetData(int i, const SpeciesData& data) 
{ 
    plants[i].SetData(data); 
    plants[i].SyncImages(); 
}
SpeciesData& EcosystemData::GetData(int i)
{ 
    return plants[i].GetData(); 
}
const SpeciesData& EcosystemData::GetData(int i) const 
{ 
    return plants[i].GetData(); 
}

void EcosystemData::SetLIF(const ScalarField2& sf) 
{ 
    sf_lif = sf; 
    lif = sf.CreateImage(0.,1.); 
}


void EcosystemData::SyncMatureTrees()
{
    sf_age = GetData(0).sf_trees;
    sf_species = GetData(0).sf_trees;
    for (int i = 0; i < sf_age.VertexSize(); ++i)
    {
        sf_species[i] = -1;
        for (int s = 0; s < plants.size(); ++s)
        {
            double height = GetData(s).sf_trees.at(i);
            if (height >= 1.)
            {
                sf_species[i] = s;
                const SpeciesParam& param = GetSpeciesParam(s);
                double age = (param.a_max / param.q) * log(2 / ((height / param.hmax) + 1) - 1);
                sf_age[i] = age;
            }
        }
    }
}

void EcosystemData::clear()
{
    plants.clear();
}

void PlantData::Delete(const ScalarField2& mask)
{
#pragma omp parallel for
    for (int i = 0; i < mask.GetSizeX(); ++i)
        for (int j = 0; j < mask.GetSizeY(); ++j)
        {
            if (mask.at(i, j) > 0) {
                plantData.sf_trees(i, j) = 0;
                plantData.sf_regenLayers(i, j) = 0;
            }
        }

    SyncImages();
}

void EcosystemData::Delete(const ScalarField2& mask)
{
    for (auto& p : plants)
        p.Delete(mask);
}

int EcosystemData::size() const { return plants.size(); }

QPixmap EcosystemData::GetPixmap(int i, MapSpecies mtype) const
{
    static const QPixmap emptyPixmap; // Objet QPixmap statique pour les cas invalides

    switch (mtype)
    {
    case MapSpecies::SEED_PROBS:
        return QPixmap::fromImage(plants[i].GetData().seedProbs.scaled(141, 141).flipped(Qt::Orientation::Horizontal));
    case MapSpecies::REGEN_LAYERS:
        return QPixmap::fromImage(plants[i].GetData().regenLayers.scaled(141, 141).flipped(Qt::Orientation::Horizontal));
    case MapSpecies::TREES:
        return QPixmap::fromImage(plants[i].GetData().trees.scaled(141, 141).flipped(Qt::Orientation::Horizontal));
    case MapSpecies::BOOST:
        return QPixmap::fromImage(plants[i].GetData().boost.scaled(141, 141).flipped(Qt::Orientation::Horizontal));
    //case MapSpecies::VIGOR:
    //  return QPixmap::fromImage(plants[i].GetData().vigor.scaled(141, 141).flipped(Qt::Orientation::Horizontal));
    case MapSpecies::LIF:
        return QPixmap::fromImage(lif.scaled(141, 141).flipped(Qt::Orientation::Horizontal));
    default:
        return emptyPixmap;
    }
}

const QImage& EcosystemData::GetImage(MapSpecies mtype, int i) const
{
    static const QImage emptyImage; // Objet QImage statique pour les cas invalides

    switch (mtype)
    {
    case MapSpecies::SEED_PROBS:
        return plants[i].GetData().seedProbs;
    case MapSpecies::REGEN_LAYERS:
        return plants[i].GetData().regenLayers;
    case MapSpecies::TREES:
        return plants[i].GetData().trees;
    case MapSpecies::BOOST:
        return plants[i].GetData().boost;
    //case MapSpecies::VIGOR:
    //  return plants[i].GetData().vigor;
    case MapSpecies::LIF:
        return lif;
    default:
        return emptyImage;
    }
}

QImage& EcosystemData::GetImage(MapSpecies mtype, int i)
{
    switch (mtype)
    {
    case MapSpecies::SEED_PROBS:
        return plants[i].GetData().seedProbs;
    case MapSpecies::REGEN_LAYERS:
        return plants[i].GetData().regenLayers;
    case MapSpecies::TREES:
        return plants[i].GetData().trees;
    case MapSpecies::BOOST:
        return plants[i].GetData().boost;
    //case MapSpecies::VIGOR:
    //  return plants[i].GetData().vigor;
    default:
    case MapSpecies::LIF:
        return lif;
    }
}


ScalarField2& EcosystemData::GetScalarField(MapSpecies mtype, int i)
{
    switch (mtype)
    {
    case MapSpecies::SEED_PROBS:
        return plants[i].GetData().sf_seedProbs;
    case MapSpecies::REGEN_LAYERS:
        return plants[i].GetData().sf_regenLayers;
    case MapSpecies::TREES:
        return plants[i].GetData().sf_trees;
    case MapSpecies::BOOST:
        return plants[i].GetData().sf_boost;
        //case MapSpecies::VIGOR:
        //  return plants[i].GetData().vigor;
    default:
    case MapSpecies::LIF:
        return sf_lif;
    }
}

void EcosystemData::GetMinMax(int i, MapSpecies mtype, double& min, double& max) const
{
        min = 0.;
        max = 0.;

        switch (mtype)
        {
        case MapSpecies::SEED_PROBS:
            min = 0.;
            max = 30.;
            break;
        case MapSpecies::REGEN_LAYERS:
            min = 0.;
            max = 4.;
            break;
        case MapSpecies::TREES:
            min = 0.; // TODO Celian passage a 0 au lieu de 4 car sinon toutes les valeurs sont a 4 dans l'image
            max = plants[i].GetParam().hmax;
            break;
        case MapSpecies::BOOST:
            min = -1;
            max = 1.;
            break;
        case MapSpecies::LIF:
            min = 0.;
            max = 1.;
            break;
        }
}

void EcosystemData::SyncScalarFields()
{
    for (PlantData& p : plants)
    {
        p.SyncScalarFields();
    }
}

void EcosystemData::SyncImages()
{
    for (PlantData& p : plants)
    {
        p.SyncImages();
    }
}


QJsonObject PlantData::SaveSpeciesParam(const QString& dirName) const
{
    QJsonObject content;
    content.insert("species", plantParam.species);
    content.insert("name", plantParam.name);
    content.insert("color", plantParam.col.GetQt().name());
    content.insert("model", plantParam.model);
    content.insert("hModel", plantParam.hModel);
    content.insert("allometryFile", QDir(dirName).relativeFilePath(plantParam.allometryFile));
    content.insert("hmax", plantParam.hmax);
    content.insert("q", plantParam.q);
    content.insert("k1", plantParam.k1);
    content.insert("k2", plantParam.k2);
    content.insert("k3", plantParam.k3);
    content.insert("c_s", plantParam.c_s);
    content.insert("r_s", plantParam.r_s);
    content.insert("c_m", plantParam.c_m);
    content.insert("r_m", plantParam.r_m);
    content.insert("c_t", plantParam.c_t);
    content.insert("r_t", plantParam.r_t);
    content.insert("c_sl", plantParam.c_sl);
    content.insert("r_sl", plantParam.r_sl);
    content.insert("gr", plantParam.gr);
    content.insert("pa_max", plantParam.pa_max);
    content.insert("a_max", plantParam.a_max);
    content.insert("a", plantParam.a);
    content.insert("ct", plantParam.ct);
    content.insert("longGrowthSeason", plantParam.longGrowthSeason);
    content.insert("enabled", plantParam.enabled);
    content.insert("p_b", plantParam.p_b);

    content.insert("qVal", allometry.qVal);
    content.insert("sdi", allometry.sdi);
    content.insert("radMax", allometry.radMax);
    content.insert("dbhMax", allometry.dbhMax);
    content.insert("radSlope", allometry.radSlope);
    content.insert("dbhSlope", allometry.dbhSlope);

    double a, b;
    plantData.sf_boost.GetRange(a, b);
    if (b > 0)
    {
        QString fileName = "Boost_" + plantParam.name + ".png";
        content.insert("boost", fileName);
        plantData.boost.save(dirName + "/" + fileName);
    }

    return content;
}

void PlantData::LoadSpeciesParam(QJsonObject obj, const QString& dirName)
{
    plantParam.species = obj["species"].toString();
    plantParam.name = obj["name"].toString();
    plantParam.col = Color(QColor(obj["color"].toString()));
    plantParam.model = obj["model"].toString();
    plantParam.hModel = obj["hModel"].toDouble();
    // Relative to the preset file, like the boost images. Presets written before that were
    // relative to the old working directory (AppGPUEcoSim/), so fall back to it.
    QString allometryFile = obj["allometryFile"].toString();
    plantParam.allometryFile = QDir(dirName).filePath(allometryFile);
    if (!QFileInfo::exists(plantParam.allometryFile))
        plantParam.allometryFile = QDir(SOLUTION_DIR "AppGPUEcoSim").filePath(allometryFile);
    plantParam.allometryFile = QDir::cleanPath(plantParam.allometryFile);
    plantParam.hmax = obj["hmax"].toDouble();
    plantParam.q = obj["q"].toDouble();
    plantParam.k1 = obj["k1"].toDouble();
    plantParam.k2 = obj["k2"].toDouble();
    plantParam.k3 = obj["k3"].toDouble();
    plantParam.c_s = obj["c_s"].toDouble();
    plantParam.r_s = obj["r_s"].toDouble();
    plantParam.c_m = obj["c_m"].toDouble();
    plantParam.r_m = obj["r_m"].toDouble();
    plantParam.c_t = obj["c_t"].toDouble();
    plantParam.r_t = obj["r_t"].toDouble();
    plantParam.c_sl = obj["c_sl"].toDouble();
    plantParam.r_sl = obj["r_sl"].toDouble();
    plantParam.gr = obj["gr"].toDouble();
    plantParam.pa_max = obj["pa_max"].toDouble();
    plantParam.a_max = obj["a_max"].toDouble();
    plantParam.a = obj["a"].toDouble();
    plantParam.ct = obj["ct"].toDouble();
    plantParam.longGrowthSeason = obj["longGrowthSeason"].toBool();
    plantParam.enabled = obj["enabled"].toBool();
    plantParam.p_b = obj["p_b"].toDouble();

    allometry.qVal = obj["qVal"].toDouble();
    allometry.sdi = obj["sdi"].toDouble();
    allometry.lifespan = obj["a_max"].toDouble();
    allometry.hMax = obj["hmax"].toDouble();
    allometry.radMax = obj["radMax"].toDouble();
    allometry.dbhMax = obj["dbhMax"].toDouble();
    allometry.radSlope = obj["radSlope"].toDouble();
    allometry.dbhSlope = obj["dbhSlope"].toDouble();

    if (obj.contains("boost"))
    {
        QString fileName = dirName + "/" + obj["boost"].toString();

        plantData.boost = QImage(fileName);
        plantData.sf_boost = ScalarField2(plantData.sf_boost.GetBox(), plantData.boost, 0., 1.);
    }
}

void PlantData::LoadAllometry()
{
    QFile file(plantParam.allometryFile);

    //qDebug() << plantParam.name;
    
    if (file.open(QFile::ReadOnly | QIODevice::Text))
    {
        QTextStream ts(&file);

        allometry.h.clear();
        allometry.rad.clear();
        allometry.dbh.clear();
        allometry.branchLen.clear();
        allometry.nbBranches.clear();

        int nbLines;
        ts >> nbLines;
        for (int i = 0; i < nbLines; ++i)
        {
            int a, n;
            float r, h, d, l;
            ts >> a >> r >> h >> d >> n >> l;

            //qDebug() << "h: " << h << " rad: " << r << " dbh: " << d << " branchLen: " << l << " nbBranches: " << n;

            allometry.h.push_back(h);
            allometry.rad.push_back(r);
            allometry.dbh.push_back(d);
            allometry.branchLen.push_back(l);
            allometry.nbBranches.push_back(n);
        }

        // assumption is that radius, height, dbh allometry values never decrease, so fix if necessary
        for (int y = 1; y < nbLines; y++)
        {
            if (allometry.rad[y] < allometry.rad[y - 1])
                allometry.rad[y] = allometry.rad[y - 1] + 0.0001f;
            if (allometry.h[y] < allometry.h[y - 1])
                allometry.h[y] = allometry.h[y - 1] + 0.0001f;
            if (allometry.dbh[y] < allometry.dbh[y - 1])
                allometry.dbh[y] = allometry.dbh[y - 1] + 0.0001f;
        }

        int maxAllomAge = allometry.h.size();
        if (plantParam.a_max > maxAllomAge)
        {
            int ma = maxAllomAge - 1;
            // calculate maxima for dbh, branch length and num branches

            // growth difference between maximum sampled allometry and maximum age
            double growgap = allometry.GrowthFn(1.0f) / allometry.GrowthFn(maxAllomAge / plantParam.a_max);

            // maxima by proportional increase of 100 year values
            allometry.blenMax = growgap * allometry.branchLen[ma];
            allometry.numbMax = (int)(growgap * allometry.nbBranches[ma]);
        }
    }
    else
    {
        qDebug() << "Error: LoadAllometry(): couldn't read from file: '" << plantParam.allometryFile << "'";
    }
}

void EcosystemData::SaveSpeciesParam(const QString& filename)
{
    QFileInfo fi(filename);
    // For all Instances
    QJsonObject content;
    QJsonArray arrayJson;
    for (const PlantData& plant : plants)
    {
        QJsonObject object = plant.SaveSpeciesParam(fi.absolutePath());
        if (!object.isEmpty())
            arrayJson.append(object);
    }

    content.insert("ObjectsCollection", arrayJson);

    QJsonDocument document;
    document.setObject(content);
    QByteArray bytes = document.toJson(QJsonDocument::Indented);

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QTextStream iStream(&file);
        iStream << bytes;
        file.close();
    }
    else
    {
        std::cout << "file open failed: " << filename.toStdString() << std::endl;
    }
}

void EcosystemData::SaveSpeciesParam()
{
    QString filename = QFileDialog::getSaveFileName(nullptr, "Save Species Parameters", SOLUTION_DIR "data/Presets/", "JSON files (*.json)");
    if (filename.isEmpty())
        return;

    SaveSpeciesParam(filename);
}

void EcosystemData::LoadSpeciesParam(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::cout << "file open failed: " << filename.toStdString() << std::endl;
        return;
    }
    QFileInfo fi(file);

    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject object = document.object();

    QJsonArray array = object["ObjectsCollection"].toArray();
    for (int i = 0; i < array.size(); ++i)
    {
        QJsonObject obj = array[i].toObject();
        
        PlantData plant;
        plant.LoadSpeciesParam(obj, fi.absolutePath());
        plant.LoadAllometry();

        plants.append(plant);
    }

}

void EcosystemData::LoadLIPs(const QDir& dir)
{
    for (int i = 0; i < plants.size(); ++i)
    {
        SpeciesParam data = plants[i].GetParam();
        QDir lipsDir = dir;
        if (lipsDir.cd(data.species))
        {
            data.lips.clear();

            QStringList filesList = lipsDir.entryList(QDir::Files);
            QString fileName;
            int sMax = 0;
            foreach(fileName, filesList)
            {
                QString height = fileName.split('_')[1];
                //qDebug() << "Reading: " << lipsDir.filePath(fileName) << ", Height = " << height.toDouble();
                LIP lip;
                lip.height = height.toDouble();
                lip.lip = ScalarField2(lipsDir.filePath(fileName));
                sMax = std::max(sMax, lip.lip.GetSizeX());

                data.lips.push_back(lip);

                double a, b;
                lip.lip.GetRange(a, b);
                //qDebug() << a << ", " << b;
            }
            std::sort(data.lips.begin(), data.lips.end(), [](const LIP& a, const LIP& b) { return a.height < b.height; });

            //qDebug() << data.species << ": loaded " << data.lips.size() << "LIPs";
            //qDebug() << "Max patch size: " << sMax;

            plants[i].SetParam(data);
        }
        else
        {
            qDebug() << "LoadLIPs: couldn't find dir: " << data.species;
        }
    }
}

void EcosystemData::LoadSpeciesParam()
{
    QString filename = QFileDialog::getOpenFileName(nullptr, "Load Species Parameters", SOLUTION_DIR "data/Presets/", "JSON files (*.json)");
    if (filename.isEmpty())
        return;

    LoadSpeciesParam(filename);
}

QVector<QString> EcosystemData::getListSpecies()
{
    QVector<QString> speciesNames;
    for (PlantData& p : plants)
    {
        speciesNames.append(p.GetParam().species);
    }

    return speciesNames;
}

double Allometry::GrowthFn(double t) const
{
    return 2 / (1 + exp(-qVal * t)) - 1;
}

double Allometry::InvGrowthFn(double n) const
{
    return -log((1 - n) / (n + 1)) / qVal;
}

// Get Nominal age from age in years

// Get age in years from nominal age

double Allometry::GetAllomAge(double height) const
{
    return InvGrowthFn(height / hMax);
}


double Allometry::GetCanopyRadiusOld(double n) const
{
    double a = n * lifespan;
    int a_min = floor(a);
    double del_a = a - a_min;

    if (a_min >= 0 && a_min < h.size() - 1)
    {
        return rad[a_min] + del_a * (rad[a_min+1] - rad[a_min]);
    }
    else
        return GrowthFn(n) * radMax;
}

double Allometry::GetCanopyRadius(double h) const
{
    return h * radSlope;
}


double Allometry::GetDBH(double n) const
{
    return n * lifespan * dbhSlope;
}



double Allometry::GetNumTreesPer4m2(double dbh) const
{
    assert(sdi != 0);

    double D_in = dbh * 100 / 2.54;
    return (sdi / pow(D_in / 10.0, 1.605)) * (4.0 / 4046.86);
}

double SpeciesParam::GrowthFunction(double age) const
{
    return ((2. / (1 + exp((age / a_max) * q))) - 1) * hmax;
}
