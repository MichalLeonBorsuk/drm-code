#include "waterfallwidget.h"
#include <QPainter>
#include <../matlib/MatlibStdToolbox.h>
#include "DRMPlot.h"

WaterfallWidget::WaterfallWidget(QWidget *parent) :
    QWidget(parent),Image(),Canvas()
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setCursor(Qt::CrossCursor); show();
}

void WaterfallWidget::paintEvent(QPaintEvent *)
{
    QPainter Painter(this);
    Painter.drawPixmap(0, 0, Canvas);
}

void WaterfallWidget::resizeEvent(QResizeEvent *e)
{
    QPixmap tmpPixmap;
    /* If only the height has changed, then copy the content
       to the newly allocated Canvas */
    if(e->oldSize().height() == e->size().height())
    {
        tmpPixmap = QPixmap(Canvas);
    }

    /* Allocate the new canvas */
    Canvas = QPixmap(e->size());


    const QColor& backgroundColor = palette().color(QPalette::Window);
    Canvas.fill(backgroundColor);

    Image = QImage(e->size().width(), 1, QImage::Format_RGB32);

    if(e->oldSize().height() == e->size().height())
    {
        QPainter p(&Canvas);
        p.drawPixmap(0, 0, e->size().width(), e->size().height(),
                     tmpPixmap, 0, 0, e->size().width(), e->size().height());
    }
}

void WaterfallWidget::updatePlot(CVector<_REAL>& vecrData)
{
    /* Resample input data */
    CVector<_REAL> *pvecrResampledData;
    Resample.Resample(&vecrData, &pvecrResampledData, Canvas.size().width(), TRUE);

    /* The scaling factor */
    const _REAL rScale = _REAL(pvecrResampledData->Size()) /  Canvas.size().width();

    /* Actual waterfall data */
    QColor color;
    QRgb *imageData = (QRgb*)Image.bits();
    for (int i = 0; i <  Canvas.size().width(); i++)
    {
        /* Init some constants */
        const int iMaxHue = 359; /* Range of "Hue" is 0-359 */
        const int iMaxSat = 255; /* Range of saturation is 0-255 */

        /* Stretch width to entire canvas width */
        const int iCurIdx =
            (int) Round(_REAL(i) * rScale);

        /* Translate dB-values in colors */
        const int iCurCol =
            (int) Round(((*pvecrResampledData)[iCurIdx] - MIN_VAL_INP_SPEC_Y_AXIS_DB) /
            (MAX_VAL_INP_SPEC_Y_AXIS_DB - MIN_VAL_INP_SPEC_Y_AXIS_DB) *
            iMaxHue);

        /* Reverse colors and add some offset (to make it look a bit nicer) */
        const int iColOffset = 60;
        int iFinalCol = iMaxHue - iColOffset - iCurCol;
        if (iFinalCol < 0) /* Prevent from out-of-range */
            iFinalCol = 0;

        /* Also change saturation to get dark colors when low level */
        int iCurSat = (int) ((1 - (_REAL) iFinalCol / iMaxHue) * iMaxSat);
        if (iCurSat < 0) /* Prevent from out-of-range */
            iCurSat = 0;

        /* Generate pixel */
        color.setHsv(iFinalCol, iCurSat, iCurSat);
        imageData[i] = color.rgb();
    }

    /* Scroll Canvas */
    Canvas.scroll(0, 1, 0, 0, Canvas.size().width(), Canvas.size().height(), 0);

    /* Paint new line (top line) */
    QPainter Painter;
    if (Painter.begin(&Canvas)) {
        /* Draw pixel line to canvas */
        Painter.drawImage(0, 0, Image);

        Painter.end();

        repaint();
    }
}
