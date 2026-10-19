#include "qte.h"
#include <QtCore/QXmlStreamWriter>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "histogram.h"

void QteWindow::createActionsDisplayExport()
{
    connect(uiw.sapling_nb, SIGNAL(clicked()), this, SLOT(CallEstimateSaplings()));
}

/**
 * @brief Export saplings as Maya instances. The number of saplings is defined by the sapling density in the simulation. The position of the saplings is defined by a disc tile (Poisson disk sampling).
 * The species of the saplings is defined by the relative density of each species in the simulation.
 */
void QteWindow::Call_ExportSaplings()
{
    double s = 2; // 10m tile size
    double r = 0.15; // 1m disc radius
    int n = 1000;  // number of instances max
    DiscTile tile = DiscTile(s, r, n);

    int maxSaplings = tile.Size();
    qDebug() << "Count: " << maxSaplings;

    HeightField heightfield = resourceData.GetHeightField();
    Box2 box = Box2(heightfield.GetBox());

    Random randX = Random::R239;

    int nbSpecies = ecosystemData.size();
    QVector<ScalarField2> saplings(nbSpecies);
    for (int s = 0; s < nbSpecies; ++s)
        saplings[s] = sim->GetSaplingsNb(s);

    Vector2 startPos = box[0];
    Vector2 endPos = box[1];
    int sx, sy, ex, ey;
    resourceData.GetHeightField().VertexInteger(startPos, sx, sy);
    resourceData.GetHeightField().VertexInteger(endPos, ex, ey);

    int cpt = 0;
    for (int i = sx; i <= ex; i++)
    {
        for (int j = sy; j < ey; j++)
        {
            QVector<double> spl(nbSpecies, 0); // Cumulative histogram of saplings in the cell
            int max = 0;
            double total = 0;
            for (int s = 0; s < nbSpecies; ++s)
            {
                int ss = saplings[s](i, j);
                spl[s] = total + ss;
                total += ss;
                if (ss > max)
                    max = ss;
            }
            if (total == 0) continue;
            
            // Histogram normalization
            for (int s = 0; s < nbSpecies; ++s)
                spl[s] = spl[s] / total;

            // Instantiate no more than the nb of samples
            max = Math::Clamp(max, 0, maxSaplings);

            for (int k = 0; k < max; k++)
            {
                Vector2 pos = tile.Vertex(k) + Vector2(s * i, s * j) + startPos;

                if (heightfield.Slope(i, j) < 0.8)
                {
                    // Select the species to instanciate
                    double r = randX.Uniform();
                    int s;
                    for (s = 0; s < nbSpecies; ++s)
                        if (r <= spl[s]) break;

                    if (saplings[s].Value(pos) <= 0) continue;

                    // Do the instanciation here
                    cpt++;
                }
            }
        }
    }

    qDebug() << "Number of saplings (virtually) instanciated : " << cpt;
}

void QteWindow::CallEstimateSaplings()
{
    qDebug() << "Estimated number of saplings (raw): " << sim->GetSaplingsCount();

    Call_ExportSaplings();
}
