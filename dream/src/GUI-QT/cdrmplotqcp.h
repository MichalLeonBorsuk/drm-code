#ifndef CDRMPLOTQCP_H
#define CDRMPLOTQCP_H

#include "DRMPlot.h"
#include <qcustomplot.h>
#include "waterfallwidget.h"

class CDRMPlotQCP : public CDRMPlot
{
public:
    CDRMPlotQCP(QCustomPlot* plot);
    ~CDRMPlotQCP();

    void SetPlotStyle(const int iNewStyleID);
    void setCaption(const QString& s);
    void setIcon(const QIcon& s);
    void setGeometry(const QRect& g);
    const QRect geometry() const;
    bool isVisible() const;
    void close();
    void hide();
    void show();
    void activate();
    void deactivate();

protected:
    void applyColors();
    void replot();
    void clearPlots();
    void setQAMGrid(const ECodScheme eCoSc);
    void setupBasicPlot(const char* titleText,
                        const char* xText, const char* yText, const char* legendText,
                        double left, double right, double bottom, double top);
    void add2ndGraph(const char* axisText, const char* legendText, double bottom, double top);
    void addxMarker(QColor color, double initialPos);
    void addyMarker(QColor color, double initialPos);
    void setupConstPlot(const char* text, ECodScheme eNewCoSc);
    void addConstellation(const char* legendText, int n);
    void setupWaterfall();

    void setData(int n, CVector<_COMPLEX>& veccData);
    void setData(int n, CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, bool autoScale=false, const QString& axisLabel="");
    void setxMarker(int n, _REAL r);
    void setxMarker(int n, _REAL l, _REAL r);
    void setyMarker(int n, _REAL r);
    void updateWaterfall(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);

private:
    void setupConst();

    QCustomPlot* plot;
    WaterFallPlot* wfplot;
    QCPItemPixmap* wfitem;
    QCPPlotTitle* title;
    QVector<QCPItemLine*> hlines, vlines;
    QVector<QCPBars*> bars;
};

#endif // CDRMPLOTQCP_H
