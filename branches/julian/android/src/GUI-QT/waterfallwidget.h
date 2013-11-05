#ifndef WATERFALLWIDGET_H
#define WATERFALLWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPixmap>
#include <QRect>
#include <vector>
#include <../GlobalDefinitions.h>

class SimpleWaterfallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SimpleWaterfallWidget(QWidget *parent = 0);

signals:

public slots:
    void     updatePlot(const std::vector<_REAL>& vec, _REAL min, _REAL max);

protected:
    QPixmap	 Canvas;
    void     paintEvent(QPaintEvent *);
    void     resizeEvent(QResizeEvent *);
};

class WaterfallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaterfallWidget(QWidget *parent = 0);

signals:

public slots:
    void     updatePlot(const std::vector<_REAL>& vec, _REAL min, _REAL max);

protected:
    QPixmap	 Canvas;
    QImage image;
    void     paintEvent(QPaintEvent *);
    void     resizeEvent(QResizeEvent *);
};

#endif // WATERFALLWIDGET_H
