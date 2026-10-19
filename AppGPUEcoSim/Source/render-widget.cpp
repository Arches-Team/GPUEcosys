#include "realtime.h"
#include "cpu.h"

#include <QtWidgets/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>


/*!
\brief Default constructor.
*/
RenderWidget::RenderWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    perspectiveProjection = true;
    cameraOrthoSize = 1000.0f;
    backgroundColor = Color::White;

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

/*!
\brief Destructor.
*/
RenderWidget::~RenderWidget()
{
    // Release shader
    skyShader.Destroy();
}

/*!
\brief Initialize OpenGL, shaders and a camera centered at origin.
*/
void RenderWidget::initializeGL()
{
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cout << "GLEW Error: " << glewGetErrorString(err) << std::endl;
        std::cin.get();
        exit(-1);
    }

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Disable vsync for proper performance measurements

    // Shader/Camera/Profiler
    ReloadShaders();

    glGenVertexArrays(1, &skyboxVAO);

    camera = Camera::View(Box(10.0));
    SetNearAndFarPlane(nearAndFarPlane);

    profiler.Init();
}

/*!
\brief Renders the scene.
*/
void RenderWidget::paintGL()
{
    // Custom update from user
    emit _signalUpdate();

    profiler.drawCallPerFrame = 0;

    // Clear
    glClearColor(float(backgroundColor[0]), float(backgroundColor[1]), float(backgroundColor[2]), float(backgroundColor[3]));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Move camera
    if (MoveAt)
    {
        if (stepAt < 15)
        {
            stepAt++;
            double alpha = double(stepAt) / 15.;
            camera.SetAt(currentAt * (1.0 - alpha) + toAt * (alpha));
        }
        else
            MoveAt = false;
    }

    // Sky
    glEnable(GL_DEPTH_TEST);
    if (useSkyShader)
    {
        RenderSky();
        profiler.drawCallPerFrame++;
        glBindVertexArray(0);
        glUseProgram(0);
    }

    // Draw meshes
    profiler.BeginGPU();

    // CPU Profiling
    if (profiler.enabled)
        profiler.Update();

    RenderUiPanels();

    // Schedule next draw
    update();
}

/*!
\brief Resize window.
\param w, h Width and height.
*/
void RenderWidget::resizeGL(int w, int h)
{
    const QSize fbSize = QSize(w, h) * devicePixelRatio();
    glViewport(0, 0, (GLint)fbSize.width(), (GLint)fbSize.height());
}

/*!
\brief Render the sky.
*/
void RenderWidget::RenderSky()
{
    glDepthFunc(GL_LEQUAL);
    skyShader.Bind();
    glBindVertexArray(skyboxVAO);
    skyShader.SetUniform(0, camera.Eye());
    skyShader.SetUniform(1, camera.At());
    skyShader.SetUniform(2, camera.Up());
    skyShader.SetUniform(3, float(width()), float(height()));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/*!
\brief Render the UI panels of the widget.
*/
void RenderWidget::RenderUiPanels()
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

    // Stats panel
    int offsetY = 0;
    if (profiler.enabled)
    {
        const int bX = 10;
        const int bY = 10;
        const int sizeX = 200;
        const int sizeY = 80;

        offsetY += sizeY + 10;

        // Background
        painter.setPen(penLineGrey);
        painter.fillRect(QRect(bX, bY, sizeX, sizeY), QColor(0, 0, 255, 25));
        painter.drawRect(bX, bY, sizeX, sizeY);

        // Text
        QFont f1, f2;
        f1.setBold(true);
        painter.setFont(f1);
        painter.setPen(penLineWhite);
        painter.drawText(10 + 5, bY + 10 + 5, "Statistics");
        painter.setFont(f2);
        painter.drawText(10 + 5, bY + 10 + 20, "CPU FPS:\t" + QString::number(profiler.framePerSecond));
        painter.drawText(10 + 5, bY + 10 + 35, "CPU Frame:\t" + QString::number(profiler.msPerFrame) + "ms");
        painter.drawText(10 + 5, bY + 10 + 50, "GPU:\t" + QString::number(profiler.elapsedTimeGPU / 1000000.0) + "ms");
        painter.drawText(10 + 5, bY + 10 + 65, "Draw calls:\t" + QString::number(profiler.drawCallPerFrame));
    }

    // Camera panel
    if (renderCameraPanel)
    {
        const int bX = 10;
        const int bY = offsetY + 10;
        const int sizeX = 400;
        const int sizeY = 30;

        painter.setPen(penLineGrey);
        painter.fillRect(QRect(bX, (bY), sizeX, sizeY), QColor(0, 0, 255, 25));
        painter.drawRect(bX, (bY), sizeX, sizeY);

        painter.setPen(penLineWhite);
        painter.drawText(bX + 10, bY + 12 + 1 * 15, camera.ToString());

        QFont f1; f1.setBold(true);
        painter.setFont(f1);
        painter.drawText(bX + bX, bY + 12, QString("Camera Position"));
    }

    painter.end();

    // Reset GL depth test
    glEnable(GL_DEPTH_TEST);
}

/*!
\brief Reload the shaders of the widget. Useful for realtime editing and fine tuning of the rendering.
*/
void RenderWidget::ReloadShaders()
{
    // Sky
    QString fullPath = QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders/skybox.glsl");
    QByteArray ba = fullPath.toLocal8Bit();
    skyShader.Initialize(ba.constData());
}

/*!
\brief Set the sky rendering mode.
\param useShader flag for using the sky shader or not.
\param clearColor background color used in case the shader is not enabled.
*/
void RenderWidget::SetSky(bool useShader, const Color& clearColor)
{
    useSkyShader = useShader;
    backgroundColor = clearColor;
}

/*!
\brief Compute a ray starting from the camera position at a given pixel.
\param pix the pixel
*/
Ray RenderWidget::ConvertPixelToRay(const QPoint& pix) const
{
    return camera.PixelToRay(pix.x(), pix.y(), width(), height());
}

/*!
\brief Capture the rendering viewport and save it to disk (as png and jpg).
\param w, h Width and height of the image.
*/
void RenderWidget::SaveScreen(int w, int h)
{
    QSize qs = size();
    resize(w, h);
    QImage image(size(), QImage::Format_ARGB32);

    // Set name according to date and time
    QString name = SOLUTION_DIR "screen-" + System::DateTime();

    // Get image
    image = grabFramebuffer();

    // Save
    image.save(name + ".png", "PNG");
    image.save(name + ".jpg", "JPG", 96);

    // Restore buffer size
    resize(qs);
}

/*!
\brief Capture the rendering viewport and save it to disk.
\param w, h Width and height of the image.
\param url Name of the exported file
*/
void RenderWidget::SaveScreen(int w, int h, const QString& url)
{
    QSize qs = size(); // Save buffersize

    resize(w, h);
    QImage image(size(), QImage::Format_ARGB32);

    // Get image
    image = grabFramebuffer();

    // Save
    image.save(url, "PNG");

    // Restore buffer size
    resize(qs);
}

/*!
\brief Set the current camera.
\param cam new camera
*/
void RenderWidget::SetCamera(const Camera& cam)
{
    camera = cam;
    camera.SetPlanes(nearAndFarPlane[0], nearAndFarPlane[1]);
}

/*!
\brief Set the near and far plane of the camera.
\param planes near and far planes
*/
void RenderWidget::SetNearAndFarPlane(const Vector2& planes)
{
    nearAndFarPlane = planes;
    camera.SetPlanes(nearAndFarPlane[0], nearAndFarPlane[1]);
}

/*!
\brief Returns the current camera.
*/
Camera RenderWidget::GetCamera() const
{
    return camera;
}

/*!
\brief Capture and return the rendering viewport.
\param w, h dimensions of the capture
*/
QImage RenderWidget::GrabScreen(int w, int h)
{
    QSize qs = size();
    resize(w, h);
    QImage image(size(), QImage::Format_ARGB32);

    // Set name according to date and time
    QString name = SOLUTION_DIR "screen-" + System::DateTime();

    // Get image
    image = grabFramebuffer();

    resize(qs);

    return image;
}


/*!
\brief Set the camera mode (perspective or orthographic).
\param perspective
*/
void RenderWidget::SetCameraMode(bool perspective)
{
    perspectiveProjection = perspective;
}

/*!
\brief Returns the current mouse position.
*/
QPoint RenderWidget::GetMousePosition() const
{
    return QPoint(x0, y0);
}

/*!
\brief Process mouse click events.
\param e Events.
*/
void RenderWidget::mousePressEvent(QMouseEvent* e)
{
    x0 = e->globalPosition().x();
    y0 = e->globalPosition().y();
    if (e->modifiers() & Qt::ControlModifier)
    {
        if (e->buttons() == Qt::LeftButton)
            emit _signalEditSceneLeft(ConvertPixelToRay(e->pos()));
        else if (e->buttons() == Qt::RightButton)
            emit _signalEditSceneRight(ConvertPixelToRay(e->pos()));
        emit _signalMouseMove();
    }
}

/*!
\brief Process the mouse release events.
\param e Events.
*/
void RenderWidget::mouseReleaseEvent(QMouseEvent*)
{
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    emit _signalMouseRelease();
    update();
}

/*!
\brief Mouse move event with camera movements.
*/
void RenderWidget::mouseMoveEvent(QMouseEvent* e)
{
    int x = e->globalPosition().x();
    int y = e->globalPosition().y();
    if ((e->modifiers() & Qt::AltModifier))
    {
        // Displacement mode 
        double MoveScale = Norm(camera.View()) * 0.015 * 0.05;
        if (e->buttons() & Qt::LeftButton)
        {
            // Alt + Left Mouse Move    : Rotation 
            camera.LeftRightRound((x0 - x) * 0.01);
            camera.UpDownRound((y0 - y) * 0.005);
        }
        else if (e->buttons() & Qt::RightButton)
        {
            // Alt + Right Mouse Move   : Forward and Backward
            camera.BackForth((y - y0) * MoveScale);
            QApplication::setOverrideCursor(QCursor(Qt::SplitVCursor));
        }
        else if (e->buttons() & Qt::MiddleButton)
        {
            // Alt + Left Mouse Move    : Plan displacement
            camera.LeftRightHorizontal((x - x0) * MoveScale);
            camera.UpDownVertical((y - y0) * MoveScale);
            QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
        }
        x0 = e->globalPosition().x();
        y0 = e->globalPosition().y();

        emit _signalMouseMove();
    }
    if (e->modifiers() & Qt::ControlModifier)
    {
        emit _signalMouseMoveEdit(ConvertPixelToRay(e->pos()));
    }
}

/*!
\brief Mouse wheel event with zoom.
*/
void RenderWidget::wheelEvent(QWheelEvent* e)
{
    if (!(e->modifiers() & Qt::ControlModifier) && !(e->modifiers() & Qt::ShiftModifier))
    {
        double MoveScale = Norm(camera.View()) * 0.025;
        if (perspectiveProjection)
        {
            if (e->angleDelta().y() > 0)
                camera.BackForth(MoveScale);
            else
                camera.BackForth(-MoveScale);
        }
        else
        {
            if (e->angleDelta().y() > 0)
                cameraOrthoSize -= MoveScale * 0.25;
            else
                cameraOrthoSize += MoveScale * 0.25;
        }
        emit _signalMouseMove();
    }
    update();
}

/*!
\brief Key press event.
*/
void RenderWidget::keyPressEvent(QKeyEvent* e)
{
    switch (e->key())
    {
        // F1: 720p screenshot in the app folder
    case Qt::Key_F1:
        SaveScreen();
        break;
        // F2: 4k screen in the app folder
    case Qt::Key_F2:
        SaveScreen(3840, 2160);
        break;
        // F3: hot reload shader
    case Qt::Key_F3:
        ReloadShaders();
        break;
    case Qt::Key_S:
        // Alt + S: Statistics
        if (e->modifiers() & Qt::AltModifier)
            profiler.enabled = !profiler.enabled;
        break;
    case Qt::Key_C:
        // Alt + C: Camera
        if (e->modifiers() & Qt::AltModifier)
            renderCameraPanel = !renderCameraPanel;
        break;
    default:
        QOpenGLWidget::keyPressEvent(e);
    }
}

/*!
\brief Key release event.
*/
void RenderWidget::keyReleaseEvent(QKeyEvent* e)
{
    QOpenGLWidget::keyReleaseEvent(e);
    update();
}
