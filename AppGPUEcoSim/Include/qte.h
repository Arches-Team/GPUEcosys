#pragma once

#include "mayarender.h"
#include "heightfield.h"
#include "realtime.h"
#include "resource_map.h"
#include "ecosystem-simulation.h"

#include "uic_main.h" 

#include <sampling.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>

enum ToolEditor
{
    TOOL_NONE,
    TOOL_GAUSSIAN,
    TOOL_RANDOM
};

class MonthsEditorWidget :public QWidget
{
    Q_OBJECT
private:
    MapType type;
    QVector<double> values;
    int n;
    int p1X, p1Y, p2X, p2Y, p3X, p3Y;   // Position des points du graphique

    QString Ordinate;                 //!< Text en ordonnee
    double ordMinValue;               //!< Texte comme valeur minimale du tableau
    double ordMaxValue;               //!< Texte comme valeur maximale du tableau

    QColor color;                     //!< Couleur du graphique

    double x0, y0;                                    // Position precedente

public:
    MonthsEditorWidget(QWidget* parent = 0, QString = QString(" "), MapType = MapType::NONE, const QColor & = QColor(100, 100, 100), double = 1.0, double = 5.0);
    ~MonthsEditorWidget();

    void setTextOrdinate(const QString&);                   // Modification du texte : Ordonnee
    void setTextMinNumOrdinate(double);                     // Modification du texte : Valeur maximal
    void setTextMaxNumOrdinate(double);                     // Modification du texte : Valeur maximal

    void setValues(const QVector<double>&);                // Chargement des valeurs
    QVector<double>& getValues();                          // Recuperation des valeurs

    void setColor(const QColor&);                           // Modifie le materiau a dessiner

    void addValue(const double);                  // Ajoute des valeurs

protected:
    void paintEvent(QPaintEvent*);                          // Dessin du graph
    void paintTerrain(QPainter& p);
    void mouseMoveEvent(QMouseEvent*);                      // Action de deplacement de souris
    void mousePressEvent(QMouseEvent*);                     // Action de clic bouton de souris

signals:
    void changed(MapType);
};

class TRWEditor : public TerrainRaytracingWidget
{
    Q_OBJECT
private:
    // Data
    ResourceData& resourceData;
    EcosystemData& ecosystemData;

    // Editor selected maps
    int month = 0;
    MapType mapType = MapType::NONE;
    int species = 0;
    MapSpecies mapTypeS = MapSpecies::NONE;

    // Widget state
    Ray ray;
    bool isLeftClick = false;
    bool isRightClick = false;
    bool editEnabled = true; //!< Painting on the maps is allowed (disabled while the simulation runs)
    bool showControls = true; //!< Display the controls panel (toggled with H)
    Vector selectedPosition;

    // Tool
    std::unique_ptr<MayaSimpleRenderer> m_rendererTool{ nullptr };
    std::unique_ptr<MayaSimpleRenderer> m_rendererSel{ nullptr };
    Vector position;
    double radius = 100.;
    double intensity = 1.;
    int nbSamples = 50;
    ToolEditor tool = TOOL_GAUSSIAN;
    Random randTool = Random::R239;

public:

    TRWEditor(ResourceData& rd, EcosystemData& ed) :resourceData(rd), ecosystemData(ed) {}
    void paintGL() override;
    void RenderUiPanelEditor();
    void RenderUiPanelControls();
    void paintMAP();
    void paintMAPGaussian(QPainter&);
    void paintMAPRandom(QPainter&);
    void paintSFGaussian();
    void paintSFRandom();
    void drawCircle(std::unique_ptr<MayaSimpleRenderer>& renderer, const Circle2& circle, const Color& color);
    void updateTool();
    void updateImage();
    void GetMinMax(double& min, double& max);
    void GetMinMaxTool(double& min, double& max);

    void editSelectedMap(MapType mtype, int month);
    void setMonth(int m);
    void editSelectedMap(MapSpecies mtype, int species);
    void setSpecies(int m);

    void select(const Vector&);

    const QImage& getImage();
    ScalarField2& GetScalarField();

    void setTool(ToolEditor t) { tool = t; }
    void setEditEnabled(bool e) { editEnabled = e; update(); }

protected:
    QString GetMapName();
    QString GetMonthNameSpecies();

public slots:
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

    void setRadius(double r) { radius = r; updateTool(); }
    void setIntensity(double i) { intensity = i; updateTool(); }
    void setIntensity(int i);
    void setSamples(double s) { nbSamples = s; updateTool(); qDebug() << s; }

signals:
    void editCamera(const Camera&);
    void valueRadiusChanged(double);
    void valueIntensityChanged(double);
    void changedResources(const MapType&);
    void changedSpecies(const MapSpecies&);
    void selectionChanged(const Vector&);
};

class Worker : public QObject
{
    Q_OBJECT

public:
    Worker(QOpenGLContext* context, QOffscreenSurface* surface, EcosystemSimulation* sim, bool debug, QObject* parent = nullptr);
    ~Worker();

    void initContext();

public slots:
    void play();
    void pause();
    void resume();
    void stop();
    void reload();

    void steps(int);

    bool isPaused() const { return paused; }

signals:
    void finished();
    void stepCompleted(int);
    void batchCompleted();

private:
    bool debug;
    bool running;
    bool paused;
    bool init;
    int nbSteps; // Steps to complete
    int step; // Current step
    EcosystemSimulation* simulator;

    QOpenGLContext* context;
    QOffscreenSurface* surface;
};

class QteWindow : public QMainWindow
{
    Q_OBJECT
private:
    Ui::EcosystemSimulation uiw; //!< Interface : QtDesigner.

    TerrainRaytracingWidget* mayawidget;
    TRWEditor* mayawidgetEditor;

    ResourceData resourceData;
    EcosystemData ecosystemData;

    QString ecosystemFile;

    QThread* workerThread = nullptr;
    Worker* worker = nullptr;
    EcosystemSimulation* sim = nullptr;
    bool mapEditionEnabled = true; //!< Maps can be edited (false while the simulation runs)

    MonthsEditorWidget* monthEditorSunLight;

    HeightField crop;

public:
    QteWindow();
    ~QteWindow();

private:
    void createActions();
    void createActionsSimulation();
    void createActionsGlobalEnvironment();
    void createActionsEcosystem();
    void createActionsDisplayExport();
    void createActionsEditor();
    void SetMapEditionEnabled(bool enabled);
    void createActionsDebug();
    void createActionsScene();

    void LoadTerrain();
    void SetViewCamera(const Camera& camera);
    void LoadEnv(const QString&);
    void LoadEcosystem(const QString&);
    void LoadSim();

    QVector<ScalarField2> ComputeSunlight(const QVector<double>& cloudiness);
    ScalarField2 ComputeMoisture(const QVector<double>& precipitations);
    ScalarField2 ComputeTemperature();
    ScalarField2 ComputeMask();

    // 2D version
    void DrawDisks(const QString& file, Box2 box, bool showSaplings);
    // 3D version
    void DrawDisksPBR(const QString& file, Box2 box, const QImage& bg = QImage());

    //! Write one column per species (series[species][i]) against x to a csv file
    bool WriteCsv(const QString& file, const QString& xLabel, const QVector<double>& x, const QVector<QVector<double>>& series);

    bool ComputeEnv(const HeightField& heightField, QString dirName);
    void ApplyMask(QImage& img, const QImage& mask, QColor color);

public slots:
    void CallUpdatedCameraRight();
    void CallUpdatedCameraLeft();
 
    void CallChart(); // Basal area
    void CallSchema(); // 2D disks
    void CallHistHeights(); // Histogram of mature trees' height

    void CallDebugVigor(); // Vigor map visualization
    void CallDebugSun();

    // Scenes
    void CallScenePyrennees2km();
    void CallScenePyrennees4km();
    void CallScenePyrennees4kmSparse();
    void CallScenePyrennees8km();
    void CallSceneBerchtesgaden2km();
    void CallSceneIsland();

    // Timings
    void CallTimingsAll();
    void CallTimings2km();
    void CallTimings4km();
    void CallTimings8km();

    // Environment Change
    void DoLandslide();
    void CallTempChange();
    void CallMoistChange();

    // Figures
    void CallTeaser();
    void CallFig13();
    void CallFig14();
    void CallFig16();
    void CallFig17();
    void CallFig18();
    void CallFig21();

    // Misc.
    void CallPrintRam();

    // Simulation
    void CallSeed();
    void CallSimStart();
    void CallSimPause();
    void CallSimStop();

    void CallSimReload();
    void CallSimYear();
    void CallSimYears();

    void CallStartFire();
    void CallUpdateWind();

    void Update();
    void UpdateStep(int);
    void UpdateBatch();

    void CallPBR(QString file = SOLUTION_DIR "pbr_disks.jpg", const QImage& bg = QImage());
    void CallPBRExport(); // 3D render, asks for the output file

    void onSimulationFinished();

    // Global Environment
    void updateResourcesPixmap();

    // - Functions
    void CallLoadTerrain();
    void CallLoadEnv();
    void CallComputeEnv();

    // - Generate maps
    void CallEnvGenSunExposure();
    void CallEnvGenMoisture();
    void CallEnvGenTemperature();
    void CallEnvGenMask();

    // - Load maps
    void CallEnvLoadSunExposure();
    void CallEnvLoadMoisture();
    void CallEnvLoadTemperature();

    // - Edit maps
    void updateEditRange();
    void CallEnvEditSunExposure();
    void CallEnvEditMoisture();
    void CallEnvEditTemperature();
    void CallEnvEditMask();
    void CallEnvSaveMask();

    void setValueIntensity(double i);
    void setValueIntensity(int i);
    void CallSelectionChanged(const Vector&);
    void setSelection();

    // - Histo
    void CallHistoChanged(MapType);
    void CallMapChanged(const MapType&);
    void CallMapChanged(const MapSpecies&);

    // Ecosystem
    void CallInitEcoPyrenean();
    void CallInitEcoBerchtesgaden();

    void CallEcoSpeciesSelected(int);
    void CallEcoSpeciesEdited();
    void CallEcoSpeciesSelectColor();

    void CallEcoSpeciesUpdateData();

    void updateEcosystemPixmap();

    // - Editor / Viewer
    void CallEcoEditSeeds();
    void CallEcoEditSaplings();
    void CallEcoEditMature();
    void CallEcoEditBoost();
    void CallEcoViewLIF();

    void CallEcoRegenSeeds();
    void CallEcoClearSeeds();
    void CallEcoClearSaplings();
    void CallEcoClearMature();
    void CallEcoClearBoost();

    void CallUpdateTool();

    // - File Save/Load
    void CallSaveEcosystemParams();
    void CallLoadEcosystemParams();

    // Export Display
    void Call_ExportSaplings();
    void CallEstimateSaplings();
};
