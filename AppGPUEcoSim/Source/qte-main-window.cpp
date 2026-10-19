
#include "qte.h"
#include "cpu.h"
#include "heightfieldshader.h"
#include "abiotics.h"

#include <QtWidgets/QFileDialog>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QImageReader>
#include "histogram.h"
#include <QtSvg/QSvgGenerator>
#include <QtGui/QImageWriter>

/*!
\class QteWindow qte.h
\brief This class implements a main window.
*/

/*!
\brief Create the main window.
*/
QteWindow::QteWindow()
{
    // Loading interface
    const QString rcc = QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/UI/resources.rcc");
    if (!QResource::registerResource(rcc))
        qDebug() << "Cannot load UI resources from" << rcc;
    uiw.setupUi(this);

    setWindowTitle("Fast Ecosystem Simulation");

    sim = nullptr;

    // Loading GLWidget
    mayawidget = new TerrainRaytracingWidget;
    QGridLayout* GLlayout = new QGridLayout;
    GLlayout->addWidget(mayawidget, 0, 0);
    GLlayout->setContentsMargins(0, 0, 0, 0);
    uiw.widget_GL->setLayout(GLlayout);

    // Loading GLWidget Editor
    mayawidgetEditor = new TRWEditor(resourceData,ecosystemData);
    QGridLayout* GLlayoutEditor = new QGridLayout;
    GLlayoutEditor->addWidget(mayawidgetEditor, 0, 0);
    GLlayoutEditor->setContentsMargins(0, 0, 0, 0);
    uiw.widget_GLEditor->setLayout(GLlayoutEditor);

    monthEditorSunLight = new MonthsEditorWidget(this, "Cloud coverage", MapType::SUNLIGHT, QColor(174, 187, 193), 0, 1);

    // Loading Curves editor
    uiw.SunExposureLayout->addWidget(monthEditorSunLight);

    uiw.doubleSpinBox_nbTrees->setEnabled(false);

    // Connecting QT Actions
    createActions();
}

/*!
\brief Destructor.
*/
QteWindow::~QteWindow()
{
    CallSimStop();

    delete mayawidget;
    delete mayawidgetEditor;
    if (sim)
        delete sim;
}

/*!
\brief Create callbacks between member slots and user interface.
*/
void QteWindow::createActions()
{
    // Simulation
    createActionsSimulation();
    createActionsGlobalEnvironment();
    createActionsEcosystem();
    createActionsDisplayExport();
    createActionsEditor();
    createActionsDebug();
    createActionsScene();

    // Connect buttons
    connect(uiw.chart_basal_area, SIGNAL(clicked()), this, SLOT(CallChart()));
    connect(uiw.disk_schema, SIGNAL(clicked()), this, SLOT(CallSchema()));

    connect(uiw.start_fire, SIGNAL(clicked()), this, SLOT(CallStartFire()));
    connect(uiw.wind_dir_X, SIGNAL(valueChanged(double)), this, SLOT(CallUpdateWind()));
    connect(uiw.wind_dir_Y, SIGNAL(valueChanged(double)), this, SLOT(CallUpdateWind()));
    connect(uiw.wind_speed, SIGNAL(valueChanged(double)), this, SLOT(CallUpdateWind()));

    connect(uiw.hist_heights, SIGNAL(clicked()), this, SLOT(CallHistHeights()));
    connect(uiw.dbg_enable, SIGNAL(clicked()), this, SLOT(CallDebugVigor()));
    connect(uiw.dbg_sun, SIGNAL(clicked()), this, SLOT(CallDebugSun()));
    connect(uiw.dbg_shadow, SIGNAL(clicked()), this, SLOT(CallDebugVigor()));
    connect(uiw.dbg_moist, SIGNAL(clicked()), this, SLOT(CallDebugVigor()));
    connect(uiw.dbg_temp, SIGNAL(clicked()), this, SLOT(CallDebugVigor()));
    connect(uiw.dbg_slope, SIGNAL(clicked()), this, SLOT(CallDebugVigor()));
    connect(uiw.dbg_boost, SIGNAL(clicked()), this, SLOT(CallDebugVigor()));
    connect(uiw.dbg_species, SIGNAL(valueChanged(int)), this, SLOT(CallDebugVigor()));
    connect(uiw.dbg_month, SIGNAL(valueChanged(int)), this, SLOT(CallDebugVigor()));
    connect(uiw.dbg_height, SIGNAL(valueChanged(double)), this, SLOT(CallDebugVigor()));
    connect(uiw.display_height, SIGNAL(clicked()), this, SLOT(Update()));
    connect(uiw.display_saplings, SIGNAL(clicked()), this, SLOT(Update()));
    connect(uiw.display_species, SIGNAL(clicked()), this, SLOT(Update()));

    // Camera sync
    connect(mayawidget, SIGNAL(_signalMouseMove()), this, SLOT(CallUpdatedCameraRight()));
    connect(mayawidgetEditor, SIGNAL(_signalMouseMove()), this, SLOT(CallUpdatedCameraLeft()));
    connect(uiw.camera_sync, SIGNAL(clicked()), this, SLOT(CallUpdatedCameraRight()));
    connect(uiw.display_fire, SIGNAL(clicked()), this, SLOT(Update()));
}

/*!
\brief Update the diplay
*/
void QteWindow::Update()
{
    qDebug() << "Update()";
    int mode = 0;
    if (uiw.display_height->isChecked())
        mode = 1;
    else if (uiw.display_saplings->isChecked())
        mode = 2;
    else if (uiw.display_fire->isChecked())
        mode = 3;

    if (uiw.dbg_enable->isChecked())
    {
        CallDebugVigor();
        return;
    }

    if (sim->isGPU())
    {
        mayawidget->UseAlbedo(false);
        mayawidget->UseExternAlbedo(true);
        sim->UpdateVisualization(mode);
    }
    else
    {
        mayawidget->UseAlbedo(true);
        mayawidget->UseExternAlbedo(false);
    }
}

/*!
rief Set the camera of the main view, and of the editor when the cameras are synchronised.
*/
void QteWindow::SetViewCamera(const Camera& camera)
{
    mayawidget->SetCamera(camera);
    CallUpdatedCameraRight();
}

void QteWindow::CallUpdatedCameraRight()
{
    if (uiw.camera_sync->isChecked())
        mayawidgetEditor->SetCamera(mayawidget->GetCamera());
}

void QteWindow::CallUpdatedCameraLeft()
{
    if (uiw.camera_sync->isChecked())
        mayawidget->SetCamera(mayawidgetEditor->GetCamera());
}


void QteWindow::CallChart()
{
    QString file = QFileDialog::getSaveFileName(this, "Export basal area", SOLUTION_DIR "BasalArea.csv", "CSV files (*.csv)");
    if (file.isEmpty())
        return;

    QVector<double> years;
    QVector<QVector<double>> data = sim->BasalArea(years);
    WriteCsv(file, "year", years, data);
}

void QteWindow::CallSchema()
{
    QString file = QFileDialog::getSaveFileName(this, "Export 2D image", SOLUTION_DIR "schema.jpg", "JPEG files (*.jpg)");
    if (file.isEmpty())
        return;
    if (!file.endsWith(".jpg", Qt::CaseInsensitive))
        file += ".jpg";

    // TODO: call DrawDisks()
    double resolution = 4096.;
    QImage image(resolution, resolution, QImage::Format::Format_RGB16);
    QPainter painter(&image);

    HeightFieldShader hf(resourceData.GetHeightField());
    QImage terrain = hf.Eric();
    sim->DrawSymbolicDisk(painter, resolution, Box2(resourceData.GetHeightField().GetBox()), terrain, false);

    image.save(file, "JPEG", 95);
}

void QteWindow::CallHistHeights()
{
    QString file = QFileDialog::getSaveFileName(this, "Export heights histogram", SOLUTION_DIR "HeightsHistogram.csv", "CSV files (*.csv)");
    if (file.isEmpty())
        return;

    QVector<double> heights;
    QVector<QVector<double>> data = sim->HeightsHistogram(heights);
    WriteCsv(file, "height", heights, data);
}

bool QteWindow::WriteCsv(const QString& file, const QString& xLabel, const QVector<double>& x, const QVector<QVector<double>>& series)
{
    QFile f(file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Cannot open" << file << "for writing";
        return false;
    }

    QTextStream out(&f);

    out << xLabel;
    for (int s = 0; s < series.size(); ++s)
        out << "," << ecosystemData.GetSpeciesParam(s).name;
    out << "\n";

    for (int i = 0; i < x.size(); ++i)
    {
        out << x[i];
        for (const auto& values : series)
            out << "," << (i < values.size() ? values[i] : 0.);
        out << "\n";
    }

    return true;
}

void QteWindow::CallStartFire()
{
    QPoint p(uiw.selection_X->value() / 2, uiw.selection_Y->value() / 2);
    Vector2 d(uiw.wind_dir_X->value(), uiw.wind_dir_Y->value());
    sim->SetWind(d, uiw.wind_speed->value());
    sim->StartFire(p);

    sim->SimulateFire(uiw.fire_iterations->value());
    Update();
}

void QteWindow::CallUpdateWind()
{
    Vector2 d(uiw.wind_dir_X->value(), uiw.wind_dir_Y->value());
    sim->SetWind(d, uiw.wind_speed->value());
}


/*!
\brief Load the terrain and set the camera.
*/
void QteWindow::LoadTerrain()
{
    HeightField* heightField = resourceData.GetPtrHeightField();

    // Initialize the camera
    Camera camera = Camera::View(heightField->GetBox());
    camera.SetPlanes(uiw.camera_near->value(), uiw.camera_far->value());
    mayawidget->SetCamera(camera);
    mayawidgetEditor->SetCamera(camera);

    // Set the heightfield
    mayawidget->SetHeightField(heightField);
    mayawidget->UseExternAlbedo(true);
    mayawidget->SetSteps(4096);

    // Set the heightfield for the editor
    mayawidgetEditor->SetHeightField(heightField);
    mayawidgetEditor->SetSteps(4096);

    setSelection();
}

void QteWindow::DrawDisks(const QString& file, Box2 box, bool showSaplings)
{
    double resolution = 4096.;
    QImage image(resolution, resolution, QImage::Format::Format_RGB16);
    QPainter painter(&image);

    HeightFieldShader hf(resourceData.GetHeightField());
    QImage terrain = hf.Eric();
    terrain.fill(QColor(210, 210, 210));

    sim->DrawSymbolicDisk(painter, resolution, box, terrain, showSaplings);

    image.save(file);
}

void QteWindow::DrawDisksPBR(const QString& file, Box2 box, const QImage& bg)
{
    double resolution = 8192.;
    QImage image(resolution, resolution, QImage::Format::Format_RGB16);
    QPainter painter(&image);

    HeightField hfTemp = resourceData.GetHeightField();

    QImage terrain;
    if (bg.isNull()) {
        HeightFieldShader hf(hfTemp);
        terrain = hf.Eric();
    }
    else
        terrain = bg;

    // Crop image
    Box2 bhf(resourceData.GetHeightField().GetBox());
    Vector2 d = bhf.Diagonal();
    Vector2 a = (box[0] - bhf[0]) * terrain.width();
    a[0] /= d[0];
    a[1] /= d[1];
    Vector2 b = (box[1] - bhf[0]) * terrain.height();
    b[0] /= d[0];
    b[1] /= d[1];
    b = b - a;

    terrain = terrain.copy(a[0], a[1], b[0], b[1]);

    sim->DrawSymbolicDisk(painter, resolution, box, terrain, false, true);

    painter.end();

    hfTemp = hfTemp.Crop(hfTemp.VertexInteger(box[0]), hfTemp.VertexInteger(box[1]));
    mayawidget->SetHeightField(&hfTemp);

    TerrainRaytracingWidget::PBR_Render_Options options;
    options.nb_dispatch = 2;
    options.anti_aliasing = 1;
    options.under_shadow_distance = 25.f;
    options.sea_level = 0.f;
    mayawidget->SetAlbedo(image);
    mayawidget->UseAlbedo(true);
    mayawidget->UseExternAlbedo(false);

    QImage img = mayawidget->PBR_Render(options);

    if (file.endsWith(".jpg"))
        img.save(file, "JPEG", 95);
    else
        img.save(file);

    mayawidget->UseAlbedo(false);
    mayawidget->UseExternAlbedo(true);
    mayawidget->SetHeightField(resourceData.GetPtrHeightField());
}
