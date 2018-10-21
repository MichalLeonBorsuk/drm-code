/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Original Author(s):
 *  Volker Fischer
 * Refactored by Julian Cable
 *
 * Description:
 *  Base class for plots
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef __DRMPLOT_H
#define __DRMPLOT_H

#include "../util/Vector.h"
#include <QColor>
#include <QDialog>
#include <QFrame>

/* Window size for standalone chart */
#define WINDOW_CHART_WIDTH 256
#define WINDOW_CHART_HEIGHT 256

/* Maximum and minimum values of x-axis of input spectrum plots */
#define MIN_VAL_INP_SPEC_Y_AXIS_DB              double(-120.0)
#define MAX_VAL_INP_SPEC_Y_AXIS_DB              double(0.0)

/* Maximum and minimum values of x-axis of input PSD (shifted) */
#define MIN_VAL_SHIF_PSD_Y_AXIS_DB              double(-85.0)
#define MAX_VAL_SHIF_PSD_Y_AXIS_DB              double(-35.0)

/* Maximum and minimum values of x-axis of SNR spectrum */
#define MIN_VAL_SNR_SPEC_Y_AXIS_DB              double(0.0)
#define MAX_VAL_SNR_SPEC_Y_AXIS_DB              double(35.0)

/* Window border for standalone chart */
#define WINDOW_BORDER 1

namespace Plot {
enum EAxis
{
    bottom = 0,
    left = 1,
    top = 2,
    right = 3
};
enum EPolicy {
    fixed = 0,
    min = 1,         // adjust the scale so that it is not larger than rMinScaleRange"
    fit = 2,         // adjust the scale maximum so that it is not more than "rMaxDisToMax"
    enlarge = 3,      // enlarge scale if needed
    first = 4,
    last = 5
};
}

enum ECharType
{
    INPUT_SIG_PSD = 0, /* default */
    TRANSFERFUNCTION = 1,
    FAC_CONSTELLATION = 2,
    SDC_CONSTELLATION = 3,
    MSC_CONSTELLATION = 4,
    POWER_SPEC_DENSITY = 5,
    INPUTSPECTRUM_NO_AV = 6,
    AUDIO_SPECTRUM = 7,
    FREQ_SAM_OFFS_HIST = 8,
    DOPPLER_DELAY_HIST = 9,
    ALL_CONSTELLATION = 10,
    SNR_AUDIO_HIST = 11,
    AVERAGED_IR = 12,
    SNR_SPECTRUM = 13,
    INPUT_SIG_PSD_ANALOG = 14,
    INP_SPEC_WATERF = 15,
    NONE_OLD = 16 /* None must always be the last element! (see settings) */
};

class QTreeWidget;
class QIcon;
class QRect;
class ReceiverController;

class PlotInterface
{
public:
    virtual void applyColors(QColor MainGridColorPlot, QColor BckgrdColorPlot)=0;
    virtual void replot()=0;
    virtual void clearPlots()=0;
    virtual void setupBasicPlot(const char* titleText,
                                const char* xText, const char* yText, const char* legendText,
                                double left, double right, double bottom, double top, QColor pc, QColor bc)=0;
    virtual void add2ndGraph(const char* axisText, const char* legendText, double bottom, double top, QColor pc)=0;
    virtual void addxMarker(QColor color, double initialPos)=0;
    virtual void addBwMarker(QColor c)=0;
    virtual void addyMarker(QColor color, double initialPos)=0;
    virtual void setupConstPlot(const char* text)=0;
    virtual void addConstellation(const char* legendText, int n)=0;
    virtual void setupWaterfall(double)=0;
    virtual void setQAMGrid(double div, int step, int substep)=0;
    virtual void setData(int n, CVector<_COMPLEX>& veccData)=0;
    virtual void setData(int n, CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, const QString& axisLabel="")=0;
    virtual void setxMarker(int n, _REAL r)=0;
    virtual void setBwMarker(int n, _REAL c, _REAL b)=0;
    virtual void setyMarker(int n, _REAL r)=0;
    virtual void updateWaterfall(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)=0;
    virtual void setWhatsThis(const QString&)=0;
    /* adjust the scale so that it is not larger than rMinScaleRange"  */
    virtual void setAutoScalePolicy(Plot::EAxis axis, Plot::EPolicy pol, double limit)=0;
    virtual QWidget* widget() const =0;
};

class CDRMPlot: public QObject
{
    Q_OBJECT
public:
    CDRMPlot(QWidget* parent=0);
    virtual ~CDRMPlot();

    void SetupChart(const ECharType eNewType, int sampleRate);
    void setupTreeWidget(QTreeWidget* tw);
    void SetPlotStyle(const int iNewStyleID);
    void update(ReceiverController* rc);
    ECharType getChartType() const {
        return CurCharType;
    }
    virtual QWidget* widget() const {
        return plot->widget();
    }

signals:
    void plotClicked(double);

protected:
    void addWhatsThisHelp();

    ECharType       CurCharType;
    int             iSigSampleRate;
    QColor          MainPenColorPlot;
    QColor          MainGridColorPlot;
    QColor          SpecLine1ColorPlot;
    QColor          SpecLine2ColorPlot;
    QColor          PassBandColorPlot;
    QColor          BckgrdColorPlot;
    PlotInterface*  plot;
private slots:
    void on_plotClicked(double d);
};

#endif
