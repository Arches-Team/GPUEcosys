#include "qte.h"
#include <QtGui/QMouseEvent>

/*
* \brief Paint the scene
*/
void TRWEditor::paintGL()
{
    glUseProgram(shaderProgram);

    glEnable(GL_DEPTH_TEST);
    TerrainRaytracingWidget::paintGL();

    // Draw the tool (hidden when edition is disabled)
    if (editEnabled && m_rendererTool)
    {
        m_rendererTool->SetCamera(camera, width(), height());
        m_rendererTool->Draw();
    }

    if (m_rendererSel)
    {
        m_rendererSel->SetCamera(camera, width(), height());
        m_rendererSel->Draw();
    }

    RenderUiPanelEditor();
    RenderUiPanelControls();
}

const QImage& TRWEditor::getImage()
{
    if (mapType != MapType::NONE) return resourceData.GetImage(mapType, month);
    else return ecosystemData.GetImage(mapTypeS, species);
}

ScalarField2& TRWEditor::GetScalarField()
{
    if (mapType != MapType::NONE) return resourceData.GetScalarField(mapType, month);
    else return ecosystemData.GetScalarField(mapTypeS, species);
}


void TRWEditor::GetMinMax(double& min, double& max)
{
    if (mapType != MapType::NONE) return resourceData.GetMinMax(mapType, month, min, max);
    else return ecosystemData.GetMinMax(species, mapTypeS, min, max);
}

void TRWEditor::GetMinMaxTool(double& min, double& max)
{
    if (mapType != MapType::NONE) return resourceData.GetMinMaxTool(mapType, month, min, max);
    else return ecosystemData.GetMinMax(species, mapTypeS, min, max);
}


/*
* \brief Paint the map
*/
void TRWEditor::paintMAP()
{
    if (!sf) return;

    switch (tool)
    {
    case TOOL_NONE:
    case TOOL_GAUSSIAN:
        paintSFGaussian();
        break;
    case TOOL_RANDOM:
        paintSFRandom();
        break;
    default:
        break;
    }

    if (MapType::TERRAIN == mapType)
    {
        UpdateInternal();
    }
}


void TRWEditor::paintMAPGaussian(QPainter& painter)
{
    const QImage& image = getImage();
    if (image.isNull()) return;

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // QT paint radial gradient
    Box2 box = sf->GetBox();
    QPointF _position = QPointF((position[0] - box[0][0]) / box.Diagonal()[0] * image.width(), (position[1] - box[0][1]) / box.Diagonal()[1] * image.height());
    double _radius = radius / box.Diagonal()[0] * image.width();

    QRadialGradient radialGrad(_position, _radius);
    if (isLeftClick)
    {
        radialGrad.setColorAt(0.0, QColor(255, 255, 255, 255 * intensity));
        radialGrad.setColorAt(1.0, QColor(255, 255, 255, 0));
    }
    else if (isRightClick)
    {
        radialGrad.setColorAt(0.0, QColor(0, 0, 0, 255 * intensity));
        radialGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    }
    else
    {
        radialGrad.setColorAt(0.0, QColor(255, 255, 255, 0));
        radialGrad.setColorAt(1.0, QColor(255, 255, 255, 0));
    }

    painter.setBrush(radialGrad);
    painter.setPen(Qt::NoPen);

    painter.drawEllipse(_position, _radius, _radius);

    SetAlbedo(image);
}

inline double CubicSmooth(const double& x, const double& r)
{
    return (1.0 - x / r) * (1.0 - x / r) * (1.0 - x / r);
}

void TRWEditor::paintSFGaussian()
{
    double minV, maxV;
    GetMinMax(minV, maxV);

    std::cout << "minV: " << minV << " maxV: " << maxV << std::endl;

    ScalarField2& sfc = GetScalarField();

    // The intensity is a target value: left click raises the values up to it, right click lowers them down to it.
    // The step depends on the map range so that the brush also moves toward a target of 0.
    // Need to clamp the modified values, so can't use the sfc.Gaussian() function
    const double step = maxV - minV;
    const QRect area = sfc.VertexIntegerArea(Circle2(position, radius));
    for (int y = area.y(); y <= area.y() + area.height(); y++) {
        for (int x = area.x(); x <= area.x() + area.width(); x++) {
            double u = SquaredNorm(position - sfc.ArrayVertex(x, y));
            if (u < radius * radius)
            {
                double& v = sfc[sfc.VertexIndex(x, y)];
                const double delta = step * CubicSmooth(u, radius * radius);
                if (isLeftClick && v < intensity)
                    v = Math::Min(v + delta, intensity);
                else if (isRightClick && v > intensity)
                    v = Math::Max(v - delta, intensity);
            }
        }
    }

    sfc.Clamp(minV, maxV); // Clamp according to the corresponding values

    if (MapType::TERRAIN == mapType)
    {
        sf = &sfc;
    }

    QImage image = sfc.CreateImage(minV, maxV);
    SetAlbedo(image);
}


void TRWEditor::paintSFRandom()
{
    double minV, maxV;
    GetMinMax(minV, maxV);

    ScalarField2& sfc = GetScalarField();

    for (int i = 0; i < nbSamples; i++)
    {
        double x = randTool.Uniform(-radius, radius);
        double y = randTool.Uniform(-radius, radius);
        if (x * x + y * y > radius * radius) continue; // Outside the circle
        QPoint point = sfc.VertexInteger(position + Vector2(x,y));
        // Raise (left click) or lower (right click) the value at the random point to the target intensity
        double& v = sfc(point.x(), point.y());
        v = isLeftClick ? Math::Max(v, intensity) : Math::Min(v, intensity);
    }
    sfc.Clamp(minV, maxV);

    if (MapType::TERRAIN == mapType)
    {
        sf = &sfc;
    }

    QImage image = sfc.CreateImage(0., maxV);
    SetAlbedo(image);
}


void TRWEditor::paintMAPRandom(QPainter& painter)
{
    const QImage& image = getImage();
    if (image.isNull()) return;

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    // QT paint random points
    Box2 box = sf->GetBox();
    QPointF _position = QPointF((position[0] - box[0][0]) / box.Diagonal()[0] * image.width(), (position[1] - box[0][1]) / box.Diagonal()[1] * image.height());
    double _radius = radius / box.Diagonal()[0] * image.width();

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 255 * intensity));

    for (int i = 0; i < nbSamples; i++)
    {
        double x = randTool.Uniform(-_radius, _radius);
        double y = randTool.Uniform(-_radius, _radius);
        if (x * x + y * y > _radius * _radius) continue; // Outside the circle
        painter.drawEllipse(QPointF(x + _position.x(), y + _position.y()), 0.6, 0.6);
    }

    SetAlbedo(image);
}

/*
* \brief Update the tool
*/
void TRWEditor::updateTool()
{
    if (!sf) return;

    // Draw a circle around the intersection point
    const Circle2 toolCircle(position, radius);
    Color toolColor = isRightClick ? Color(0.2, 0.2 + 0.6 * intensity, 0.2) : Color(0.2 + 0.6 * intensity, 0.2, 0.2);
    drawCircle(m_rendererTool, toolCircle, toolColor);
    
    const Circle2 selection(selectedPosition, 0.1);
    Color selColor(0, 255, 0, 255);
    drawCircle(m_rendererSel, selection, selColor);
}

void TRWEditor::drawCircle(std::unique_ptr<MayaSimpleRenderer>& renderer, const Circle2& circle, const Color& color)
{
    // Delete the circle renderer
    if (renderer)
    {
        renderer->DeleteBuffers();
        renderer = nullptr;
    }

    QVector<Vector> circlePoints{};
    Box2 box = sf->GetBox();
    for (int i = 0; i < 100; i++)
    {
        Vector2 pos = circle.Vertex(Math::Angle(i,100));
        pos[0] = max(box[0][0], min(box[1][0] - 0.1, pos[0]));
        pos[1] = max(box[0][1], min(box[1][1] - 0.1, pos[1]));

        if (sf->Inside(pos))
            circlePoints.append(pos.ToVector(sf->Value(pos)));
    }

    renderer = std::make_unique<MayaSimpleRenderer>(circlePoints, color, GL_LINE_LOOP);
    renderer->setDepthTest(true);
    renderer->setLineWidth(4.);
}

void TRWEditor::updateImage()
{
    SetAlbedo(getImage());
    UseAlbedo(true);
}

/*
* \brief Edit the selected map Resources
*/
void TRWEditor::editSelectedMap(MapType mtype, int m) {
    mapType = mtype;
    mapTypeS = MapSpecies::NONE;
    month = m;

    const QImage& image = resourceData.GetImage(mapType, month);

    SetAlbedo(image);
    UseAlbedo(true);
}

void TRWEditor::setMonth(int m) 
{ 
    month = m; 

    if (mapType == MapType::NONE) return;
    const QImage& image = resourceData.GetImage(mapType, month);

    SetAlbedo(image);
    UseAlbedo(true);
}


void TRWEditor::setSpecies(int m)
{
    species = m;

    if (mapTypeS == MapSpecies::NONE) return;
    const QImage& image = ecosystemData.GetImage(mapTypeS, species);

    SetAlbedo(image);
    UseAlbedo(true);
}

void TRWEditor::select(const Vector& sel)
{
    selectedPosition = sel;

    updateTool();
}

/*
* \brief Edit the selected map Species
*/
void TRWEditor::editSelectedMap(MapSpecies mtype, int s)
{
    mapType = MapType::NONE;
    mapTypeS = mtype;
    species = s;

    const QImage& image = ecosystemData.GetImage(mapTypeS, s);

    SetAlbedo(image);
    UseAlbedo(true);
}

/*
* \brief Event when the mouse is moved
*/
void TRWEditor::mouseMoveEvent(QMouseEvent* e)
{
    if (!sf) return;

    // Get the intersection point
    ray = camera.PixelToRay(e->pos().x(), e->pos().y() - 1, width(), height());
    double t;
    HeightField* _hf = (HeightField*)sf;
    _hf->Intersect(ray, t, position, _hf->GetBox(), camera.GetFar());

    // Paint the map
    if (editEnabled && (isLeftClick || isRightClick))
    {
        paintMAP();
    }

    // Update the tool
    updateTool();

    TerrainRaytracingWidget::mouseMoveEvent(e);
}

/*
* \brief Event when a mouse button is pressed
*/
void TRWEditor::mousePressEvent(QMouseEvent* e)
{
    if (!sf) return;

    if (e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier)
    {
        // Get the intersection point
        Ray ray = camera.PixelToRay(e->pos().x(), e->pos().y() - 1, width(), height());
        double t;
        HeightField* _hf = (HeightField*)sf;
        _hf->Intersect(ray, t, selectedPosition, _hf->GetBox(), camera.GetFar());

        emit selectionChanged(selectedPosition);

        updateTool();
    }

    if (editEnabled && (e->modifiers() & Qt::ControlModifier))
    {
        if (e->button() == Qt::LeftButton)
            isLeftClick = true;
        else if (e->button() == Qt::RightButton)
            isRightClick = true;

        // Paint the map
        paintMAP();

        // Update the tool
        updateTool();
    }
    else
        TerrainRaytracingWidget::mousePressEvent(e);
}

/*
* \brief Event when a mouse button is released
*/
void TRWEditor::mouseReleaseEvent(QMouseEvent* e)
{
    if (!sf) return;

    if (isLeftClick || isRightClick)
    {
        isLeftClick = false;
        isRightClick = false;

        // TODO
        if (mapType != MapType::NONE)
            emit changedResources(mapType);
        if (mapTypeS != MapSpecies::NONE)
            emit changedSpecies(mapTypeS);
    }

    TerrainRaytracingWidget::mouseReleaseEvent(e);
}

/*
* \brief Event when the mouse wheel is used
*/
void TRWEditor::wheelEvent(QWheelEvent* e)
{
    if (!sf) return;

    int numDegrees = e->angleDelta().y() / 8;
    int numSteps = numDegrees / 10;

    if (e->modifiers() & Qt::ControlModifier)
    {
        radius += numSteps;
        if (radius <= 0.1) radius = 0.1;

        emit valueRadiusChanged(radius);
        updateTool();
    }
    else if (e->modifiers() & Qt::ShiftModifier)
    {
        // Same range as the intensity spinbox/slider, 20 wheel steps from min to max
        double _min, _max;
        GetMinMaxTool(_min, _max);
        intensity += numSteps * (_max - _min) / 20.;
        intensity = Math::Clamp(intensity, _min, _max);

        emit valueIntensityChanged(intensity);
        updateTool();
    }

    TerrainRaytracingWidget::wheelEvent(e);
}

/*
* \brief Event when a key is pressed
*/
void TRWEditor::keyPressEvent(QKeyEvent* e)
{
    // H: show/hide the controls panel
    if (e->key() == Qt::Key_H)
    {
        showControls = !showControls;
        update();
    }
    else
        TerrainRaytracingWidget::keyPressEvent(e);
}

/*
* \brief Render the panel listing the mouse and keyboard controls
*/
void TRWEditor::RenderUiPanelControls()
{
    // We need to unbind VAO and program for now because it causes problem with below command.
    glBindVertexArray(0);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen penLineGrey(QColor(50, 50, 50));
    QPen penLineWhite(QColor(250, 250, 250));
    QPen penLineDisabled(QColor(150, 150, 150));
    QFont f1, f2;
    f1.setBold(true);

    if (!showControls)
    {
        painter.setFont(f2);
        painter.setPen(penLineWhite);
        painter.drawText(10, height() - 10, "H: show controls");
        painter.end();
        glEnable(GL_DEPTH_TEST);
        return;
    }

    const QStringList camera = {
        "Alt + Left drag", "Rotate",
        "Alt + Right drag", "Move forward/backward",
        "Alt + Middle drag", "Pan",
        "Wheel", "Zoom",
        "Left click", "Select a cell",
    };
    const QStringList edition = {
        "Ctrl + Left drag", "Raise up to intensity",
        "Ctrl + Right drag", "Lower down to intensity",
        "Ctrl + Wheel", "Brush radius",
        "Shift + Wheel", "Brush intensity",
    };

    // Panel
    const int lineH = 15;
    const int nbLines = 3 + camera.size() / 2 + edition.size() / 2 + 1; // titles + lines + hide hint
    const int sizeX = 290;
    const int sizeY = nbLines * lineH + 10;
    const int bX = 10;
    const int bY = height() - 10 - sizeY;
    const int colX = bX + 125;

    // Background
    painter.setPen(penLineGrey);
    painter.fillRect(QRect(bX, bY, sizeX, sizeY), QColor(0, 0, 255, 25));
    painter.drawRect(bX, bY, sizeX, sizeY);

    int y = bY + lineH;
    auto drawSection = [&](const QString& title, const QStringList& lines, const QPen& pen) {
        painter.setFont(f1);
        painter.setPen(pen);
        painter.drawText(bX + 5, y, title);
        y += lineH;
        painter.setFont(f2);
        for (int i = 0; i + 1 < lines.size(); i += 2)
        {
            painter.drawText(bX + 10, y, lines[i]);
            painter.drawText(colX, y, lines[i + 1]);
            y += lineH;
        }
    };

    painter.setFont(f1);
    painter.setPen(penLineWhite);
    painter.drawText(bX + 5, y, "Controls");
    y += lineH;

    drawSection("View", camera, penLineWhite);
    drawSection(editEnabled ? "Edition" : "Edition (disabled while simulating)", edition, editEnabled ? penLineWhite : penLineDisabled);

    painter.setFont(f2);
    painter.setPen(penLineDisabled);
    painter.drawText(bX + 5, y, "H: hide controls");

    painter.end();

    // Reset GL depth test
    glEnable(GL_DEPTH_TEST);
}

void TRWEditor::RenderUiPanelEditor()
{
    if (!sf) return;
    if (mapType == MapType::NONE && mapTypeS == MapSpecies::NONE) return;

    // We need to unbind VAO and program for now because it causes problem with below command.
    glBindVertexArray(0);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen penLineGrey(QColor(50, 50, 50));
    QPen penLineWhite(QColor(250, 250, 250));

    // Panel
    const int sizeX = 240;
    const int sizeY = (mapType== MapType::MASK ? 20 : mapTypeS == MapSpecies::LIF ? 40 : 60);
    const int bX = width()-10- sizeX;
    const int bY = 10;

    // Background
    painter.setPen(penLineGrey);
    painter.fillRect(QRect(bX, bY, sizeX, sizeY), QColor(0, 0, 255, 25));
    painter.drawRect(bX, bY, sizeX, sizeY);

    // Text
    QFont f1, f2;
    f1.setBold(true);
    painter.setFont(f1);
    painter.setPen(penLineWhite);
    painter.drawText(bX + 5, bY + 10 + 5, "Editing Map:\t" + GetMapName());
    if (!(mapType == MapType::MASK || mapTypeS == MapSpecies::LIF)) painter.drawText(bX + 5, bY + 10 + 25, GetMonthNameSpecies());

    int i, j;
    double val = 0.0;
    if (mapType != MapType::MASK && mapTypeS == MapSpecies::NONE)
    {
        const ScalarField2& sf = resourceData.GetScalarField(mapType, month);
        sf.CellInteger(position, i, j);
        val = sf.at(i, j);
        if (mapType == MapType::TEMPERATURE)
        {
            double min = resourceData.GetTempMap().GetMinTemps()[month];
            double max = resourceData.GetTempMap().GetMaxTemps()[month];
            val = (max - min) * val + min;
        }
        painter.drawText(bX + 5, bY + 10 + 45, "Value:\t" + QString::number(val));
    }
    else
    {
        if (mapType == MapType::NONE && mapTypeS != MapSpecies::NONE)
        {
            const SpeciesData& sd = ecosystemData.GetData(species);
            const ScalarField2& sfSP = sd.sf_seedProbs;
            const ScalarField2& sfR = sd.sf_regenLayers;
            const ScalarField2& sfT = sd.sf_trees;
            const ScalarField2& sfB = sd.sf_boost;
            const ScalarField2& sfLif = ecosystemData.GetSFLIF();

            switch (mapTypeS)
            {
            case MapSpecies::NONE:
                break;
            case MapSpecies::SEED_PROBS:
                sfSP.CellInteger(position, i, j);
                if (i < 0 || i >= sfSP.GetSizeX() || j < 0 || j >= sfSP.GetSizeY()) break;
                val = sfSP.at(i, j);
                break;
            case MapSpecies::REGEN_LAYERS:
                sfR.CellInteger(position, i, j);
                if (i < 0 || i >= sfR.GetSizeX() || j < 0 || j >= sfR.GetSizeY()) break;
                val = sfR.at(i, j);
                break;
            case MapSpecies::TREES:
                sfT.CellInteger(position, i, j);
                if (i < 0 || i >= sfT.GetSizeX() || j < 0 || j >= sfT.GetSizeY()) break;
                val = sfT.at(i, j);
                break;
            case MapSpecies::BOOST:
                sfB.CellInteger(position, i, j);
                if (i < 0 || i >= sfB.GetSizeX() || j < 0 || j >= sfB.GetSizeY()) break;
                val = sfB.at(i, j);
                break;
            case MapSpecies::LIF:
                sfLif.CellInteger(position, i, j);
                if (i < 0 || i >= sfLif.GetSizeX() || j < 0 || j >= sfLif.GetSizeY()) break;
                val = sfLif.at(i, j);
                break;
            default:
                break;
            }
            if (mapTypeS != MapSpecies::LIF) painter.drawText(bX + 5, bY + 10 + 45, "Value:\t" + QString::number(val));
            else painter.drawText(bX + 5, bY + 10 + 25, "Value:\t" + QString::number(val));
        }
    }
    
    painter.end();

    // Reset GL depth test
    glEnable(GL_DEPTH_TEST);
}


void TRWEditor::setIntensity(int i) 
{ 
    if (i < 0) i = 0;
    if (i > 100) i = 100;
    double _i = double(i) / 100.;
    double _min, _max;
    GetMinMax(_min, _max);
    intensity = _i * (_max - _min) + _min;

    updateTool(); 
}

QString TRWEditor::GetMapName()
{
    if (mapType != MapType::NONE) 
        switch (mapType)
        {
        case MapType::SUNLIGHT:
            return "Sunlight";
        case MapType::MOISTURE:
            return "Moisture";
        case MapType::TEMPERATURE:
            return "Temperature";
        case MapType::MASK:
            return "Mask";
        default:
            return "None";
        }
    else
        switch (mapTypeS)
        {
        case MapSpecies::SEED_PROBS:
            return "Seed Probability";
        case MapSpecies::REGEN_LAYERS:
            return "Saplings";
        case MapSpecies::TREES:
            return "Individual Trees";
        case MapSpecies::LIF:
            return "Light Interference Field";
        default:
            return "None";
        }
}

QString TRWEditor::GetMonthNameSpecies()
{
    QStringList Months = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
    if (mapType != MapType::NONE)
        return "Month :\t"+Months[month];
    else
        return "Species :\t" + ecosystemData.GetSpeciesParam(species).name;
}
