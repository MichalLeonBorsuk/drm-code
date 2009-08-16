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

#if !defined(DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_)
#define DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_symbol.h>
#include <QTimer>
#include "../Parameter.h"
#include "../util/Settings.h"
#include <deque>
#include <map>

/* Definitions ****************************************************************/
#define GUI_CONTROL_UPDATE_WATERFALL			100	/* Milliseconds */

/* Define the plot color profiles */
/* BLUEWHITE */
#define BLUEWHITE_MAIN_PEN_COLOR_PLOT			Qt::blue
#define BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION	Qt::blue
#define BLUEWHITE_BCKGRD_COLOR_PLOT				Qt::white
#define BLUEWHITE_MAIN_GRID_COLOR_PLOT			Qt::gray
#define BLUEWHITE_SPEC_LINE1_COLOR_PLOT			Qt::red
#define BLUEWHITE_SPEC_LINE2_COLOR_PLOT			Qt::black
#define BLUEWHITE_PASS_BAND_COLOR_PLOT			QColor(192, 192, 255)

/* GREENBLACK */
#define GREENBLACK_MAIN_PEN_COLOR_PLOT			Qt::green
#define GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION	Qt::red
#define GREENBLACK_BCKGRD_COLOR_PLOT			Qt::black
#define GREENBLACK_MAIN_GRID_COLOR_PLOT			QColor(128, 0, 0)
#define GREENBLACK_SPEC_LINE1_COLOR_PLOT		Qt::yellow
#define GREENBLACK_SPEC_LINE2_COLOR_PLOT		Qt::blue
#define GREENBLACK_PASS_BAND_COLOR_PLOT			QColor(0, 96, 0)

/* BLACKGREY */
#define BLACKGREY_MAIN_PEN_COLOR_PLOT			Qt::black
#define BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION	Qt::green
#define BLACKGREY_BCKGRD_COLOR_PLOT				Qt::gray
#define BLACKGREY_MAIN_GRID_COLOR_PLOT			Qt::white
#define BLACKGREY_SPEC_LINE1_COLOR_PLOT			Qt::blue
#define BLACKGREY_SPEC_LINE2_COLOR_PLOT			Qt::yellow
#define BLACKGREY_PASS_BAND_COLOR_PLOT			QColor(128, 128, 128)


/* Maximum and minimum values of x-axis of input spectrum plots */
#define MIN_VAL_INP_SPEC_Y_AXIS_DB				double( -125.0)
#define MAX_VAL_INP_SPEC_Y_AXIS_DB				double( -25.0)

/* Maximum and minimum values of x-axis of input PSD (shifted) */
#define MIN_VAL_SHIF_PSD_Y_AXIS_DB				double( -85.0)
#define MAX_VAL_SHIF_PSD_Y_AXIS_DB				double( -35.0)

/* Maximum and minimum values of x-axis of SNR spectrum */
#define MIN_VAL_SNR_SPEC_Y_AXIS_DB				double( 0.0)
#define MAX_VAL_SNR_SPEC_Y_AXIS_DB				double( 35.0)


/* Classes ********************************************************************/
class PlotDetails
{
public:
    int timerInterval; // milliseconds
    QString aboutText;
    //void start()=0;
    //void stop()=0;
};

class SpectrogramData: public QwtRasterData
{
public:
    SpectrogramData():QwtRasterData(),data(),scale(),height(0)
    {
    }

    virtual QwtRasterData *copy() const;

    virtual QwtDoubleInterval range() const
    {
        return QwtDoubleInterval(MIN_VAL_INP_SPEC_Y_AXIS_DB, MAX_VAL_INP_SPEC_Y_AXIS_DB);
    }

    virtual double value(double x, double y) const;
    void setHeight(size_t h);
    void setData(vector<double>& row);

protected:
    deque<vector<double> > data;
    vector<double> scale;
    size_t height;
};

class CDRMPlot : public QObject
{

    Q_OBJECT

public:

	enum EPlotType
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

	CDRMPlot(QwtPlot*, CParameter*);
	virtual ~CDRMPlot();

	void SetupChart(const EPlotType eNewType);
	void SetupChartNow();
	void UpdateChartNow();
	EPlotType GetChartType() const {return CurrentChartType;}
	void Update() {OnTimerChart();}
	void start();
	void stop();
	void load(const CSettings& s, const string& section);
	bool save(CSettings& s, const string& section);

	void SetPlotStyle(const int iNewStyleID);

protected:
	void SetAvIR();
	void SetTranFct();
	void SetAudioSpectrum();
	void SetPSD();
	void SetSNRSpectrum();
	void SetInpSpectrum();
	void SetInpPSD();
	void SetInpSpecWaterf();
	void SetFreqSamOffsHist();
	void SetDopplerDelayHist();
	void SetSNRAudHist();
	void SetFACConst();
	void SetSDCConst();
	void SetMSCConst();
	void SetAllConst();
	void UpdateAvIR();
	void UpdateTranFct();
	void UpdateAudioSpectrum();
	void UpdatePSD();
	void UpdateSNRSpectrum();
	void UpdateInpSpectrum();
	void UpdateInpPSD();
	void UpdateInpSpecWaterf();
	void UpdateFreqSamOffsHist();
	void UpdateDopplerDelayHist();
	void UpdateSNRAudHist();
	void UpdateFACConst();
	void UpdateSDCConst();
	void UpdateMSCConst();
	void UpdateAllConst();
    void SetData(QwtPlotCurve* curve, vector<_COMPLEX>& veccData);
    void startPlot(EPlotType);
    void endPlot(EPlotType);
    _REAL GetSymbolDuration();
    _REAL GetFrameDuration();

	void SpectrumPlotDefaults(const QString&, const QString&, uint);
	void SetDCCarrier(double);
    void ConstellationPlotDefaults(const QString& title, double limit, int n);
    QwtPlotCurve* ScatterCurve(const QString& title, const QwtSymbol& s);

	/* Colors */
	QColor			MainPenColorPlot;
	QColor			MainPenColorConst;
	QColor			MainGridColorPlot;
	QColor			SpecLine1ColorPlot;
	QColor			SpecLine2ColorPlot;
	QColor			PassBandColorPlot;
	QColor			BckgrdColorPlot;

	/* Axis Titles */
	QwtText         leftTitle, rightTitle, bottomTitle;

	QSize			LastCanvasSize;
	ECodScheme      eCurSDCCodingScheme;
	ECodScheme      eCurMSCCodingScheme;

	QwtPlotCurve	*main1curve, *main2curve;
	QwtPlotCurve	*DCCarrierCurve, *BandwidthMarkerCurve;
	QwtPlotCurve	*curve1, *curve2, *curve3, *curve4, *curve5, *curve6;
	QwtSymbol		MarkerSymFAC, MarkerSymSDC, MarkerSymMSC;
    QwtPlotGrid*    grid;
    QwtPlotSpectrogram* spectrogram;
    SpectrogramData spectrogramData;

	EPlotType		CurrentChartType;
	EPlotType		WantedChartType;
	bool		    bOnTimerCharMutexFlag;
	QTimer			TimerChart;

    CParameter&     Parameters;

    QwtPlot*        plot;
    int             styleId;
    map<int,PlotDetails> plotDetails;

public slots:
	void OnClicked(const QwtDoublePoint& e);
	void OnTimerChart();

signals:
	void xAxisValSet(double);
};


#endif // DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_
