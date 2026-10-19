#include "mayarender.h"

#include "cpu.h"
#include <QtGui/QMatrix4x4>

/*!
\brief Build a simple renderer.
*/
MayaSimpleRenderer::MayaSimpleRenderer() :lineType(lineType), color(color)
{
    program.Initialize(QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders/simple.glsl").toLocal8Bit());
    QVector<Vector> lines;
    lines.append(Vector(0., 0., 0.));
    lines.append(Vector::Z);
    Color color(1., 0., 0.);

    initSizeLines = lines.size();
    currentSizeLines = lines.size();

    //Build the VBO and map it
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferStorage(GL_ARRAY_BUFFER, lines.size() * sizeof(VectorFloat), nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
    map = (VectorFloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, lines.size() * sizeof(VectorFloat), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

    //Upload data and signal OpenGL
    for (int i = 0; i < lines.size(); i++)
    {
        map[i] = VectorFloat(lines[i][0], lines[i][1], lines[i][2]);
    }

    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

    //Create the VAO and enable the position attrib.
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VectorFloat), (void*)offsetof(VectorFloat, x));
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*!
\brief Build a simple renderer.
\param lines All points on the line(s).
\param color Color of the lines.
\param lineType Type of line to draw.
*/
MayaSimpleRenderer::MayaSimpleRenderer(const QVector<Vector>& lines, const Color& color, GLenum lineType) : lineType(lineType), color(color)
{
    program.Initialize(QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders/simple.glsl").toLocal8Bit());

    initSizeLines = lines.size();
    currentSizeLines = lines.size();

    //Build the VBO and map it
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferStorage(GL_ARRAY_BUFFER, lines.size() * sizeof(VectorFloat), nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
    map = (VectorFloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, lines.size() * sizeof(VectorFloat), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

    //Upload data and signal OpenGL
    for (int i = 0; i < lines.size(); i++)
    {
        map[i] = VectorFloat(lines[i]);
    }

    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

    //Create the VAO and enable the position attrib.
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VectorFloat), (void*)offsetof(VectorFloat, x));
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*!
\brief Build a simple renderer.
\param lines All points on the line(s).
\param n Number of points.
\param color Color of the lines.
\param lineType Type of line to draw.
*/
MayaSimpleRenderer::MayaSimpleRenderer(Vector* lines, int n, const Color& color, GLenum lineType) : lineType(lineType), color(color)
{
    program.Initialize(QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders/simple.glsl").toLocal8Bit());

    initSizeLines = n;
    currentSizeLines = n;

    //Build the VBO and map it
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferStorage(GL_ARRAY_BUFFER, n * sizeof(VectorFloat), nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
    map = (VectorFloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, n * sizeof(VectorFloat), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

    //Upload data and signal OpenGL
    for (int i = 0; i < n; i++)
    {
        map[i] = VectorFloat(lines[i]);
    }

    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

    //Create the VAO and enable the position attrib.
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VectorFloat), (void*)offsetof(VectorFloat, x));
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*!
* \brief Set the camera.
* \param c Camera to set.
*/
void MayaSimpleRenderer::SetCamera(const Camera& c, int width, int height)
{
    program.Bind();

    // Set the camera
    QMatrix4x4 viewMatrix = QMatrix4x4();
    viewMatrix.lookAt(QVector3D(c.Eye()[0], c.Eye()[1], c.Eye()[2]), QVector3D(c.At()[0], c.At()[1], c.At()[2]), QVector3D(c.Up()[0], c.Up()[1], c.Up()[2]));

    QMatrix4x4 projMatrix;
    projMatrix.perspective(
        qRadiansToDegrees(c.GetAngleOfViewV(width, height)), // Qt attend des degres
        static_cast<float>(width) / static_cast<float>(height),
        c.GetNear(),
        c.GetFar()
    );

    // Set the uniforms
    glUniformMatrix4fv(0, 1, GL_FALSE, viewMatrix.constData());
    glUniformMatrix4fv(1, 1, GL_FALSE, projMatrix.constData());

    glUseProgram(0);
}

/*!
\brief Draw the renderer.
*/
void MayaSimpleRenderer::Draw()
{
    if (depthTest)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }
    else
        glDisable(GL_DEPTH_TEST);

    glLineWidth(lineWidth);

    //Bind program and VAO
    program.Bind();
    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VectorFloat), (void*)offsetof(VectorFloat, x));

    //Update color and draw
    glUniform4f(2, color[0], color[1], color[2], color[3]);

    glDrawArrays(lineType, 0, currentSizeLines);
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}

/*!
\brief Delete all buffers
*/
void MayaSimpleRenderer::DeleteBuffers()
{
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO); VBO = 0;
    }
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO); VAO = 0;
    }

    program.Destroy();
}
