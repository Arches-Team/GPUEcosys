#include "ecosystem-simulation.h"
#include "heightfieldshader.h"
#include "histogram.h"

uint64_t Timings::GpuTotal() const
{
    return gpuCanopy + gpuStamp + gpuLif + gpuRegenLayers + gpuIndividuals + gpuSeeding
        + gpuSeedSource + gpuSeedProb;
}

Timings Timings::operator+(const Timings& other)
{
    Timings t = *this;
    t.canopy += other.canopy;
    t.individuals += other.individuals;
    t.lif += other.lif;
    t.regenLayers += other.regenLayers;
    t.seeding += other.seeding;
    t.seedProb += other.seedProb;
    t.totalYear += other.totalYear;

    t.gpuCanopy += other.gpuCanopy;
    t.gpuIndividuals += other.gpuIndividuals;
    t.gpuStamp += other.gpuStamp;
    t.gpuLif += other.gpuLif;
    t.gpuRegenLayers += other.gpuRegenLayers;
    t.gpuSeeding += other.gpuSeeding;
    t.gpuSeedSource += other.gpuSeedSource;
    t.gpuSeedProb += other.gpuSeedProb;
    return t;
}


Timings Timings::operator/(double n)
{
    Timings t = *this;
    t.canopy /= n;
    t.individuals /= n;
    t.lif /= n;
    t.regenLayers /= n;
    t.seeding /= n;
    t.seedProb /= n;
    t.totalYear /= n;

    t.gpuCanopy = uint64_t(gpuCanopy / n);
    t.gpuIndividuals = uint64_t(gpuIndividuals / n);
    t.gpuStamp = uint64_t(gpuStamp / n);
    t.gpuLif = uint64_t(gpuLif / n);
    t.gpuRegenLayers = uint64_t(gpuRegenLayers / n);
    t.gpuSeeding = uint64_t(gpuSeeding / n);
    t.gpuSeedSource = uint64_t(gpuSeedSource / n);
    t.gpuSeedProb = uint64_t(gpuSeedProb / n);
    return t;
}

EcosystemSimulation::EcosystemSimulation(ResourceData& resourceData, EcosystemData& ecosystemData)
    : resourceData(resourceData), ecosystemData(ecosystemData)
{
    box = Box2(resourceData.GetHeightField().GetBox());
    sizeX = box.Diagonal()[0];
    sizeY = box.Diagonal()[1];

    nx2 = sizeX / 2;
    ny2 = sizeY / 2;
    nx10 = (nx2 + 4) / 5;
    ny10 = (ny2 + 4) / 5;
    nx20 = (nx2 + 9) / 10;
    ny20 = (ny2 + 9) / 10;
    nbSpecies = ecosystemData.size();
    year = 0;

    globalStats.basalArea = QVector<QVector<double>>(nbSpecies);
    localStats.individualHeights = QVector<QVector<double>>(nbSpecies);
    localStats.saplingHeights = QVector<QVector<double>>(nbSpecies);
    localStats.x = 0;
    localStats.y = 0;
}

EcosystemSimulation::~EcosystemSimulation()
{
    Delete();
}

void EcosystemSimulation::Load()
{
    assert(nbSpecies > 0);

    maxLips = 0;
    maxLipRadius = 0;
    lipRadiusPerBucket.clear();
    const int lipCenter = dimLIP / 2;
    for (int i = 0; i < ecosystemData.size(); ++i)
    {
        const QVector<LIP>& lips = ecosystemData.GetSpeciesParam(i).lips;
        if (maxLips < lips.size())
            maxLips = lips.size();
        if (lipRadiusPerBucket.size() < lips.size())
            lipRadiusPerBucket.resize(lips.size());

        // Each patch is uploaded at t = (dimLIP - s) / 2, so it spans [t, t + s - 1].
        // Everything outside contributes a factor of exactly 1 to the LIF.
        for (int j = 0; j < lips.size(); ++j)
        {
            int s = lips[j].lip.GetSizeX();
            int t = (dimLIP - s) / 2;
            int r = Clamp(std::max(lipCenter - t, t + s - 1 - lipCenter), 0, lipCenter);

            lipRadiusPerBucket[j] = std::max(lipRadiusPerBucket[j], r);
            maxLipRadius = std::max(maxLipRadius, r);
        }
    }

    for (int j = 1; j < lipRadiusPerBucket.size(); ++j)
        lipRadiusPerBucket[j] = std::max(lipRadiusPerBucket[j], lipRadiusPerBucket[j - 1]);

    QString radii;
    for (int j = 0; j < lipRadiusPerBucket.size(); ++j)
        radii += QString("[%1-%2m]=%3 ").arg(j * 5).arg((j + 1) * 5).arg(lipRadiusPerBucket[j]);
    qDebug() << "LIP gather radius: " << maxLipRadius << " cells max; per height bucket: " << radii;

    UpdateTerrainShading();

    ScalarField2 slope = resourceData.GetHeightField().Slope(true).Resized(sizeX / 2, sizeY / 2);
    slope.Atan();
    resourceData.SetSlope(slope);

    Init();
    LoadParams();
    LoadResources();
}

void EcosystemSimulation::UpdateTerrainShading()
{
    HeightField hf = resourceData.GetHeightField().Resized(sizeX / 2, sizeY / 2);
    HeightFieldShader hs(hf);
    terrainShading = hs.ShadedRelief();
}

void EcosystemSimulation::Clear()
{
    Delete();
    Load();

    year = 0;
    globalStats.years.clear();
    globalStats.basalArea = QVector<QVector<double>>(nbSpecies);
    localStats.individualHeights = QVector<QVector<double>>(nbSpecies);
    localStats.saplingHeights = QVector<QVector<double>>(nbSpecies);
    localStats.x = 0;
    localStats.y = 0;
}

void EcosystemSimulation::SimulateYear(int nb, bool debug)
{

    QElapsedTimer tStep, tYear, tTotal;
    Timings times;
    std::vector<Timings> allTimes(nb);

    ResetGPUTimings();

    tTotal.restart();
    for (int i = 0; i < nb; ++i)
    {
        BeginGPUTimingFrame();
        areInstancesDirty = true;
        isNbTreesDirty = true;

        tYear.restart();

        tStep.restart();
        if (limit <= 100)
            ComputeCanopyHeight();
        times.canopy = tStep.elapsed();

        tStep.restart();
        if (limit <= 100)
            ComputeLIF();
        times.lif = tStep.elapsed();

        tStep.restart();
        StepRegenLayers();
        times.regenLayers = tStep.elapsed();

        tStep.restart();
        if (limit <= 100)
            StepIndividuals();
        times.individuals = tStep.elapsed();

        tStep.restart();
        Seeding();
        times.seeding = tStep.elapsed();

        tStep.restart();
        ComputeSeedProb();
        times.seedProb = tStep.elapsed();

        // The passes are only ordered by memory barriers now, so the host runs ahead of the
        // device. Without this the wall clock below would measure command submission, and
        // the readbacks of GetStats() / GetNbTrees() could observe an unfinished year.
        SyncEndOfYear();

        times.totalYear = tYear.elapsed();
        allTimes.at(i) = times;

        if (debug)
            GetStats();

        // The per-pass CPU figures are host submission times now that the passes are only
        // ordered by memory barriers, so they are off by default: they would be read as
        // GPU cost, and formatting them every year is itself a measurable part of the
        // wall clock. The GPU breakdown printed after the loop is the one to use.
        qDebug() << "    Year" << year << ":" << GetTimeString(times.totalYear);
        if (debug)
        {
            qDebug() << "      (host submission only, not GPU time)";
            qDebug() << "      Canopy: " << GetTimeString(times.canopy);
            qDebug() << "      LIF: " << GetTimeString(times.lif);
            qDebug() << "      RegenLayers: " << GetTimeString(times.regenLayers);
            qDebug() << "      Individuals: " << GetTimeString(times.individuals);
            qDebug() << "      Seeding: " << GetTimeString(times.seeding);
            qDebug() << "      SeedProb: " << GetTimeString(times.seedProb);
        }

        year++;
    }
    lastRunTime = GetTimeString(tTotal.elapsed());

    // The GPU queries lag the simulation by a few years, so they are only merged in here,
    // once every one of them has been harvested.
    CollectGPUTimings(allTimes);
    averageTimings = std::reduce(allTimes.begin(), allTimes.end()) / allTimes.size();

    qDebug() << "\n";
    qDebug() << "========== Total Time: ==========";
    qDebug() << "\t" << nb << " years simulated in " << lastRunTime;
    qDebug() << "\t" << GetNbTrees() << " large trees alive";

    if (averageTimings.GpuTotal() > 0)
    {
        qDebug() << "  === Average GPU time per year (GL_TIME_ELAPSED, kernels only) ===";
        qDebug() << "    Total: " << GetGpuTimeString(averageTimings.GpuTotal());
        qDebug() << "      Canopy: " << GetGpuTimeString(averageTimings.gpuCanopy);
        qDebug() << "      Stamp: " << GetGpuTimeString(averageTimings.gpuStamp);
        qDebug() << "      LIF: " << GetGpuTimeString(averageTimings.gpuLif);
        qDebug() << "      RegenLayers: " << GetGpuTimeString(averageTimings.gpuRegenLayers);
        qDebug() << "      Individuals: " << GetGpuTimeString(averageTimings.gpuIndividuals);
        qDebug() << "      Seeding: " << GetGpuTimeString(averageTimings.gpuSeeding);
        qDebug() << "      SeedSource: " << GetGpuTimeString(averageTimings.gpuSeedSource);
        qDebug() << "      SeedProb: " << GetGpuTimeString(averageTimings.gpuSeedProb);
    }
}

void EcosystemSimulation::SimulateFire(int iterations)
{
    areInstancesDirty = true;
    isNbTreesDirty = true;
    for (int i = 0; i < iterations; ++i)
        StepFire();
}

QString EcosystemSimulation::GetTimeString(int t)
{
    int m, s, ms;
    System::MsToMinutes(t, m, s, ms);
    return QString("%1:%2:%3").arg(m).arg(s, 2, 10, QChar('0')).arg(ms, 3, 10, QChar('0'));
}

QString EcosystemSimulation::GetGpuTimeString(uint64_t ns)
{
    return QString("%1 ms").arg(ns / 1.e6, 0, 'f', 3);
}

QString EcosystemSimulation::GetTimingsReport() const
{
    const Timings& t = averageTimings;

    QString r;
    r += QString("Average Year Time (wall clock): %1\n").arg(GetTimeString(t.totalYear));
    r += QString("Average GPU time per year (GL_TIME_ELAPSED, kernels only): %1\n")
        .arg(GetGpuTimeString(t.GpuTotal()));
    r += QString("  Canopy: %1\n").arg(GetGpuTimeString(t.gpuCanopy));
    r += QString("  Stamp: %1\n").arg(GetGpuTimeString(t.gpuStamp));
    r += QString("  LIF: %1\n").arg(GetGpuTimeString(t.gpuLif));
    r += QString("  Regen Layers: %1\n").arg(GetGpuTimeString(t.gpuRegenLayers));
    r += QString("  Individuals: %1\n").arg(GetGpuTimeString(t.gpuIndividuals));
    r += QString("  Seeding: %1\n").arg(GetGpuTimeString(t.gpuSeeding));
    r += QString("  Seed Source: %1\n").arg(GetGpuTimeString(t.gpuSeedSource));
    r += QString("  Seed Prob: %1\n").arg(GetGpuTimeString(t.gpuSeedProb));
    return r;
}

void EcosystemSimulation::SetWind(Vector2 d, double s)
{
    windDir = d;
    windSpeed = s;
}

void EcosystemSimulation::fillEcosystemData()
{
    qDebug() << "void ILandSimulation::fillEcosystemData()";

    QVector<SpeciesData> speciesData(12);

    for (int s = 0; s < nbSpecies; ++s)
    {
        const SpeciesParam& param = ecosystemData.GetSpeciesParam(s);

        speciesData[s].sf_seedProbs = GetSeedProbs(s);
        speciesData[s].sf_regenLayers = GetRegenLayers(s);

#pragma omp parallel for
        for (int i = 0; i < speciesData[s].sf_regenLayers.GetSizeX() * speciesData[s].sf_regenLayers.GetSizeY(); i++)
        {
            double t = speciesData[s].sf_regenLayers[i];
            double h = param.GrowthFunction(t);
            speciesData[s].sf_regenLayers[i] = h;
        }

        if (ecosystemData.GetData(s).sf_boost.GetSizeX() == 0)
            speciesData[s].sf_boost = GetBoost(s);
        else
            speciesData[s].sf_boost = ecosystemData.GetData(s).sf_boost;

        speciesData[s].sf_trees = ScalarField2(box, sizeX / 2, sizeY / 2, 0);
        speciesData[s].sf_carbon = GetCarbonPool();
    }

    ScalarField2 sf_species = GetSpecies();
    ScalarField2 sf_age = GetAge();

    double sizeSF = sf_species.VertexSize();

#pragma omp parallel for
    for (int i = 0; i < sf_species.VertexSize(); i++)
    {
        int species = sf_species[i];
        if (species < 63)
        {
            const SpeciesParam& param = ecosystemData.GetSpeciesParam(species);
            double h = param.GrowthFunction(sf_age[i]);
            speciesData[species].sf_trees[i] = h;
        }
        else
            sf_species[i] = 63;
    }

    for (int s = 0; s < nbSpecies; ++s)
        ecosystemData.SetData(s, speciesData[s]);

    ecosystemData.SetLIF(GetLIF());
}

void EcosystemSimulation::SetTempChange(double t, int s, int e)
{
    tempChange = t;
    tempChangeStart = s;
    tempChangeEnd = e;
}

void EcosystemSimulation::SetMoistChange(double m, int s, int e)
{
    moistChange = m;
    moistChangeStart = s;
    moistChangeEnd = e;
}

const QVector<TreeInfo>& EcosystemSimulation::GetTreeInstances()
{
    if (areInstancesDirty)
        ComputeTreeInstances();
    return instances;
}

QVector<TreeInfo> EcosystemSimulation::GetSaplings(int& nb, QVector<int>& h)
{
    QVector<ScalarField2> regenLayers(nbSpecies);
    for (int s = 0; s < nbSpecies; ++s)
        regenLayers[s] = GetRegenLayers(s);

    return GetSaplings(nb, h, regenLayers);
}

QVector<TreeInfo> EcosystemSimulation::GetSaplings(int& nb, QVector<int>& h,
    const QVector<ScalarField2>& regenLayers)
{
    QVector<TreeInfo> saplings;
    QVector<double> heights;

    for (int s = 0; s < nbSpecies; ++s)
    {
        const Allometry& allom = ecosystemData.GetAllometry(s);
        const SpeciesParam& param = ecosystemData.GetSpeciesParam(s);
        if (!param.enabled) continue;

        // Sapling heights, from the nominal age the caller already read back
        ScalarField2 sH(Box2(regenLayers[s].GetBox()), regenLayers[s].GetSizeX(), regenLayers[s].GetSizeY(), 0);
        int size = sH.VertexSize();
    #pragma omp parallel for
        for (int i = 0; i < size; i++)
        {
            double t = regenLayers[s].at(i);
            double h = ecosystemData.GetSpeciesParam(s).GrowthFunction(t);
            sH[i] = h;
        }

        // Get Sapling numbers
        ScalarField2 sN(Box2(sH.GetBox()), sH.GetSizeX(), sH.GetSizeY(), 0);
        Random randX = Random::R239;

        for (int i = 0; i < sN.VertexSize(); ++i)
        {
            double h = sH[i];
            if (h < 1.5) {
                sN[i] = 0;
                continue;
            }

            double na = allom.GetAllomAge(h);
            double d = allom.GetDBH(na);
            double n = allom.GetNumTreesPer4m2(d);
            sN[i] = n;

            if (n > 0 && n < 1)
                n = randX.Uniform() < n;
            int in = int(n);

            for (int j = 0; j < in; ++j)
                heights.append(h);
        }


        for (int i = 0; i < sN.VertexSize(); ++i)
        {
            if (sN[i] <= 0) continue;
            TreeInfo sap;
            sap.alive = true;
            sap.isSapling = true;
            sap.h = sH[i];
            sap.s = s;
            // i + nx * j;
            sap.cx = i % sN.GetSizeX();
            sap.cy = i / sN.GetSizeX();
            sap.n = sN[i];
            Vector2 pos = sN.CellCenter(sap.cx, sap.cy);
            sap.x = pos[0];
            sap.y = pos[1];

            saplings.append(sap);
        }

    }

    qDebug() << "Count: " << heights.size();
    nb = heights.size();

    Histogram hist(50, 1, 50, heights);
    h = hist.GetHistogram();
    return saplings;
}

size_t EcosystemSimulation::GetNbTrees()
{
    // Counting does not need the instance list. Building it evaluates the growth function
    // for every tree and fills a vector of hundreds of thousands of TreeInfo, which is a
    // lot of work for a number printed at the end of a run.
    if (!isNbTreesDirty)
        return cachedNbTrees;

    ScalarField2 sf_species = GetSpecies();
    ScalarField2 sf_age = GetAge();

    // Same predicate as ComputeTreeInstances()
    size_t count = 0;
    for (int i = 0; i < sf_species.VertexSize(); ++i)
        if (sf_species.at(i) < 63 && sf_age.at(i) >= 0)
            ++count;

    cachedNbTrees = count;
    isNbTreesDirty = false;
    return count;
}

ScalarField2 EcosystemSimulation::GetSaplingsNb(int s) const
{
    // Get Sapling heights
    ScalarField2 sH = GetRegenLayers(s);
    int size = sH.VertexSize();
#pragma omp parallel for
    for (int i = 0; i < size; i++)
    {
        double t = sH[i];
        double h = ecosystemData.GetSpeciesParam(s).GrowthFunction(t);
        sH[i] = h;
    }

    // Get Sapling numbers
    ScalarField2 sN(Box2(sH.GetBox()), sH.GetSizeX(), sH.GetSizeY(), 0);
    const Allometry& allom = ecosystemData.GetAllometry(s);
    const SpeciesParam& param = ecosystemData.GetSpeciesParam(s);
    if (!param.enabled) return sN;

#pragma omp parallel for
    for (int i = 0; i < sN.VertexSize(); ++i)
    {
        double h = sH[i];
        if (h < 1.5) {
            sN[i] = 0;
            continue;
        }
        double na = allom.GetAllomAge(h);
        double d = allom.GetDBH(na);
        sN[i] = allom.GetNumTreesPer4m2(d);
    }

    return sN;
}

size_t EcosystemSimulation::GetSaplingsCount()
{
    // Get updated data
    for (int s = 0; s < nbSpecies; ++s)
    {
        ecosystemData.GetData(s).sf_regenLayers = GetRegenLayers(s);
        int size = ecosystemData.GetData(s).sf_regenLayers.VertexSize();
#pragma omp parallel for
        for (int i = 0; i < size; i++)
        {
            double t = ecosystemData.GetData(s).sf_regenLayers[i];
            double h = ecosystemData.GetSpeciesParam(s).GrowthFunction(t);
            ecosystemData.GetData(s).sf_regenLayers[i] = h;
        }
    }


    Random randX = Random::R239;

    // Count saplings
    size_t count = 0;
    int size = ecosystemData.GetData(0).sf_regenLayers.VertexSize();
    for (int i = 0; i < size; ++i)
        {
            double m = 0;
            int nn = 0;
            for (int s = 0; s < nbSpecies; ++s)
            {
                Allometry allom = ecosystemData.GetAllometry(s);
                const SpeciesParam& param = ecosystemData.GetSpeciesParam(s);
                if (!param.enabled) continue;
                double h = ecosystemData.GetData(s).sf_regenLayers[i];
                if (h < 1.5) continue;
                double na = allom.GetAllomAge(h);
                double d = allom.GetDBH(na);
                double n = allom.GetNumTreesPer4m2(d);
                if (n > 0 && n < 1)
                    n = randX.Uniform() < n;
                n = int(n);

                if (nn < n) {
                    m = h;
                    nn = n;
                }
            }
            count += nn;
        }

    return count;
}

void EcosystemSimulation::ComputeTreeInstances()
{
    ScalarField2 sf_species = GetSpecies();
    ScalarField2 sf_age = GetAge();

    instances.clear();
    for (int i = 0; i < sf_species.VertexSize(); ++i)
    {
        double s = sf_species.at(i);
        double a = sf_age.at(i);
        if (s < 63 && a >= 0)
        {
            TreeInfo tree;
            tree.alive = true;
            tree.isSapling = false;
            tree.n = 1;

            tree.s = s;
            const SpeciesParam& param = ecosystemData.GetSpeciesParam(s);
            tree.h = param.GrowthFunction(a);
            tree.cx = i % (sizeX / 2);
            tree.cy = i / (sizeX / 2);

            Vector2 pos = sf_species.CellCenter(tree.cx, tree.cy);
            tree.x = pos[0];
            tree.y = pos[1];

            instances.push_back(tree);
        }
    }
    areInstancesDirty = false;
    cachedNbTrees = instances.size();
    isNbTreesDirty = false;
}

unsigned int EcosystemSimulation::DecodeSpecies(unsigned char data) const
{
    return data & 0x3Fu;
}

unsigned int EcosystemSimulation::DecodeBurnState(unsigned char data) const
{
    return (data >> 6) & 0x3u;
}

ScalarField2 EcosystemSimulation::CombineSpeciesBurn(const ScalarField2& s, const ScalarField2& b)
{
    ScalarField2 res(s.GetBox(), s.GetSizeX(), s.GetSizeY());

#pragma omp parallel for
    for (int i = 0; i < s.GetSizeX(); ++i)
        for (int j = 0; j < s.GetSizeY(); ++j)
        {
            uchar sp = static_cast<uchar>(s.at(i, j));
            uchar bs = static_cast<uchar>(b.at(i, j));

            sp = std::min(sp, (uchar) 0x3Fu);
            assert(bs <= 0x3u);

            res(i, j) = (bs << 6) | sp;
        }
    return res;
}

uint32_t EcosystemSimulation::Hash(uint32_t x, uint32_t y, uint32_t z)
{
    uint32_t h = x * 374761393u
        + y * 668265263u
        + z * 1446648777u;

    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}


void EcosystemSimulation::DrawSymbolicDisk(QPainter& painter, QVector<TreeInfo>& trees, Box2 box, Vector2 scale, int borderWidth, int useAlpha)
{
    constexpr int alpha = 0.85 * 255;
    constexpr double INV_U32 = 1.0 / 4294967295.0;

    std::sort(trees.begin(), trees.end(), [](const TreeInfo& a, const TreeInfo& b) { return a.h < b.h; });

    for (const TreeInfo& tree : trees)
    {
        const SpeciesParam& param = ecosystemData.GetSpeciesParam(tree.s);
        const SpeciesData& data = ecosystemData.GetData(tree.s);
        const Allometry& allom = ecosystemData.GetAllometry(tree.s);

        // Skip trees if necessary
        if (!tree.alive) continue;
        if (!box.Inside(Vector2(tree.x, tree.y))) continue;
        if (!param.enabled) continue;

        // Random displacement [-1.; 1]
        double jx = Hash(tree.cx, tree.cy, 0) * INV_U32 * 2.0 - 1.0;
        double jy = Hash(tree.cx, tree.cy, 1) * INV_U32 * 2.0 - 1.0;

        // Draw tree
        Vector2 p = (Vector2(tree.x + jx, tree.y + jy) - box[0]);
        QPointF pos(p[0] * scale[0], p[1] * scale[1]);
        double n = allom.GetAllomAge(tree.h);
        double rad = allom.GetCanopyRadius(tree.h);
        rad = rad * scale[0];

        QColor brushCol = param.col.GetQt();
        if (useAlpha)
            brushCol.setAlpha(alpha);
        painter.setBrush(brushCol);
        QColor penCol = brushCol.darker();
        QPen pen(penCol);
        pen.setWidth(borderWidth);

        painter.setPen(pen);
        painter.drawEllipse(pos, rad, rad);
    }
}

void EcosystemSimulation::DrawSymbolicDisk(QPainter& painter, double resolution, Box2 box, const QImage& bg, bool showSaplings, bool useAlpha)
{
    painter.setRenderHint(QPainter::Antialiasing);

    Box2 hfBox(resourceData.GetHeightField().GetBox());
    Vector2 scale((resolution - 1) / (box[1][0] - box[0][0]), (resolution - 1) / (box[1][0] - box[0][0]));

    int borderWidth = std::max(1., 0.3 * resolution / box.Diagonal()[0]);

    QImage terrain = bg.scaled(resolution, resolution);
    painter.drawImage(QRectF(0., 0., resolution, resolution), terrain);

    QVector<TreeInfo> trees = GetTreeInstances();
    DrawSymbolicDisk(painter, trees, box, scale, borderWidth, useAlpha);

    if (showSaplings) {
        int nb; QVector<int> h;
        QVector<TreeInfo> saplings = GetSaplings(nb, h);
        DrawSymbolicDisk(painter, saplings, box, scale, borderWidth, useAlpha);
    }
}

QImage EcosystemSimulation::GetMapBranches() const
{
    QImage img(sizeX / 2, sizeY / 2, QImage::Format::Format_Grayscale8);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);

    double scalePixel = (sizeX / 2) / resourceData.GetHeightField().GetBox().Diagonal()[0];
    ScalarField2 species = GetSpecies();
    ScalarField2 age = GetAge();
    ScalarField2 carbonPool = GetCarbonPool();

    for (int i = 0; i < species.VertexSize(); ++i)
    {
        int s = species[i];
        if (s < 63)
        {
            Allometry allom = ecosystemData.GetAllometry(s);
            const SpeciesParam& params = ecosystemData.GetSpeciesParam(s);
            double h = params.GrowthFunction(age[i]);
            double r = allom.GetCanopyRadius(h);
            double decayLocal = 100 + 100 * Math::Clamp(carbonPool[i], 0., 1.);

            int x = i % (sizeX / 2);
            int y = i / (sizeX / 2);
            QPointF position(x, y);

            // Gradient color
            QRadialGradient gradient(position, r);
            gradient.setColorAt(0.0, QColor(decayLocal, decayLocal, decayLocal, 255));
            gradient.setColorAt(1.0, QColor(decayLocal, decayLocal, decayLocal, 0));

            painter.setBrush(gradient);
            painter.setPen(QColor(0, 0, 0, 0));

            // Draw
            painter.drawEllipse(position, r, r);
        }
    }

    return img;
}

void EcosystemSimulation::GetStats()
{
    // One transfer per grid instead of one per consumer. This used to issue 4 + 2 * nbSpecies
    // full grid readbacks every year -- including nbSpecies whole regeneration layers just to
    // sample the single observed cell of GetLocalStats().
    ScalarField2 species = GetSpecies();
    ScalarField2 age = GetAge();

    QVector<ScalarField2> regenLayers(nbSpecies);
    for (int s = 0; s < nbSpecies; ++s)
        regenLayers[s] = GetRegenLayers(s);

    GetLocalStats(species, age, regenLayers); 
    GetGlobalStats(species, age, regenLayers);
}

void EcosystemSimulation::GetLocalStats(const ScalarField2& species, const ScalarField2& age,
    const QVector<ScalarField2>& regenLayers)
{
    localStats.years.push_back(year);
    int obs_sp = species.at(localStats.x, localStats.y);
    for (int s = 0; s < nbSpecies; ++s)
    {
        const SpeciesParam& param = ecosystemData.GetSpeciesParam(s);
        const ScalarField2& saplings = regenLayers[s];

        double t = saplings.at(localStats.x, localStats.y);
        double h = param.GrowthFunction(t);
        localStats.saplingHeights[s].push_back(h);

        if (s == obs_sp)
        {
            double t = age.at(localStats.x, localStats.y);
            double h = param.GrowthFunction(t);
            localStats.individualHeights[s].push_back(h);
        }
        else
            localStats.individualHeights[s].push_back(0);
    }
}

void EcosystemSimulation::GetGlobalStats(const ScalarField2& species, const ScalarField2& age,
    const QVector<ScalarField2>& regenLayers)
{
    globalStats.years.push_back(year);
    for (int i = 0; i < nbSpecies; ++i)
        globalStats.basalArea[i].push_back(0);

    globalStats.avgHeight = 0;
    globalStats.nbTrees = 0;

    for (int i = 0; i < species.VertexSize(); ++i)
    {
        // GetSpecies() already decoded, so no second DecodeSpecies here
        double s = species.at(i);
        if (s < nbSpecies)
        {
            const Allometry& allom = ecosystemData.GetAllometry(s);
            const SpeciesParam& param = ecosystemData.GetSpeciesParam(s);

            double t = age.at(i);
            double h = param.GrowthFunction(t);
            double n = allom.GetAllomAge(h);
            double dbh = allom.GetDBH(n);
            if (isinf(dbh))
            {
                qDebug() << "ERROR: inf dbh"; // t is inf ?
                continue;
            }
            if (dbh > 0.075) {
                globalStats.basalArea[s].back() += Math::Pi * Math::Sqr(dbh / 2);
                globalStats.nbTrees++;
                globalStats.avgHeight += h;
            }
        }
    }

    ComputeSaplingsBasalArea(regenLayers);

    double ba = 0;
    for (int i = 0; i < nbSpecies; ++i)
        ba += globalStats.basalArea[i].back();

    if (globalStats.nbTrees > 0)
        globalStats.avgHeight /= globalStats.nbTrees;

    // Number of hectares simulated
    double ha = sizeX * sizeY * 0.0001;

    qDebug() << "Basal area (m2 / ha): " << ba / ha;
    qDebug() << "Average height (m): " << globalStats.avgHeight;
    qDebug() << "Density (t / ha): " << globalStats.nbTrees / ha;
}

void EcosystemSimulation::ComputeSaplingsBasalArea(const QVector<ScalarField2>& regenLayers)
{
    int nb; QVector<int> h;
    QVector<TreeInfo> trees = GetSaplings(nb, h, regenLayers);

    for (auto t : trees) {
        const Allometry& allom = ecosystemData.GetAllometry(t.s);
        const SpeciesParam& param = ecosystemData.GetSpeciesParam(t.s);

        double n = allom.GetAllomAge(t.h);
        double dbh = allom.GetDBH(n);
        if (isinf(dbh))
        {
            qDebug() << "ERROR: inf dbh"; // t is inf ?
            continue;
        }
        if (dbh > 0.075)
        {
            globalStats.basalArea[t.s].back() += Math::Pi * Math::Sqr(dbh / 2);
            globalStats.nbTrees++;
            globalStats.avgHeight += t.h;
        }


    }
}

void EcosystemSimulation::Observe(Vector2 pos)
{
    int i, j;
    ScalarField2 species = GetSpecies();
    species.CellInteger(pos, i, j);
    i = Clamp(i, 0, species.GetSizeX());
    j = Clamp(j, 0, species.GetSizeY());

    if (i != localStats.x || j != localStats.y) 
    {
        localStats.x = i;
        localStats.y = j;
        localStats.years.clear();  
        localStats.individualHeights = QVector<QVector<double>>(nbSpecies);
        localStats.saplingHeights = QVector<QVector<double>>(nbSpecies);
    }
}

QVector<QVector<double>> EcosystemSimulation::BasalArea(QVector<double>& years) const
{
    double simArea = sizeX * sizeY * 0.0001; // Simulation area in ha

    years = globalStats.years;
    QVector<QVector<double>> data(nbSpecies);
    for (int s = 0; s < nbSpecies; ++s)
    {
        double speciesMax = 0;
        for (auto n : globalStats.basalArea[s])
        {
            double val = n / simArea;
            data[s].push_back(val);

            if (val > speciesMax)
                speciesMax = val;
        }
        qDebug() << "Max basal area for " << ecosystemData.GetSpeciesParam(s).name << ": " << speciesMax;
    }

    return data;
}

QVector<QVector<double>> EcosystemSimulation::HeightsHistogram(QVector<double>& heights)
{
    heights.clear();
    int first = int(limit);
    for (int i = first; i <= 50; ++i)
        heights.push_back(i);

    const auto& trees = GetTreeInstances();

    QVector<QVector<double>> data(nbSpecies, QVector<double>(heights.size(), 0));

    for (const auto& t : trees)
    {
        if (!t.alive) continue;
        int index = Clamp((int)t.h - first, 0, int(heights.size()) - 1);
        data[t.s][index] += 1;
    }

    return data;
}

QVector<double> EcosystemSimulation::HeightsHistogram()
{
    const auto& trees = GetTreeInstances();

    QVector<double> data(50, 0);

    for (const auto& t : trees)
    {
        if (!t.alive) continue;
        int index = Math::Min((int)t.h, 50);
        if (index >= 50) continue;
        data[index] += 1;
    }
    
    return data;
}
