#include "qte.h"
#include "triangle.h"

#include <QtCore/QThread>
#include <QtGui/QImageReader>
#include <omp.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    QImageReader::setAllocationLimit(2048);

    QteWindow w;
    w.showMaximized();
    w.show();

    return app.exec();
}
