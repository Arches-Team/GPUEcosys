#pragma once

#include "gpu-wrapper.h"
#include "scalarfield.h"
#include "heightfield.h"
#include "cpu.h"
#include "gpu-texture.h"
#include "resource_map.h"
#include "ecosystem-data.h"
#include "gpu-timer.h"

#include <cstdint>
#include <vector>

struct TreeInfo
{
    bool alive;
    bool isSapling;

    int s;
    double h;
    int cx, cy; // Cell coordinates
    double x, y; // World coordinates

    double C; // Carbon pool (mature trees only)
    int n; // Number of trees (saplings only)
};

struct Timings
{
    // CPU wall-clock, in milliseconds. Includes host-side overhead (uniform uploads,
    // driver calls) and, as long as GPUSync() blocks, the GPU work as well.
    int totalYear = 0;

    int canopy = 0;
    int lif = 0;
    int regenLayers = 0;
    int individuals = 0;
    int seedProb = 0;
    int seeding = 0;

    // Pure kernel time on the GPU, in nanoseconds (GL_TIME_ELAPSED around each dispatch).
    // Zero until the queries have been collected at the end of the run.
    uint64_t gpuCanopy = 0;
    uint64_t gpuStamp = 0;
    uint64_t gpuLif = 0;
    uint64_t gpuRegenLayers = 0;
    uint64_t gpuIndividuals = 0;
    uint64_t gpuSeedSource = 0;
    uint64_t gpuSeedProb = 0;
    uint64_t gpuSeeding = 0;

    uint64_t GpuTotal() const;

    Timings operator+(const Timings& other);
    Timings operator/(double n);
};

struct LocalStats
{
    int x, y; // Observed cell coordinates
    QVector<double> years;

    QVector<QVector<double>> individualHeights;
    QVector<QVector<double>> saplingHeights;
};

struct GlobalStats
{
    QVector<double> years;

    QVector<QVector<double>> basalArea;
    double avgHeight = 0;
    int nbTrees = 0;
};

class EcosystemSimulation
{
public:
    EcosystemSimulation(ResourceData&, EcosystemData&);
    ~EcosystemSimulation();

    void SimulateYear(int nb = 1, bool debug = false);
    void SimulateFire(int iterations);
    void Clear();

    void Load();
    virtual void Init() = 0;
    virtual void LoadParams() = 0;
    virtual void LoadResources() = 0;
    virtual void LoadEcosystem(bool updateTrees = true) = 0;
    virtual void UpdateVisualization(int mode = 0) = 0;
    void UpdateTerrainShading();

    void fillEcosystemData();
    void SetTempChange(double, int, int);
    void SetMoistChange(double, int, int);

    virtual void ComputeSeedProb() = 0;
    virtual void ComputeMask(int s, int r, double t, bool useThresh = true) = 0;
    virtual void Seeding() = 0;
    virtual void DebugVigor(int, int, double, bool, bool, bool, bool, bool, bool, bool, bool) = 0;
    virtual void StartFire(QPoint p) = 0;

    virtual QString GetMemoryUsed() const = 0;
    virtual QString GetMemoryUsedCommon() const = 0;
    virtual QString GetMemoryUsedSpecies() const = 0;
    virtual bool isGPU() const = 0;

    void enableSeeding(bool enable) { doSeeding = enable; }
    void enableDeath(bool enable) { doDeath = enable; }
    void enableWind(bool enable) { useWind = enable; }
    void SetWind(Vector2 windDir, double windSpeed);

    virtual ScalarField2 GetLIF() const = 0;
    virtual ScalarField2 GetAge() const = 0;
    virtual ScalarField2 GetSpecies() const = 0;
    virtual ScalarField2 GetBurnState() const = 0;
    virtual ScalarField2 GetCarbonPool() const = 0;
    virtual ScalarField2 GetBoost(int s) const = 0;
    virtual ScalarField2 GetRegenLayers(int s) const = 0;
    virtual ScalarField2 GetSeedProbs(int s) const = 0;
    virtual ScalarField2 GetCanopyMaxHeight() const = 0;
    virtual ScalarField2 GetCanopyAvgHeight() const = 0;
    virtual ScalarField2 GetDebug() const = 0;

    ScalarField2 GetSaplingsNb(int s) const;
    size_t GetSaplingsCount();

    const QVector<TreeInfo>& GetTreeInstances();
    QVector<TreeInfo> GetSaplings(int& nb, QVector<int>& h);
    //! Overload reusing regeneration layers the caller has already read back
    QVector<TreeInfo> GetSaplings(int& nb, QVector<int>& h, const QVector<ScalarField2>& regenLayers);
    size_t GetNbTrees();
    int GetYear() { return year; }

    void DrawSymbolicDisk(QPainter& painter, QVector<TreeInfo>& trees, Box2 box, Vector2 scale, int borderWidth, int useAlpha);
    void DrawSymbolicDisk(QPainter& painter, double res, Box2 box, const QImage& bg, bool showSaplings, bool useAlpha = true);

    QImage GetMapBranches() const;

    QString GetLastRunTime() const { return lastRunTime; }
    Timings GetAverageTimings() const { return averageTimings; }

    /*!
    * \brief Multi-line timing report for exports.
    *
    * Reports the wall clock and the GPU breakdown. The per-pass CPU fields of Timings are
    * deliberately left out: since the passes are ordered by memory barriers rather than by
    * a blocking sync, they only measure host submission and read as zero.
    */
    QString GetTimingsReport() const;

    void SetLimit(double l) { limit = l; }

    void GetStats();
    // All three take the grids GetStats() read back once, rather than each fetching its own
    void GetLocalStats(const ScalarField2& species, const ScalarField2& age,
        const QVector<ScalarField2>& regenLayers);
    void GetGlobalStats(const ScalarField2& species, const ScalarField2& age,
        const QVector<ScalarField2>& regenLayers);
    void ComputeSaplingsBasalArea(const QVector<ScalarField2>& regenLayers);
    void Observe(Vector2 pos);

    QVector<QVector<double>> BasalArea(QVector<double>& years) const;
    QVector<QVector<double>> HeightsHistogram(QVector<double>& heights);
    QVector<double> HeightsHistogram();

protected:
    virtual void ComputeCanopyHeight() = 0;
    virtual void ComputeLIF() = 0;
    virtual void StepRegenLayers() = 0;
    //! Runs the whole growth season: the implementation owns the month loop
    virtual void StepIndividuals() = 0;
    virtual void StepFire() = 0;

    // GPU timing hooks. No-ops for a non-GPU implementation.
    virtual void ResetGPUTimings() {}
    virtual void BeginGPUTimingFrame() {}
    virtual void CollectGPUTimings(std::vector<Timings>&) {}

    //! Block until the year is actually finished on the device, so that the wall-clock
    //! measurement means something and the readbacks that follow see the final state.
    virtual void SyncEndOfYear() {}

    static QString GetTimeString(int);
    static QString GetGpuTimeString(uint64_t ns);

    virtual void Delete() {}

    void ComputeTreeInstances();

    unsigned int DecodeSpecies(unsigned char data) const;
    unsigned int DecodeBurnState(unsigned char data) const;
    ScalarField2 CombineSpeciesBurn(const ScalarField2& s, const ScalarField2& b);

    uint32_t Hash(uint32_t x, uint32_t y, uint32_t z);
protected:
    int year;  // Current year

    Box2 box; // Box of the simulated area
    int sizeX; // size of the simulated area in m
    int sizeY; // size of the simulated area in m

    // Grid dimensions in cells. The coarse ones are rounded *up* so that the uv / 5 and
    // uv / 10 conversions the shaders perform always land inside them: when the terrain is
    // not a multiple of 20 m, a plain division leaves the last row and column indexing past
    // the end of the canopy and seed textures. Such a read returns zero by specification,
    // so the border was silently wrong rather than crashing.
    int nx2, ny2;   // 2 m cells: species, age, lif, regen layers, resources
    int nx10, ny10; // 10 m cells: canopy, boost
    int nx20, ny20; // 20 m cells: seed probabilities and counts

    ResourceData& resourceData;
    EcosystemData& ecosystemData;
    QImage terrainShading;

    QVector<TreeInfo> instances;
    bool areInstancesDirty = true;

    // Counting trees does not need the instance list, but the result still has to be cached:
    // callers such as Worker::play() query it after doneCurrent(), and a readback without a
    // current GL context silently returns zeros -- which the tree predicate accepts, so every
    // cell of the grid would be counted as a tree.
    size_t cachedNbTrees = 0;
    bool isNbTreesDirty = true;

    // Abiotic conditions change over time
    double tempChange = 0; // Change in degrees Celsius (additive)
    int tempChangeStart = -1;
    int tempChangeEnd = -1;
    double moistChange = 1; // Change in the moisture index (multiplicative)
    int moistChangeStart = -1;
    int moistChangeEnd = -1;

    // WildFire
    Vector2 windDir;
    double windSpeed;

    // Misc. settings
    bool doSeeding = true;
    bool doDeath = true;
    int nbSpecies;
    bool useWind = false;

    int maxLips = 0;
    // Largest offset at which any loaded LIP is non-zero, in cells. The patches are stored
    // centered in a dimLIP slot padded with zeros, so the LIF gather can stop here. In
    // practice the tallest bucket uses the whole slot, so this global bound buys nothing.
    int maxLipRadius = 0;
    // Same bound per height bucket, as a prefix max: a 10m block whose tallest tree falls in
    // bucket b may also hold trees of every smaller bucket, so the bound must cover 0..b.
    QVector<int> lipRadiusPerBucket;
    static constexpr int dimLIP = 41;
    static constexpr int dimSeed = 25;

    // Growth cycles (long = 9 months, short = 6 months)
    static constexpr int start_lgc = 2;
    static constexpr int end_lgc = 10;
    static constexpr int start_sgc = 3;
    static constexpr int end_sgc = 8;
    static constexpr int seedMonth = 3;

    double limit = 4.0;

    QString lastRunTime;
    Timings averageTimings;

    LocalStats localStats;
    GlobalStats globalStats;
};

class EcosystemSimulationGPU : public EcosystemSimulation
{
public:
    EcosystemSimulationGPU(ResourceData&, EcosystemData&);

    void Init() override;
    void LoadParams() override;
    void LoadResources() override;
    void LoadEcosystem(bool updateTrees = true) override;
    void UpdateVisualization(int mode = 0) override;

    void CompileShaders();

    void ComputeSeedProb() override;
    void ComputeMask(int s, int r, double t, bool useThresh = true) override;
    void Seeding() override;
    void DebugVigor(int, int, double, bool, bool, bool, bool, bool, bool, bool, bool) override;
    void StartFire(QPoint p) override;

    QString GetMemoryUsed() const override;
    QString GetMemoryUsedCommon() const override;
    QString GetMemoryUsedSpecies() const override;

    bool isGPU() const override;

    ScalarField2 GetLIF() const override;   
    ScalarField2 GetAge() const override;
    ScalarField2 GetSpecies() const override;
    ScalarField2 GetBurnState() const override;
    ScalarField2 GetCarbonPool() const override;
    ScalarField2 GetBoost(int s) const override;
    ScalarField2 GetRegenLayers(int s) const override;
    ScalarField2 GetSeedProbs(int s) const override;
    ScalarField2 GetCanopyMaxHeight() const override;
    ScalarField2 GetCanopyAvgHeight() const override;
    ScalarField2 GetDebug() const override;
    GLuint GetOutputTexture();

protected:
    void ComputeCanopyHeight() override;
    void ComputeLIF() override;
    void StepRegenLayers() override;
    void StepIndividuals() override;
    void StepFire() override;

    void ResetGPUTimings() override;
    void BeginGPUTimingFrame() override;
    void CollectGPUTimings(std::vector<Timings>&) override;
    void SyncEndOfYear() override;

    void Delete() override;

private:
    //! Upload the packed species / burn state, one byte per cell as the texture stores it
    void UploadSpecies(const ScalarField2& combined);
    void ComputeStamp();
    void ComputeSeedSource();
    void DispatchCompute(int x, int y);

    //! Order two compute passes that talk to each other through images. No fence, no CPU
    //! stall: this is all a pass of the yearly pipeline needs.
    void GPUBarrier();

    //! Full stall: everything visible to everyone, and the CPU waits for the device. Only
    //! before a readback, before the widget samples a texture, or to close a year.
    void GPUSync();

private:
    // ====== Buffers ======
    GLBuffer gpu_sParams; // Species parameters
    GLBuffer gpu_colors; // Species colors
    GLBuffer gpu_tempBounds; // Min / max of temperature
    GLBuffer gpu_lipRadius; // Largest non-zero LIP offset, per height bucket

    // ====== Textures ======
    GLTexture2DArray gpu_resources; // Temperature, Moisture, Sunlight, Slope
    GLTexture2DArray gpu_boost; // Vigor boost maps
    GLTexture2D gpu_mask;// Mask of each tree

    GLTexture2DArray gpu_regenLayers; // Regeneration layers
    GLTexture2D gpu_species;// Species of each tree
    GLTexture2D gpu_age;// Age of each tree
    GLTexture2D gpu_carbonPool;// Carbon Pool of each tree

    GLTexture2DArray gpu_sProbs; // Seed probability
    GLTexture2DArray gpu_seedCount; // Nb of individuals per 20m cell, per species
    GLTexture2DArray gpu_seedHeight; // Sum of their heights, per 20m cell, per species
    GLTexture2D gpu_canopy; // Canopy Height
    GLTexture2D gpu_lif; // Light Interference Field
    GLTexture2D gpu_stamp; // Per cell: LIP scale and height bucket (see compute_stamp.glsl)
    GLTexture2DArray gpu_lips; // Light Interference Patterns

    GLTexture2D gpu_terrainShaded;
    GLTexture2D gpu_albedo;
    GLTexture2D gpu_debug;
    
    GPUTimer gpuTimer;

    // ====== Shaders ======
    GLShader shader_lif;
    GLShader shader_sProb;
    GLShader shader_seedSource;
    GLShader shader_stamp;
    GLShader shader_canopy;
    GLShader shader_seeding;
    GLShader shader_regenLayers;
    GLShader shader_individuals;
    GLShader shader_visu;
    GLShader shader_debug;
    GLShader shader_fire;
    GLShader shader_mask;

    static constexpr int localSizeX = 8;
    static constexpr int localSizeY = 8;
};
