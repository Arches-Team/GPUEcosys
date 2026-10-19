#pragma once

#include "gpu-wrapper.h"
#include "heightfield.h"
#include "vectorfield.h"
#include "cpu.h"

class GPUHeightFieldAnalysis 
{
protected:
    GLShader shader;                //!< Glsl program.
    GLuint hfTextureBuffer = 0;     //!< %Heightfield texture.
    GLBuffer inHfBuffer;            //!< %Heightfield buffer, used by CountPits(), with single precision.
    GLBuffer inHfBufferDouble;  //!< %Heightfield buffer, with double precision.
    GLBuffer outBuffer;         //!< Output scalar buffer.
    GLBuffer outVecBuffer;      //!< Output 2D vector buffer.
    GLBuffer outIntBuffer;      //!< Output integer buffer.
    GLBuffer outDoubleBuffer;       //!< Output double buffer.

    Box box; // GUNDY Heriter de Array2 pour eviter de stocker les dimensions du heightfield dans chaque classe 
    int nx = 0, ny = 0;
    int totalBufferSize = 0;
    int dispatchSize = 1;
public:
    GPUHeightFieldAnalysis();
    ~GPUHeightFieldAnalysis();

    ScalarField2 FractionalLaplacian(const HeightField&, int, double);

    void FractionalGradient(const HeightField&, double, int, VectorField2&, ScalarField2&);

    ScalarField2 Accessibility(const HeightField&, const double&, int, int = 2256);


    ScalarField2 Visibility(const HeightField&, const Vector&, int = 2256);

protected:
    void Init(int nx, int ny);
    void InitForTexture(const HeightField&);

protected:
    ScalarField2 GetDataToScalarField() const;
};
