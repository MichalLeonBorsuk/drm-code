#ifndef CDRMPLOTQCP_H
#define CDRMPLOTQCP_H

#include "DRMPlot.h"
#include <qcustomplot.h>
#include "waterfallwidget.h"

class CDRMPlotQCP : public CDRMPlot
{
public:
    CDRMPlotQCP(QWidget* parent=0);
    ~CDRMPlotQCP();

    QWidget* widget() const { return plot; }

protected:
    void applyColors();
    void replot();
    void clearPlots();
    void setQAMGrid(double div, int step, int substep);
    void setupBasicPlot(const char* titleText,
                        const char* xText, const char* yText, const char* legendText,
                        double left, double right, double bottom, double top);
    void add2ndGraph(const char* axisText, const char* legendText, double bottom, double top);
    void addxMarker(QColor color, double initialPos);
    void addBwMarker();
    void addyMarker(QColor color, double initialPos);
    void setupConstPlot(const char* text);
    void addConstellation(const char* legendText, int n);
    void setupWaterfall();

    void setData(int n, CVector<_COMPLEX>& veccData);
    void setData(int n, CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, bool autoScale=false, const QString& axisLabel="");
    void setxMarker(int n, _REAL r);
    void setBwMarker(int n, _REAL l, _REAL r);
    void setyMarker(int n, _REAL r);
    void updateWaterfall(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    void setWhatsThis(const QString& s) { plot->setWhatsThis(s); }

private:

    QCustomPlot* plot;
    WaterFallPlot* wfplot;
    QCPItemPixmap* wfitem;
    QCPPlotTitle* title;
    QVector<QCPItemLine*> hlines, vlines;
    QVector<QCPBars*> bars;
};

#endif // CDRMPLOTQCP_H
