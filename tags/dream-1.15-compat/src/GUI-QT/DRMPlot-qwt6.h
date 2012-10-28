/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *
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

#ifndef __DRMPLOT_QWT6_H
#define __DRMPLOT_QWT6_H

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_plot_picker.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qwhatsthis.h>
#include <QIcon>
#include "../util/Vector.h"
#include "../Parameter.h"
#include "../DrmReceiver.h"

#if QWT_VERSION < 0x050200
# error "Qwt 5.2 or higher needed"
#endif

/* Definitions ****************************************************************/
#define GUI_CONTROL_UPDATE_WATERFALL			100	/* Milliseconds */

/* Maximum and minimum values of y-axis of input spectrum plots */
#define MIN_VAL_INP_SPEC_Y_AXIS_DB				((double) -125.0)
#define MAX_VAL_INP_SPEC_Y_AXIS_DB				((double) -25.0)

/* Maximum and minimum values of y-axis of input PSD (shifted) */
#define MIN_VAL_SHIF_PSD_Y_AXIS_DB				((double) -85.0)
#define MAX_VAL_SHIF_PSD_Y_AXIS_DB				((double) -35.0)

/* Maximum and minimum values of y-axis of SNR spectrum */
#define MIN_VAL_SNR_SPEC_Y_AXIS_DB				((double) 0.0)
#define MAX_VAL_SNR_SPEC_Y_AXIS_DB				((double) 35.0)


/* Classes ********************************************************************/
class Chart: public QObject
{
public:
    Chart(CDRMReceiver *pDRMRec, QwtPlot* p);
    virtual void Setup();
    virtual void Update()=0;
    void SetPlotStyle(const int iNewStyleID);
protected:
    void SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    CDRMReceiver* receiver;
    QwtPlot*      plot;
    QwtPlotGrid*  grid;;
    CPlotManager* plotManager;
    QwtPlotCurve* main;
    QColor			MainPenColorPlot;
    QColor			MainPenColorConst;
    QColor			MainGridColorPlot;
    QColor			SpecLine1ColorPlot;
    QColor			SpecLine2ColorPlot;
    QColor			PassBandColorPlot;
    QColor			BckgrdColorPlot;
};


class Chart2: public Chart
{
public:
    Chart2(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
protected:
    void SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2, CVector<_REAL>& vecrScale);
    QwtPlotCurve* main2;
    QwtLegend *legend;
};

class SpectrumChart: public Chart
{
public:
    SpectrumChart(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
protected:
    void SetDCCarrier();
    QwtPlotCurve* carrier;
};

class InpSpecWaterf: public Chart
{
public:
    InpSpecWaterf(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
protected:
    QPixmap* canvas;
};

class AudioSpec: public Chart
{
public:
    AudioSpec(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
};

class AvIR: public Chart
{
public:
    AvIR(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
protected:
    void SetVerticalBounds(
        const _REAL rStartGuard, const _REAL rEndGuard,
        const _REAL rBeginIR, const _REAL rEndIR);
    void SetHorizontalBounds( _REAL rScaleMin, _REAL rScaleMax, _REAL rLowerB, _REAL rHigherB);
    QwtPlotCurve *curve1,*curve2,*curve3,*curve4,*curve5;
};

class SNRSpectrum: public Chart
{
public:
    SNRSpectrum(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
    void AutoScale(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
};

class TranFct: public Chart2
{
public:
    TranFct(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
};

class FreqSamOffsHist: public Chart2
{
public:
    FreqSamOffsHist(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
    void AutoScale(CVector<_REAL>& vecrData, CVector<_REAL>& vecrData2, CVector<_REAL>& vecrScale);
};

class DopplerDelayHist: public Chart2
{
public:
    DopplerDelayHist(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
};

class SNRAudHist: public Chart2
{
public:
    SNRAudHist(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
    void AutoScale(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
};

class PSD: public SpectrumChart
{
public:
    PSD(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
};


class InpSpec: public SpectrumChart
{
public:
    InpSpec(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
};

class InpPSD: public SpectrumChart
{
public:
    InpPSD(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
};

class AnalogInpPSD: public InpPSD
{
    Q_OBJECT
public:
    AnalogInpPSD(CDRMReceiver *pDRMRec, QwtPlot* p);
    void Setup();
    void Update();
protected:
    void SetBWMarker(const _REAL rBWCenter, const _REAL rBWWidth);
    QwtPlotCurve* bw;
    QwtPlotPicker*	picker;
public slots:
    void on_selected(const QPointF& pos);
signals:
    void xAxisValSet(double);
};

class ConstellationChart: public Chart
{
public:
    ConstellationChart(CDRMReceiver* pDRMRec, QwtPlot* plot);
    void Setup();
    void Update();
protected:
    int points;
    QwtSymbol *symbol;
    void SetData(QwtPlotCurve*, const CVector<_COMPLEX>&);
    void SetSymbol(QwtPlotCurve* curve, QwtSymbol*);
};

class FACConst: public ConstellationChart
{
public:
    FACConst(CDRMReceiver* pDRMRec, QwtPlot* plot);
    void Setup();
    void Update();
};

class SDCConst: public ConstellationChart
{
public:
    SDCConst(CDRMReceiver* pDRMRec, QwtPlot* plot);
    void Setup();
    void Update();
};

class MSCConst: public ConstellationChart
{
public:
    MSCConst(CDRMReceiver* pDRMRec, QwtPlot* plot);
    void Setup();
    void Update();
};

class AllConst: public ConstellationChart
{
public:
    AllConst(CDRMReceiver* pDRMRec, QwtPlot* plot);
    void Setup();
    void Update();
protected:
    QwtSymbol *symbolMSC, *symbolSDC, *symbolFAC;
    QwtPlotCurve *main2,*main3;
    QwtLegend *legend;
};

class CDRMPlot : public QObject
{
    Q_OBJECT

public:
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

    CDRMPlot(QwtPlot*);
    virtual ~CDRMPlot() {}

    QwtPlot*	plot;
    int plotStyle;

    /* This function has to be called before chart can be used! */
    void SetRecObj(CDRMReceiver* pNDRMRec) {
        pDRMRec = pNDRMRec;
    }

    ECharType GetChartType() const {
        return CurCharType;
    }
    void Update() {
        OnTimerChart();
    }
    void setGeometry(const QRect& g) {
        plot->setGeometry(g);
    }
    void setCaption(const QString& s) {
        plot->setWindowTitle(s);
    }
    void setIcon(const QIcon& s) {
        plot->setWindowIcon(s);
    }

    bool isVisible() {
        return plot->isVisible();
    }
    QRect geometry() {
        return plot->geometry();
    }
    void close() {
        plot->close();
    }
    void show() {
        plot->show();
    }

    void SetupChart(ECharType);
    void SetPlotStyle(const int iNewStyleID);

protected:

    QSize			LastCanvasSize;

    ECharType		CurCharType;
    ECharType		InitCharType;

    _BOOLEAN		bOnTimerCharMutexFlag;

    CDRMReceiver*	pDRMRec;
    Chart*		chart;


public slots:
	void OnTimerChart();
signals:
	void xAxisValSet(double);
};


#endif
