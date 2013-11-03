#ifndef WATERFALLWIDGET_H
#define WATERFALLWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QPixmap>
#include <QRect>
#include <../util/Vector.h>

class WaterfallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaterfallWidget(QWidget *parent = 0);

signals:

public slots:
    void     updatePlot(CVector<_REAL>& vecrData, _REAL min, _REAL max);

protected:
    QPixmap	 Canvas;
    void     paintEvent(QPaintEvent *);
};

#endif // WATERFALLWIDGET_H
