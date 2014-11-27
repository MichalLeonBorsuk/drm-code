#include "cdrmplotqcp.h"
#include <qcustomplot.h>

CDRMPlotQCP::CDRMPlotQCP(QCustomPlot *plot):CDRMPlot(),plot(NULL),wfplot(NULL),wfitem(NULL),
    title(NULL),hlines(),vlines()
{
    SetPlotStyle(0); // TODO - get from settings?
    this->plot = plot;
    title = new QCPPlotTitle(plot);
    QFont f = title->font();
    f.setPointSize(10);
    title->setFont(f);

    QCPLayoutGrid *ll = new QCPLayoutGrid;

    //ll->addElement(0, 0, plot->legend);
    //ll->setMargins(QMargins(20,30,20,60));

    //plot->plotLayout()->addElement(0, 1, ll);
    // TODO position and size legend better
    //plot->plotLayout()->setColumnStretchFactor(0, 4); // plot shall have 80% of width
    //plot->plotLayout()->setColumnStretchFactor(1, 1); // legend only 20% (4:1 = 80:20)
    plot->plotLayout()->insertRow(0); // inserts an empty row above the default axis rect
    plot->plotLayout()->addElement(0, 0, title);

}

CDRMPlotQCP::~CDRMPlotQCP()
{
}

void CDRMPlotQCP::clearPlots()
{
    plot->clearGraphs();
    plot->clearItems();
    plot->clearPlottables();
    //for(int i=0; i<vlines.size(); i++)
    //    delete vlines[i];
    vlines.clear();
    //for(int i=0; i<hlines.size(); i++)
    //    delete hlines[i];
    hlines.clear();
}

void CDRMPlotQCP::replot()
{
    plot->replot();
}

void CDRMPlotQCP::SetPlotStyle(const int iNewStyleID)
{
}

void CDRMPlotQCP::setCaption(const QString& s)
{
}

void CDRMPlotQCP::setIcon(const QIcon& s)
{
}

void CDRMPlotQCP::setGeometry(const QRect& g)
{
}

const QRect CDRMPlotQCP::geometry() const
{
    return QRect();
}

bool CDRMPlotQCP::isVisible() const
{
    return false;
}

void CDRMPlotQCP::close()
{
}

void CDRMPlotQCP::hide()
{
}

void CDRMPlotQCP::show()
{
}

void CDRMPlotQCP::activate()
{

}

void CDRMPlotQCP::deactivate()
{

}

void CDRMPlotQCP::applyColors()
{

}

void CDRMPlotQCP::setupBasicPlot(const char* titleText, const char* xText, const char* yText, const char* legendText,
                                 double left, double right, double bottom, double top)
{
    title->setText(QObject::tr(titleText));
    /* Fixed scale */
    plot->xAxis->setLabel(QObject::tr(xText));
    plot->xAxis->setRange(left, right);

    plot->xAxis2->setVisible(false);

    plot->yAxis->setRange(bottom, top);
    plot->yAxis->setLabel(QObject::tr(yText));

    plot->addGraph();
    plot->graph(0)->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    plot->graph(0)->setName(QObject::tr(legendText));
    plot->graph(0)->addToLegend();

    plot->legend->setVisible(false);
}

void CDRMPlotQCP::add2ndGraph(const char* axisText, const char* legendText, double bottom, double top)
{
    plot->yAxis2->setVisible(true);
    plot->yAxis2->setLabel(QObject::tr(axisText));
    plot->yAxis2->setRange(bottom, top);

    plot->addGraph();

    plot->graph(1)->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    plot->graph(1)->setValueAxis(plot->yAxis2);
    plot->graph(1)->setName(QObject::tr(legendText));
    plot->graph(1)->addToLegend();

    plot->legend->setVisible(true);

}

void CDRMPlotQCP::addxMarker(QColor color, double initialPos)
{
    QCPItemLine* l = new QCPItemLine(plot);
    l->setPen(QPen(color, 1, Qt::DashLine));
    plot->addItem(l);
    l->start->setCoords(initialPos, plot->yAxis->range().lower);
    l->end->setCoords(initialPos, plot->yAxis->range().upper);
    vlines << l;
}

void CDRMPlotQCP::addyMarker(QColor color, double initialPos)
{
    QCPItemLine* l = new QCPItemLine(plot);
    l->setPen(QPen(color, 1, Qt::DashLine));
    plot->addItem(l);
    l->start->setCoords(initialPos, plot->xAxis->range().lower);
    l->end->setCoords(initialPos, plot->xAxis->range().upper);
    hlines << l;
}

void CDRMPlotQCP::setupWaterfall()
{
    title->setText(QObject::tr("Waterfall Spectrum"));
    wfitem = new QCPItemPixmap(plot);
    wfplot = new WaterFallPlot();
    wfplot->resize(plot->axisRect()->size());
    plot->addItem(wfitem);
    plot->legend->setVisible(false);
    plot->xAxis->setLabel(QObject::tr("Frequency [kHz]"));
    plot->yAxis->setVisible(false);
}

void CDRMPlotQCP::setQAMGrid(const ECodScheme eCoSc)
{
    double div;
    int step;
    int substep = 4;
    switch (eCoSc)
    {
    case CS_1_SM: /* QAM4 */
        div = 2.0 / sqrt(2.0);
        step = 1;
        break;

    case CS_2_SM: /* QAM16 */
        div = 4.0 / sqrt(10.0);
        step = 2;
        break;

    default: /* QAM64 */
        div = 8.0 / sqrt(42.0);
        step = 4;
        break;
    }
    int i;
    double pos;
    QVector <double> ticks;
    QVector <double> subticks;

    for (i=0; i<=step*2; i++)
    {
        pos = -div + div / step * i;
        /* Keep 2 digit after the point */
        pos = Round(pos * 100.0) / 100.0;
        ticks << pos;
    }

    substep *= step;
    for (i=0; i<=substep*2; i++)
    {
        pos = -div + div / substep * i;
        /* Keep 2 digit after the point */
        pos = Round(pos * 100.0) / 100.0;
        subticks << pos;
    }
    plot->xAxis->setTickVector(ticks);
    plot->xAxis->setRange(ticks[0], ticks[ticks.size()-1]);
    plot->yAxis->setRange(ticks[0], ticks[ticks.size()-1]);
    plot->yAxis->setTickVector(ticks);
}

void CDRMPlotQCP::setupConst()
{
    plot->yAxis2->setVisible(false);

    plot->xAxis->setAutoTicks(false);
    plot->xAxis->setVisible(true);;
    plot->xAxis->setLabel(QObject::tr("Real"));

    plot->yAxis->setAutoTicks(false);
    plot->yAxis->setVisible(true);
    plot->yAxis->setLabel(QObject::tr("Imaginary"));
    plot->legend->setVisible(false);
}

void CDRMPlotQCP::setupConstPlot(const char* text, ECodScheme eNewCoSc)
{
    setupConst();
    setQAMGrid(eNewCoSc);
    title->setText(QObject::tr(text));
}

void CDRMPlotQCP::addConstellation(const char *legendText, int c)
{
    QCPGraph* g = plot->addGraph();
    g->setName(QObject::tr(legendText));
    switch(c) {
    case 0:
        g->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, Qt::black, 6));
        break;
    case 1:
        g->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDiamond, Qt::red, 6));
        break;
    case 2:
        g->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, Qt::blue, 6));
        break;
    default:
        ;
    }
    g->setLineStyle(QCPGraph::lsNone);
    if(plot->graphCount()>1)
        plot->legend->setVisible(true);
}

void CDRMPlotQCP::updateWaterfall(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
    if(wfplot->pixmap.size()!=plot->axisRect()->size())
        wfplot->resize(plot->axisRect()->size());
    wfplot->updatePlot(vecrData, plot->yAxis->range().lower, plot->yAxis->range().upper);
    wfitem->setPixmap(wfplot->pixmap);
}

void CDRMPlotQCP::setData(int n, CVector<_COMPLEX>& veccData)
{
    QVector<double> k,v;
    for(int i=0; i<veccData.Size(); i++) {
        double re = veccData[i].real();
        double im = veccData[i].imag();
        k << re;
        v << im;
    }
    plot->graph(n)->setData(k, v);
}

void CDRMPlotQCP::setData(int n, CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, bool autoScale, const QString &axisLabel)
{
    QVector<double> k,v;
    for(int i=0; i<vecrData.Size(); i++) {
        k << vecrScale[i];
        v << vecrData[i];
    }
    plot->graph(n)->setData(k, v);
    if(autoScale)
        plot->graph(n)->rescaleKeyAxis(true, true);
    plot->graph(n)->rescaleValueAxis(true);
    if(axisLabel != "") {
        if(n==0)
            plot->yAxis->setLabel(axisLabel);
        else
            plot->yAxis2->setLabel(axisLabel);
    }
}

void CDRMPlotQCP::setxMarker(int n, _REAL r)
{
    vlines[n]->start->setCoords(r, plot->yAxis->range().lower);
    vlines[n]->end->setCoords(r, plot->yAxis->range().upper);
}

void CDRMPlotQCP::setxMarker(int n, _REAL c, _REAL w)
{
    QVector<double>	x, y;
    x << c;
    y << plot->yAxis->range().lower; // full height
    bars[n]->setData(x, y);
    bars[n]->setWidth(w);
}

void CDRMPlotQCP::setyMarker(int n, _REAL r)
{
    hlines[n]->start->setCoords(r, plot->xAxis->range().lower);
    hlines[n]->end->setCoords(r, plot->xAxis->range().upper);
}
