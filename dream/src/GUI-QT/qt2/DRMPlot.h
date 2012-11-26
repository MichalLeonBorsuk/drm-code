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
#include <qwt_plot_canvas.h>
#include <qwt_scldraw.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qwhatsthis.h>
#include "../resample/Resample.h"
#include "../util/Vector.h"
#include "../Parameter.h"
#include "../DrmReceiver.h"


/* Definitions ****************************************************************/
#define GUI_CONTROL_UPDATE_WATERFALL			100	/* Milliseconds */


/* Define the plot color profiles */
/* BLUEWHITE */
#define BLUEWHITE_MAIN_PEN_COLOR_PLOT			blue
#define BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION	blue
#define BLUEWHITE_BCKGRD_COLOR_PLOT				white
#define BLUEWHITE_MAIN_GRID_COLOR_PLOT			gray
#define BLUEWHITE_SPEC_LINE1_COLOR_PLOT			red
#define BLUEWHITE_SPEC_LINE2_COLOR_PLOT			black
#define BLUEWHITE_PASS_BAND_COLOR_PLOT			QColor(192, 192, 255)

/* GREENBLACK */
#define GREENBLACK_MAIN_PEN_COLOR_PLOT			green
#define GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION	red
#define GREENBLACK_BCKGRD_COLOR_PLOT			black
#define GREENBLACK_MAIN_GRID_COLOR_PLOT			QColor(128, 0, 0)
#define GREENBLACK_SPEC_LINE1_COLOR_PLOT		yellow
#define GREENBLACK_SPEC_LINE2_COLOR_PLOT		blue
#define GREENBLACK_PASS_BAND_COLOR_PLOT			QColor(0, 96, 0)

/* BLACKGREY */
#define BLACKGREY_MAIN_PEN_COLOR_PLOT			black
#define BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION	green
#define BLACKGREY_BCKGRD_COLOR_PLOT				gray
#define BLACKGREY_MAIN_GRID_COLOR_PLOT			white
#define BLACKGREY_SPEC_LINE1_COLOR_PLOT			blue
#define BLACKGREY_SPEC_LINE2_COLOR_PLOT			yellow
#define BLACKGREY_PASS_BAND_COLOR_PLOT			QColor(128, 128, 128)


/* Maximum and minimum values of x-axis of input spectrum plots */
#define MIN_VAL_INP_SPEC_Y_AXIS_DB				((double) -125.0)
#define MAX_VAL_INP_SPEC_Y_AXIS_DB				((double) -25.0)

/* Maximum and minimum values of x-axis of input PSD (shifted) */
#define MIN_VAL_SHIF_PSD_Y_AXIS_DB				((double) -85.0)
#define MAX_VAL_SHIF_PSD_Y_AXIS_DB				((double) -35.0)

/* Maximum and minimum values of x-axis of SNR spectrum */
#define MIN_VAL_SNR_SPEC_Y_AXIS_DB				((double) 0.0)
#define MAX_VAL_SNR_SPEC_Y_AXIS_DB				((double) 35.0)


/* Classes ********************************************************************/
class CDRMPlot : public QwtPlot
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

	CDRMPlot(QWidget *p = 0, const char *name = 0);
	virtual ~CDRMPlot() {}

	/* This function has to be called before chart can be used! */
	void SetRecObj(CDRMReceiver* pNDRMRec) {pDRMRec = pNDRMRec;}

	void SetupChart(const ECharType eNewType);
	ECharType GetChartType() const {return CurCharType;}
	void Update() {OnTimerChart();}

	void SetAvIR(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, 
				 _REAL rLowerB, _REAL rHigherB,
				 const _REAL rStartGuard, const _REAL rEndGuard, 
				 const _REAL rBeginIR, const _REAL rEndIR);
	void SetTranFct(CVector<_REAL>& vecrData, CVector<_REAL>& vecrData2,
					CVector<_REAL>& vecrScale);
	void SetAudioSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetSNRSpectrum(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetInpSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
					const _REAL rDCFreq);
	void SetInpPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
				   const _REAL rDCFreq, const _REAL rBWCenter = (_REAL) 0.0,
				   const _REAL rBWWidth = (_REAL) 0.0);
	void SetInpSpecWaterf(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetFreqSamOffsHist(CVector<_REAL>& vecrData, CVector<_REAL>& vecrData2,
							CVector<_REAL>& vecrScale,
							const _REAL rFreqOffAcquVal);
	void SetDopplerDelayHist(CVector<_REAL>& vecrData,
							 CVector<_REAL>& vecrData2,
							 CVector<_REAL>& vecrScale);
	void SetSNRAudHist(CVector<_REAL>& vecrData,
					   CVector<_REAL>& vecrData2,
					   CVector<_REAL>& vecrScale);
	void SetFACConst(CVector<_COMPLEX>& veccData);
	void SetSDCConst(CVector<_COMPLEX>& veccData, ECodScheme eNewCoSc);
	void SetMSCConst(CVector<_COMPLEX>& veccData, ECodScheme eNewCoSc);
	void SetAllConst(CVector<_COMPLEX>& veccMSC,
					 CVector<_COMPLEX>& veccSDC,
					 CVector<_COMPLEX>& veccFAC);
	void SetPlotStyle(const int iNewStyleID);

protected:
	void SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2,
				 CVector<_REAL>& vecrScale);
	void SetData(CVector<_COMPLEX>& veccData);
	void SetData(CVector<_COMPLEX>& veccMSCConst,
				 CVector<_COMPLEX>& veccSDCConst,
				 CVector<_COMPLEX>& veccFACConst);
	void SetQAM4Grid();
	void SetQAM16Grid();
	void SetQAM64Grid();

	void SetupAvIR();
	void SetupTranFct();
	void SetupAudioSpec();
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
	void SetupInpPSD();
	void SetupInpSpecWaterf();

	void AddWhatsThisHelpChar(const ECharType NCharType);
    virtual void showEvent(QShowEvent* pEvent);
	virtual void hideEvent(QHideEvent* pEvent);

	/* Colors */
	QColor			MainPenColorPlot;
	QColor			MainPenColorConst;
	QColor			MainGridColorPlot;
	QColor			SpecLine1ColorPlot;
	QColor			SpecLine2ColorPlot;
	QColor			PassBandColorPlot;
	QColor			BckgrdColorPlot;

	QSize			LastCanvasSize;

	ECharType		CurCharType;
	ECharType		InitCharType;
	long			main1curve, main2curve;
	long			curve1, curve2, curve3, curve4, curve5, curve6;
	QwtSymbol		MarkerSym1, MarkerSym2, MarkerSym3;

	_BOOLEAN		bOnTimerCharMutexFlag;
	QTimer			TimerChart;

	CSpectrumResample Resample;

	CDRMReceiver*	pDRMRec;
	int				iAudSampleRate;
	int				iSigSampleRate;

public slots:
	void OnClicked(const QMouseEvent& e);
	void OnTimerChart();

signals:
	void xAxisValSet(double);
};


#endif // DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_
