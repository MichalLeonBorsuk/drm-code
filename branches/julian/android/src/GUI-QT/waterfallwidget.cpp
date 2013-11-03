#include "waterfallwidget.h"
#include <QPainter>
#include <../matlib/MatlibStdToolbox.h>

WaterfallWidget::WaterfallWidget(QWidget *parent) :
    QWidget(parent),Canvas()
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setCursor(Qt::CrossCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void WaterfallWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    // let the painter scale the data to the window
    painter.setWindow(Canvas.rect());
    painter.drawPixmap(0, 0, Canvas);
}

void WaterfallWidget::resizeEvent(QResizeEvent *e)
{
    if(e->size().height() != e->oldSize().height())
    {
        // increase/decrease the vertical history
        QPixmap tmp = Canvas;

        Canvas = QPixmap(Canvas.width(), e->size().height());
        Canvas.fill(QColor::fromRgb(0, 0, 0));

        QRect r(QPoint(0,0), tmp.size());

        QPainter p(&Canvas);
        p.drawPixmap(r, tmp, r);
    }
}

void WaterfallWidget::updatePlot(CVector<_REAL>& vecrData, _REAL min, _REAL max)
{
    /* Init some constants */
    const int iMaxHue = 359; /* Range of "Hue" is 0-359 */
    const int iMaxSat = 255; /* Range of saturation is 0-255 */

    if(vecrData.Size() != Canvas.width())
    {
        // size the canvas to the data
        Canvas = QPixmap(vecrData.Size(), height());

        // always use a black background
        Canvas.fill(QColor::fromRgb(0, 0, 0));
    }

    /* Scroll Canvas */
    Canvas.scroll(0, 1, Canvas.rect());

    QPainter painter;
    painter.begin(&Canvas);

    /* Actual waterfall data */
    for (int i = 0; i <  vecrData.Size(); i++)
    {
        /* Translate dB-values to colors */
        _REAL norm = (vecrData[i] - min) / (max - min);
        int hue = iMaxHue - 60 - int(Round(norm * iMaxHue));

        /* Prevent out-of-range */
        if(hue>iMaxHue) hue = iMaxHue;
        if (hue < 0) hue = 0;

        /* Also change saturation to get dark colors when low level */
        int sat = int((1 - double(hue) / iMaxHue) * iMaxSat);
        /* Prevent out-of-range */
        if (sat > iMaxSat) sat = iMaxSat;
        if (sat < 0) sat = 0;

        /* Generate pixel */
        painter.setPen(QColor::fromHsv(hue, sat, sat));
        painter.drawPoint(i, 0);
    }

    painter.end();
    update();
}
