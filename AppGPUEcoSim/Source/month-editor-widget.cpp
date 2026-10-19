#include "qte.h"
#include <QtWidgets/QFileDialog>
#include <QtGui/QMouseEvent>

/*!
\class MayaGraphWidget maya.h
\brief This class implemements a simple widget for controlling 2D functions.
*/

/*!
\brief Creates a generic 2D function widget.
The corresponding 2D function is linear.
*/
MonthsEditorWidget::MonthsEditorWidget(QWidget* parent, QString Ord, MapType t, const QColor& c, double minOrdVal, double maxOrdVal) 
    : QWidget(parent), Ordinate(Ord), color(c), type(t), ordMinValue(minOrdVal), ordMaxValue(maxOrdVal)
{
    n = 12;
    values.resize(n, (maxOrdVal - minOrdVal)/2.);

    x0 = 0;
    y0 = 0;
}

/*!
\brief Destructeur
*/
MonthsEditorWidget::~MonthsEditorWidget()
{
}

/*!
\brief Modification du text Afficher sur l'ordonnee
*/
void MonthsEditorWidget::setTextOrdinate(const QString& text)
{
    Ordinate = text;
    update();
}

/*!
\brief Modification de la valeur maximale sur l'abscisse
*/
void MonthsEditorWidget::setTextMinNumOrdinate(double num)
{
    ordMinValue = num;
    update();
}

/*!
\brief Modification de la valeur maximale sur l'ordonnee
*/
void MonthsEditorWidget::setTextMaxNumOrdinate(double num)
{
    ordMaxValue = num;
    update();
}

/*!
\brief Dessin de l'ensemble du graphique
*/
void MonthsEditorWidget::paintEvent(QPaintEvent*)
{
    // Initialisation des points (Attention, ne pas mettre dans le constructeur
    p1X = 25;
    p1Y = 15;
    p2X = p1X;
    p2Y = height() - 15;
    p3X = width() - 15;
    p3Y = p2Y;

    // Table first letter months
    QString months[12] = { "J", "F", "M", "A", "M", "J", "J", "A", "S", "O", "N", "D" };

    QPainter painter(this);
    // Dessin du fond
    painter.setPen(Qt::white);
    painter.fillRect(p1X, p1Y, p3X - p1X, p2Y - p1Y, QBrush(Qt::white));

    // Dessin de l'abscisse et de l'ordonnee
    painter.setPen(Qt::black);
    painter.drawLine(p1X, p1Y - 4, p2X, p2Y + 2);
    painter.drawLine(p1X, p1Y - 4, p1X - 3, p1Y);
    painter.drawLine(p1X, p1Y - 4, p1X + 3, p1Y);
    painter.drawLine(p2X - 2, p2Y, p3X + 4, p3Y);
    painter.drawLine(p3X + 4, p3Y, p3X, p3Y - 3);
    painter.drawLine(p3X + 4, p3Y, p3X, p3Y + 3);
    // Dessin des traits
    painter.drawLine(p1X - 2, p1Y, p1X + 2, p1Y);
    painter.drawLine(p1X - 2, (p2Y + p1Y) / 2, p1X + 2, (p2Y + p1Y) / 2);
    painter.drawLine(p3X, p3Y - 2, p3X, p3Y + 2);
    painter.drawLine((p2X + p3X) / 2, p3Y - 2, (p2X + p3X) / 2, p3Y + 2);
    painter.drawLine((3 * p2X + p3X) / 4, p3Y - 2, (3 * p2X + p3X) / 4, p3Y + 2);
    painter.drawLine((p2X + p3X * 3) / 4, p3Y - 2, (p2X + p3X * 3) / 4, p3Y + 2);

    paintTerrain(painter);

    // Dessin du texte
    painter.setPen(Qt::black);

    painter.drawText(QRectF(0, p1Y - 10 , p1X - 3, 15), Qt::AlignRight | Qt::AlignTop, QString().setNum(ordMaxValue));
    painter.drawText(QRectF(0, p2Y - 10 , p1X - 3, 15), Qt::AlignRight | Qt::AlignTop, QString().setNum(ordMinValue));
    painter.drawText(QRectF(0, (p1Y+p2Y)/2. - 10, p1X - 3, 15), Qt::AlignRight | Qt::AlignTop, QString().setNum((ordMinValue+ ordMaxValue)/2.));


    painter.setBrush(QBrush(color));

    int x1, x2;
    int n = 12;
    for (int i = 0; i < n; i++)
    {
        x1 = (n * p2X + i * (p3X - p2X)) / n;
        x2 = (n * p1X + (i + 1) * (p3X - p2X)) / n;

        painter.drawText(QRectF(x1, p2Y, x2-x1, 15), Qt::AlignCenter | Qt::AlignTop, months[i]);
    }

    painter.setPen(QColor(60, 60, 60));
    painter.drawText(p1X - 2, p1Y - 5, Ordinate);
}

/*!
\brief Dessin du terrain
*/
void MonthsEditorWidget::paintTerrain(QPainter& p)
{
    int htmpR;

    p.setPen(Qt::black);
    p.setBrush(QBrush(color));

    // Dessin de la roche
    int x1, x2;
    for (int i = 0; i < n; i++)
    {
        x1 = (n * p2X + i * (p3X - p2X)) / n;
        x2 = (n * p1X + (i + 1) * (p3X - p2X)) / n;


        htmpR = values[i] / ordMaxValue * (p2Y - p1Y); // TODO
        p.drawRect(x1, p2Y, x2 - x1, -htmpR);
    }
}


/*!
\brief Action effectuer lors d'un clic de souris
*/
void MonthsEditorWidget::mousePressEvent(QMouseEvent* e)
{

    int n = 12;
    if (n != 0)
    {
        int posX = e->position().x() - p2X;
        int posY = (p2Y - p1Y) - (e->position().y() - p1Y);

        if (posX < 0) posX = 0;
        if (posX >= p3X - p2X) posX = p3X - p2X - 1;

        if (posY < 0) posY = 0;
        if (posY > p2Y - p1Y) posY = p2Y - p1Y;

        int    x = posX * n / (p3X - p2X);
        double y = (double)posY / ((double)p2Y - p1Y) * ordMaxValue;

        values[x] = y;

        x0 = x;
        y0 = y;
        update();

        emit changed(type);
    }
}

/*!
\brief Action effectuer lors d'un mouvement de souris
\param e Event.
*/
void MonthsEditorWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (n != 0)
    {
        int posX = e->position().x() - p2X;
        int posY = (p2Y - p1Y) - (e->position().y() - p1Y);

        if (posX < 0) posX = 0;
        if (posX > p3X - p2X) posX = p3X - p2X;

        if (posY < 0) posY = 0;
        if (posY > p2Y - p1Y) posY = p2Y - p1Y;

        int    x = posX * n / (p3X - p2X);
        double y = (double)posY / ((double)p2Y - p1Y);

        // Intepolation lineaire entre l'ancienne position et la nouvelle position
        for (int i = 0; i < fabs(x0 - x); i++) {
            double a = i / fabs(x0 - x);
            int xt;
            double yt;
            if (x0 < x) {
                xt = x0 + i;
                yt = y0 * (1.0 - a) + a * y;
            }
            else {
                xt = x + i;
                yt = y * (1.0 - a) + a * y0;
            }

            yt *= ordMaxValue;

            values[xt] = yt;
        }

        x0 = x;
        y0 = y;
        update();

        emit changed(type);
    }
}

void MonthsEditorWidget::setColor(const QColor& c) { 
    color = c; 
}

void MonthsEditorWidget::setValues(const QVector<double>& v)
{
    values = v;
    update();
}

void MonthsEditorWidget::addValue(const double value)
{
    for (int i = 0; i < values.size(); i++)
    {
        values[i] += value;
    }
    update(); 
    emit changed(type);
}

/*
* \brief Recuperation des valeurs
*/
QVector<double>& MonthsEditorWidget::getValues()
{
    return values;
}

