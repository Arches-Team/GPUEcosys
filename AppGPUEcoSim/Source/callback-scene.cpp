#include "qte.h"
#include "heightfieldshader.h"

inline QString GetTimeString(int t)
{
    int m, s, ms;
    System::MsToMinutes(t, m, s, ms);
    return QString("%1:%2:%3").arg(m).arg(s, 2, 10, QChar('0')).arg(ms, 3, 10, QChar('0'));
}

void QteWindow::createActionsScene()
{
    // Scenes
    connect(uiw.scene_pyrennees_2km, SIGNAL(clicked()), this, SLOT(CallScenePyrennees2km()));
    connect(uiw.scene_pyrennees_4km, SIGNAL(clicked()), this, SLOT(CallScenePyrennees4km()));
    connect(uiw.scene_pyrennees_4km_sparse, SIGNAL(clicked()), this, SLOT(CallScenePyrennees4kmSparse()));
    connect(uiw.scene_pyrennees_8km, SIGNAL(clicked()), this, SLOT(CallScenePyrennees8km()));

    connect(uiw.scene_berchtesgaden_2km, SIGNAL(clicked()), this, SLOT(CallSceneBerchtesgaden2km()));
    connect(uiw.scene_island, SIGNAL(clicked()), this, SLOT(CallSceneIsland()));

    // Figures
    connect(uiw.fig_teaser, SIGNAL(clicked()), this, SLOT(CallTeaser()));
    connect(uiw.fig_13, SIGNAL(clicked()), this, SLOT(CallFig13()));
    connect(uiw.fig_14, SIGNAL(clicked()), this, SLOT(CallFig14()));
    connect(uiw.fig_16, SIGNAL(clicked()), this, SLOT(CallFig16()));
    connect(uiw.fig_17, SIGNAL(clicked()), this, SLOT(CallFig17()));
    connect(uiw.fig_18, SIGNAL(clicked()), this, SLOT(CallFig18()));
    connect(uiw.fig_21, SIGNAL(clicked()), this, SLOT(CallFig21()));

    // Misc.
    connect(uiw.ram, SIGNAL(clicked()), this, SLOT(CallPrintRam()));

    // Timings
    connect(uiw.timings_all, SIGNAL(clicked()), this, SLOT(CallTimingsAll()));
    connect(uiw.timings_2km, SIGNAL(clicked()), this, SLOT(CallTimings2km()));
    connect(uiw.timings_4km, SIGNAL(clicked()), this, SLOT(CallTimings4km()));
    connect(uiw.timings_8km, SIGNAL(clicked()), this, SLOT(CallTimings8km()));
}

void QteWindow::CallScenePyrennees2km()
{
    ecosystemFile = SOLUTION_DIR "data/Presets/fixSun_v18.json";

    const QString dataDir = SOLUTION_DIR "data/Environments/Pyrennees_2km";
    LoadEnv(dataDir);
    SetViewCamera(Camera(Vector(-1432.55, -2092.32, 3797.96), Vector(-19.6953, -135.712, 1440.63), Vector(0.409062, 0.566497, 0.715367), 1, 1, 1, 50000));
}

void QteWindow::CallScenePyrennees4km()
{
    ecosystemFile = SOLUTION_DIR "data/Presets/fixSun_v18.json";

    const QString dataDir = SOLUTION_DIR "data/Environments/Pyrennees_4km";
    LoadEnv(dataDir);
    SetViewCamera(Camera(Vector(-2235.27, -5015.88, 5646.25), Vector(238.01, 34.4068, 1203.47), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
}

void QteWindow::CallScenePyrennees4kmSparse()
{
    ecosystemFile = SOLUTION_DIR "data/Presets/fixSun_v18.json";

    const QString dataDir = SOLUTION_DIR "data/Environments/Pyrennees_4km_sparse";
    LoadEnv(dataDir);
    SetViewCamera(Camera(Vector(-2330.01, -5099.34, 5498.64), Vector(143.272, -49.0544, 1055.86), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
}

void QteWindow::CallScenePyrennees8km()
{
    ecosystemFile = SOLUTION_DIR "data/Presets/fixSun_v18.json";

    const QString dataDir = SOLUTION_DIR "data/Environments/Pyrennees_8km";
    LoadEnv(dataDir);
    SetViewCamera(Camera(Vector(5142.97, 9589.17, 6539.25), Vector(176.109, 361.957, 558.135), Vector(-0.234954, -0.436487, 0.86849), 1, 1, 1, 50000));
}

void QteWindow::CallSceneBerchtesgaden2km()
{
    ecosystemFile = SOLUTION_DIR "data/Presets/Berchtesgaden_v9.json";

    const QString dataDir = SOLUTION_DIR "data/Environments/Berchtesgaden_2km";
    LoadEnv(dataDir);
    SetViewCamera(Camera(Vector(3892.74, -2284.09, 2404.5), Vector(274.004, -34.4474, 1345.27), Vector(-0.204881, 0.127367, 0.970465), 1, 1, 1, 50000));
}

void QteWindow::CallSceneIsland()
{
    ecosystemFile = SOLUTION_DIR "data/Presets/Berchtesgaden_v9.json";

    const QString dataDir = SOLUTION_DIR "data/Environments/Island";
    LoadEnv(dataDir);
    SetViewCamera(Camera(Vector(6879.68, 2642.25, 6096.29), Vector(335.435, 95.2187, 767.28), Vector(-0.563342, -0.219254, 0.796601), 1, 1, 1, 50000));
}

void QteWindow::CallTimingsAll()
{
    CallTimings2km();
    CallTimings4km();
    CallTimings8km();
}

void QteWindow::CallTimings2km()
{
    CallScenePyrennees2km();
    sim->SimulateYear(100, false);

    sim->SimulateYear(600, false);

    QFile file(SOLUTION_DIR "results/timing_2km.txt"); 
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file for writing:" << file.errorString();
        return;
    }

    QTextStream out(&file);

    out << "600 years run in " << sim->GetLastRunTime() << "\n";
    out << "Number of mature trees: " << sim->GetNbTrees() << "\n";
    out << "Number of saplings: " << sim->GetSaplingsCount() << "\n";
    out << "GPU Resources used: " << sim->GetMemoryUsed() << "\n";

    out << sim->GetTimingsReport();

    CallPBR(SOLUTION_DIR "results/pbr_2km.jpg");

    CallEstimateSaplings();

    file.close();
}

void QteWindow::CallTimings4km()
{
    CallScenePyrennees4km();
    sim->SimulateYear(100, false);

    sim->SimulateYear(600, false);

    QFile file(SOLUTION_DIR "results/timing_4km.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file for writing:" << file.errorString();
        return;
    }

    QTextStream out(&file);

    out << "600 years run in " << sim->GetLastRunTime() << "\n";
    out << "Number of mature trees: " << sim->GetNbTrees() << "\n";
    out << "Number of saplings: " << sim->GetSaplingsCount() << "\n";
    out << "GPU Resources used: " << sim->GetMemoryUsed() << "\n";

    out << sim->GetTimingsReport();
    CallPBR(SOLUTION_DIR "results/pbr_4km.jpg");

    CallEstimateSaplings();

    file.close();
}

void QteWindow::CallTimings8km()
{
    CallScenePyrennees8km();
    sim->SimulateYear(100, false);

    sim->SimulateYear(600, false);

    QFile file(SOLUTION_DIR "results/timing_8km.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file for writing:" << file.errorString();
        return;
    }

    QTextStream out(&file);

    out << "600 years run in " << sim->GetLastRunTime() << "\n";
    out << "Number of mature trees: " << sim->GetNbTrees() << "\n";
    out << "Number of saplings: " << sim->GetSaplingsCount() << "\n";
    out << "GPU Resources used: " << sim->GetMemoryUsed() << "\n";

    out << sim->GetTimingsReport();

    CallPBR(SOLUTION_DIR "results/pbr_8km.jpg");

    CallEstimateSaplings();

    file.close();
}

void QteWindow::CallPrintRam()
{
    qDebug() << "GPU Resources used in total: " << sim->GetMemoryUsed();
    qDebug() << "GPU Resources used for common parts: " << sim->GetMemoryUsedCommon();
    qDebug() << "GPU Resources used for species-specific parts (per species): " << sim->GetMemoryUsedSpecies();
}

void QteWindow::DoLandslide()
{
    QImage img(SOLUTION_DIR "data/Control/landslide.png");
    ScalarField2 mask(img);

    ecosystemData.Delete(mask);
    sim->LoadEcosystem();

    double minH = 1346;
    double maxH = 2069;
    img = QImage(SOLUTION_DIR "data/Environments/Pyrennees_4km_landslide/heightField.png");
    qDebug() << "HF: " << img.width() << ", " << img.height();
    Box2 box(4000, 4000);
    resourceData.SetHeightField(HeightField(box, img, minH, maxH));

    ScalarField2 slope = resourceData.GetHeightField().Slope(true);
    slope.Atan();
    resourceData.SetSlope(slope);

    img = QImage(SOLUTION_DIR "data/Environments/Pyrennees_4km_landslide/moisture.png");
    resourceData.GetMoistMap().GetImage() = img;
    resourceData.GetMoistMap().SyncScalarField();

    for (int i = 0; i < 12; ++i) {
        img = QImage(QString(SOLUTION_DIR "data/Environments/Pyrennees_4km_landslide/sunlight_%1.png").arg(i));
        resourceData.GetSunMap().GetImage(i) = img;
    }
    resourceData.GetSunMap().SyncScalarFields();

    TemperatureMap& tm = resourceData.GetTempMap();

    ScalarField2 temp = ComputeTemperature();
    img = temp.CreateImage(0, 1);

    double heightTemps;

    QVector<double> minT(12), maxT(12), valueSimT(12);
    valueSimT = { 2.7, 1.9, 4.5, 7.3, 10.8, 14.9, 17.8, 17.6, 13.9, 10.2, 5.4, 3.4 };
    heightTemps = 1386; // Gresolet

    double deltaMin = (minH - heightTemps) / -153.8;
    double deltaMax = (maxH - heightTemps) / -153.8;
    for (int i = 0; i < 12; ++i)
    {
        minT[i] = deltaMin + valueSimT[i];
        maxT[i] = deltaMax + valueSimT[i];
    }

    tm = TemperatureMap(box, img, minT, maxT);

    sim->UpdateTerrainShading();
    sim->LoadResources();

    // Set the heightfield
    mayawidget->SetHeightField(resourceData.GetPtrHeightField());
    mayawidget->UseExternAlbedo(true);
    mayawidget->SetSteps(4096);

    // Set the heightfield for the editor
    mayawidgetEditor->SetHeightField(resourceData.GetPtrHeightField());
    mayawidgetEditor->SetSteps(4096);

    sim->UpdateVisualization();
}

void QteWindow::ApplyMask(QImage& img, const QImage& mask, QColor color)
{
    // Build colored overlay
    QImage overlay(mask.size(), QImage::Format_ARGB32);
    overlay.fill(color);

    // Convert mask to alpha
    overlay.setAlphaChannel(mask);

    // Composite
    QPainter painter(&img);
    painter.drawImage(0, 0, overlay);
    painter.end();
}

void QteWindow::CallTeaser()
{
    CallScenePyrennees4km();
    sim->SimulateYear(600);

    SetViewCamera(Camera(Vector(-2330.01, -5099.34, 5698.64), Vector(143.272, -49.0544, 1255.86), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_teaser_1.jpg");

    sim->fillEcosystemData();

    QImage img(SOLUTION_DIR "data/Control/boost-valley-4km.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 1) = img;
    img = QImage(SOLUTION_DIR "data/Control/boost-valley-4km-bis.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 4) = img;

    ecosystemData.SyncScalarFields();
    sim->LoadEcosystem();

    uiw.climate_temp->setValue(8);
    uiw.climate_start->setValue(600);
    uiw.climate_end->setValue(601);
    CallTempChange();
    sim->SimulateYear(300);
    sim->fillEcosystemData();

    SetViewCamera(Camera(Vector(-2330.01, -5099.34, 5698.64), Vector(143.272, -49.0544, 1255.86), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_teaser_2.jpg");

    DoLandslide();
    if (sim) sim->LoadEcosystem();

    SetViewCamera(Camera(Vector(-2330.01, -5099.34, 5698.64), Vector(143.272, -49.0544, 1255.86), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_teaser_3.jpg");
    SetViewCamera(Camera(Vector(-781.671, -315.354, 2525.27), Vector(-292.378, 683.751, 1646.34), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_teaser_3_zoom_1.jpg");
    SetViewCamera(Camera(Vector(-1125.81, -2794.34, 2554.2), Vector(166.177, -87.9946, 1147.3), Vector(0.182977, 0.383283, 0.905325), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_teaser_3_zoom_2.jpg");

    UpdateStep(sim->GetYear());
    UpdateBatch();
}

void QteWindow::CallFig13()
{
    CallScenePyrennees2km();

    sim->SimulateYear(600);

    Box2 box(Vector2(-950, 550), Vector2(-550, 950));

    SetViewCamera(Camera(Vector(-518.548, 1419.15, 1748.71), Vector(-768.626, 799.884, 1582.69), Vector(-0.0903335, -0.223691, 0.970465), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_13.1.jpg", box);

    sim->fillEcosystemData();

    QImage img(SOLUTION_DIR "data/Control/boost-pyrennees.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 4) = img;

    img = QImage(SOLUTION_DIR "data/Control/boost-pyrennees-bis.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 0) = img;
    ecosystemData.GetImage(MapSpecies::BOOST, 1) = img;
    ecosystemData.GetImage(MapSpecies::BOOST, 2) = img;
    ecosystemData.GetImage(MapSpecies::BOOST, 3) = img;
    ecosystemData.SyncScalarFields();

    sim->LoadEcosystem();
    
    sim->SimulateYear(50);
    SetViewCamera(Camera(Vector(-518.548, 1419.15, 1748.71), Vector(-768.626, 799.884, 1582.69), Vector(-0.0903335, -0.223691, 0.970465), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_13.2.jpg", box);

    sim->SimulateYear(50);
    SetViewCamera(Camera(Vector(-518.548, 1419.15, 1748.71), Vector(-768.626, 799.884, 1582.69), Vector(-0.0903335, -0.223691, 0.970465), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_13.3.jpg", box);

    sim->SimulateYear(50);
    SetViewCamera(Camera(Vector(-518.548, 1419.15, 1748.71), Vector(-768.626, 799.884, 1582.69), Vector(-0.0903335, -0.223691, 0.970465), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_13.4.jpg", box);

    UpdateStep(sim->GetYear());
    UpdateBatch();
}

void QteWindow::CallFig14()
{
    CallScenePyrennees2km();
    sim->SimulateYear(600);

    Box2 box(Vector2(500, -1000), Vector2(1000, -500));
    SetViewCamera(Camera(Vector(559.488, -1294.76, 2339.1), Vector(924.766, -509.501, 1193.07), Vector(0.336493, 0.723376, 0.60291), 1, 1, 1, 50000));

    DrawDisksPBR(SOLUTION_DIR "results/fig_14_1.jpg", box);

    UpdateStep(sim->GetYear());
    UpdateBatch();
}

void QteWindow::CallFig16()
{
    CallScenePyrennees2km();
    sim->SimulateYear(600);

    Box2 box(Vector2(-1000, 450), Vector2(-750, 700));
    SetViewCamera(Camera(Vector(-1152.41, 807.717, 1928.74), Vector(-368.61, 94.5752, 1158.3), Vector(0.434961, -0.395752, 0.80882), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_16_1.jpg", box);

    uiw.climate_temp->setValue(4);
    uiw.climate_start->setValue(600);
    uiw.climate_end->setValue(601);

    sim->SimulateYear(100, false);
    SetViewCamera(Camera(Vector(-1152.41, 807.717, 1928.74), Vector(-368.61, 94.5752, 1158.3), Vector(0.434961, -0.395752, 0.80882), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_16_2.jpg", box);

    sim->SimulateYear(100, false);
    SetViewCamera(Camera(Vector(-1152.41, 807.717, 1928.74), Vector(-368.61, 94.5752, 1158.3), Vector(0.434961, -0.395752, 0.80882), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_16_3.jpg", box);

    sim->SimulateYear(100, false);
    SetViewCamera(Camera(Vector(-1152.41, 807.717, 1928.74), Vector(-368.61, 94.5752, 1158.3), Vector(0.434961, -0.395752, 0.80882), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_16_4.jpg", box);

    UpdateStep(sim->GetYear());
    UpdateBatch();
}

void QteWindow::CallFig17()
{
    CallScenePyrennees4kmSparse();
    sim->SimulateYear(600);
    sim->fillEcosystemData();

    HeightFieldShader hf(resourceData.GetHeightField());
    QImage terrain = hf.Eric();
    QImage water(SOLUTION_DIR "data/Environments/Pyrennees_4km_sparse/mask_water.png");
    ApplyMask(terrain, water, QColor(167, 199, 231));

    SetViewCamera(Camera(Vector(-2330.01, -5099.34, 5498.64), Vector(143.272, -49.0544, 1055.86), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_17_1.jpg", terrain);

    QImage img(SOLUTION_DIR "data/Environments/Pyrennees_4km_sparse/mask_all.png");
    ScalarField2 mask(img);

    resourceData.GetMaskMap().GetScalarField() = mask;
    resourceData.SyncImages();
    sim->LoadResources();

    ecosystemData.Delete(mask);
    ecosystemData.SyncImages();

    sim->LoadEcosystem();

    QImage fields(SOLUTION_DIR "data/Environments/Pyrennees_4km_sparse/mask_grass_fields.png");
    ApplyMask(terrain, fields, QColor(246, 232, 168));

    SetViewCamera(Camera(Vector(-2330.01, -5099.34, 5498.64), Vector(143.272, -49.0544, 1055.86), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_17_2.jpg", terrain);

    img = QImage(SOLUTION_DIR "data/Control/square.png");
    mask = ScalarField2(img);

    ApplyMask(terrain, img, QColor(246, 232, 168));

    ecosystemData.Delete(mask);
    ecosystemData.SyncImages();
    sim->LoadEcosystem();

    SetViewCamera(Camera(Vector(-2330.01, -5099.34, 5498.64), Vector(143.272, -49.0544, 1055.86), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_17_3.jpg", terrain);

    img = QImage(SOLUTION_DIR "data/Control/fir.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 2) = img;
    img = QImage(SOLUTION_DIR "data/Control/beech.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 4) = img;

    ecosystemData.SyncScalarFields();
    sim->LoadEcosystem();

    sim->SimulateYear(100);

    SetViewCamera(Camera(Vector(-2330.01, -5099.34, 5498.64), Vector(143.272, -49.0544, 1055.86), Vector(0.272656, 0.556746, 0.784661), 1, 1, 1, 50000));
    
    CallPBR(SOLUTION_DIR "results/fig_17_4.jpg", terrain);

    Box2 box(Vector2(-1100, -900), Vector2(-600, -400));
    SetViewCamera(Camera(Vector(-1151.35, -1328.03, 1544.82), Vector(-588.667, -207.585, 642.81), Vector(0.262088, 0.521884, 0.811755), 1, 1, 1, 50000));
    DrawDisksPBR(SOLUTION_DIR "results/fig_14_2.jpg", box, terrain);

    UpdateStep(sim->GetYear());
    UpdateBatch();
}

void QteWindow::CallFig18()
{
    CallSceneIsland();

    sim->SimulateYear(600, false);
    sim->fillEcosystemData();
    Update();

    HeightFieldShader hf(resourceData.GetHeightField());
    QImage terrain = hf.Eric();
    QImage water(SOLUTION_DIR "data/Environments/Island/mask_water.png");

    ApplyMask(terrain, water, QColor(167, 199, 231));
    CallPBR(SOLUTION_DIR "results/fig_18_1.jpg", terrain);

    QImage img;

    // Zone 1 boost
    img = QImage(SOLUTION_DIR "data/Control/island_8.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 2) = img;

    // Zone 2 boost
    img = QImage(SOLUTION_DIR "data/Control/island_10.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 1) = img;
    img = QImage(SOLUTION_DIR "data/Control/island_9.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 0) = img;

    // Zone 3 sparsification
    img = QImage(SOLUTION_DIR "data/Control/island_4.png");
    ecosystemData.GetImage(MapSpecies::BOOST, 3) = img;

    ecosystemData.SyncScalarFields();
    sim->LoadEcosystem();

    sim->SimulateYear(600, false);
    sim->fillEcosystemData();
    Update();

    SetViewCamera(Camera(Vector(6879.68, 2642.25, 6096.29), Vector(335.435, 95.2187, 767.28), Vector(-0.563342, -0.219254, 0.796601), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_18_2.jpg", terrain);

    // Set fire start
    QPoint p(2787 / 2, 3415 / 2);
    Vector2 d(-0.5, -0.5);
    sim->SetWind(d, 1);
    sim->StartFire(p);
    sim->SimulateFire(1000);

    sim->fillEcosystemData();

    QPoint q(5173 / 2, 3065 / 2);
    sim->StartFire(q);
    sim->SimulateFire(1000);

    sim->fillEcosystemData();
    ScalarField2 burned = sim->GetBurnState();
    for (int i = 0; i < burned.VertexSize(); ++i)
        burned[i] = (burned[i] == 3);


    img = burned.CreateImage();
    img.save(SOLUTION_DIR "results/fig_18_burnMap.png");

    SetViewCamera(Camera(Vector(6879.68, 2642.25, 6096.29), Vector(335.435, 95.2187, 767.28), Vector(-0.563342, -0.219254, 0.796601), 1, 1, 1, 50000));
    CallPBR(SOLUTION_DIR "results/fig_18_3.jpg", terrain);

    UpdateStep(sim->GetYear());
    UpdateBatch();
}

void QteWindow::CallFig21()
{
    CallScenePyrennees4kmSparse();

    sim->SimulateYear(1165, false);
    sim->fillEcosystemData();
    Update();

    SetViewCamera(Camera(Vector(951.814, 398.367, 1446.05), Vector(954.396, 398.226, 1443.85), Vector(0.646031, -0.035307, 0.762494), 1, 1, 1, 50000));
    Box2 box(Vector2(1025, 390), Vector2(1080, 445));
    DrawDisks(SOLUTION_DIR "results/fig_21_1.jpg", box, true); // Year 900

    sim->SimulateYear(20, false);
    DrawDisks(SOLUTION_DIR "results/fig_21_2.jpg", box, true); // Year 920

    sim->SimulateYear(20, false);
    DrawDisks(SOLUTION_DIR "results/fig_21_3.jpg", box, true); // Year 940

    sim->SimulateYear(20, false);
    DrawDisks(SOLUTION_DIR "results/fig_21_4.jpg", box, true); // Year 960

    sim->SimulateYear(20, false);
    DrawDisks(SOLUTION_DIR "results/fig_21_5.jpg", box, true); // Year 980

    sim->SimulateYear(40, false);
    DrawDisks(SOLUTION_DIR "results/fig_21_6.jpg", box, true); // Year 1020

    sim->SimulateYear(40, false);
    DrawDisks(SOLUTION_DIR "results/fig_21_7.jpg", box, true); // Year 1060

    UpdateStep(sim->GetYear());
    UpdateBatch();
}