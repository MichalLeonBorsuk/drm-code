#include "waterfallwidget.h"
#include <QPainter>
#include <../matlib/MatlibStdToolbox.h>
#include <algorithm>

/* Init some constants */
static const int maxHue = 359; /* Range of "Hue" is 0-359 */
static const int maxSat = 255; /* Range of saturation is 0-255 */

static QColor fromReal(_REAL val)
{
    int hue = maxHue - 60 - int(Round(val * maxHue));

    /* Prevent out-of-range */
    if(hue>maxHue) hue = maxHue;
    if (hue < 0) hue = 0;

    /* Also change saturation to get dark colors when low level */
    int sat = int((1 - double(hue) / maxHue) * maxSat);
    /* Prevent out-of-range */
    if (sat > maxSat) sat = maxSat;
    if (sat < 0) sat = 0;

    return QColor::fromHsv(hue, sat, sat);
}

SimpleWaterfallWidget::SimpleWaterfallWidget(QWidget *parent) :
    QWidget(parent),Canvas()
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setCursor(Qt::CrossCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SimpleWaterfallWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    // let the painter scale the data to the window
    painter.setWindow(Canvas.rect());
    painter.drawPixmap(0, 0, Canvas);
}

void SimpleWaterfallWidget::resizeEvent(QResizeEvent *e)
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

void SimpleWaterfallWidget::updatePlot(const vector<_REAL>& vec, _REAL min, _REAL max)
{
    if(int(vec.size()) != Canvas.width())
    {
        // size the canvas to the data
        Canvas = QPixmap(vec.size(), height());

        // always use a black background
        Canvas.fill(QColor::fromRgb(0, 0, 0));
    }

    /* Scroll Canvas */
    Canvas.scroll(0, 1, Canvas.rect());

    QPainter painter;
    painter.begin(&Canvas);

    /* Actual waterfall data */
    for (size_t i = 0; i <  vec.size(); i++)
    {
        /* Translate dB-values to colors */
        _REAL norm = (vec[i] - min) / (max - min);

        /* Generate pixel */
        painter.setPen(fromReal(norm));
        painter.drawPoint(i, 0);
    }

    painter.end();
    update();
}

WaterfallWidget::WaterfallWidget(QWidget *parent) :
    QWidget(parent),Canvas(),image()
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setCursor(Qt::CrossCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void WaterfallWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, Canvas);
}

void WaterfallWidget::resizeEvent(QResizeEvent *e)
{
    QPixmap tmp = Canvas;
    Canvas = QPixmap(e->size());
    image = QImage( e->size().width(), 1, QImage::Format_RGB32);

    // first time ever - initialise to black
    if(tmp.size().width()==0)
    {
        tmp = QPixmap(e->size());
        // always use a black background
        tmp.fill(QColor::fromRgb(0, 0, 0));
    }
    // vertical stretch - fill below old with black
    if(Canvas.height()>tmp.height())
        Canvas.fill(QColor::fromRgb(0, 0, 0));

    // copy old data, scaling to fit horizontally but making space below
    // TODO - the width scaling is slightly off.
    QPainter p(&Canvas);
    QPoint origin(0,0);
    QSize top(Canvas.width(), tmp.height());
        p.drawPixmap(QRect(origin, top), tmp, QRect(origin, tmp.size()));
}

void WaterfallWidget::updatePlot(const vector<_REAL>& vec, _REAL min, _REAL max)
{
    int w = image.width();
    /* Scroll Canvas */
    Canvas.scroll(0, 1, Canvas.rect());

    size_t interval = int(floor(_REAL(vec.size())/_REAL(w)));
    if(interval==0)
    {
        interval = 1; // TODO handle stretching
    }
    vector<_REAL>::const_iterator first = vec.begin();
    for(int i=0; i<w; i++)
    {
        vector<_REAL>::const_iterator last = first+interval;
        vector<_REAL>::const_iterator biggest = max_element(first, last);
        // reduce dB value by 6 dB to avoid FSD and normalise
        _REAL norm = ((*biggest) - 6.0 - min) / (max - min);
        // Translate normalised value to a color and generate pixel
        ((QRgb*)image.scanLine(0))[i] = fromReal(norm).rgb();
        first=last;
    }
    QPainter painter(&Canvas);
    painter.drawImage(0, 0, image);
    update();
}
