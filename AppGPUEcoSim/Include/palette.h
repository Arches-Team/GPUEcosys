// Palettes

#pragma once

#include "color.h"
#include "box.h"

class QImage;
class QGraphicsScene;

class GenericPalette {
public:
    virtual Color GetColor(double) const;
    // Draw
    virtual QImage Draw(int = 32, int = 1024) const;
    virtual QImage CreateImage(int, int = 16, bool = false) const;

    virtual void Draw(QGraphicsScene&, const Box2&) const;
};


class AnalyticPalette : public GenericPalette {
protected:
    int n = 0; //!< Palette identifier.
    bool r = false; //!< Reverse flag.
public:
    explicit AnalyticPalette(int = 0, bool = false);
    virtual Color GetColor(double) const;

public:
    static Color BrownGreyGreen(double);
    static Color GreenBrownGrey(double);
    static Color CoolWarm(double);
    static Color MatlabJet(double);
    static Color BlueGreen(double);
    static Color WhiteRed(double);
    static Color BlueGreyBrown(double);
    static Color GeologyGreenYellow(double);
    static Color GeologyGreenYellow2(double);
    static Color BrownGreen(double);
    static Color WhiteBlue(double);
    static Color WhiteBrown(double);
    static Color GreenOrange(double);

    AnalyticPalette Reversed() const;
private:
    static Color Diverging(const Color&, const Color&, const Color&, double);
    double Reverse(double) const;
};

/*!
\brief Return the reversed palette.
*/
inline AnalyticPalette AnalyticPalette::Reversed() const
{
    return AnalyticPalette(n, !r);
}


