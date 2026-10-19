#pragma once

#include "heightfield.h"

class HeightFieldShader
{
protected:
    const HeightField& heightfield;
public:
    explicit HeightFieldShader(const HeightField&);
    //! Empty
    ~HeightFieldShader();

    QImage ShadedRelief() const;
    QImage Eric() const;
};

