#include "ecosystem-simulation.h"

#include "heightfieldshader.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtGui/QOpenGLContext>

namespace {
    // Species Data, packed for usage in GPU buffer
    struct SpeciesParamGPU
    {
        float q;
        float k1, k2, k3;

        float c_s;
        float r_s;
        float c_m;
        float r_m;

        float c_t;
        float r_t;
        float c_sl;
        float r_sl;

        float a_max;
        float a;
        int longGrowthSeason;
        float hmax;

        float pa_max;
        int enabled;
        float ct;
        float crSlope;

        float p_b;
        float pad_1, pad_2, pad_3;

        SpeciesParamGPU(const SpeciesParam& sp, const Allometry& a);
        SpeciesParamGPU() = default;
    };
}

SpeciesParamGPU::SpeciesParamGPU(const SpeciesParam& sp, const Allometry& a)
    : q(sp.q), k1(sp.k1), k2(sp.k2), k3(sp.k3),
    c_s(sp.c_s), r_s(sp.r_s), c_m(sp.c_m), r_m(sp.r_m),
    c_t(sp.c_t), r_t(sp.r_t), c_sl(sp.c_sl), r_sl(sp.r_sl),
    a_max(sp.a_max), a(sp.a), longGrowthSeason(sp.longGrowthSeason), hmax(sp.hmax),
    pa_max(sp.pa_max), enabled(sp.enabled), ct(sp.ct),
    crSlope(a.radSlope), p_b(sp.p_b), pad_1(0), pad_2(0), pad_3(0)
{
}

EcosystemSimulationGPU::EcosystemSimulationGPU(ResourceData& resourceData, EcosystemData& ecosystemData)
    : EcosystemSimulation(resourceData, ecosystemData)
{
}

void EcosystemSimulationGPU::Delete()
{
    // ====== Buffers ======
    gpu_sParams.Destroy();
    gpu_colors.Destroy();
    gpu_tempBounds.Destroy();
    gpu_lipRadius.Destroy();

    // ====== Textures ======
    gpu_resources.Destroy();
    gpu_boost.Destroy();
    gpu_mask.Destroy();

    gpu_regenLayers.Destroy();
    gpu_species.Destroy();
    gpu_age.Destroy();
    gpu_carbonPool.Destroy();

    gpu_sProbs.Destroy();
    gpu_seedCount.Destroy();
    gpu_seedHeight.Destroy();
    gpu_canopy.Destroy();
    gpu_lif.Destroy();
    gpu_stamp.Destroy();
    gpu_lips.Destroy();

    gpu_terrainShaded.Destroy();
    gpu_albedo.Destroy();
    gpu_debug.Destroy();

    gpuTimer.Destroy();

    // ====== Shaders ======
    shader_lif.Destroy();
    shader_sProb.Destroy();
    shader_seedSource.Destroy();
    shader_stamp.Destroy();
    shader_canopy.Destroy();
    shader_seeding.Destroy();
    shader_regenLayers.Destroy();
    shader_individuals.Destroy();
    shader_visu.Destroy();
    shader_debug.Destroy();
}

namespace {
    // Image stores are not automatically visible to glGetTextureImage. The passes of the
    // yearly pipeline no longer end with a full sync, so every readback has to ask for the
    // writes to be made visible first.
    //
    // This is also the single choke point every Get* readback goes through, which makes it
    // the place to catch a missing context. Reading a texture without a current one does not
    // fail: glGetTextureImage simply leaves the buffer untouched, so it reads back as zeros
    // -- and species 0 with age 0 satisfies the live-tree predicate, which is how counting
    // trees once returned the whole grid. A plausible wrong number is worse than an error.
    void BarrierForReadback()
    {
        if (QOpenGLContext::currentContext() == nullptr)
        {
            qCritical() << "GPU readback with no current OpenGL context: the result would be "
                "all zeros. The call has to sit inside makeCurrent() / doneCurrent(), or on "
                "a thread that owns a context.";
            return;
        }

        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    }
}

ScalarField2 EcosystemSimulationGPU::GetLIF() const
{
    BarrierForReadback();
    ScalarField2 sf_lif = gpu_lif.CreateScalarField<float>(GL_RED, GL_FLOAT);
    sf_lif.Translate(Vector2(-0.5, -0.5));
    sf_lif.Scale(box.Diagonal());
    return sf_lif;
}

ScalarField2 EcosystemSimulationGPU::GetAge() const
{
    BarrierForReadback();
    ScalarField2 sf_age = gpu_age.CreateScalarField<float>(GL_RED, GL_FLOAT);
    sf_age.Translate(Vector2(-0.5, -0.5));
    sf_age.Scale(box.Diagonal());
    return sf_age;
}

ScalarField2 EcosystemSimulationGPU::GetSpecies() const
{
    BarrierForReadback();
    ScalarField2 sf_species = gpu_species.CreateScalarField<unsigned char>(GL_RED_INTEGER, GL_UNSIGNED_BYTE);
    sf_species.Translate(Vector2(-0.5, -0.5));
    sf_species.Scale(box.Diagonal());

#pragma omp parallel for
    for (int i = 0; i < sf_species.GetSizeX(); ++i)
        for (int j = 0; j < sf_species.GetSizeY(); ++j)
        {
            double d = sf_species.at(i, j);
            sf_species(i, j) = DecodeSpecies(d);
        }

    return sf_species;
}

ScalarField2 EcosystemSimulationGPU::GetBurnState() const
{
    BarrierForReadback();
    ScalarField2 sf_species = gpu_species.CreateScalarField<unsigned char>(GL_RED_INTEGER, GL_UNSIGNED_BYTE);
    sf_species.Translate(Vector2(-0.5, -0.5));
    sf_species.Scale(box.Diagonal());

#pragma omp parallel for
    for (int i = 0; i < sf_species.GetSizeX(); ++i)
        for (int j = 0; j < sf_species.GetSizeY(); ++j)
        {
            double d = sf_species.at(i, j);
            sf_species(i, j) = DecodeBurnState(d);
        }

    return sf_species;
}

ScalarField2 EcosystemSimulationGPU::GetCarbonPool() const
{
    BarrierForReadback();
    ScalarField2 sf_carbonPool = gpu_carbonPool.CreateScalarField<float>(GL_RED, GL_FLOAT);
    sf_carbonPool.Translate(Vector2(-0.5, -0.5));
    sf_carbonPool.Scale(box.Diagonal());
    return sf_carbonPool;
}

ScalarField2 EcosystemSimulationGPU::GetBoost(int s) const
{
    BarrierForReadback();
    ScalarField2 sf_boost = gpu_boost.CreateScalarField<float>(GL_RED, GL_FLOAT, s);
    sf_boost.Translate(Vector2(-0.5, -0.5));
    sf_boost.Scale(box.Diagonal());
    return sf_boost;
}

ScalarField2 EcosystemSimulationGPU::GetRegenLayers(int s) const
{
    BarrierForReadback();
    ScalarField2 sf_regenLayers = gpu_regenLayers.CreateScalarField<float>(GL_RED, GL_FLOAT, s);
    sf_regenLayers.Translate(Vector2(-0.5, -0.5));
    sf_regenLayers.Scale(box.Diagonal());
    return sf_regenLayers;
}

ScalarField2 EcosystemSimulationGPU::GetSeedProbs(int s) const
{
    BarrierForReadback();
    ScalarField2 sf_sProbs = gpu_sProbs.CreateScalarField<float>(GL_RED, GL_FLOAT, s);
    sf_sProbs.Translate(Vector2(-0.5, -0.5));
    sf_sProbs.Scale(box.Diagonal());
    return sf_sProbs;
}

ScalarField2 EcosystemSimulationGPU::GetCanopyMaxHeight() const
{
    BarrierForReadback();
    ScalarField2 sf_canopy = gpu_canopy.CreateScalarField<float>(GL_RED, GL_FLOAT);
    sf_canopy.Translate(Vector2(-0.5, -0.5));
    sf_canopy.Scale(box.Diagonal());
    return sf_canopy;
}

ScalarField2 EcosystemSimulationGPU::GetCanopyAvgHeight() const
{
    BarrierForReadback();
    ScalarField2 sf_canopy = gpu_canopy.CreateScalarField<float>(GL_GREEN, GL_FLOAT);
    sf_canopy.Translate(Vector2(-0.5, -0.5));
    sf_canopy.Scale(box.Diagonal());
    return sf_canopy;
}

ScalarField2 EcosystemSimulationGPU::GetDebug() const
{
    BarrierForReadback();
    ScalarField2 sf_debug = gpu_debug.CreateScalarField<float>(GL_RED, GL_FLOAT);
    sf_debug.Translate(Vector2(-0.5, -0.5));
    sf_debug.Scale(box.Diagonal());
    return sf_debug;
}

GLuint EcosystemSimulationGPU::GetOutputTexture()
{
    UpdateVisualization();
    return gpu_albedo.GetID();
}

void EcosystemSimulationGPU::UploadSpecies(const ScalarField2& combined)
{
    // gpu_species is R8UI and the packed species / burn state is 8 bits by construction.
    // Transferring it as GL_UNSIGNED_INT made the driver narrow every texel on the way in
    // and widen it on the way out, for four times the bandwidth -- and it is what raised
    // the "converts data from 4-bit integer to 1-bit integer" pixel-path warning.
    std::vector<unsigned char> bytes(combined.VertexSize());
    for (int i = 0; i < combined.VertexSize(); ++i)
        bytes[i] = static_cast<unsigned char>(combined.at(i));

    gpu_species.SetData(GL_RED_INTEGER, GL_UNSIGNED_BYTE, bytes.data());
}

void EcosystemSimulationGPU::Init()
{
    // ====== Buffers ======
    gpu_sParams.Generate();
    gpu_colors.Generate();
    gpu_tempBounds.Generate();

    // Computed by Load() just before this call, from the LIP patches actually loaded
    gpu_lipRadius.Generate();
    std::vector<int> radii(std::max(1, int(lipRadiusPerBucket.size())), maxLipRadius);
    for (int i = 0; i < lipRadiusPerBucket.size(); ++i)
        radii[i] = lipRadiusPerBucket[i];
    gpu_lipRadius.SetData(GL_SHADER_STORAGE_BUFFER, radii.size() * sizeof(int), radii.data());

    // ====== Textures ======
    gpu_resources.Generate(nx2, ny2, 9 + 3 + resourceData.GetWindMap().IsSet(), GL_R16F);
    gpu_boost.Generate(nx10, ny10, nbSpecies, GL_R16F);
    gpu_mask.Generate(nx2, ny2, GL_R16F);

    gpu_regenLayers.Generate(nx2, ny2, nbSpecies, GL_R32F);
    gpu_species.Generate(nx2, ny2, GL_R8UI);
    gpu_age.Generate(nx2, ny2, GL_R32F);
    gpu_carbonPool.Generate(nx2, ny2, GL_R32F);

    gpu_sProbs.Generate(nx20, ny20, nbSpecies, GL_R32F);
    gpu_seedCount.Generate(nx20, ny20, nbSpecies, GL_R32F);
    gpu_seedHeight.Generate(nx20, ny20, nbSpecies, GL_R32F);
    gpu_canopy.Generate(nx10, ny10, GL_RG32F, 2);
    gpu_lif.Generate(nx2, ny2, GL_R32F);
    gpu_stamp.Generate(nx2, ny2, GL_RG32F, 2);
    gpu_lips.Generate(dimLIP, dimLIP * maxLips, nbSpecies, GL_R32F);

    ScalarField2 sf_default(box, nx2, ny2, 0);
    gpu_age.SetData(GL_RED, GL_FLOAT, sf_default);
    gpu_carbonPool.SetData(GL_RED, GL_FLOAT, sf_default);
    sf_default += 1;
    gpu_lif.SetData(GL_RED, GL_FLOAT, sf_default);

    // One entry per 2m cell, matching the texture: these used to be allocated at sizeX by
    // sizeY, i.e. four times the size the upload actually reads
    ScalarField2 s(box, nx2, ny2, 63);
    ScalarField2 bs(box, nx2, ny2, 0); // The initial burn state
    UploadSpecies(CombineSpeciesBurn(s, bs));

    ScalarField2 sp_default(box, nx20, ny20, 0.1);
    QVector<float> defaultLIP(dimLIP * dimLIP * maxLips, 0.f);
    ScalarField2 defaultBoost(Box2(resourceData.GetHeightField().GetBox()), nx10, ny10, 0);
    for (int i = 0; i < nbSpecies; ++i)
    {
        // Load a default values
        gpu_lips.SetData(GL_RED, GL_FLOAT, i, defaultLIP.data());
        gpu_boost.SetData(GL_RED, GL_FLOAT, i, defaultBoost);
        gpu_sProbs.SetData(GL_RED, GL_FLOAT, i, sp_default);

        // Load each individual patch
        const QVector<LIP>& lips = ecosystemData.GetSpeciesParam(i).lips;
        for (int j = 0; j < lips.size(); ++j)
        {
            int s = lips[j].lip.GetSizeX();
            int t = (dimLIP - s) / 2;
            gpu_lips.SetData(GL_RED, GL_FLOAT, i, { t, t + j * dimLIP }, { s, s }, lips[j].lip);
        }
    }

    gpu_terrainShaded.Generate(nx2, ny2, GL_R32F);
    gpu_albedo.Generate(nx2, ny2, GL_RGBA8, 4);
    glTextureParameteri(gpu_albedo.GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(gpu_albedo.GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gpu_debug.Generate(nx2, ny2, GL_R32F);

    HeightField hf = resourceData.GetHeightField().Resized(nx2, ny2);
    HeightFieldShader hs(hf);
    ScalarField2 terrainShaded(hs.ShadedRelief());
    gpu_terrainShaded.SetData(GL_RED, GL_FLOAT, terrainShaded);

    // ====== Shaders ======
    CompileShaders();
}

void EcosystemSimulationGPU::CompileShaders()
{
    QString shaderDir = QString::fromStdString(
        std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders");

    QByteArray defs = QString("#define nbSpecies %1\n").arg(nbSpecies).toUtf8();
    shader_lif.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_lif.glsl", defs.constData());
    shader_sProb.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_sprob.glsl", defs.constData());
    shader_seedSource.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_seedsource.glsl", defs.constData());
    shader_stamp.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_stamp.glsl", defs.constData());
    shader_canopy.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_canopy.glsl", defs.constData());
    shader_seeding.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_seeding.glsl", defs.constData());
    shader_regenLayers.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_regenLayers.glsl", defs.constData());
    shader_individuals.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_individuals.glsl", defs.constData());
    shader_visu.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_visu.glsl", defs.constData());
    shader_debug.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_debug.glsl", defs.constData());
    shader_fire.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_fire.glsl", defs.constData());
    shader_mask.InitializeWithIncludes(shaderDir.toLocal8Bit(), "compute_mask.glsl", defs.constData());
}

void EcosystemSimulationGPU::LoadParams()
{
    assert(nbSpecies > 0);
    qDebug() << "GPULoadParams()";

    std::vector<SpeciesParamGPU> sParamsGPU(nbSpecies);
    for (int s = 0; s < nbSpecies; ++s)
        sParamsGPU[s] = SpeciesParamGPU(ecosystemData.GetSpeciesParam(s), ecosystemData.GetAllometry(s));

    // compute_sprob.glsl now divides by the per-species dispersal parameters instead of
    // using the global 30 / 200 / 0.2 it used to hardcode, so a missing value is fatal
    for (int s = 0; s < nbSpecies; ++s)
        if (sParamsGPU[s].k1 <= 0 || sParamsGPU[s].k2 <= 0)
            qWarning() << "Species" << s << ecosystemData.GetSpeciesParam(s).name
                << "has a degenerate seed dispersal kernel: k1 =" << sParamsGPU[s].k1
                << ", k2 =" << sParamsGPU[s].k2 << "-- seed probabilities will be NaN.";

    gpu_sParams.SetData(GL_SHADER_STORAGE_BUFFER, sParamsGPU.size() * sizeof(SpeciesParamGPU), sParamsGPU.data());

    std::vector<float> colorsGPU;
    for (int s = 0; s < nbSpecies; ++s)
    {
        Color col = ecosystemData.GetSpeciesParam(s).col;
        colorsGPU.push_back(col[0]);
        colorsGPU.push_back(col[1]);
        colorsGPU.push_back(col[2]);
        colorsGPU.push_back(1);
    }

    gpu_colors.SetData(GL_SHADER_STORAGE_BUFFER, colorsGPU.size() * sizeof(float), colorsGPU.data());
}

void EcosystemSimulationGPU::LoadResources()
{
    assert(nbSpecies > 0);
    qDebug() << "GPULoadResources()";

    TemperatureMap& tm = resourceData.GetTempMap();
    QVector<float> tempBounds(9 * 2);
    for (int i = 0; i < 9; ++i)
    {
        int m = i + start_lgc;
        gpu_resources.SetData(GL_RED, GL_FLOAT, i, resourceData.GetScalarField(MapType::SUNLIGHT, m));

        tempBounds[i * 2] = tm.GetMinTemps()[m];
        tempBounds[i * 2 + 1] = tm.GetMaxTemps()[m];
    }
    gpu_tempBounds.SetData(GL_SHADER_STORAGE_BUFFER, tempBounds.size() * sizeof(float), tempBounds.data());

    gpu_resources.SetData(GL_RED, GL_FLOAT, 9, resourceData.GetScalarField(MapType::MOISTURE, 0));
    gpu_resources.SetData(GL_RED, GL_FLOAT, 10, tm.GetScalarField());
    gpu_resources.SetData(GL_RED, GL_FLOAT, 11, resourceData.GetSlope());

    if (resourceData.GetWindMap().IsSet())
        gpu_resources.SetData(GL_RED, GL_FLOAT, 12, resourceData.GetWindMap().GetScalarField());

    gpu_mask.SetData(GL_RED, GL_FLOAT, resourceData.GetMaskMap().GetScalarField());

    ScalarField2 terrainShaded(terrainShading);
    gpu_terrainShaded.SetData(GL_RED, GL_FLOAT, terrainShaded);
}

void EcosystemSimulationGPU::LoadEcosystem(bool updateTrees)
{
    assert(nbSpecies > 0);
    qDebug() << "GPULoadEcosystem()";

    areInstancesDirty = true;
    isNbTreesDirty = true;

    for (int s = 0; s < nbSpecies; ++s)
    {
        gpu_sProbs.SetData(GL_RED, GL_FLOAT, s, ecosystemData.GetData(s).sf_seedProbs);
        gpu_regenLayers.SetData(GL_RED, GL_FLOAT, s, ecosystemData.GetData(s).sf_regenLayers);
        gpu_boost.SetData(GL_RED, GL_FLOAT, s, ecosystemData.GetData(s).sf_boost);
    }

    if (updateTrees)
    {
        ecosystemData.SyncMatureTrees();
        UploadSpecies(CombineSpeciesBurn(ecosystemData.GetSFSpecies(), GetBurnState()));
        gpu_age.SetData(GL_RED, GL_FLOAT, ecosystemData.GetSFAge());
    }
}

void EcosystemSimulationGPU::UpdateVisualization(int mode)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Visualization");

    shader_visu.Bind();

    shader_visu.SetUniform("nx", nx2);
    shader_visu.SetUniform("ny", ny2);
    shader_visu.SetUniform("mode", mode);
    shader_visu.SetUniform("limit", float(limit)); 

    gpu_species.Bind(0, GL_READ_ONLY);
    gpu_age.Bind(1, GL_READ_ONLY);
    gpu_terrainShaded.Bind(2, GL_READ_ONLY);
    gpu_albedo.Bind(3, GL_WRITE_ONLY);
    gpu_regenLayers.Bind(4, GL_READ_ONLY); 

    gpu_colors.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 1);

    DispatchCompute(nx2, ny2);

    GPUSync();
    shader_visu.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::ComputeCanopyHeight()
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Canopy Height"); 
     
    shader_canopy.Bind(); 

    shader_canopy.SetUniform("nx", nx2);
    shader_canopy.SetUniform("ny", ny2);
    shader_canopy.SetUniform("cx", nx10);
    shader_canopy.SetUniform("cy", ny10);

    gpu_species.Bind(0, GL_READ_ONLY);
    gpu_age.Bind(1, GL_READ_ONLY);
    gpu_canopy.Bind(2, GL_WRITE_ONLY);

    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 0);

    gpuTimer.Begin(GPUTimer::Canopy);
    DispatchCompute(nx10, ny10);
    gpuTimer.End(); 

    GPUBarrier();
    shader_canopy.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::ComputeStamp()
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Stamp");

    shader_stamp.Bind();

    shader_stamp.SetUniform("nx", nx2);
    shader_stamp.SetUniform("ny", ny2);

    gpu_species.Bind(0, GL_READ_ONLY);
    gpu_age.Bind(1, GL_READ_ONLY);
    gpu_canopy.Bind(2, GL_READ_ONLY);
    gpu_stamp.Bind(3, GL_WRITE_ONLY);

    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 0);

    gpuTimer.Begin(GPUTimer::Stamp);
    DispatchCompute(nx2, ny2);
    gpuTimer.End();

    GPUBarrier();
    shader_stamp.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::ComputeLIF()
{
    // Per-cell scale and height bucket, hoisted out of the 41x41 gather below
    ComputeStamp();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "LIF");

    shader_lif.Bind();

    shader_lif.SetUniform("nx", nx2);
    shader_lif.SetUniform("ny", ny2);
    shader_lif.SetUniform("N", dimLIP);
    shader_lif.SetUniform("R", maxLipRadius);

    gpu_species.Bind(0, GL_READ_ONLY);
    gpu_age.Bind(1, GL_READ_ONLY);
    gpu_lif.Bind(2, GL_WRITE_ONLY);
    gpu_lips.Bind(3, GL_READ_ONLY);
    gpu_canopy.Bind(4, GL_READ_ONLY);
    gpu_stamp.Bind(5, GL_READ_ONLY);
    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    gpu_lipRadius.BindAt(GL_SHADER_STORAGE_BUFFER, 1);

    shader_lif.SetUniform("nbLips", maxLips);

    gpuTimer.Begin(GPUTimer::LIF);
    DispatchCompute(nx2, ny2);
    gpuTimer.End();

    GPUBarrier();
    shader_lif.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::StepRegenLayers()
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Step Regeneration Layers");

    shader_regenLayers.Bind();

    shader_regenLayers.SetUniform("nx", nx2);
    shader_regenLayers.SetUniform("ny", ny2);
    shader_regenLayers.SetUniform("start_lgc", start_lgc);
    shader_regenLayers.SetUniform("end_lgc", end_lgc);
    shader_regenLayers.SetUniform("start_sgc", start_sgc);
    shader_regenLayers.SetUniform("end_sgc", end_sgc);
    shader_regenLayers.SetUniform("tempChange", float(tempChange));
    shader_regenLayers.SetUniform("moistChange", float(moistChange));
    shader_regenLayers.SetUniform("tempChangeEnd", tempChangeEnd);
    shader_regenLayers.SetUniform("tempChangeStart", tempChangeStart);
    shader_regenLayers.SetUniform("moistChangeEnd", moistChangeEnd);
    shader_regenLayers.SetUniform("moistChangeStart", moistChangeStart);
    shader_regenLayers.SetUniform("year", year);
    shader_regenLayers.SetUniform("limit", float(limit));
    shader_regenLayers.SetUniform("useWind", int(resourceData.GetWindMap().IsSet() && useWind));

    gpu_resources.Bind(0, GL_READ_ONLY);
    gpu_lif.Bind(1, GL_READ_ONLY);
    gpu_species.Bind(2, GL_READ_WRITE);
    gpu_age.Bind(3, GL_WRITE_ONLY);
    gpu_carbonPool.Bind(4, GL_WRITE_ONLY);
    gpu_regenLayers.Bind(5, GL_READ_WRITE);
    gpu_boost.Bind(6, GL_READ_ONLY);
    gpu_canopy.Bind(7, GL_READ_ONLY);

    gpu_tempBounds.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 1);

    gpuTimer.Begin(GPUTimer::RegenLayers);
    DispatchCompute(nx2, ny2);
    gpuTimer.End();

    GPUBarrier();
    shader_regenLayers.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::StepIndividuals()
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Step Individuals");

    // The month loop lives here rather than in SimulateYear: only monthIndex changes
    // between the 9 dispatches, so the program, the 8 image bindings, the 2 storage
    // buffers and the 17 other uniforms are set once instead of nine times. That is
    // 64 image bindings and 136 uniform lookups saved per simulated year, which is
    // where most of the host-side cost of the pipeline was.
    shader_individuals.Bind();

    shader_individuals.SetUniform("nx", nx2);
    shader_individuals.SetUniform("ny", ny2);
    shader_individuals.SetUniform("year", year);
    shader_individuals.SetUniform("N", dimLIP);
    shader_individuals.SetUniform("start_lgc", start_lgc);
    shader_individuals.SetUniform("end_lgc", end_lgc);
    shader_individuals.SetUniform("start_sgc", start_sgc);
    shader_individuals.SetUniform("end_sgc", end_sgc);
    shader_individuals.SetUniform("checkDeath", int(doDeath));
    shader_individuals.SetUniform("nbLips", (int)maxLips);
    shader_individuals.SetUniform("tempChange", float(tempChange));
    shader_individuals.SetUniform("moistChange", float(moistChange));
    shader_individuals.SetUniform("tempChangeEnd", tempChangeEnd);
    shader_individuals.SetUniform("tempChangeStart", tempChangeStart);
    shader_individuals.SetUniform("moistChangeEnd", moistChangeEnd);
    shader_individuals.SetUniform("moistChangeStart", moistChangeStart);
    shader_individuals.SetUniform("useWind", int(resourceData.GetWindMap().IsSet() && useWind));

    gpu_resources.Bind(0, GL_READ_ONLY);
    gpu_lif.Bind(1, GL_READ_ONLY);
    gpu_lips.Bind(2, GL_READ_ONLY);
    gpu_species.Bind(3, GL_READ_WRITE);
    gpu_age.Bind(4, GL_READ_WRITE);
    gpu_carbonPool.Bind(5, GL_READ_WRITE);
    gpu_boost.Bind(6, GL_READ_ONLY);
    gpu_canopy.Bind(7, GL_READ_ONLY);

    gpu_tempBounds.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 1);

    for (int i = 0; i < 9; ++i)
    {
        shader_individuals.SetUniform("monthIndex", i);

        // The 9 queries accumulate into a single per-year figure
        gpuTimer.Begin(GPUTimer::Individuals);
        DispatchCompute(nx2, ny2);
        gpuTimer.End();

        // Each month writes age / carbonPool / species and the next one reads them back
        GPUBarrier();
    }

    shader_individuals.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::StepFire()
{
    double minH, maxH;
    resourceData.GetHeightField().GetRange(minH, maxH);
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Fire");

    shader_fire.Bind();

    shader_fire.SetUniform("nx", nx2);
    shader_fire.SetUniform("ny", ny2);
    shader_fire.SetUniform("year", year);
    shader_fire.SetUniform("minH", (float)minH);
    shader_fire.SetUniform("maxH", (float)maxH);
    shader_fire.SetUniform("windDirX", (float)windDir[0]);
    shader_fire.SetUniform("windDirY", (float)windDir[1]);
    shader_fire.SetUniform("windSpeed", (float)windSpeed);
    shader_fire.SetUniform("useWind", int(resourceData.GetWindMap().IsSet() && useWind));

    gpu_resources.Bind(0, GL_READ_ONLY);
    gpu_species.Bind(1, GL_READ_WRITE);
    gpu_age.Bind(2, GL_READ_WRITE);
    gpu_regenLayers.Bind(3, GL_WRITE_ONLY);
    gpu_canopy.Bind(4, GL_READ_ONLY);
    gpu_carbonPool.Bind(5, GL_READ_ONLY);

    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 0);

    DispatchCompute(nx2, ny2);

    GPUSync();
    shader_fire.Unbind();   

    glPopDebugGroup();
}

void EcosystemSimulationGPU::Seeding()
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Seeding");

    shader_seeding.Bind();

    shader_seeding.SetUniform("nx", nx2);
    shader_seeding.SetUniform("ny", ny2);
    shader_seeding.SetUniform("year", year);
    shader_seeding.SetUniform("seedMonth", seedMonth - start_lgc);
    shader_seeding.SetUniform("tempChange", float(tempChange));
    shader_seeding.SetUniform("moistChange", float(moistChange));
    shader_seeding.SetUniform("tempChangeEnd", tempChangeEnd);
    shader_seeding.SetUniform("tempChangeStart", tempChangeStart);
    shader_seeding.SetUniform("moistChangeEnd", moistChangeEnd);
    shader_seeding.SetUniform("moistChangeStart", moistChangeStart);
    shader_seeding.SetUniform("useWind", int(resourceData.GetWindMap().IsSet() && useWind));

    gpu_resources.Bind(0, GL_READ_ONLY);
    gpu_lif.Bind(1, GL_READ_ONLY);
    gpu_mask.Bind(2, GL_READ_ONLY);
    gpu_sProbs.Bind(3, GL_READ_ONLY);
    gpu_regenLayers.Bind(4, GL_READ_WRITE);
    gpu_boost.Bind(5, GL_READ_ONLY);
    gpu_canopy.Bind(6, GL_READ_ONLY);
    gpu_species.Bind(7, GL_READ_WRITE);

    gpu_tempBounds.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 1);

    gpuTimer.Begin(GPUTimer::Seeding);
    DispatchCompute(nx2, ny2);
    gpuTimer.End();

    GPUBarrier();
    shader_seeding.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::ComputeSeedSource()
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Seed Source");

    shader_seedSource.Bind();

    shader_seedSource.SetUniform("nx", nx20);
    shader_seedSource.SetUniform("ny", ny20);
    shader_seedSource.SetUniform("gx", nx2);
    shader_seedSource.SetUniform("gy", ny2);

    gpu_species.Bind(0, GL_READ_ONLY);
    gpu_age.Bind(1, GL_READ_ONLY);
    gpu_seedCount.Bind(2, GL_WRITE_ONLY);
    gpu_seedHeight.Bind(3, GL_WRITE_ONLY);

    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 0);

    gpuTimer.Begin(GPUTimer::SeedSource);
    DispatchCompute(nx20, ny20);
    gpuTimer.End();

    GPUBarrier();
    shader_seedSource.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::ComputeSeedProb()
{
    // Builds the per-cell species histogram that the dispersion kernel below reads
    ComputeSeedSource();

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Seeding Probability");

    shader_sProb.Bind();

    shader_sProb.SetUniform("nx", nx20);
    shader_sProb.SetUniform("ny", ny20);
    shader_sProb.SetUniform("N", dimSeed);

    gpu_seedCount.Bind(0, GL_READ_ONLY);
    gpu_sProbs.Bind(1, GL_WRITE_ONLY);
    gpu_seedHeight.Bind(2, GL_READ_ONLY);

    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 0);

    gpuTimer.Begin(GPUTimer::SeedProb);
    DispatchCompute(nx20, ny20);
    gpuTimer.End();

    GPUBarrier();
    shader_sProb.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::ComputeMask(int s, int r, double t, bool useThresh)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Compute Mask");

    shader_mask.Bind();

    shader_mask.SetUniform("nx", nx2);
    shader_mask.SetUniform("ny", ny2);
    shader_mask.SetUniform("year", year);
    shader_mask.SetUniform("sp", s);
    shader_mask.SetUniform("radius", r);
    shader_mask.SetUniform("threshold", (float)t);
    shader_mask.SetUniform("useThresh", (int)useThresh);

    gpu_albedo.Bind(0, GL_WRITE_ONLY);
    gpu_debug.Bind(1, GL_WRITE_ONLY);
    gpu_age.Bind(2, GL_READ_ONLY);
    gpu_species.Bind(3, GL_READ_WRITE);
    gpu_mask.Bind(4, GL_READ_ONLY);

    gpu_colors.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 1);

    DispatchCompute(nx2, ny2);

    GPUSync();
    shader_mask.Unbind();

    glPopDebugGroup();
}


void EcosystemSimulationGPU::DebugVigor(int s, int m, double h, bool useSun, bool useShadow, bool useMoist, bool useTemp, bool useSlope, bool useBoost, bool removeTreeContrib, bool showDominant)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Debug Vigor");

    shader_debug.Bind();

    shader_debug.SetUniform("nx", nx2);
    shader_debug.SetUniform("ny", ny2);
    shader_debug.SetUniform("N", dimLIP);

    shader_debug.SetUniform("sp", s);
    shader_debug.SetUniform("m", m);
    shader_debug.SetUniform("h", (float)h);
    shader_debug.SetUniform("tempChange", float(tempChange));
    shader_debug.SetUniform("moistChange", float(moistChange));
    shader_debug.SetUniform("tempChangeEnd", tempChangeEnd);
    shader_debug.SetUniform("tempChangeStart", tempChangeStart);
    shader_debug.SetUniform("moistChangeEnd", moistChangeEnd);
    shader_debug.SetUniform("moistChangeStart", moistChangeStart);
    shader_debug.SetUniform("year", year);

    shader_debug.SetUniform("useSun", (int)useSun);
    shader_debug.SetUniform("useShadow", (int)useShadow);
    shader_debug.SetUniform("useMoist", (int)useMoist);
    shader_debug.SetUniform("useTemp", (int)useTemp);
    shader_debug.SetUniform("useSlope", (int)useSlope);
    shader_debug.SetUniform("useBoost", (int)useBoost);
    shader_debug.SetUniform("removeTreeContrib", (int)removeTreeContrib);
    shader_debug.SetUniform("showDominant", (int)showDominant);
    shader_debug.SetUniform("useWind", int(resourceData.GetWindMap().IsSet() && useWind));

    gpu_resources.Bind(0, GL_READ_ONLY);
    gpu_albedo.Bind(1, GL_WRITE_ONLY);
    gpu_debug.Bind(2, GL_WRITE_ONLY);
    gpu_boost.Bind(3, GL_READ_ONLY);
    gpu_canopy.Bind(4, GL_READ_ONLY);
    gpu_lif.Bind(5, GL_READ_ONLY);
    gpu_lips.Bind(6, GL_READ_ONLY);

    gpu_tempBounds.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    gpu_colors.BindAt(GL_SHADER_STORAGE_BUFFER, 1);
    gpu_sParams.BindAt(GL_SHADER_STORAGE_BUFFER, 2);

    DispatchCompute(nx2, ny2);

    GPUSync();
    shader_debug.Unbind();

    glPopDebugGroup();
}

void EcosystemSimulationGPU::StartFire(QPoint p)
{
    ScalarField2 s = GetSpecies();
    ScalarField2 b = GetBurnState();
    b(p) = 2;

    qDebug() << "BS coords: " << p.x() << ", " << p.y();

    auto t = CombineSpeciesBurn(s, b);
    double a, c;
    t.GetRange(a, c);
    qDebug() << "BS: " << a << ", " << c;

    UploadSpecies(CombineSpeciesBurn(s, b));
}

void EcosystemSimulationGPU::ResetGPUTimings()
{
    gpuTimer.Reset();
}

void EcosystemSimulationGPU::BeginGPUTimingFrame()
{
    gpuTimer.BeginFrame();
}

void EcosystemSimulationGPU::CollectGPUTimings(std::vector<Timings>& times)
{
    gpuTimer.Flush();

    const std::vector<GPUTimer::PassTimes>& history = gpuTimer.History();
    for (size_t i = 0; i < times.size() && i < history.size(); ++i)
    {
        const GPUTimer::PassTimes& t = history[i];
        times[i].gpuCanopy = t[GPUTimer::Canopy];
        times[i].gpuStamp = t[GPUTimer::Stamp];
        times[i].gpuLif = t[GPUTimer::LIF];
        times[i].gpuRegenLayers = t[GPUTimer::RegenLayers];
        times[i].gpuIndividuals = t[GPUTimer::Individuals];
        times[i].gpuSeeding = t[GPUTimer::Seeding];
        times[i].gpuSeedSource = t[GPUTimer::SeedSource];
        times[i].gpuSeedProb = t[GPUTimer::SeedProb];
    }
}

void EcosystemSimulationGPU::DispatchCompute(int x, int y)
{
    // Ceiling division: the previous (x / localSize) + 1 launched a whole extra row and
    // column of workgroups whenever the grid divided evenly, which they spent returning
    // on the uv >= nx guard
    int dispatchSizeX = (x + localSizeX - 1) / localSizeX;
    int dispatchSizeY = (y + localSizeY - 1) / localSizeY;
    glDispatchCompute(dispatchSizeX, dispatchSizeY, 1);
}

void EcosystemSimulationGPU::GPUBarrier()
{
    // The passes of the yearly pipeline only communicate through imageLoad / imageStore,
    // so this single bit is enough to order them. The SSBOs (sParams, tempBounds) are
    // read-only in the shaders and written by the host before the run.
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void EcosystemSimulationGPU::GPUSync()
{
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
    glDeleteSync(fence);
}

void EcosystemSimulationGPU::SyncEndOfYear()
{
    GPUSync();
}

QString EcosystemSimulationGPU::GetMemoryUsed() const
{
    size_t mem = 0;
    // Textures
    mem += gpu_resources.GetMemoryUsed();
    mem += gpu_boost.GetMemoryUsed();
    mem += gpu_mask.GetMemoryUsed();
    mem += gpu_sProbs.GetMemoryUsed();
    mem += gpu_regenLayers.GetMemoryUsed();
    mem += gpu_species.GetMemoryUsed();
    mem += gpu_age.GetMemoryUsed();
    mem += gpu_carbonPool.GetMemoryUsed();
    mem += gpu_canopy.GetMemoryUsed();
    mem += gpu_lif.GetMemoryUsed();
    mem += gpu_lips.GetMemoryUsed();
    mem += gpu_stamp.GetMemoryUsed();
    mem += gpu_seedCount.GetMemoryUsed();
    mem += gpu_seedHeight.GetMemoryUsed();

    // Buffers
    mem += gpu_sParams.GetMemoryUsed(GL_SHADER_STORAGE_BUFFER);
    mem += gpu_tempBounds.GetMemoryUsed(GL_SHADER_STORAGE_BUFFER);

    size_t gib, mib, kib;
    System::BytesToGib(mem, gib, mib, kib);

    return QString("%1 Gib, %2 MiB and %3 KiB (or %4 bytes)").arg(gib).arg(mib).arg(kib).arg(mem);

}

QString EcosystemSimulationGPU::GetMemoryUsedCommon() const
{
    size_t mem = 0;
    // Textures
    mem += gpu_resources.GetMemoryUsed();
    mem += gpu_mask.GetMemoryUsed();
    mem += gpu_species.GetMemoryUsed();
    mem += gpu_age.GetMemoryUsed();
    mem += gpu_carbonPool.GetMemoryUsed();
    mem += gpu_canopy.GetMemoryUsed();
    mem += gpu_lif.GetMemoryUsed();
    mem += gpu_stamp.GetMemoryUsed();

    // Buffers
    mem += gpu_tempBounds.GetMemoryUsed(GL_SHADER_STORAGE_BUFFER);

    size_t gib, mib, kib;
    System::BytesToGib(mem, gib, mib, kib);

    return QString("%1 Gib, %2 MiB and %3 KiB (or %4 bytes)").arg(gib).arg(mib).arg(kib).arg(mem);
}

QString EcosystemSimulationGPU::GetMemoryUsedSpecies() const
{
    size_t mem = 0;
    // Textures
    mem += gpu_boost.GetMemoryUsed();
    mem += gpu_sProbs.GetMemoryUsed();
    mem += gpu_seedCount.GetMemoryUsed();
    mem += gpu_seedHeight.GetMemoryUsed();
    mem += gpu_regenLayers.GetMemoryUsed();
    mem += gpu_lips.GetMemoryUsed();

    // Buffers
    mem += gpu_sParams.GetMemoryUsed(GL_SHADER_STORAGE_BUFFER);

    mem /= nbSpecies;

    size_t gib, mib, kib;
    System::BytesToGib(mem, gib, mib, kib);

    return QString("%1 Gib, %2 MiB and %3 KiB (or %4 bytes)").arg(gib).arg(mib).arg(kib).arg(mem);
}

bool EcosystemSimulationGPU::isGPU() const
{
    return true;
}