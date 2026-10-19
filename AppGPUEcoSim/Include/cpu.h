// Cpu  

#pragma once

#include <QtCore/QString>
#include <QtCore/QElapsedTimer>
#include <QtGui/QPainter>

#include "box.h"

class System
{
protected:
    static bool avx; //!< Advanced Vector Extensions flag.
public:
#ifdef _MSC_VER
#endif
    static QString GetEnv(const QString&);
    static QString GetResource(const QString&, const QString & = QString(""));
    static void BytesToGib(size_t, size_t&, size_t&, size_t&);

    static void MsToMinutes(int, int&, int&, int&);
    static QString Elapsed(const QElapsedTimer&);
    static void ShowElapsed(const QElapsedTimer&);
    static QString DateTime();
    
    static bool Avx();
protected:
};
