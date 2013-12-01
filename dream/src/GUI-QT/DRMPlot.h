/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Original Author(s):
 *	Volker Fischer
 *
 * Qwt 5-6 conversion Author(s):
 *  David Flamand
 *
 * Description:
 *	Custom settings of the qwt-plot, Support Qwt version 5.0.0 to 6.1.0(+)
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

#ifndef __DRMPLOT_QWT5_H
#define __DRMPLOT_QWT5_H

#include <QTimer>
#include <QDialog>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#if QWT_VERSION < 0x060000
#include <qwt_symbol.h>
#endif
#include <../Parameter.h>
#include <../util/Vector.h>
#include "receivercontroller.h"

/* Window size for standalone chart */
#define WINDOW_CHART_WIDTH 256
#define WINDOW_CHART_HEIGHT 256

/* Window border for standalone chart */
#define WINDOW_BORDER 1

/* Define the plot font size */
#define PLOT_FONT_SIZE 8

/* Classes ********************************************************************/

class WaterfallWidget;
class CDRMReceiver;
class QTreeWidget;
class QResizeEvent;
class QwtPlotPicker;
class QwtLegend;

/* This class is needed to handle events for standalone window chart */
class QwtPlotDialog : public QDialog
{
	Q_OBJECT

public:
    QwtPlotDialog(QWidget* parent) : QDialog(parent)
	{
		setWindowFlags(Qt::Window);
		resize(WINDOW_CHART_WIDTH, WINDOW_CHART_HEIGHT);
		Frame = new QFrame(this);
		Frame->setFrameStyle(QFrame::Panel|QFrame::Sunken);
		Frame->setLineWidth(WINDOW_BORDER);
		Plot = new QwtPlot(Frame);
	}
    ~QwtPlotDialog() { }
	QwtPlot *GetPlot() { return Plot; }
	void show() { QDialog::show(); emit activate(); }
	void hide() { emit deactivate(); QDialog::hide(); }

protected:
	QFrame *Frame;
	QwtPlot *Plot;
	void reject() { emit deactivate(); QDialog::reject(); }
    void resizeEvent(QResizeEvent *e);

signals:
	void activate();
	void deactivate();
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

    CDRMPlot(QWidget*, QwtPlot*, ReceiverController*);
	~CDRMPlot();

	QwtPlot         *plot;

	void SetupChart(const ECharType eNewType);
	ECharType GetChartType() const { return CurCharType; }
	void UpdateAnalogBWMarker();
	void SetPlotStyle(const int iNewStyleID);

    void setupTreeWidget(QTreeWidget* tw);
    void setCaption(const QString& s) { if (DialogPlot) DialogPlot->setWindowTitle(s); }
	void setIcon(const QIcon& s) { if (DialogPlot) DialogPlot->setWindowIcon(s); }
	void setGeometry(const QRect& g) { if (DialogPlot) DialogPlot->setGeometry(g); }
	bool isVisible() { if (DialogPlot) return DialogPlot->isVisible(); else return FALSE; }
	const QRect geometry() { if (DialogPlot) return DialogPlot->geometry(); else return QRect(); }
	void close() { if (DialogPlot) delete this; }
	void hide() { if (DialogPlot) DialogPlot->hide(); }
	void show() { if (DialogPlot) DialogPlot->show(); }


protected:
	void SetVerticalBounds(
		const _REAL rStartGuard, const _REAL rEndGuard,
		const _REAL rBeginIR, const _REAL rEndIR);
	void SetHorizontalBounds( _REAL rScaleMin, _REAL rScaleMax, _REAL rLowerB, _REAL rHigherB);
	void SetInpSpecWaterf(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetDCCarrier(const _REAL rDCFreq);
	void SetBWMarker(const _REAL rBWCenter, const _REAL rBWWidth);
	void AutoScale(CVector<_REAL>& vecrData, CVector<_REAL>& vecrData2,
		CVector<_REAL>& vecrScale);
	void AutoScale2(CVector<_REAL>& vecrData,
		CVector<_REAL>& vecrData2,
		CVector<_REAL>& vecrScale);
	void AutoScale3(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2,
		CVector<_REAL>& vecrScale);
	void SetData(QwtPlotCurve& curve, CVector<_COMPLEX>& veccData);
	void SetData(CVector<_COMPLEX>& veccData);
	void SetData(CVector<_COMPLEX>& veccMSCConst,
		CVector<_COMPLEX>& veccSDCConst,
		CVector<_COMPLEX>& veccFACConst);
	void SetGrid(double div, int step, int substep);
	void SetQAMGrid(const ECodScheme eCoSc);

	void PlotDefaults();
	void PlotForceUpdate();
	void PlotSetLegendFont();

	void SetupAvIR();
	void SetupTranFct();
	void SetupAudioSpec(_BOOLEAN bAudioDecoder);
	void SetupFreqSamOffsHist();
	void SetupDopplerDelayHist();
	void SetupSNRAudHist();
	void SetupPSD();
	void SetupSNRSpectrum();
	void SetupInpSpec();
	void SetupFACConst();
	void SetupSDCConst(const ECodScheme eNewCoSc);
	void SetupMSCConst(const ECodScheme eNewCoSc);
	void SetupAllConst();
	void SetupInpPSD(_BOOLEAN bAnalog = FALSE);
	void SetupInpSpecWaterf();

	void AddWhatsThisHelpChar(const ECharType NCharType);

    ReceiverController* controller;
	QwtPlot         *SuppliedPlot;
	QwtPlotDialog   *DialogPlot;

	bool            bActive;

    /* Colors */
	QColor			MainPenColorPlot;
	QColor			MainPenColorConst;
	QColor			MainGridColorPlot;
	QColor			SpecLine1ColorPlot;
	QColor			SpecLine2ColorPlot;
	QColor			PassBandColorPlot;
	QColor			BckgrdColorPlot;

	ECharType		CurCharType;
	ECharType		InitCharType;
	ECodScheme		eLastSDCCodingScheme;
	ECodScheme		eLastMSCCodingScheme;
	_BOOLEAN		bLastAudioDecoder;

	QwtText			leftTitle, rightTitle, bottomTitle;

	QwtPlotCurve	main1curve, main2curve;
	QwtPlotCurve	curve1, curve2, curve3, curve4, curve5;
	QwtPlotCurve	hcurvegrid, vcurvegrid;
	QwtPlotGrid		grid;
#if QWT_VERSION < 0x060000
	QwtSymbol		MarkerSym1, MarkerSym2, MarkerSym3;
#endif
	QwtPlotPicker	*picker;
	QwtLegend		*legend;

	QTimer			TimerChart;
	CDRMReceiver	*pDRMRec;

	/* Waterfall spectrum stuff */
    WaterfallWidget* waterfallWidget;
	int				iAudSampleRate;
	int				iSigSampleRate;
	int				iLastXoredSampleRate;
	int				iLastChanMode;

public slots:
#if QWT_VERSION < 0x060000
	void OnSelected(const QwtDoublePoint &pos);
#else
	void OnSelected(const QPointF &pos);
#endif
	void OnTimerChart();
	void activate();
	void deactivate();

signals:
	void xAxisValSet(double);
};


#endif
