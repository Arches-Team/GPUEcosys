#include "qte.h"

#include "heightfieldshader.h"

#include <QtWidgets/QFileDialog>

void QteWindow::createActionsDebug()
{
    connect(uiw.pbr_disks, SIGNAL(clicked()), this, SLOT(CallPBRExport()));
}

void QteWindow::CallPBRExport()
{
    QString file = QFileDialog::getSaveFileName(this, "Export 3D render", SOLUTION_DIR "pbr_disks.jpg", "JPEG files (*.jpg)");
    if (file.isEmpty())
        return;
    if (!file.endsWith(".jpg", Qt::CaseInsensitive))
        file += ".jpg";

    CallPBR(file);
}

void QteWindow::CallPBR(QString file, const QImage& bg)
{
    double resolution = 8192;
    QImage image(resolution, resolution, QImage::Format::Format_RGB16);
    QPainter painter(&image);

    QImage terrain;
    if (bg.isNull()) {
        HeightFieldShader hf(resourceData.GetHeightField());
        terrain = hf.Eric();
    }
    else
        terrain = bg;

    sim->DrawSymbolicDisk(painter, resolution, Box2(resourceData.GetHeightField().GetBox()), terrain, false, uiw.pbr_useAlpha->isChecked());

    TerrainRaytracingWidget::PBR_Render_Options options;
    options.nb_dispatch = uiw.pbr_dispatch->value();
    options.anti_aliasing = uiw.pbr_aa->value();
    options.under_shadow_distance = 150.0f;
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
}
