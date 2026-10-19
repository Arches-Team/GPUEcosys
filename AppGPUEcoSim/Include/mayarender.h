#pragma once

#include <QtCore/QtCore>
#include "evector.h"
#include "camera.h"
#include "gpu-wrapper.h"
#include "color.h"

#include "evectorfloat.h"

class MayaSimpleRenderer
{
protected:
    GLShader program; //!< Program used to draw line.
    GLuint VBO; //!< Where we store our vertex.
    GLuint VAO; //!< VAO.
    Color color = Color::White; //!< Color of our lines.
    VectorFloat* map = nullptr; //!< CPU map of our VBO.

    GLuint initSizeLines; //!< Initial size of our lines vector.
    GLuint currentSizeLines; //!< How many vertex there is in our VBO.

    GLenum lineType; //!< type of line to draw: lines or line loops.

    bool depthTest = true;
    float lineWidth = 1.0; //!< Width.
public:
    MayaSimpleRenderer();
    MayaSimpleRenderer(const QVector<Vector>&, const Color&, GLenum);
    MayaSimpleRenderer(Vector*, int, const Color&, GLenum);

    void DeleteBuffers();
    void Draw();
    void SetCamera(const Camera& c, int width, int height);

    void setDepthTest(bool d) { depthTest = d; }
    void setLineWidth(float l) { lineWidth = l; }
};

