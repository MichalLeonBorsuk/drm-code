#ifndef WATERFALLWIDGET_H
#define WATERFALLWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPixmap>
#include <QRect>
#include <../util/Vector.h>
#include <../resample/Resample.h>

class SimpleWaterfallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SimpleWaterfallWidget(QWidget *parent = 0);

signals:

public slots:
    void     updatePlot(vector<_REAL>& vecrData, _REAL min, _REAL max);

protected:
    QPixmap	 Canvas;
    void     paintEvent(QPaintEvent *);
    void     resizeEvent(QResizeEvent *);
};

class FasterWaterfallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FasterWaterfallWidget(QWidget *parent = 0);

signals:

public slots:
    void     updatePlot(vector<_REAL>& vecrData, _REAL min, _REAL max);

protected:
    QPixmap	 Canvas;
    void     paintEvent(QPaintEvent *);
    void     resizeEvent(QResizeEvent *);
};

class FastestWaterfallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FastestWaterfallWidget(QWidget *parent = 0);

signals:

public slots:
    void     updatePlot(CVector<_REAL>& vecrData, _REAL min, _REAL max);

protected:
    QPixmap	 Canvas;
    CSpectrumResample resample;
    void     paintEvent(QPaintEvent *);
    void     resizeEvent(QResizeEvent *);
};

typedef FastestWaterfallWidget WaterfallWidget;

#endif // WATERFALLWIDGET_H
