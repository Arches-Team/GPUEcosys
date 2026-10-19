#include "gpu-texture.h"

namespace
{
    bool _HasSingleChannel(GLenum format)
    {
        switch (format)
        {
        // Single color channel formats
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_DEPTH_COMPONENT:
        case GL_STENCIL_INDEX:
        case GL_RED_INTEGER:
            return true;

        default:
            return false;
        }
    }
}

bool GLTexture2D::HasSingleChannel(GLenum format) const
{
    return _HasSingleChannel(format);
}

bool GLTexture2DArray::HasSingleChannel(GLenum format) const
{
    return _HasSingleChannel(format);
}

size_t GLTexture2D::GetMemoryUsed() const
{
    size_t bytes;
    switch (internalFormat) {
    case GL_R8UI:
        bytes = 1;
        break;
    case GL_R16F:
        bytes = 2;
        break;
    case GL_RGBA8:
    case GL_R32F:
    case GL_R32I:
        bytes = 4;
        break;
    case GL_RG32F:
        bytes = 8;
        break;
    default:
        bytes = 0;
        qDebug() << "Error, unknown texture format !";
        return 0;
    }
    return width * height * channels * bytes;
}

size_t GLTexture2DArray::GetMemoryUsed() const
{
    size_t bytes;
    switch (internalFormat) {
    case GL_R16F:
        bytes = 2;
        break;
    case GL_R32F:
        bytes = 4;
        break;
    default:
        bytes = 0;
        qDebug() << "Error, unknown texture format !";
        return 0;
    }
    return width * height * count * bytes;
}

void GLTexture2D::Generate(uint w, uint h, GLenum f, uint c)
{
    width = w;
    height = h;
    channels = c;
    internalFormat = f;

    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, internalFormat, width, height);
    glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
}

void GLTexture2D::Destroy()
{
    glDeleteTextures(1, &texture);
}

/*!
 * \brief Send the data to the GPU
 * 
 * \param format The format of the pixel data (for example GL_RGBA)
 * \param type The type of the pixel data (for example GL_UNSIGNED_BYTE)
 * \param data The pixel data
*/
void GLTexture2D::SetData(GLenum format, GLenum type, void* data)
{
    assert(texture != 0);
    glTextureSubImage2D(texture, 0, 0, 0, width, height, format, type, data);
}

void GLTexture2D::SetData(GLenum format, GLenum type, const ScalarField2& data)
{
    if (!HasSingleChannel(format)) {
        qDebug() << "Error, cannot fit several channels in a ScalarField2...";
    }
    if (type == GL_INT) {
        std::vector<int> d(data.VertexSize());
        for (int i = 0; i < data.VertexSize(); ++i)
            d[i] = data.at(i);
        SetData(format, type, d.data());
    }
    else if (type == GL_UNSIGNED_INT) {
        std::vector<unsigned int> d(data.VertexSize());
        for (int i = 0; i < data.VertexSize(); ++i)
            d[i] = data.at(i);
        SetData(format, type, d.data());
    }
    else
        SetData(format, type, &(data.GetAsFloats()[0]));
}

void GLTexture2D::Bind(uint unit, GLenum access)
{
    assert(texture != 0);
    glBindImageTexture(unit, texture, 0, GL_TRUE, 0, access, internalFormat);
}


/*!
 * \brief Retrieve data from the GPU
 * 
 * \param format The format of the pixel data (for example GL_RGBA)
 * \param type The type of the pixel data (for example GL_UNSIGNED_BYTE)
 * \param size The size of the buffer (in bits)
 * \param data The buffer
*/
void GLTexture2D::GetData(GLenum format, GLenum type, int size, void* data) const
{
    assert(texture != 0);
    glGetTextureSubImage(texture, 0, 0, 0, 0, width, height, 1, format, type, size, data);
}




void GLTexture2DArray::Generate(uint w, uint h, uint c, GLenum f)
{
    width = w;
    height = h;
    count = c;
    internalFormat = f;

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &texture);
    glTextureStorage3D(texture, 1, internalFormat, width, height, count);
    glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
}

void GLTexture2DArray::Destroy()
{
    assert(texture != 0);
    glDeleteTextures(1, &texture);
}

/*!
 * \brief Send the data to the GPU
 * 
 * \param format The format of the pixel data (for example GL_RGBA)
 * \param type The type of the pixel data (for example GL_UNSIGNED_BYTE)
 * \param i The index of the image sent
 * \param data The pixel data
*/
void GLTexture2DArray::SetData(GLenum format, GLenum type, int i, void* data)
{
    assert(texture != 0);
    glTextureSubImage3D(texture, 0, 0, 0, i, width, height, 1, format, type, data);
}

void GLTexture2DArray::SetData(GLenum format, GLenum type, int i, const QPoint& a, const QPoint& b, void* data)
{
    assert(texture != 0);
    glTextureSubImage3D(texture, 0, a.x(), a.y(), i, b.x(), b.y(), 1, format, type, data);
}

void GLTexture2DArray::SetData(GLenum format, GLenum type, int i, const ScalarField2& data)
{
    if (!HasSingleChannel(format)) {
        qDebug() << "Error, cannot fit several channels in a ScalarField2...";
    }
    std::vector<float> vec(data.VertexSize());
    for (int j = 0; j < data.VertexSize(); ++j)
    {
        vec[j] = data.at(j);
    }
    SetData(format, type, i, vec.data());
}

void GLTexture2DArray::SetData(GLenum format, GLenum type, int i, const QPoint& a, const QPoint& b, const ScalarField2& data)
{
    if (!HasSingleChannel(format)) {
        qDebug() << "Error, cannot fit several channels in a ScalarField2...";
    }
    std::vector<float> vec(data.VertexSize());
    for (int j = 0; j < data.VertexSize(); ++j)
    {
        vec[j] = data.at(j);
    }
    SetData(format, type, i, a, b, vec.data());
}

void GLTexture2DArray::Bind(uint unit, GLenum access)
{
    assert(texture != 0);
    glBindImageTexture(unit, texture, 0, GL_TRUE, 0, access, internalFormat);
}


/*!
 * \brief Retrieve data from the GPU
 * 
 * \param format The format of the pixel data (for example GL_RGBA)
 * \param type The type of the pixel data (for example GL_UNSIGNED_BYTE)
 * \param i The index of teh image retreived
 * \param size The size of the buffer (in bits)
 * \param data The buffer
*/
void GLTexture2DArray::GetData(GLenum format, GLenum type, int i, int size, void* data) const
{
    assert(texture != 0);
    glGetTextureSubImage(texture, 0, 0, 0, i, width, height, 1, format, type, size, data);
}
