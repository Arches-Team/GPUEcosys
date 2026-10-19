#pragma once


#include <QtCharts/QtCharts>
#include <QtCharts/QLineSeries>
#include "evector.h"


class DrawChart {
private:
    QSize display_window_size = QSize(1600, 1000);
    QChart* chart = nullptr;
    QChartView* chartView = nullptr;
    QVector<QAbstractSeries*> series_list;
    QAbstractAxis* axis_X = nullptr;
    bool logX = false;
    bool logY = false;

    // plot_type
    // 0 : undefined
    // 1 : line or scatter
    // 2 : bars
    int plot_type = 0;
    // bar_type
    bool fltx = false;

    // Axis Attributes
    Vector2 range_X;
    Vector2 range_Y;
    int nb_tick_X = 10;
    int nb_tick_Y = 5;
    QString title_X = "";
    QString title_Y = "";

public:
    DrawChart(QString title = "DrawChart");

    void AddLine(const QVector<double>& values);                                          // QLineSeries
    void AddLine(const QVector<int>& values);                                             // QLineSeries
    void AddLine(const QVector<double>& x, const QVector<double>& y, const QColor& color = QColor(), QString name = "");  // QLineSeries
    void AddScatter(const QVector<double>& values);                                       // QScatterSeries
    void AddScatter(const QVector<int>& values);                                          // QScatterSeries
    void AddScatter(const QVector<double>& x, const QVector<double>& y);                  // QScatterSeries
    void AddBars(const QVector<int>& values);                                             // QBarSeries
    void AddBars(const QVector<int>& values, double min, double max);                     // QBarSeries

    void SetTitleX(QString title) { title_X = title; };
    void SetTitleY(QString title) { title_Y = title; };
    void SetNbTickX(int n) { nb_tick_X = n; };
    void SetNbTickY(int n) { nb_tick_Y = n; };
    void SetRangeX(double a, double b) { range_X = Vector2(a, b); };
    void SetRangeY(double a, double b) { range_Y = Vector2(a, b); };
    void SetLog(bool x, bool y) { logX = x; logY = y; }

    void Display();
    void ExportPNG(QString filename);

private:
    void SetUpChartView();

    void UpdateRanges(Vector2 new_range_X, Vector2 new_range_Y);
    void AddAxis();
};
