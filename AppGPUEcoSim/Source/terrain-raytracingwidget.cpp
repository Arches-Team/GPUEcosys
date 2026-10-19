#include "realtime.h"
#include "scalarfield.h"
#include "cpu.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

/*!
\class TerrainRaytracingWidget realtime.h
\brief %Heightfield rendering widget based on sphere-tracing.
*/

/*!
\brief Default constructor.
*/
TerrainRaytracingWidget::TerrainRaytracingWidget(QWidget* parent) : RenderWidget(parent)
{
}

/*!
\brief Destructor. Release shader program, vao and textures.
*/
TerrainRaytracingWidget::~TerrainRaytracingWidget()
{
    release_program(shaderProgram);
    glDeleteVertexArrays(1, &raytraceVAO);
    glDeleteTextures(1, &albedoTextureBuffer);

    hfBuffer.Destroy();
    shadingBuffer.Destroy();
}

/*!
\brief Initialize OpenGL, shaders and a camera centered at origin.
*/
void TerrainRaytracingWidget::initializeGL()
{
    RenderWidget::initializeGL();

    QString fullPath = QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders/heightfield_raytrace.glsl");
    QByteArray ba = fullPath.toLocal8Bit();
    shaderProgram = read_program(ba.data());

    camera = Camera::View(Box(10.0));
    cameraAngleOfViewV = camera.GetAngleOfViewV(width(), height());

    glGenVertexArrays(1, &raytraceVAO);
    hfBuffer.Generate();
    shadingBuffer.Generate();
    glGenTextures(1, &albedoTextureBuffer);

    glBindVertexArray(0);
    glUseProgram(0);
}

/*!
\brief Renders the scene.
*/
void TerrainRaytracingWidget::paintGL()
{
    // Custom update from user
    emit _signalUpdate();

    profiler.drawCallPerFrame = 0;

    // Framebuffer size in physical pixels, computed (rounded) the same way Qt allocates the FBO.
    // Qt truncates when setting the viewport, which leaves an unpainted row/column with fractional DPI scaling.
    const QSize fbSize = size() * devicePixelRatio();
    glViewport(0, 0, fbSize.width(), fbSize.height());

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

    // Draw
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "albedo"), 1);
    glActiveTexture(GL_TEXTURE1);
    // Albedo
    if (useAlbedo)
        glBindTexture(GL_TEXTURE_2D, albedoTextureBuffer);
    else if (useExternAlbedo)
        glBindTexture(GL_TEXTURE_2D, externAlbedo);
    else
        glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Uniforms - Camera (in GL coordinate system, xzy)
    glUniform3f(0, camera.Eye()[0], camera.Eye()[1], camera.Eye()[2]);
    glUniform3f(1, camera.At()[0], camera.At()[1], camera.At()[2]);
    glUniform3f(2, camera.Up()[0], camera.Up()[1], camera.Up()[2]);
    glUniform1f(3, camera.GetAngleOfViewV(width(), height()));

    glUniform2f(4, float(fbSize.width()), float(fbSize.height()));

    if (hfBuffer.GetBuffer() != 0)
    {
        // Bind buffers
        hfBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
        shadingBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 1);

        // Uniforms - Heightfield
        glUniform2f(5, bbox[0][0], bbox[0][1]);
        glUniform2f(6, bbox[1][0], bbox[1][1]);
        glUniform2f(7, zMin, zMax);
        glUniform1f(8, K);
        glUniform2i(9, nx, ny);
    }
    glUniform1i(10, int(useAlbedo || useExternAlbedo));
    glUniform1i(11, int(useWireframe));
    glUniform1i(12, int(useCost));
    glUniform1i(13, int(useElevationShading));
    glUniform1i(14, int(useGreenBrownYellow));
    //glUniform1i(16, int(useShadingBuffer));
    glUniform2f(30, float(nearAndFarPlane[0]), float(nearAndFarPlane[1]));

    glUniform1i(32, int(show_sides));
    glUniform2i(34, render_res_x, render_res_y);
    glUniform1i(35, int(show_under));

    Matrix4Float view = Matrix4Float::LookAt(camera.Eye(), camera.At(), camera.Up());
    Matrix4Float projection = Matrix4Float::Perspective(Math::RadianToDegree(camera.GetAngleOfViewV(width(), height())), nearAndFarPlane[0], nearAndFarPlane[1], width(), height());
    glUniformMatrix4fv(36, 1, GL_FALSE, &view(0, 0));
    glUniformMatrix4fv(37, 1, GL_FALSE, &projection(0, 0));

    // Draw
    profiler.BeginGPU();

    glBindVertexArray(raytraceVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
    profiler.drawCallPerFrame++;

    profiler.EndGPU();

    // Draw sky
    glEnable(GL_DEPTH_TEST);
    if (useSkyShader)
    {
        RenderSky();
        profiler.drawCallPerFrame++;
        glBindVertexArray(0);
        glUseProgram(0);
    }

    // CPU Profiling
    if (profiler.enabled)
        profiler.Update();
    RenderUiPanels();

    update();
}

/*!
\brief Internal update function of the widget.

Updates the internal height buffer from the heightfield pointer, recomputes the Lipschitz constant and min/max elevations.
*/
void TerrainRaytracingWidget::SetHeightField(ScalarField2* sfPtr)
{
    sf = sfPtr;
    UpdateInternal();
}

/*!
\brief Update the GPU data from the internal CPU heightfield pointer.

This function is computationally intensive as it recomputes the Lipschitz constant of the terrain and send data to the GPU.
*/
void TerrainRaytracingWidget::UpdateInternal()
{
    makeCurrent(); // do not remove

    tmpData.resize(sf->VertexSize());

    // Min/Max elevation
    double zMinDouble, zMaxDouble;
    sf->GetRange(zMinDouble, zMaxDouble);
    zMin = float(zMinDouble);
    zMax = float(zMaxDouble);

    nx = sf->GetSizeX();
    ny = sf->GetSizeY();
    bbox = sf->GetBox();

    // This avoids problem with rendering a flat heightfield.
    if (zMin == zMax)
        zMax += 10.0f;

    // Global Lipschitz constant
    K = sf->K();
    K = sqrt(1.0 + K * K);

    // Heightfield data texture
    for (int i = 0; i < sf->VertexSize(); i++)
        tmpData[i] = sf->at(i);
    hfBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * sf->VertexSize(), &tmpData.front(), GL_STREAM_READ);

    for (int i = 0; i < sf->VertexSize(); i++)
        tmpData[i] = 1.0f;
    shadingBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * sf->VertexSize(), &tmpData.front(), GL_STREAM_READ);
}

/*!
\brief Reload the shaders of the widget. Useful for realtime editing and fine tuning of the rendering.
*/
void TerrainRaytracingWidget::ReloadShaders()
{
    RenderWidget::ReloadShaders();

    QString fullPath = QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders/heightfield_raytrace.glsl");
    QByteArray ba = fullPath.toLocal8Bit();
    shaderProgram = read_program(ba.data());
}

/*!
\brief Function override from AbstractTerrainWidget.

Used to store the camera vertical angle of view, which avoids calling atan() intensively.

\param cam new camera
*/
void TerrainRaytracingWidget::SetCamera(const Camera& cam)
{
    RenderWidget::SetCamera(cam);
    cameraAngleOfViewV = camera.GetAngleOfViewV(width(), height());
}

/*!
\brief Update the albedo texture with an existing OpenGL texture.

UseAlbedo(true) should have been called before, or no update will be performed.
\param texture Texture.
*/
void TerrainRaytracingWidget::SetExternAlbedo(GLuint texture)
{
    if (sf)
    {
        makeCurrent();

        glUseProgram(shaderProgram);

        glProgramUniform1i(shaderProgram, glGetUniformLocation(shaderProgram, "albedo"), 1);
        externAlbedo = texture;

        glUseProgram(0);
    }
}

/*!
\brief Update the albedo texture flag.
\param u new value for albedo flag.
*/
void TerrainRaytracingWidget::UseExternAlbedo(bool u)
{
    useExternAlbedo = u;
}

/*!
\brief Update the albedo texture. UseAlbedo(true) should have been
called before, or no update will be performed.
\param albedo texture
*/
void TerrainRaytracingWidget::SetAlbedo(const QImage& albedo)
{
    if (sf)
    {
        makeCurrent();

        QImage albedoGL = albedo.convertedTo(QImage::Format::Format_RGBA8888);

        glUseProgram(shaderProgram);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, albedoTextureBuffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, 3, albedoGL.width(), albedoGL.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, albedoGL.bits());
        glProgramUniform1i(shaderProgram, glGetUniformLocation(shaderProgram, "albedo"), 1);

        glUseProgram(0);
    }
}

/*!
\brief Update the internal albedo texture flag.
\param u new value for albedo flag.
*/
void TerrainRaytracingWidget::UseAlbedo(bool u)
{
    useAlbedo = u;
}

/*!
\brief Set the wireframe flag.
\param wire flag
*/
void TerrainRaytracingWidget::UseWireframe(bool wire)
{
    useWireframe = wire;
}

/*!
\brief Set the cost shading flag.
\param cost flag
*/
void TerrainRaytracingWidget::UseCost(bool cost)
{
    useCost = cost;
}

/*!
\brief Set the elevation shading flag.
\param u flag
*/
void TerrainRaytracingWidget::UseElevationShading(bool u)
{
    useElevationShading = u;
    if (useElevationShading)
        useGreenBrownYellow = false;
}

/*!
\brief Set the shading from color map flag.
\param u flag
*/
void TerrainRaytracingWidget::UseGreenBrownYellowShading(bool u)
{
    useGreenBrownYellow = u;
    if (useGreenBrownYellow)
        useElevationShading = false;
}

/*!
\brief Set the elevation range for the terrain.
\param min, max elevation range
*/
void TerrainRaytracingWidget::SetElevationRange(double min, double max)
{
    zMin = float(min);
    zMax = float(max);
    if (zMin == zMax)
        zMax += 10.0f;
}

/*!
\brief Set the maximum number of raymarching step.
\param s steps
*/
void TerrainRaytracingWidget::SetSteps(int s)
{
    glUseProgram(shaderProgram);
    glUniform1i(28, s);
    glUseProgram(0);
}

/*!
\brief Set the smooth shadow flag.
\param use flag
*/
void TerrainRaytracingWidget::UseSmoothShadow(bool use)
{
    glUseProgram(shaderProgram);
    glUniform1i(17, int(use));
    glUseProgram(0);
}

/*!
\brief Set the number of smooth shadow steps.
\param steps number of steps
*/
void TerrainRaytracingWidget::SetSmoothShadowSteps(int steps)
{
    glUseProgram(shaderProgram);
    glUniform1i(18, steps);
    glUseProgram(0);
}

/*!
\brief Set the maximum number of smooth shadow raymarching steps.
\param steps max number of steps
*/
void TerrainRaytracingWidget::SetSmoothShadowMarchingSteps(int steps)
{
    glUseProgram(shaderProgram);
    glUniform1i(19, steps);
    glUseProgram(0);
}

/*!
\brief Set the minimum marching epsilon for smooth shadow rays.
\param eps marching epsilon
*/
void TerrainRaytracingWidget::SetSmoothShadowMarchingEpsilon(float eps)
{
    glUseProgram(shaderProgram);
    glUniform1f(20, eps);
    glUseProgram(0);
}

/*!
\brief Set the smooth shadow strength.
\param strength
*/
void TerrainRaytracingWidget::SetSmoothShadowStrength(float strength)
{
    glUseProgram(shaderProgram);
    glUniform1f(21, strength);
    glUseProgram(0);
}

/*!
\brief Set the smooth shadow flag.
\param use flag
*/
void TerrainRaytracingWidget::UseSelfShadow(bool use)
{
    glUseProgram(shaderProgram);
    glUniform1i(22, int(use));
    glUseProgram(0);
}

/*!
\brief Set the maximum number of self shadow raymarching steps.
\param steps max number of steps
*/
void TerrainRaytracingWidget::SetSelfShadowMarchingSteps(int steps)
{
    glUseProgram(shaderProgram);
    glUniform1i(23, steps);
    glUseProgram(0);
}

/*!
\brief Set the minimum marching epsilon for self shadow rays.
\param eps marching epsilon
*/
void TerrainRaytracingWidget::SetSelfShadowMarchingEpsilon(float eps)
{
    glUseProgram(shaderProgram);
    glUniform1f(24, eps);
    glUseProgram(0);
}

/*!
\brief Set the self shadow strength.
\param strength
*/
void TerrainRaytracingWidget::SetSelfShadowStrength(float strength)
{
    glUseProgram(shaderProgram);
    glUniform1f(25, strength);
    glUseProgram(0);
}

/*!
\brief Set the minimum stepping distance for raymarching.
\param eps minimum stepping distance
*/
void TerrainRaytracingWidget::SetEpsilon(float eps)
{
    glUseProgram(shaderProgram);
    glUniform1f(29, eps);
    glUseProgram(0);
}

/*!
\brief Set the directional light.
\param light light direction
*/
void TerrainRaytracingWidget::SetLight(const Vector& light)
{
    glUseProgram(shaderProgram);
    VectorFloat l = VectorFloat(light);
    glUniform3f(26, l.x, l.y, l.z);
    glUseProgram(0);
}

/*!
\brief Set the amount of antialiasing.
\param AA antialiasing, should be between [1, 128].
*/
void TerrainRaytracingWidget::SetAntialiasing(int AA)
{
    glUseProgram(shaderProgram);
    glUniform1i(27, AA);
    glUseProgram(0);
}

/*!
\brief Chooses to show terrain sides.
\param showSides true to show grey terrain sides, false for transparency.
*/
void TerrainRaytracingWidget::ShowSides(bool showSides)
{
    show_sides = showSides;
}

/*!
\brief Chooses to show terrain undersides.
\param showUnder true to show underside of the terrain with the same texture, false for transparency.
*/
void TerrainRaytracingWidget::ShowUnderside(bool showUnder)
{
    show_under = showUnder;
}

/*!
\brief Manually change the underlying heightfield buffer.

This function is useful when performing erosion on the gpu - no need to retrieve the data on the CPU
and manually recreate all the buffers.

Note that the buffer must contain data whose size is equal to the base heightfield.

\param buffer initialized buffer with elevation data.
*/
void TerrainRaytracingWidget::UpdateBuffer(GLuint buffer)
{
    hfBuffer = GLBuffer(buffer);
}

/*!
\brief Set the additional shading buffer that can be used by the glsl.
\param shading 2D shading map, must be the same size as the underlying heightfield.
*/
void TerrainRaytracingWidget::SetShading(const ScalarField2& shading)
{
    for (int i = 0; i < sf->VertexSize(); i++)
        tmpData[i] = shading.at(i);
    shadingBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * sf->VertexSize(), &tmpData.front(), GL_STREAM_READ);
}

/*!
\brief Set the additional shading flag.
\param use flag
*/
void TerrainRaytracingWidget::UseShading(bool use)
{
    useShadingBuffer = use;
}

/*!
\brief Hot reload of the glsl program. Properly rebinds the albedo texture.
*/
void TerrainRaytracingWidget::Reload()
{
    release_program(shaderProgram);

    ReloadShaders();

    glUseProgram(shaderProgram);
    glProgramUniform1i(shaderProgram, glGetUniformLocation(shaderProgram, "albedo"), 1);
    glUseProgram(0);
}

static uchar float_to_uchar_color(float color) {
    color *= 256;
    if (color > 255)
        color = 255;
    if (color < 0)
        color = 0;
    return static_cast<uchar>(color);
};

ScalarField2* TerrainRaytracingWidget::GetHeightField() const {
    return sf;
}
GLBuffer TerrainRaytracingWidget::GetHeightFieldBuffer() const {
    return hfBuffer;
}
Box2 TerrainRaytracingWidget::GetBoundingBox() const {
    return bbox;
}
Vector2 TerrainRaytracingWidget::GetHeightFieldRange() const {
    return Vector2(double(zMin), double(zMax));
}
float TerrainRaytracingWidget::GetLipschitzConstant() const {
    return K;
}
QPoint TerrainRaytracingWidget::GetHeightFieldSize() const {
    return QPoint(nx, ny);
}
Vector2 TerrainRaytracingWidget::GetNearAndFarPlane() const {
    return Vector2(double(nearAndFarPlane[0]), double(nearAndFarPlane[1]));
}
bool TerrainRaytracingWidget::IsUsingAlbedo() const {
    return useAlbedo;
}
GLuint TerrainRaytracingWidget::GetAlbedoTex() const {
    return albedoTextureBuffer;
}

QImage TerrainRaytracingWidget::PBR_Render(const TerrainRaytracingWidget::PBR_Render_Options& opt) const {

    int width = opt.width;
    int height = opt.height;

    GLShader shader = GLShader();

    QString fullPath = QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders/pbr_terrain.glsl");
    shader.Initialize(fullPath.toLocal8Bit().constData());

    double hmin, hmax;
    sf->GetRange(hmin, hmax);
    float h_sea = std::max(0.f, opt.sea_level) * (hmax - hmin) + hmin;

    // ISOLINES
    std::vector<float> iso_heights = opt.iso_heights;
    if (iso_heights.size() == 0 && opt.iso_wave_length > 0.f) {

        float iso_wave_length = std::max(1.f, opt.iso_wave_length);
        float h = h_sea - opt.iso_wave_length;
        while (h < hmax) {
            h += iso_wave_length;
            iso_heights.push_back(h);
        }
    }
    else if (iso_heights.size() == 0 && opt.nb_iso > 0) {

        for (int i = 0; i < opt.nb_iso; i++) {
            float h = h_sea + (hmax - h_sea) * (float(i + (opt.sea_level > 0.f ? 0 : 1)) / (opt.nb_iso + 1));
            iso_heights.push_back(h);
        }
    }

    // Uniform
    hfBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    glUniform3f(0, camera.Eye()[0], camera.Eye()[1], camera.Eye()[2]);
    glUniform3f(1, camera.At()[0], camera.At()[1], camera.At()[2]);
    glUniform3f(2, camera.Up()[0], camera.Up()[1], camera.Up()[2]);
    if (this->width() / this->height() > width / height) {
        glUniform1f(3, camera.GetAngleOfViewV(this->width(), this->height()));
    }
    else {
        glUniform1f(3, camera.GetAngleOfViewV(width, height));
    }
    glUniform2f(4, width, height);
    glUniform2f(5, bbox[0][0], bbox[0][1]);
    glUniform2f(6, bbox[1][0], bbox[1][1]);
    glUniform2f(7, zMin, zMax);
    glUniform1f(8, K);
    glUniform2i(9, nx, ny);
    glUniform1i(10, int(iso_heights.size()));
    glUniform1i(11, opt.orthogonal_projection);

    glUniform3f(13, float(opt.main_isolines_color[0]), float(opt.main_isolines_color[1]), float(opt.main_isolines_color[2]));
    glUniform3f(14, float(opt.lesser_isolines_color[0]), float(opt.lesser_isolines_color[1]), float(opt.lesser_isolines_color[2]));
    glUniform1f(15, opt.main_isolines_width);
    glUniform1f(16, opt.lesser_isolines_width);
    glUniform1i(17, opt.main_isolines_period);

    glUniform3f(18, float(opt.sea_shore_color[0]), float(opt.sea_shore_color[1]), float(opt.sea_shore_color[2]));
    glUniform3f(19, float(opt.sea_deep_color[0]), float(opt.sea_deep_color[1]), float(opt.sea_deep_color[2]));
    glUniform1f(20, opt.sea_level);

    glUniform3f(21, float(opt.terrain_color[0]), float(opt.terrain_color[1]), float(opt.terrain_color[2]));
    glUniform3f(22, float(opt.sky_color[0]), float(opt.sky_color[1]), float(opt.sky_color[2]));

    glUniform1i(25, opt.nb_samples_per_shader);
    glUniform1i(27, opt.anti_aliasing);
    glUniform2f(30, float(nearAndFarPlane[0]), float(nearAndFarPlane[1]));

    glUniform1f(31, opt.under_shadow_distance);

    glUniform3f(32, float(opt.light_1[0]), float(opt.light_1[1]), float(opt.light_1[2]));
    glUniform3f(33, float(opt.light_2[0]), float(opt.light_2[1]), float(opt.light_2[2]));

    glUniform1f(34, opt.h_threshold);
    glUniform1i(35, int(opt.show_outside_shadows));

    glUniform1f(36, opt.alpha_shadows);

    // texture
    shader.Bind();

    GLuint outImage;
    glCreateTextures(GL_TEXTURE_2D, 1, &outImage);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outImage);
    glTextureParameteri(outImage, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(outImage, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(outImage, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(outImage, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(outImage, 1, GL_RGBA32F, width, height);
    glBindImageTexture(0, outImage, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    if (useAlbedo) {
        std::cout << "tex bind" << std::endl;
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, albedoTextureBuffer);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(12, 1);
    }

    QElapsedTimer time;
    time.restart();

    for (int i = 0; i < opt.nb_dispatch; i++) {
        std::cout << "rendering ... " << int(100. * i / opt.nb_dispatch) << '\r';
        glUniform1i(23, i); // dispatch_id
        glDispatchCompute(ceil(width / 8.0), ceil(height / 8.0), 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glFinish();
    }

    std::cout << " rendered in " << time.elapsed() << "ms." << std::endl;

    GLsizei bufferSize = 4 * width * height;
    std::vector<float> buffer(bufferSize);
    std::vector<uchar> byte_buffer(bufferSize);

    glBindTexture(GL_TEXTURE_2D, outImage);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, buffer.data());

    glClear(GL_COLOR_BUFFER_BIT);
    shader.Destroy();
    glDeleteTextures(1, &outImage);

    for (int i = 0; i < bufferSize / 4; i++) {
        byte_buffer[4 * i + 0] = float_to_uchar_color(buffer[4 * i + 2]);
        byte_buffer[4 * i + 1] = float_to_uchar_color(buffer[4 * i + 1]);
        byte_buffer[4 * i + 2] = float_to_uchar_color(buffer[4 * i + 0]);
        byte_buffer[4 * i + 3] = float_to_uchar_color(buffer[4 * i + 3]);
    }

    QImage image = QImage(byte_buffer.data(), width, height, QImage::Format_ARGB32).flipped();

    return image;
}
void TerrainRaytracingWidget::SendRenderResolution(int width, int height) {
    render_res_x = width;
    render_res_y = height;
}

void TerrainRaytracingWidget::keyPressEvent(QKeyEvent* e) {

    static bool is_showing = false;

    if (e->key() == Qt::Key_R) {
        is_showing = !is_showing;
        glUseProgram(shaderProgram);
        glUniform1i(33, int(is_showing));
        glUseProgram(0);
    }
    else {
        RenderWidget::keyPressEvent(e);
    }
}
