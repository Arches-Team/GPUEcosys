#pragma once

#include "gpu-shader.h"
#include "scalarfield.h"
#include "QtCore/qdebug.h"

// OpenGL Storage Texture wrapper
class GLTexture2D
{
public:
    inline GLTexture2D()
        : texture(0), width(0), height(0), channels(0), internalFormat(GL_RGBA8) {
    }
    inline ~GLTexture2D() {}

    GLuint GetID() { return texture; }
    size_t GetMemoryUsed() const;

    void Generate(uint width, uint height, GLenum f, uint channels = 1);
    void Destroy();

    void SetData(GLenum format, GLenum type, void* data);
    void SetData(GLenum format, GLenum type, const ScalarField2& data);
    void Bind(uint unit, GLenum access);

    void GetData(GLenum format, GLenum type, int size, void* data) const;

    template <typename T>
    void GetData(GLenum format, GLenum type, std::vector<T>& data) const
    {
        GetData(format, type, int(data.size()) * sizeof(T), data.data());
    }

    template <typename T>
    void GetData(GLenum format, GLenum type, QVector<T>& data) const
    {
        GetData(format, type, int(data.size()) * sizeof(T), data.data());
    }

    /*!
     * \brief Get the image data from the GPU and outputs it in a ScalarField2
    */
    template <typename T>
    ScalarField2 CreateScalarField(GLenum format, GLenum type) const
    {
        if (!HasSingleChannel(format)) {
            qDebug() << "Error, cannot fit several channels in a ScalarField2...";
            return ScalarField2();
        }
        int size = width * height;
        std::vector<T> vec(size);
        GetData(format, type, vec);

        ScalarField2 sf(Box2(), width, height);
        for (int i = 0; i < size; ++i)
            sf[i] = vec[i];
        return sf;
    }

private:
    bool HasSingleChannel(GLenum format) const;

private:
    GLuint texture = 0; //!< Texture identifier.
    GLenum internalFormat= GL_RGBA8; //!< Internal format for the texture.
    uint width = 0, height = 0, channels = 0; //!< Width, height and number of channels of the texture.
};

// OpenGL Storage Texture Array
// (for Compute Shaders)
class GLTexture2DArray
{
public:
    inline GLTexture2DArray()    : texture(0), width(0), height(0), count(0), internalFormat(GL_RGBA8) {  }
    inline GLTexture2DArray(GLuint t) : texture(t) {}
    inline ~GLTexture2DArray() {}

    void Generate(uint width, uint height, uint count, GLenum f);
    void Destroy();

    size_t GetMemoryUsed() const;

    void SetData(GLenum format, GLenum type, int i, void* data);
    void SetData(GLenum format, GLenum type, int i, const ScalarField2& data);

    /*!
     * \brief Set parts of the image
     * \param a, b Pixel coordinates of the image subset
    */
    void SetData(GLenum format, GLenum type, int i, const QPoint& a, const QPoint& b, void* data);

    /*!
     * \brief Set parts of the image
     * \param a, b Pixel coordinates of the image subset
    */
    void SetData(GLenum format, GLenum type, int i, const QPoint& a, const QPoint& b, const ScalarField2& data);

    void Bind(uint unit, GLenum access);

    void GetData(GLenum format, GLenum type, int i, int size, void* data) const;

    template <typename T>
    void GetData(GLenum format, GLenum type, int i, std::vector<T>& data) const
    {
        GetData(format, type, i, int(data.size()) * sizeof(T), data.data());
    }

    template <typename T>
    void GetData(GLenum format, GLenum type, int i, QVector<T>& data) const
    {
        GetData(format, type, i, int(data.size()) * sizeof(T), data.data());
    }

    template <typename T>
    ScalarField2 CreateScalarField(GLenum format, GLenum type, int i) const
    {
        if (!HasSingleChannel(format)) {
            qDebug() << "Error, cannot fit several channels in a ScalarField2...";
            return ScalarField2();
        }
        int size = width * height;
        std::vector<T> vec(size);
        GetData(format, type, i, vec);

        ScalarField2 sf(Box2(), width, height);
        for (int i = 0; i < size; ++i)
            sf[i] = vec[i];
        return sf;
    }

private:
    bool HasSingleChannel(GLenum format) const;

private:
    GLuint texture = 0;
    GLenum internalFormat;
    uint width, height;
    uint count;
};
