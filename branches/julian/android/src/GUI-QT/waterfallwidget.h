#ifndef WATERFALLWIDGET_H
#define WATERFALLWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPixmap>
#include <QRect>
#include <../util/Vector.h>
#include "../resample/Resample.h"

class WaterfallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaterfallWidget(QWidget *parent = 0);

signals:

public slots:
    void     updatePlot(CVector<_REAL>& vecrData);

protected:
    CSpectrumResample	Resample;
    QImage   Image;
    QPixmap	 Canvas;
    void     paintEvent(QPaintEvent *);
    void     resizeEvent(QResizeEvent  *);

};

#endif // WATERFALLWIDGET_H
