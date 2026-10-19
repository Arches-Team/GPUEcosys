#include "gputerrainsimu.h"
#include "cpu.h"

/*!
\class GPUHeightFieldAnalysis gputerrainsimu.h
\brief Parallelized heightfield analysis.
*/

/*!
\brief Constructor.

Initialize everthing to zero.
*/
GPUHeightFieldAnalysis::GPUHeightFieldAnalysis()
{
}

/*!
\brief Destroys OpenGl buffers and shader program.
*/
GPUHeightFieldAnalysis::~GPUHeightFieldAnalysis()
{
    shader.Destroy();
    // inHfBuffer is not destroyed because we are not owner of it
    inHfBufferDouble.Destroy();
    outBuffer.Destroy();
    outVecBuffer.Destroy();
    outIntBuffer.Destroy();
    outDoubleBuffer.Destroy();
    glDeleteTextures(1, &hfTextureBuffer);
}

/*!
\brief Init function from a given heightfield.

Initialize a GPU texture (and not a buffer).

This function is useful when the single precision is not an issue for the calling function.

\param hf %Heightfield.
*/
void GPUHeightFieldAnalysis::InitForTexture(const HeightField& hf)
{
    nx = hf.GetSizeX();
    ny = hf.GetSizeY();
    box = hf.GetBox();
    totalBufferSize = hf.VertexSize();
    dispatchSize = (max(nx, ny) / 8) + 1;

    QString fullPath = QString::fromStdString(std::string(SOLUTION_DIR) + "/AppGPUEcoSim/Shaders/heightfield_analysis.glsl");
    QByteArray ba = fullPath.toLocal8Bit();
    shader.Initialize(ba.data());

    if (hfTextureBuffer == 0)
        glGenTextures(1, &hfTextureBuffer);
    QImage hfTexture = hf.CreateImage(false);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hfTextureBuffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hfTexture.width(), hfTexture.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, hfTexture.bits());

    outBuffer.Generate();
    outBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * totalBufferSize, 0, GL_DYNAMIC_READ);

    outVecBuffer.Generate();
    outVecBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 2 * totalBufferSize, 0, GL_STREAM_READ);

    outIntBuffer.Generate();
    outIntBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 1, 0, GL_STREAM_READ);

    shader.Bind();
    shader.SetUniform("heightfield", 0);
    shader.SetUniform("a", float(box[0][0]), float(box[0][1]), float(box[0][2]));
    shader.SetUniform("b", float(box[1][0]), float(box[1][1]), float(box[1][2]));
    Vector2 cellDiagonal = hf.CellDiagonal();
    shader.SetUniform("cellDiagonal", float(cellDiagonal[0]), float(cellDiagonal[1]));
    shader.SetUniform("nx", nx);
    shader.SetUniform("ny", ny);
    shader.SetUniform("lipschitz", (float)hf.K());
    shader.Unbind();
}

/*!
\brief Compute the fractional Laplacian.
\param hf %Heightfield
\param m Mask size
\param s Fractional parameter, should be in [0,1].
*/
ScalarField2 GPUHeightFieldAnalysis::FractionalLaplacian(const HeightField& hf, int m, double s)
{
    InitForTexture(hf);

    shader.Bind();

    // Uniforms
    shader.SetUniform("func", 1);
    shader.SetUniform("m", m);
    shader.SetUniform("sFrac", (float)s);

    // Buffers
    outBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    outVecBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 1);
    outIntBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 3);

    glDispatchCompute(dispatchSize, dispatchSize, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Temporary float array for OpenGL
    ScalarField2 ret = GetDataToScalarField();

    shader.Unbind();
    return ret;
}

/*!
\brief Compute fractional gradient and fractional gradient norm fields.
\param hf heightfield
\param s Fractional parameter, should be in [0,1].
\param m mask size
\param grad returned fractional gradient field
\param gradNorm returned fractional gradient norm
*/
void GPUHeightFieldAnalysis::FractionalGradient(const HeightField& hf, double s, int m, VectorField2& grad, ScalarField2& gradNorm)
{
    InitForTexture(hf);

    shader.Bind();

    // Uniforms
    shader.SetUniform("func", 2);
    shader.SetUniform("m", m);
    shader.SetUniform("sFrac", (float)s);

    // Buffers
    outBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    outVecBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 1);
    outIntBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 3);

    glDispatchCompute(dispatchSize, dispatchSize, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Get data scalar
    std::vector<float> data;
    data.resize(totalBufferSize);
    outBuffer.GetData(data);
    gradNorm = ScalarField2(Box2(box), nx, ny, 0.0);
    for (int i = 0; i < totalBufferSize; i++)
        gradNorm[i] = double(data[i]);

    // Get data vector 2D
    data.resize(totalBufferSize * 2);
    outVecBuffer.GetData(data);
    grad = VectorField2(Box2(box), nx, ny, Vector2::Null);
    for (int i = 0; i < totalBufferSize; i++)
        grad[i] = Vector2(double(data[i * 2 + 0]), double(data[i * 2 + 1]));

    shader.Unbind();
}

/*!
 \brief Compute accessibility.

 \sa HeightField::Accessbility

 \param hf Heightfield
 \param r Radius.
 \param n Number of rays.
 \param maxSteps Number of maximum steps for sphere tracing
 */
ScalarField2 GPUHeightFieldAnalysis::Accessibility(const HeightField& hf, const double& r, int n, int maxSteps)
{
    InitForTexture(hf);

    shader.Bind();

    // Uniforms
    shader.SetUniform("func", 0);
    shader.SetUniform("r", float(r));
    shader.SetUniform("N", n);
    shader.SetUniform("maxSteps", maxSteps);

    // Buffers
    outBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    outVecBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 1);
    outIntBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 3);

    glDispatchCompute(dispatchSize, dispatchSize, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Temporary float array for OpenGL
    ScalarField2 ret = GetDataToScalarField();

    shader.Unbind();
    return ret;
}

/*!
\brief Get float buffer data from the GPU and convert it to the ScalarField2 double format.
*/
ScalarField2 GPUHeightFieldAnalysis::GetDataToScalarField() const
{
    // Temporary float array for OpenGL
    std::vector<float> data;
    data.resize(totalBufferSize);
    outBuffer.GetData(data);

    ScalarField2 ret = ScalarField2(Box2(box), nx, ny, 0.0);
    for (int i = 0; i < totalBufferSize; i++)
    {
        ret[i] = double(data[i]);
    }
    return ret;
}

/*!
 \brief Computes the visibility of the sun on the terrain
 \param hf The HeightField
 \param sun The sun vector
 \param maxSteps Number of maximum steps for sphere tracing
 */
ScalarField2 GPUHeightFieldAnalysis::Visibility(const HeightField& hf, const Vector& sun, int maxSteps)
{
    InitForTexture(hf);

    shader.Bind();

    // Uniforms
    shader.SetUniform("func", 14);
    shader.SetUniform("light", float(sun[0]), float(sun[1]), float(sun[2]));
    shader.SetUniform("maxSteps", maxSteps);

    // Buffers
    outBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
    outVecBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 1);
    outIntBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 3);

    glDispatchCompute(dispatchSize, dispatchSize, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Temporary float array for OpenGL
    std::vector<float> data;
    data.resize(totalBufferSize);
    outBuffer.GetData(data);

    ScalarField2 ret = ScalarField2(Box2(box), nx, ny, 0.0);
    for (int i = 0; i < totalBufferSize; i++)
        ret[i] = double(data[i]);

    shader.Unbind();
    return ret;
}
