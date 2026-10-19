#include "qte.h"
#include "abiotics.h"
#include "histogram.h"
#include <QtWidgets/QFileDialog>
#include <QtCore/QElapsedTimer>
#include <QtGui/QImageReader>
    
/*!
\brief Create callbacks between member slots and user interface.
*/
void QteWindow::createActionsGlobalEnvironment()
{
    // Terrain
    connect(uiw.terrain_loadImage, SIGNAL(clicked()), this, SLOT(CallLoadTerrain()));

    // Environment
    connect(uiw.compute_env, SIGNAL(clicked()), this, SLOT(CallComputeEnv()));
    connect(uiw.load_env, SIGNAL(clicked()), this, SLOT(CallLoadEnv()));

    // Generate maps
    connect(uiw.pushButton_Env_Gen_SunExposure, SIGNAL(clicked()), this, SLOT(CallEnvGenSunExposure()));
    connect(uiw.pushButton_Env_Gen_Moisture, SIGNAL(clicked()), this, SLOT(CallEnvGenMoisture()));
    connect(uiw.pushButton_Env_Gen_Temperature, SIGNAL(clicked()), this, SLOT(CallEnvGenTemperature()));
    connect(uiw.pushButton_Env_Gen_Mask, SIGNAL(clicked()), this, SLOT(CallEnvGenMask()));

    // Load maps
    connect(uiw.pushButton_Env_Load_SunExposure, SIGNAL(clicked()), this, SLOT(CallEnvLoadSunExposure()));
    connect(uiw.pushButton_Env_Load_Moisture, SIGNAL(clicked()), this, SLOT(CallEnvLoadMoisture()));
    connect(uiw.pushButton_Env_Load_Temperature, SIGNAL(clicked()), this, SLOT(CallEnvLoadTemperature()));

    // Edit maps
    connect(uiw.pushButton_Env_Edit_SunExposure, SIGNAL(clicked()), this, SLOT(CallEnvEditSunExposure()));
    connect(uiw.pushButton_Env_Edit_Moisture, SIGNAL(clicked()), this, SLOT(CallEnvEditMoisture()));
    connect(uiw.pushButton_Env_Edit_Temperature, SIGNAL(clicked()), this, SLOT(CallEnvEditTemperature()));
    connect(uiw.pushButton_Env_Edit_Mask, SIGNAL(clicked()), this, SLOT(CallEnvEditMask()));
    connect(uiw.pushButton_Env_Save_Mask, SIGNAL(clicked()), this, SLOT(CallEnvSaveMask()));

    // Climate Change
    connect(uiw.climate_temp, SIGNAL(valueChanged(double)), this, SLOT(CallTempChange()));
    connect(uiw.climate_start, SIGNAL(valueChanged(int)), this, SLOT(CallTempChange()));
    connect(uiw.climate_end, SIGNAL(valueChanged(int)), this, SLOT(CallTempChange()));

    connect(uiw.climate_moist, SIGNAL(valueChanged(double)), this, SLOT(CallMoistChange()));
    connect(uiw.climate_start_moist, SIGNAL(valueChanged(int)), this, SLOT(CallMoistChange()));
    connect(uiw.climate_end_moist, SIGNAL(valueChanged(int)), this, SLOT(CallMoistChange()));

    // Histo
    connect(monthEditorSunLight, SIGNAL(changed(MapType)), this, SLOT(CallHistoChanged(MapType)));

    connect(mayawidgetEditor, SIGNAL(changedResources(MapType)), this, SLOT(CallMapChanged(MapType)));
 
    connect(uiw.Environment_Month, SIGNAL(valueChanged(int)), this, SLOT(updateResourcesPixmap()));
}

void QteWindow::CallTempChange()
{
    if (sim)
        sim->SetTempChange(uiw.climate_temp->value(), uiw.climate_start->value(), uiw.climate_end->value());
}

void QteWindow::CallMoistChange()
{
    if (sim)
        sim->SetMoistChange(uiw.climate_moist->value(), uiw.climate_start->value(), uiw.climate_end->value());
}

/*!
* \brief Update the resources pixmap.
*/
void QteWindow::updateResourcesPixmap()
{
    int month = uiw.Environment_Month->value()-1;

    // Update pixmap
    QImage img = resourceData.GetHeightField().CreateImage();
    uiw.label_Image_Terrain->setPixmap(QPixmap::fromImage(img.scaled(141, 141).flipped(Qt::Orientation::Horizontal)));

    uiw.label_Image_SunExposure->setPixmap(resourceData.GetPixmap(MapType::SUNLIGHT, month));
    uiw.label_Image_Moisture->setPixmap(resourceData.GetPixmap(MapType::MOISTURE, month));
    uiw.label_Image_Temperature->setPixmap(resourceData.GetPixmap(MapType::TEMPERATURE , month));
    uiw.label_Image_Mask->setPixmap(resourceData.GetPixmap(MapType::MASK, month));

    mayawidgetEditor->setMonth(month);
}

/*!
* \brief Load a specific environment from a directory.
* \param dir Directory containing the environment.
*/
void QteWindow::LoadEnv(const QString& dir)
{
    CallSimStop();

    QImageReader::setAllocationLimit(1024);

    QElapsedTimer timer;
    timer.restart();

    qDebug() << "Loading Env from " << dir;
    QFile file(dir + "/info.txt");
    if (!file.open(QFile::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error, could not open info.txt file...";
        return;
    }

    QTextStream ts(&file);

    double dimX, dimY, minH, maxH;
    QVector<double> minS(12), maxS(12), valueSimS(12);
    QVector<double> minT(12), maxT(12), valueSimT(12);
    QVector<double> valueSimM(12);
    double minM, maxM;
    while (!ts.atEnd())
    {
        QString line = ts.readLine().trimmed();
        QStringList fields = line.split(" ");
        if (fields[0] == "heightField:")
        {
            dimX = fields[1].toDouble();
            dimY = fields[2].toDouble();
            minH = fields[3].toDouble();
            maxH = fields[4].toDouble();
        }
        else if (fields[0] == "sunlight:")
        {
            for (int i = 0; i < 12; ++i)
            {
                line = ts.readLine().trimmed();
                fields = line.split(" ");
                minS[i] = fields[0].toDouble();
                maxS[i] = fields[1].toDouble();
            }
        }
        else if (fields[0] == "moisture:")
        {
            line = ts.readLine().trimmed();
            fields = line.split(" ");
            minM = fields[0].toDouble();
            maxM = fields[1].toDouble();
        }
    }

    // Load height field
    QImage img(dir + "/heightField.png");
    int W = img.width();
    int H = img.height();
    int x = W / 2;
    int y = W / 2;

    uiw.label_Image_Terrain->setPixmap(QPixmap::fromImage(img.scaled(141, 141)));

    Box2 box(dimX, dimY); 
    resourceData.SetHeightField(HeightField(box, img, minH, maxH));

    qDebug() << "Heightfield loaded";

    uiw.terrain_dimX->setValue(dimX);
    uiw.terrain_dimY->setValue(dimY);
    uiw.terrain_minH->setValue(minH);
    uiw.terrain_maxH->setValue(maxH);

    img = QImage(dir + "/mask.png");
    MaskMap& mkm = resourceData.GetMaskMap();
    mkm = MaskMap(box, img);
    qDebug() << "Mask loaded";

    QVector<QImage> imgs(12);
    QVector<ScalarField2> maps(12);

    // Load sun exposure map
    for (int i = 0; i < 12; ++i)
    {
        QString name = QString(dir + "/sunlight_%1.png").arg(i);
        imgs[i] = QImage(name);
    }
    valueSimS = { 0.63, 0.65, 0.66, 0.57, 0.58, 0.65, 0.71, 0.65, 0.62, 0.58, 0.56, 0.61 };
    SunlightMap& slm = resourceData.GetSunMap();
    slm = SunlightMap(box, imgs, minS, maxS, valueSimS);

    qDebug() << "Sun maps loaded";

    // Load moisture map
    img = QImage(dir + "/moisture.png");
    valueSimM = {38.3, 40.1, 94.5, 126.9, 121.3, 90.4, 93.8, 82.1, 87.0, 88.0, 119.6, 27.7};
    MoistureMap& mm = resourceData.GetMoistMap();
    mm = MoistureMap(box, img, minM, maxM, valueSimM);
    qDebug() << "Moisture loaded";

    TemperatureMap& tm = resourceData.GetTempMap();

    ScalarField2 temp = ComputeTemperature();
    img = temp.CreateImage(0, 1);

    double heightTemps;

    valueSimT = {2.7, 1.9, 4.5, 7.3, 10.8, 14.9, 17.8, 17.6, 13.9, 10.2, 5.4, 3.4};
    heightTemps = 1386; // Gresolet

    if (dir.contains("Berchtesgaden")) {
        valueSimT = { -7, -5, 0, 2, 7, 12, 14, 14, 10, 6, 1, -5 };
        heightTemps = 800; // Berchtesgaden
    }

    double deltaMin = (minH - heightTemps) / -153.8;
    double deltaMax = (maxH - heightTemps) / -153.8;
    for (int i = 0; i < 12; ++i)
    {
        minT[i] = deltaMin + valueSimT[i];
        maxT[i] = deltaMax + valueSimT[i];
    }

    tm = TemperatureMap(box, img, minT, maxT);

    img = QImage(dir + "/wind.png");
    resourceData.SetWindMap(WindMap(box, img, 0, 1));

    qDebug() << "Loaded data from disk in: " << System::Elapsed(timer);

    for (int i = 0; i < ecosystemData.size(); ++i)
    {
        ecosystemData.GetData(i).sf_boost = ScalarField2(Box2(resourceData.GetHeightField().GetBox()), dimX / 10, dimX / 10, 0);
        ecosystemData.GetData(i).boost = ecosystemData.GetData(i).sf_boost.CreateImage();
    }

    timer.restart();
    LoadTerrain();
    qDebug() << "Terrain Loaded in: " << System::Elapsed(timer);

    LoadEcosystem(ecosystemFile);

    LoadSim();

    // Edit the sunlight map by default
    CallEnvEditSunExposure();
}

void QteWindow::LoadSim()
{
    // Set the simulation
    QElapsedTimer timer;
    timer.restart();
    sim = new EcosystemSimulationGPU(resourceData, ecosystemData);
    qDebug() << "Sim Created in: " << System::Elapsed(timer);

    timer.restart();
    sim->Load();
    qDebug() << "Sim Data loaded on the GPU in: " << System::Elapsed(timer);

    timer.restart();
    // Update display
    if (sim->isGPU())
    {
        mayawidget->UseExternAlbedo(true);
        mayawidget->UseAlbedo(false);
        mayawidget->SetExternAlbedo(static_cast<EcosystemSimulationGPU*>(sim)->GetOutputTexture());
    }
    else
    {
        mayawidget->UseAlbedo(true);
        mayawidget->UseExternAlbedo(false);
    }

    updateResourcesPixmap();

    Update();
    qDebug() << "UI updated in: " << System::Elapsed(timer);
    
    qDebug() << "GPU Resources used: " << sim->GetMemoryUsed();
}

//////////////////////////////// Terrain //////////////////////////////////////

/*!
* \brief Load a terrain from an image.
*/
void QteWindow::CallLoadTerrain()
{

    QString fileName = QFileDialog::getOpenFileName(this, "Open HeighMap", SOLUTION_DIR, "PNG file (*.png)");
    if (!fileName.isEmpty())
    {
        QImage img(fileName);
        Box2 box(uiw.terrain_dimX->value(), uiw.terrain_dimY->value());
        resourceData.SetHeightField( HeightField(box, img, uiw.terrain_minH->value(), uiw.terrain_maxH->value()));
        LoadTerrain();
    }
}

//////////////////////////////// ENV functions //////////////////////////////////////

QVector<ScalarField2> QteWindow::ComputeSunlight(const QVector<double>& cloudiness)
{
    SunlightSimulator sun(uiw.Environment_Latitude->value(), Normalized(Vector(0, -1, 0)));
    sun.Simulate(resourceData.GetHeightField(), 100, 1024);
    return sun.TotalSunlight(cloudiness);
}

ScalarField2 QteWindow::ComputeMoisture(const QVector<double>& precipitations)
{
    return Moisture::ProxySMI(resourceData.GetHeightField());
}

ScalarField2 QteWindow::ComputeTemperature()
{
    ScalarField2 temp = resourceData.GetHeightField();
    temp.Normalize();
    return temp;
}

ScalarField2 QteWindow::ComputeMask()
{
    ScalarField2 moist = resourceData.GetScalarField(MapType::MOISTURE, 0);
    ScalarField2 mask(Box2(moist.GetBox()), moist.GetSizeX(), moist.GetSizeY(), 0);
    double threshold = uiw.mask_threshold->value();

    for (int i = 0; i < mask.GetSizeX(); ++i)
        for (int j = 0; j < mask.GetSizeY(); ++j)
        {
            if (moist(i, j) > threshold)
                mask(i, j) = 1;
        }

    return mask;
}

bool QteWindow::ComputeEnv(const HeightField& heightField, QString dirName)
{
    // ====== Info file ======

    Vector hfSize = heightField.GetBox().Size();

    QFile file(dirName + "/info.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Error, could not open info.txt file";
        return false;
    }
    QTextStream ts(&file);

    // ====== HeightField ======
    double minH, maxH;
    heightField.GetRange(minH, maxH);
    ts << "heightField: " << hfSize[0] << " " << hfSize[1] << " " << minH << " " << maxH << "\n";

    Box2 box(heightField.GetBox());
    QImage img = heightField.CreateImage();
    img.save(dirName + "/heightField.png");
    qDebug() << "HeightField: " << minH << ", " << maxH;

    resourceData.SetHeightField(heightField);

    // ====== Sunlight ======
    QVector<double> cloudiness = monthEditorSunLight->getValues();
    auto maps = ComputeSunlight(cloudiness);

    QVector<QImage> imgs(12);
    QVector<double> minS(12);
    QVector<double> maxS(12);
    ts << "sunlight: " << "\n";
    for (int i = 0; i < 12; ++i)
    {
        QString file = QString(dirName + "/sunlight_%1.png").arg(i);
        imgs[i] = maps[i].CreateImage();
        imgs[i].save(file);
        maps[i].GetRange(minS[i], maxS[i]);

        ts << "  " << minS[i] << " " << maxS[i] << "\n";
        qDebug() << "Sunlight " << i << ": " << minS[i] << ", " << maxS[i];

    }
    resourceData.GetSunMap() = SunlightMap(box, imgs, minS, maxS, cloudiness);


    // ====== Moisture ======
    QVector<double> precipitations = { 38.3, 40.1, 94.5, 126.9, 121.3, 90.4, 93.8, 82.1, 87.0, 88.0, 119.6, 27.7 };
    auto smi = ComputeMoisture(precipitations);

    double minM, maxM;
    ts << "moisture: " << "\n";
    img = smi.CreateImage();
    img.save(dirName + "/moisture.png");
    smi.GetRange(minM, maxM);
    ts << "  " << minM << " " << maxM << "\n";
    qDebug() << "Moisture: " << minM << ", " << maxM;

    resourceData.GetMoistMap() = MoistureMap(box, img, minM, maxM, precipitations);


    // ====== Mask ======
    ScalarField2 mask = ComputeMask();
    img = mask.CreateImage(0., 1.);
    img.save(dirName + "/mask.png");
    qDebug() << "Mask saved";

    resourceData.GetMaskMap() = MaskMap(box, img);


    // ====== Temperature ======
    QVector<double> valueSimT = { 2.7, 1.9, 4.5, 7.3, 10.8, 14.9, 17.8, 17.6, 13.9, 10.2, 5.4, 3.4 };
    ScalarField2 temp = ComputeTemperature();
    img = temp.CreateImage(0, 1);

    double heightTemps = 1386; // Gresolet
    double deltaMin = (minH - heightTemps) / -153.8;
    double deltaMax = (maxH - heightTemps) / -153.8;
    for (int i = 0; i < 12; ++i)
    {
        minS[i] = deltaMin + valueSimT[i];
        maxS[i] = deltaMax + valueSimT[i];
    }

    resourceData.GetTempMap() = TemperatureMap(box, img, minS, maxS);

    return true;
}

void QteWindow::CallComputeEnv()
{
    const HeightField& heightField = resourceData.GetHeightField();

    const QString dataDir = SOLUTION_DIR "data/Simulation";
    QString dirName = QFileDialog::getExistingDirectory(this, "Select Environment Folder", dataDir, QFileDialog::ShowDirsOnly);
    if (!dirName.isEmpty() && heightField.GetSizeX() > 0)
    {
        QElapsedTimer t;
        t.restart();
        if (!ComputeEnv(heightField, dirName))
            return;
        qDebug() << "Computed maps in: " << System::Elapsed(t);

        // Load the computed environment (default ecosystem if none was loaded yet)
        if (ecosystemFile.isEmpty())
            ecosystemFile = SOLUTION_DIR "data/Presets/fixSun_v18.json";
        LoadEnv(dirName);
    }
}

/*!
* \brief Load an environment.
*/
void QteWindow::CallLoadEnv()
{
    const QString dataDir = SOLUTION_DIR "data/Simulation";
    QString dirName = QFileDialog::getExistingDirectory(this, "Open Environment Folder", dataDir, QFileDialog::ShowDirsOnly);
    if (!dirName.isEmpty())
    {
        LoadEnv(dirName);
    }
}


//////////////////////////////// GENERATE MAPS //////////////////////////////////////

/*!
* \brief Generate sun exposure map.
*/
void QteWindow::CallEnvGenSunExposure()
{
    QVector<ScalarField2> maps = ComputeSunlight(resourceData.GetSunMap().GetSimParams());

    int month = uiw.Environment_Month->value()-1;
    for (int i = 0; i < 12; ++i)
    {
        ScalarField2& sun = resourceData.GetSunMap().GetScalarField(i);
        sun = maps[i];
    }

    resourceData.SyncImages();
    uiw.label_Image_SunExposure->setPixmap(resourceData.GetPixmap(MapType::SUNLIGHT, month));
    mayawidgetEditor->updateImage();
    sim->LoadResources();
}

/*!
* \brief Generate moisture map.
*/
void QteWindow::CallEnvGenMoisture()
{
    ScalarField2& moist = resourceData.GetMoistMap().GetScalarField();

    moist = ComputeMoisture(resourceData.GetMoistMap().GetPrecipitations());

    resourceData.SyncImages();
    uiw.label_Image_Moisture->setPixmap(resourceData.GetPixmap(MapType::MOISTURE, 0));
    mayawidgetEditor->updateImage();
    sim->LoadResources();
}

/*!
* \brief Generate temperature map.
*/
void QteWindow::CallEnvGenTemperature()
{
    ScalarField2& temp = resourceData.GetTempMap().GetScalarField();

    temp = ComputeTemperature();

    resourceData.SyncImages();
    uiw.label_Image_Temperature->setPixmap(resourceData.GetPixmap(MapType::TEMPERATURE, 0));
    mayawidgetEditor->updateImage();
    sim->LoadResources();
}

/*!
* \brief Generate mask map.
*/
void QteWindow::CallEnvGenMask()
{
    ScalarField2& mask = resourceData.GetMaskMap().GetScalarField();

    mask = ComputeMask();

    resourceData.SyncImages();
    uiw.label_Image_Mask->setPixmap(resourceData.GetPixmap(MapType::MASK, 0));
    mayawidgetEditor->updateImage();
    sim->LoadResources();
}

//////////////////////////////// LOAD MAPS //////////////////////////////////////

/*!
* \brief Select sun exposure map to edit.
*/
void QteWindow::CallEnvLoadSunExposure()
{
    qDebug() << "Load Sun Map";
    QString fileName = QFileDialog::getOpenFileName(this, "Open Sun Map", SOLUTION_DIR, "PNG file (*.png)");
    if (!fileName.isEmpty())
    {
        int month = uiw.Environment_Month->value()-1;
        resourceData.GetSunMap().GetImage(month) = QImage(fileName);
        CallMapChanged(MapType::SUNLIGHT);
        mayawidgetEditor->updateImage();
    }
}

void QteWindow::CallEnvLoadMoisture()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Moisture Map", SOLUTION_DIR, "PNG file (*.png)");
    if (!fileName.isEmpty())
    {
        resourceData.GetMoistMap().GetImage() = QImage(fileName);
        CallMapChanged(MapType::MOISTURE);
        mayawidgetEditor->updateImage();
    }
}

void QteWindow::CallEnvLoadTemperature()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Temperature Map", SOLUTION_DIR, "PNG file (*.png)");
    if (!fileName.isEmpty())
    {
        resourceData.GetTempMap().GetImage() = QImage(fileName);
        CallMapChanged(MapType::TEMPERATURE);
        mayawidgetEditor->updateImage();
    }
}

void QteWindow::setValueIntensity(double i)
{
    double _min, _max;
    mayawidgetEditor->GetMinMaxTool(_min, _max);

    int value =  ((i - _min) / (_max - _min) ) * 100.0;

    uiw.horizontalSlider_EditorTool_intensity->setValue(value);


    qDebug() << "Intensity: " << value;

}

void QteWindow::setValueIntensity(int i)
{
    double _min, _max;
    mayawidgetEditor->GetMinMaxTool(_min, _max);

    double value = _min + (double)i * (_max - _min) / 100.0;
    uiw.doubleSpinBox_EditorTool_intensity->setValue(value);

    qDebug() << "Intensity: " << value;
}

void QteWindow::updateEditRange()
{
    double _min, _max;
    mayawidgetEditor->GetMinMaxTool(_min, _max);

    uiw.label_intensity_min->setText(QString::number(_min));
    uiw.label_intensity_max->setText(QString::number(_max));

    uiw.doubleSpinBox_EditorTool_intensity->setMinimum(_min);
    uiw.doubleSpinBox_EditorTool_intensity->setMaximum(_max);

    uiw.doubleSpinBox_EditorTool_intensity->setValue((_min+_max)/2.);
}

/*!
* \brief Select sun exposure map to edit.
*/
void QteWindow::CallEnvEditSunExposure()
{
    mayawidgetEditor->editSelectedMap(MapType::SUNLIGHT, uiw.Environment_Month->value()-1);
    updateEditRange();
}

/*!
* \brief Select moisture map to edit.
*/
void QteWindow::CallEnvEditMoisture()
{
    mayawidgetEditor->editSelectedMap(MapType::MOISTURE, uiw.Environment_Month->value()-1);
    updateEditRange();
}

/*!
* \brief Select temperature map to edit.
*/
void QteWindow::CallEnvEditTemperature()
{
    mayawidgetEditor->editSelectedMap(MapType::TEMPERATURE, uiw.Environment_Month->value()-1);
    updateEditRange();
}

/*!
* \brief Select mask map to edit.
*/
void QteWindow::CallEnvEditMask()
{
    mayawidgetEditor->editSelectedMap(MapType::MASK, uiw.Environment_Month->value() - 1);
    updateEditRange();
}

void QteWindow::CallEnvSaveMask()
{
    QString file = QFileDialog::getSaveFileName(nullptr, "Save Mask", SOLUTION_DIR "data/Control/", "PNG files (*.png)");
    if (file.isEmpty())
        return;

    resourceData.GetMaskMap().GetImage().save(file);
}


//////////////////////////////// HISTO //////////////////////////////////////

void QteWindow::CallHistoChanged(MapType type)
{
    switch (type)
    {
    case MapType::SUNLIGHT:
        resourceData.GetSunMap().SetSimParams(monthEditorSunLight->getValues());
        break;
    case MapType::MOISTURE:
        break;
    case MapType::TEMPERATURE:
        break;
    }
}

void QteWindow::CallMapChanged(const MapType& type)
{
    qDebug() << "Map changed ";
    int month = uiw.Environment_Month->value()-1;

    resourceData.SyncImages();

    switch (type)
    {
    case MapType::TERRAIN:
        uiw.label_Image_Terrain->setPixmap(resourceData.GetPixmap(type, month));
        break;
    case MapType::SUNLIGHT:
        uiw.label_Image_SunExposure->setPixmap(resourceData.GetPixmap(type, month));
        break;
    case MapType::MOISTURE:
        uiw.label_Image_Moisture->setPixmap(resourceData.GetPixmap(type, month));
        break;
    case MapType::TEMPERATURE:
        uiw.label_Image_Temperature->setPixmap(resourceData.GetPixmap(type, month));
        break;
    case MapType::MASK:
        uiw.label_Image_Mask->setPixmap(resourceData.GetPixmap(type, month));
        // Only remove the plants in the masked areas once the simulation has been started
        if (worker)
        {
            ecosystemData.Delete(resourceData.GetScalarField(MapType::MASK, 0));
            sim->LoadEcosystem();
        }
        Update();
        break;
    }

    sim->LoadResources();
}

void QteWindow::CallDebugSun()
{
    uiw.dbg_shadow->setDisabled(!uiw.dbg_sun->isChecked());
    CallDebugVigor();
}

void QteWindow::CallDebugVigor()
{
    if (sim)
    {
        if (uiw.dbg_enable->isChecked())
            sim->DebugVigor(uiw.dbg_species->value(), uiw.dbg_month->value(), uiw.dbg_height->value(), uiw.dbg_sun->isChecked(), uiw.dbg_shadow->isChecked(), uiw.dbg_moist->isChecked(), uiw.dbg_temp->isChecked(), uiw.dbg_slope->isChecked(), uiw.dbg_boost->isChecked(), false, false);
        else
            Update();
    }
}
