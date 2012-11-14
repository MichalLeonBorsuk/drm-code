/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Custom settings of the qwt-plot
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

#include "DRMPlot.h"


/* Implementation *************************************************************/
CDRMPlot::CDRMPlot(QWidget *p, const char *name) :
	QwtPlot (p, name), CurCharType(NONE_OLD), InitCharType(NONE_OLD),
	bOnTimerCharMutexFlag(FALSE), pDRMRec(NULL)
{
	/* Grid defaults */
	enableGridX(TRUE);
	enableGridY(TRUE);

	enableGridXMin(FALSE);
	enableGridYMin(FALSE);

	/* Legend should be on the right side */
	setLegendPos(QwtPlot::Right);

	/* Fonts */
	setTitleFont(QFont("SansSerif", 8, QFont::Bold));
	setAxisFont(QwtPlot::xBottom, QFont("SansSerif", 8));
	setAxisFont(QwtPlot::yLeft, QFont("SansSerif", 8));
	setAxisFont(QwtPlot::yRight, QFont("SansSerif", 8));
	setAxisTitleFont(QwtPlot::xBottom, QFont("SansSerif", 8));
	setAxisTitleFont(QwtPlot::yLeft, QFont("SansSerif", 8));
	setAxisTitleFont(QwtPlot::yRight, QFont("SansSerif", 8));

	/* Global frame */
	setFrameStyle(QFrame::Panel|QFrame::Sunken);
	setLineWidth(2);
	setMargin(10);

	/* Canvas */
	setCanvasLineWidth(0);

	/* Set default style */
	SetPlotStyle(0);

	/* Connections */
	connect(this, SIGNAL(plotMouseReleased(const QMouseEvent&)),
		this, SLOT(OnClicked(const QMouseEvent&)));
	connect(&TimerChart, SIGNAL(timeout()),
		this, SLOT(OnTimerChart()));

	TimerChart.stop();
}

void CDRMPlot::OnTimerChart()
{
	/* In some cases, if the user moves the mouse very fast over the chart
	   selection list view, this function is called by two different threads.
	   Somehow, using QMutex does not help. Therefore we introduce a flag for
	   doing this job. This solution is a work-around. TODO: better solution */
	if (bOnTimerCharMutexFlag == TRUE)
		return;

	bOnTimerCharMutexFlag = TRUE;

	/* CHART ******************************************************************/
	CVector<_REAL>		vecrData;
	CVector<_REAL>		vecrData2;
	CVector<_COMPLEX>	veccData1, veccData2, veccData3;
	CVector<_REAL>		vecrScale;
	_REAL				rLowerBound, rHigherBound;
	_REAL				rStartGuard, rEndGuard;
	_REAL				rPDSBegin, rPDSEnd;
	_REAL				rFreqAcquVal;
	_REAL				rCenterFreq, rBandwidth;

	CParameter& Parameters = *pDRMRec->GetParameters();
	Parameters.Lock();
	_REAL rDCFrequency = Parameters.GetDCFrequency();
	ECodScheme eSDCCodingScheme = Parameters.eSDCCodingScheme;
	ECodScheme eMSCCodingScheme = Parameters.eMSCCodingScheme;
	string audiodecoder = Parameters.audiodecoder;
	Parameters.Unlock();

	CPlotManager& PlotManager = *pDRMRec->GetPlotManager();

	switch (CurCharType)
	{
	case AVERAGED_IR:
		/* Get data from module */
		PlotManager.GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
			rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);

		/* Prepare graph and set data */
		SetAvIR(vecrData, vecrScale, rLowerBound, rHigherBound,
			rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);
		break;

	case TRANSFERFUNCTION:
		/* Get data from module */
		PlotManager.GetTransferFunction(vecrData, vecrData2, vecrScale);

		/* Prepare graph and set data */
		SetTranFct(vecrData, vecrData2, vecrScale);
		break;

	case POWER_SPEC_DENSITY:
		/* Get data from module */
		pDRMRec->GetOFDMDemod()->GetPowDenSpec(vecrData, vecrScale);

		/* Prepare graph and set data */
		SetPSD(vecrData, vecrScale);
		break;

	case SNR_SPECTRUM:
		/* Get data from module */
		PlotManager.GetSNRProfile(vecrData, vecrScale);

		/* Prepare graph and set data */
		SetSNRSpectrum(vecrData, vecrScale);
		break;

	case INPUTSPECTRUM_NO_AV:
		/* Get data from module */
		pDRMRec->GetReceiveData()->GetInputSpec(vecrData, vecrScale);

		/* Prepare graph and set data */
		SetInpSpec(vecrData, vecrScale, rDCFrequency);
		break;

	case INP_SPEC_WATERF:
		/* Get data from module */
		pDRMRec->GetReceiveData()->GetInputSpec(vecrData, vecrScale);

		/* Prepare graph and set data */
		SetInpSpecWaterf(vecrData, vecrScale);
		break;

	case INPUT_SIG_PSD:
		/* Get data from module */
		PlotManager.GetInputPSD(vecrData, vecrScale);

		/* Prepare graph and set data */
		SetInpPSD(vecrData, vecrScale, rDCFrequency);
		break;

	case INPUT_SIG_PSD_ANALOG:
		/* Get data and parameters from modules */
		pDRMRec->GetReceiveData()->GetInputPSD(vecrData, vecrScale);
		pDRMRec->GetAMDemod()->GetBWParameters(rCenterFreq, rBandwidth);

		/* Prepare graph and set data */
		SetInpPSD(vecrData, vecrScale,
			pDRMRec->GetAMDemod()->GetCurMixFreqOffs(), rCenterFreq,
			rBandwidth);
		break;

	case AUDIO_SPECTRUM:
		/* Get data from module */
		pDRMRec->GetWriteData()->GetAudioSpec(vecrData, vecrScale);
		if(audiodecoder=="")
		{
			setTitle(tr("No audio decoding possible"));
		}
		else
		{
			setTitle(tr("Audio Spectrum"));
		}
		/* Prepare graph and set data */
		SetAudioSpec(vecrData, vecrScale);
		break;

	case FREQ_SAM_OFFS_HIST:
		/* Get data from module */
		PlotManager.GetFreqSamOffsHist(vecrData, vecrData2, vecrScale, rFreqAcquVal);

		/* Prepare graph and set data */
		SetFreqSamOffsHist(vecrData, vecrData2, vecrScale, rFreqAcquVal);
		break;

	case DOPPLER_DELAY_HIST:
		/* Get data from module */
		PlotManager.GetDopplerDelHist(vecrData, vecrData2, vecrScale);

		/* Prepare graph and set data */
		SetDopplerDelayHist(vecrData, vecrData2, vecrScale);
		break;

	case SNR_AUDIO_HIST:
		/* Get data from module */
		PlotManager.GetSNRHist(vecrData, vecrData2, vecrScale);

		/* Prepare graph and set data */
		SetSNRAudHist(vecrData, vecrData2, vecrScale);
		break;

	case FAC_CONSTELLATION:
		/* Get data vector */
		pDRMRec->GetFACMLC()->GetVectorSpace(veccData1);

		/* Prepare graph and set data */
		SetFACConst(veccData1);
		break;

	case SDC_CONSTELLATION:
		/* Get data vector */
		pDRMRec->GetSDCMLC()->GetVectorSpace(veccData1);

		/* Prepare graph and set data */
		SetSDCConst(veccData1, eSDCCodingScheme);
		break;

	case MSC_CONSTELLATION:
		/* Get data vector */
		pDRMRec->GetMSCMLC()->GetVectorSpace(veccData1);

		/* Prepare graph and set data */
		SetMSCConst(veccData1, eMSCCodingScheme);
		break;

	case ALL_CONSTELLATION:
		/* Get data vectors */
		pDRMRec->GetMSCMLC()->GetVectorSpace(veccData1);
		pDRMRec->GetSDCMLC()->GetVectorSpace(veccData2);
		pDRMRec->GetFACMLC()->GetVectorSpace(veccData3);

		/* Prepare graph and set data */
		SetAllConst(veccData1, veccData2, veccData3);
		break;

	case NONE_OLD:
		break;
	}

	/* "Unlock" mutex flag */
	bOnTimerCharMutexFlag = FALSE;
}

void CDRMPlot::SetupChart(const ECharType eNewType)
{
	if (eNewType != NONE_OLD)
	{
		/* Set internal variable */
		CurCharType = eNewType;

		/* Update help text connected with the plot widget */
		AddWhatsThisHelpChar(eNewType);

		/* Update chart */
		OnTimerChart();

		/* Set up timer */
		switch (eNewType)
		{
		case INP_SPEC_WATERF:
			/* Very fast update */
			TimerChart.changeInterval(GUI_CONTROL_UPDATE_WATERFALL);
			break;

		case AVERAGED_IR:
		case TRANSFERFUNCTION:
		case POWER_SPEC_DENSITY:
		case INPUT_SIG_PSD:
		case INPUT_SIG_PSD_ANALOG:
		case SNR_SPECTRUM:
			/* Fast update */
			TimerChart.changeInterval(GUI_CONTROL_UPDATE_TIME_FAST);
			break;

		case FAC_CONSTELLATION:
		case SDC_CONSTELLATION:
		case MSC_CONSTELLATION:
		case ALL_CONSTELLATION:
		case INPUTSPECTRUM_NO_AV:
		case AUDIO_SPECTRUM:
		case FREQ_SAM_OFFS_HIST:
		case DOPPLER_DELAY_HIST:
		case SNR_AUDIO_HIST:
			/* Slow update of plot */
			TimerChart.changeInterval(GUI_CONTROL_UPDATE_TIME);
			break;

		case NONE_OLD:
			break;
		}
	}
}

void CDRMPlot::showEvent(QShowEvent*)
{
	/* Activate real-time timers when window is shown */
	SetupChart(CurCharType);

	/* Update window */
	OnTimerChart();
}

void CDRMPlot::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timers when window is hide to save CPU power */
	TimerChart.stop();
}

void CDRMPlot::SetPlotStyle(const int iNewStyleID)
{
	QColor BckgrdColorPlot;

	switch (iNewStyleID)
	{
	case 1:
		MainPenColorPlot = GREENBLACK_MAIN_PEN_COLOR_PLOT;
		MainPenColorConst = GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION;
		BckgrdColorPlot = GREENBLACK_BCKGRD_COLOR_PLOT;
		MainGridColorPlot = GREENBLACK_MAIN_GRID_COLOR_PLOT;
		SpecLine1ColorPlot = GREENBLACK_SPEC_LINE1_COLOR_PLOT;
		SpecLine2ColorPlot = GREENBLACK_SPEC_LINE2_COLOR_PLOT;
		PassBandColorPlot = GREENBLACK_PASS_BAND_COLOR_PLOT;
		break;

	case 2:
		MainPenColorPlot = BLACKGREY_MAIN_PEN_COLOR_PLOT;
		MainPenColorConst = BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION;
		BckgrdColorPlot = BLACKGREY_BCKGRD_COLOR_PLOT;
		MainGridColorPlot = BLACKGREY_MAIN_GRID_COLOR_PLOT;
		SpecLine1ColorPlot = BLACKGREY_SPEC_LINE1_COLOR_PLOT;
		SpecLine2ColorPlot = BLACKGREY_SPEC_LINE2_COLOR_PLOT;
		PassBandColorPlot = BLACKGREY_PASS_BAND_COLOR_PLOT;
		break;

	case 0: /* 0 is default */
	default:
		MainPenColorPlot = BLUEWHITE_MAIN_PEN_COLOR_PLOT;
		MainPenColorConst = BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION;
		BckgrdColorPlot = BLUEWHITE_BCKGRD_COLOR_PLOT;
		MainGridColorPlot = BLUEWHITE_MAIN_GRID_COLOR_PLOT;
		SpecLine1ColorPlot = BLUEWHITE_SPEC_LINE1_COLOR_PLOT;
		SpecLine2ColorPlot = BLUEWHITE_SPEC_LINE2_COLOR_PLOT;
		PassBandColorPlot = BLUEWHITE_PASS_BAND_COLOR_PLOT;
		break;
	}

	/* Apply colors */
	setGridMajPen(QPen(MainGridColorPlot, 0, DotLine));
	setGridMinPen(QPen(MainGridColorPlot, 0, DotLine));
	setCanvasBackground(QColor(BckgrdColorPlot));

	/* Make sure that plot are being initialized again */
	InitCharType = NONE_OLD;
}

void CDRMPlot::SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	double* pdData = new double[vecrData.Size()];
	double* pdScale = new double[vecrScale.Size()];

	/* Copy data from vectors in temporary arrays */
	const int iScaleSize = vecrScale.Size();
	for (int i = 0; i < iScaleSize; i++)
	{
		pdData[i] = vecrData[i];
		pdScale[i] = vecrScale[i];
	}

	setCurveData(main1curve, pdScale, pdData, vecrData.Size());

	delete[] pdData;
	delete[] pdScale;
}

void CDRMPlot::SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2,
					   CVector<_REAL>& vecrScale)
{
	double* pdData1 = new double[vecrData1.Size()];
	double* pdData2 = new double[vecrData2.Size()];
	double* pdScale = new double[vecrScale.Size()];

	/* Copy data from vectors in temporary arrays */
	const int iScaleSize = vecrScale.Size();
	for (int i = 0; i < iScaleSize; i++)
	{
		pdData1[i] = vecrData1[i];
		pdData2[i] = vecrData2[i];
		pdScale[i] = vecrScale[i];
	}

	setCurveData(main1curve, pdScale, pdData1, vecrData1.Size());
	setCurveData(main2curve, pdScale, pdData2, vecrData2.Size());

	delete[] pdData1;
	delete[] pdData2;
	delete[] pdScale;
}

void CDRMPlot::SetData(CVector<_COMPLEX>& veccData)
{
	/* Copy data from vectors in temporary arrays */
	const int iDataSize = veccData.Size();
	for (int i = 0; i < iDataSize; i++)
	{
		const long lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym1);
		setMarkerPos(lMarkerKey, veccData[i].real(), veccData[i].imag());
	}
}

void CDRMPlot::SetData(CVector<_COMPLEX>& veccMSCConst,
					   CVector<_COMPLEX>& veccSDCConst,
					   CVector<_COMPLEX>& veccFACConst)
{
	int i;

	/* Copy data from vectors in temporary arrays */
	const int iMSCSize = veccMSCConst.Size();
	for (i = 0; i < iMSCSize; i++)
	{
		const long lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym1);
		setMarkerPos(lMarkerKey,
			veccMSCConst[i].real(), veccMSCConst[i].imag());
	}

	const int iSDCSize = veccSDCConst.Size();
	for (i = 0; i < iSDCSize; i++)
	{
		const long lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym2);
		setMarkerPos(lMarkerKey,
			veccSDCConst[i].real(), veccSDCConst[i].imag());
	}

	const int iFACSize = veccFACConst.Size();
	for (i = 0; i < iFACSize; i++)
	{
		const long lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym3);
		setMarkerPos(lMarkerKey,
			veccFACConst[i].real(), veccFACConst[i].imag());
	}
}

void CDRMPlot::SetupAvIR()
{
	/* Init chart for averaged impulse response */
	setTitle(tr("Channel Impulse Response"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Time [ms]"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("IR [dB]"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Insert curves */
	clear();
	curve1 = insertCurve(tr("Guard-interval beginning"));
	curve2 = insertCurve(tr("Guard-interval end"));
	curve3 = insertCurve(tr("Estimated begin of impulse response"));
	curve4 = insertCurve(tr("Estimated end of impulse response"));
	setCurvePen(curve1, QPen(SpecLine1ColorPlot, 1, DotLine));
	setCurvePen(curve2, QPen(SpecLine1ColorPlot, 1, DotLine));
	setCurvePen(curve3, QPen(SpecLine2ColorPlot, 1, DotLine));
	setCurvePen(curve4, QPen(SpecLine2ColorPlot, 1, DotLine));

	curve5 = insertCurve(tr("Higher Bound"));
#ifdef _DEBUG_
	curve6 = insertCurve(tr("Lower bound"));
	setCurvePen(curve5, QPen(SpecLine1ColorPlot));
	setCurvePen(curve6, QPen(SpecLine2ColorPlot));
#else
	setCurvePen(curve5, QPen(SpecLine1ColorPlot, 1, DotLine));
#endif

	/* Add main curve */
	main1curve = insertCurve(tr("Channel Impulse Response"));

	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetAvIR(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
					   _REAL rLowerB, _REAL rHigherB,
					   const _REAL rStartGuard, const _REAL rEndGuard,
					   const _REAL rBeginIR, const _REAL rEndIR)
{
	/* First check if plot must be set up */
	if (InitCharType != AVERAGED_IR)
	{
		InitCharType = AVERAGED_IR;
		SetupAvIR();
	}

	if (vecrScale.Size() != 0)
	{
		/* Fixed scale */
		const double cdAxMinLeft = (double) -20.0;
		const double cdAxMaxLeft = (double) 40.0;
		setAxisScale(QwtPlot::yLeft, cdAxMinLeft, cdAxMaxLeft);

		/* Vertical bounds -------------------------------------------------- */
		double dX[2], dY[2];

		/* These bounds show the beginning and end of the guard-interval */
		dY[0] = cdAxMinLeft;
		dY[1] = cdAxMaxLeft;

		/* Left bound */
		dX[0] = dX[1] = rStartGuard;
		setCurveData(curve1, dX, dY, 2);

		/* Right bound */
		dX[0] = dX[1] = rEndGuard;
		setCurveData(curve2, dX, dY, 2);

		/* Estimated begin of impulse response */
		dX[0] = dX[1] = rBeginIR;
		setCurveData(curve3, dX, dY, 2);

		/* Estimated end of impulse response */
		dX[0] = dX[1] = rEndIR;
		setCurveData(curve4, dX, dY, 2);


		/* Data for the actual impulse response curve */
		SetData(vecrData, vecrScale);


		/* Horizontal bounds ------------------------------------------------ */
		/* These bounds show the peak detection bound from timing tracking */
		dX[0] = vecrScale[0];
		dX[1] = vecrScale[vecrScale.Size() - 1];

#ifdef _DEBUG_
		/* Lower bound */
		dY[0] = dY[1] = rLowerB;
		setCurveData(curve6, dX, dY, 2);

		/* Higher bound */
		dY[0] = dY[1] = rHigherB;
#else
		/* Only include highest bound */
		dY[0] = dY[1] = Max(rHigherB, rLowerB);
#endif
		setCurveData(curve5, dX, dY, 2);

		/* Adjust scale for x-axis */
		setAxisScale(QwtPlot::xBottom, (double) vecrScale[0],
			(double) vecrScale[vecrScale.Size() - 1]);

		replot();
	}
	else
	{
		/* No input data, clear plot (by resetting it) */
		SetupAvIR();
	}
}

void CDRMPlot::SetupTranFct()
{
	/* Init chart for transfer function */
	setTitle(tr("Channel Transfer Function / Group Delay"));
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("TF [dB]"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	setAxisTitle(QwtPlot::yRight, tr("Group Delay [ms]"));
	setAxisScale(QwtPlot::yRight, (double) -50.0, (double) 50.0);

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) -85.0, (double) -35.0);

	/* Add main curves */
	clear();
	main1curve = insertCurve(tr("Transf. Fct."));
	main2curve = insertCurve(tr("Group Del."),
		QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(main2curve, QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));

	/* Legend */
	enableLegend(TRUE, main1curve);
	enableLegend(TRUE, main2curve);
}

void CDRMPlot::SetTranFct(CVector<_REAL>& vecrData, CVector<_REAL>& vecrData2,
						  CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (InitCharType != TRANSFERFUNCTION)
	{
		InitCharType = TRANSFERFUNCTION;
		SetupTranFct();
	}

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) vecrScale.Size());

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetupAudioSpec()
{
	/* Init chart for audio spectrum */
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, "AS [dB]");
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) -100.0, (double) -20.0);
	double dBandwidth = (double) SOUNDCRD_SAMPLE_RATE / 2400; /* 20.0 for 48 kHz */
	if (dBandwidth < (double) 20.0)
		dBandwidth = (double) 20.0;

	setAxisScale(QwtPlot::xBottom, (double) 0.0, dBandwidth);

	/* Add main curve */
	clear();
	main1curve = insertCurve(tr("Audio Spectrum"));

	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetAudioSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (InitCharType != AUDIO_SPECTRUM)
	{
		InitCharType = AUDIO_SPECTRUM;
		SetupAudioSpec();
	}

	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupFreqSamOffsHist()
{
	/* Init chart for transfer function. Enable right axis, too */
	setTitle(tr("Rel. Frequency Offset / Sample Rate Offset History"));
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Time [s]"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yRight, tr("Sample Rate Offset [Hz]"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Add main curves */
	clear();
	main1curve = insertCurve(tr("Freq."));
	main2curve = insertCurve(tr("Samp."), QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(main2curve, QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));

	/* Legend */
	enableLegend(TRUE, main1curve);
	enableLegend(TRUE, main2curve);
}

void CDRMPlot::SetFreqSamOffsHist(CVector<_REAL>& vecrData,
								  CVector<_REAL>& vecrData2,
								  CVector<_REAL>& vecrScale,
								  const _REAL rFreqOffAcquVal)
{
	/* First check if plot must be set up */
	if (InitCharType != FREQ_SAM_OFFS_HIST)
	{
		InitCharType = FREQ_SAM_OFFS_HIST;
		SetupFreqSamOffsHist();
	}

	QString strYLeftLabel = tr("Freq. Offset [Hz] rel. to ") +
		QString().setNum(rFreqOffAcquVal) + " Hz";
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, strYLeftLabel);

	/* Customized auto-scaling. We adjust the y scale so that it is not larger
	   than rMinScaleRange"  */
	const _REAL rMinScaleRange = (_REAL) 1.0; /* Hz */

	/* Get maximum and minimum values */
	_REAL MaxFreq = -_MAXREAL;
	_REAL MinFreq = _MAXREAL;
	_REAL MaxSam = -_MAXREAL;
	_REAL MinSam = _MAXREAL;

	const int iSize = vecrScale.Size();
	for (int i = 0; i < iSize; i++)
	{
		if (vecrData[i] > MaxFreq)
			MaxFreq = vecrData[i];

		if (vecrData[i] < MinFreq)
			MinFreq = vecrData[i];

		if (vecrData2[i] > MaxSam)
			MaxSam = vecrData2[i];

		if (vecrData2[i] < MinSam)
			MinSam = vecrData2[i];
	}

	/* Apply scale to plot */
	setAxisScale(QwtPlot::yLeft, (double) Floor(MinFreq / rMinScaleRange),
		(double) Ceil(MaxFreq / rMinScaleRange));
	setAxisScale(QwtPlot::yRight, (double) Floor(MinSam / rMinScaleRange),
		(double) Ceil(MaxSam / rMinScaleRange));
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetupDopplerDelayHist()
{
	/* Init chart for transfer function. Enable right axis, too */
	setTitle(tr("Delay / Doppler History"));
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("Delay [ms]"));
	setAxisTitle(QwtPlot::yRight, tr("Doppler [Hz]"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) 0.0, (double) 10.0);
	setAxisScale(QwtPlot::yRight, (double) 0.0, (double) 4.0);

	/* Add main curves */
	clear();
	main1curve = insertCurve(tr("Delay"));
	main2curve = insertCurve(tr("Doppler"), QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(main2curve, QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));

	/* Legend */
	enableLegend(TRUE, main1curve);
	enableLegend(TRUE, main2curve);
}

void CDRMPlot::SetDopplerDelayHist(CVector<_REAL>& vecrData,
								   CVector<_REAL>& vecrData2,
								   CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (InitCharType != DOPPLER_DELAY_HIST)
	{
		InitCharType = DOPPLER_DELAY_HIST;
		SetupDopplerDelayHist();
	}

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetupSNRAudHist()
{
	/* Init chart for transfer function. Enable right axis, too */
	setTitle(tr("SNR / Correctly Decoded Audio History"));
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("SNR [dB]"));
	setAxisTitle(QwtPlot::yRight, tr("Corr. Dec. Audio / DRM-Frame"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Add main curves */
	clear();
	main1curve = insertCurve("SNR");
	main2curve = insertCurve("Audio", QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(main2curve, QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));

	/* Legend */
	enableLegend(TRUE, main1curve);
	enableLegend(TRUE, main2curve);
}

void CDRMPlot::SetSNRAudHist(CVector<_REAL>& vecrData,
							 CVector<_REAL>& vecrData2,
							 CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (InitCharType != SNR_AUDIO_HIST)
	{
		InitCharType = SNR_AUDIO_HIST;
		SetupSNRAudHist();
	}

	/* Customized auto-scaling. We adjust the y scale maximum so that it
	   is not more than "rMaxDisToMax" to the curve */
	const int iMaxDisToMax = 5; /* dB */
	const int iMinValueSNRYScale = 15; /* dB */

	/* Get maximum value */
	_REAL MaxSNR = -_MAXREAL;

	const int iSize = vecrScale.Size();
	for (int i = 0; i < iSize; i++)
	{
		if (vecrData[i] > MaxSNR)
			MaxSNR = vecrData[i];
	}

	/* Quantize scale to a multiple of "iMaxDisToMax" */
	double dMaxYScaleSNR =
		(double) (Ceil(MaxSNR / iMaxDisToMax) * iMaxDisToMax);

	/* Bound at the minimum allowed value */
	if (dMaxYScaleSNR < (double) iMinValueSNRYScale)
		dMaxYScaleSNR = (double) iMinValueSNRYScale;

	/* Ratio between the maximum values for audio and SNR. The ratio should be
	   chosen so that the audio curve is not in the same range as the SNR curve
	   under "normal" conditions to increase readability of curves.
	   Since at very low SNRs, no audio can received anyway so we do not have to
	   check whether the audio y-scale is in range of the curve */
	const _REAL rRatioAudSNR = (double) 1.5;
	const double dMaxYScaleAudio = dMaxYScaleSNR * (double) rRatioAudSNR;

	/* Apply scale to plot */
	setAxisScale(QwtPlot::yLeft, (double) 0.0, dMaxYScaleSNR);
	setAxisScale(QwtPlot::yRight, (double) 0.0, dMaxYScaleAudio);
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetupPSD()
{
	/* Init chart for power spectram density estimation */
	setTitle(tr("Shifted Power Spectral Density of Input Signal"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("PSD [dB]"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	setAxisScale(QwtPlot::yLeft, MIN_VAL_SHIF_PSD_Y_AXIS_DB,
		MAX_VAL_SHIF_PSD_Y_AXIS_DB);

	/* Insert line for DC carrier */
	clear();
	curve1 = insertCurve(tr("DC carrier"));
	setCurvePen(curve1, QPen(SpecLine1ColorPlot, 1, DotLine));

	double dX[2], dY[2];
	dX[0] = dX[1] = (_REAL) VIRTUAL_INTERMED_FREQ / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = MIN_VAL_SHIF_PSD_Y_AXIS_DB;
	dY[1] = MAX_VAL_SHIF_PSD_Y_AXIS_DB;

	setCurveData(curve1, dX, dY, 2);

	/* Add main curve */
	main1curve = insertCurve(tr("Shifted PSD"));

	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (InitCharType != POWER_SPEC_DENSITY)
	{
		InitCharType = POWER_SPEC_DENSITY;
		SetupPSD();
	}

	/* Set actual data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupSNRSpectrum()
{
	/* Init chart for power spectram density estimation */
	setTitle(tr("SNR Spectrum (Weighted MER on MSC Cells)"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("WMER [dB]"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Add main curve */
	clear();
	main1curve = insertCurve(tr("SNR Spectrum"));

	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetSNRSpectrum(CVector<_REAL>& vecrData,
							  CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (InitCharType != SNR_SPECTRUM)
	{
		InitCharType = SNR_SPECTRUM;
		SetupSNRSpectrum();
	}

	const int iSize = vecrScale.Size();

	/* Fixed scale for x-axis */
	setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) iSize);

	/* Fixed / variable scale (if SNR is in range, use fixed scale otherwise
	   enlarge scale) */
	/* Get maximum value */
	_REAL rMaxSNR = -_MAXREAL;
	for (int i = 0; i < iSize; i++)
	{
		if (vecrData[i] > rMaxSNR)
			rMaxSNR = vecrData[i];
	}

	double dMaxScaleYAxis = MAX_VAL_SNR_SPEC_Y_AXIS_DB;

	if (rMaxSNR > dMaxScaleYAxis)
	{
		const double rEnlareStep = (double) 10.0; /* dB */
		dMaxScaleYAxis = ceil(rMaxSNR / rEnlareStep) * rEnlareStep;
	}

	/* Set scale */
	setAxisScale(QwtPlot::yLeft, MIN_VAL_SNR_SPEC_Y_AXIS_DB, dMaxScaleYAxis);

	/* Set actual data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupInpSpec()
{
	/* Init chart for power spectram density estimation */
	setTitle(tr("Input Spectrum"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("Input Spectrum [dB]"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
		MAX_VAL_INP_SPEC_Y_AXIS_DB);

	/* Insert line for DC carrier */
	clear();
	curve1 = insertCurve(tr("DC carrier"));
	setCurvePen(curve1, QPen(SpecLine1ColorPlot, 1, DotLine));

	/* Add main curve */
	main1curve = insertCurve(tr("Input spectrum"));

	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetInpSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
						  const _REAL rDCFreq)
{
	/* First check if plot must be set up */
	if (InitCharType != INPUTSPECTRUM_NO_AV)
	{
		InitCharType = INPUTSPECTRUM_NO_AV;
		SetupInpSpec();
	}

	/* Insert line for DC carrier */
	double dX[2], dY[2];
	dX[0] = dX[1] = rDCFreq / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
	dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB;

	setCurveData(curve1, dX, dY, 2);

	/* Insert actual spectrum data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupInpPSD()
{
	int		i;
	double	dX[2], dY[2];

	/* Init chart for power spectram density estimation */
	setTitle(tr("Input PSD"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("Input PSD [dB]"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale */
	const double dXScaleMax = (double) SOUNDCRD_SAMPLE_RATE / 2000;
	setAxisScale(QwtPlot::xBottom, (double) 0.0, dXScaleMax);

	setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
		MAX_VAL_INP_SPEC_Y_AXIS_DB);

	/* Insert line for bandwidth marker */
	clear();
	curve1 = insertCurve(tr("Filter bandwidth"));

	/* Make sure that line is bigger than the current plots height. Do this by
	   setting the width to a very large value. TODO: better solution */
	setCurvePen(curve1, QPen(PassBandColorPlot, 10000));

	/* Since we want to have the "filter bandwidth" bar behind the grid, we have
	   to draw our own grid after the previous curve was inserted. TODO: better
	   solution */
	/* y-axis: get major ticks */
	const int iNumMajTicksYAx = axisScale(QwtPlot::yLeft)->majCnt();

	/* Make sure the grid does not end close to the border of the canvas.
	   Introduce some "margin" */
	dX[0] = -dXScaleMax;
	dX[1] = dXScaleMax + dXScaleMax;

	/* Draw the grid for y-axis */
	for (i = 0; i < iNumMajTicksYAx; i++)
	{
		const long curvegrid = insertCurve(tr("My Grid"));
		setCurvePen(curvegrid, QPen(MainGridColorPlot, 0, DotLine));

		dY[0] = dY[1] = axisScale(QwtPlot::yLeft)->majMark(i);
		setCurveData(curvegrid, dX, dY, 2);
	}

	/* x-axis: get major ticks */
	const int iNumMajTicksXAx = axisScale(QwtPlot::xBottom)->majCnt();

	/* Make sure the grid does not end close to the border of the canvas.
	   Introduce some "margin" */
	const double dDiffY =
		MAX_VAL_INP_SPEC_Y_AXIS_DB - MIN_VAL_INP_SPEC_Y_AXIS_DB;

	dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB - dDiffY;
	dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB + dDiffY;

	/* Draw the grid for x-axis */
	for (i = 0; i < iNumMajTicksXAx; i++)
	{
		const long curvegrid = insertCurve(tr("My Grid"));
		setCurvePen(curvegrid, QPen(MainGridColorPlot, 0, DotLine));

		dX[0] = dX[1] = axisScale(QwtPlot::xBottom)->majMark(i);
		setCurveData(curvegrid, dX, dY, 2);
	}

	/* Insert line for DC carrier */
	curve2 = insertCurve(tr("DC carrier"));
	setCurvePen(curve2, QPen(SpecLine1ColorPlot, 1, DotLine));

	/* Add main curve */
	main1curve = insertCurve(tr("Input PSD"));

	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetInpPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
						  const _REAL rDCFreq, const _REAL rBWCenter,
						  const _REAL rBWWidth)
{
	/* First check if plot must be set up. The char type "INPUT_SIG_PSD_ANALOG"
	   has the same initialization as "INPUT_SIG_PSD" */
	if (InitCharType != INPUT_SIG_PSD)
	{
		InitCharType = INPUT_SIG_PSD;
		SetupInpPSD();
	}

	/* Insert line for DC carrier */
	double dX[2], dY[2];
	dX[0] = dX[1] = rDCFreq / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
	dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB;

	setCurveData(curve2, dX, dY, 2);

	/* Insert marker for filter bandwidth if required */
	if (rBWWidth != (_REAL) 0.0)
	{
		dX[0] = (rBWCenter - rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;
		dX[1] = (rBWCenter + rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;

		/* Take the min-max values from scale to get vertical line */
		dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
		dY[1] = MIN_VAL_INP_SPEC_Y_AXIS_DB;

		setCurveData(curve1, dX, dY, 2);
	}
	else
		setCurveData(curve1, NULL, NULL, 0);

	/* Insert actual spectrum data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupInpSpecWaterf()
{
	setTitle(tr("Waterfall Input Spectrum"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	enableAxis(QwtPlot::yLeft, FALSE);

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	/* Clear old plot data */
	clear();

	/* Clear background */
	LastCanvasSize = canvas()->size(); /* Initial canvas size */
	QPixmap Canvas(LastCanvasSize);
	Canvas.fill(backgroundColor());
	canvas()->setBackgroundPixmap(Canvas);
}

void CDRMPlot::SetInpSpecWaterf(CVector<_REAL>& vecrData, CVector<_REAL>&)
{
	int i, iStartScale, iEndScale;
	CVector<_REAL> *pvecrResampledData;

	/* First check if plot must be set up */
	if (InitCharType != INP_SPEC_WATERF)
	{
		InitCharType = INP_SPEC_WATERF;
		SetupInpSpecWaterf();
	}

	/* Calculate sizes */
	const QSize CanvSize = canvas()->size();
	int iLenScale = axisScaleDraw(QwtPlot::xBottom)->length();
	if ((iLenScale > 0) && (iLenScale < CanvSize.width()))
	{
		/* Calculate start and end of scale (needed for the borders) */
		iStartScale = (CanvSize.width() - iLenScale) / 2 - lineWidth();
		iEndScale = iLenScale + iStartScale;
	}
	else
	{
		/* Something went wrong, use safe parameters */
		iStartScale = 0;
		iEndScale = iLenScale = CanvSize.width();
	}

	/* Resample input data */
	Resample.Resample(&vecrData, &pvecrResampledData, iLenScale, TRUE);

	const QPixmap* pBPixmap = canvas()->backgroundPixmap();

	QPixmap Canvas(CanvSize);
	/* In case the canvas width has changed or there is no bitmap, reset
	   background */
	if ((pBPixmap == NULL) || (LastCanvasSize.width() != CanvSize.width()))
		Canvas.fill(backgroundColor());
	else
	{
		/* If height is larger, write background color in new space */
		if (LastCanvasSize.height() < CanvSize.height())
		{
			/* Prepare bitmap for copying background color */
			QPixmap CanvasTMP(CanvSize);
			CanvasTMP.fill(backgroundColor());

			/* Actual copy */
			bitBlt(&Canvas, 0, LastCanvasSize.height(), &CanvasTMP, 0, 0,
				CanvSize.width(), CanvSize.height() - LastCanvasSize.height(),
				Qt::CopyROP);
		}

		/* Move complete block one line further. Use old bitmap */
		bitBlt(&Canvas, 0, 1, pBPixmap, 0, 0,
			CanvSize.width(), CanvSize.height() - 1, Qt::CopyROP);
	}

	/* Store current canvas size */
	LastCanvasSize = CanvSize;

	/* Paint new line (top line) */
	QPainter Painter;
	Painter.begin(&Canvas);

	/* Generate pixel, left of the scale (left border) */
	Painter.setPen(backgroundColor());
	for (i = 0; i < iStartScale; i++)
		Painter.drawPoint(i, 0); /* line 0 -> top line */

	/* The scaling factor */
	const _REAL rScale = _REAL(pvecrResampledData->Size()) / iLenScale;

	/* Actual waterfall data */
	for (i = iStartScale; i < iEndScale; i++)
	{
		/* Init some constants */
		const int iMaxHue = 359; /* Range of "Hue" is 0-359 */
		const int iMaxSat = 255; /* Range of saturation is 0-255 */

		/* Stretch width to entire canvas width */
		const int iCurIdx =
			(int) Round(_REAL(i - iStartScale) * rScale);

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
		Painter.setPen(QColor(iFinalCol, iCurSat, iCurSat, QColor::Hsv));
		Painter.drawPoint(i, 0); /* line 0 -> top line */
	}

	/* Generate pixel, right of scale (right border) */
	Painter.setPen(backgroundColor());
	for (i = iEndScale; i < CanvSize.width(); i++)
		Painter.drawPoint(i, 0); /* line 0 -> top line */

	Painter.end();

	/* Show the bitmap */
	canvas()->setBackgroundPixmap(Canvas);

	replot();
}

void CDRMPlot::SetupFACConst()
{
	/* Init chart for FAC constellation */
	setTitle(tr("FAC Constellation"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale (2 / sqrt(2)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.4142, (double) 1.4142);
	setAxisScale(QwtPlot::yLeft, (double) -1.4142, (double) 1.4142);

	/* Set marker symbol */
	MarkerSym1.setStyle(QwtSymbol::Ellipse);
	MarkerSym1.setSize(4);
	MarkerSym1.setPen(QPen(MainPenColorConst));
	MarkerSym1.setBrush(QBrush(MainPenColorConst));

	/* Insert grid */
	clear();
	SetQAM4Grid();
}

void CDRMPlot::SetFACConst(CVector<_COMPLEX>& veccData)
{
	/* First check if plot must be set up */
	if (InitCharType != FAC_CONSTELLATION)
	{
		InitCharType = FAC_CONSTELLATION;
		SetupFACConst();
	}

	removeMarkers();
	SetData(veccData);
	replot();
}

void CDRMPlot::SetupSDCConst(const ECodScheme eNewCoSc)
{
	/* Init chart for SDC constellation */
	setTitle(tr("SDC Constellation"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale (4 / sqrt(10)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.2649, (double) 1.2649);
	setAxisScale(QwtPlot::yLeft, (double) -1.2649, (double) 1.2649);

	/* Insert grid */
	clear();
	if (eNewCoSc == CS_1_SM)
		SetQAM4Grid();
	else
		SetQAM16Grid();

	/* Set marker symbol */
	MarkerSym1.setStyle(QwtSymbol::Ellipse);
	MarkerSym1.setSize(4);
	MarkerSym1.setPen(QPen(MainPenColorConst));
	MarkerSym1.setBrush(QBrush(MainPenColorConst));
}

void CDRMPlot::SetSDCConst(CVector<_COMPLEX>& veccData, ECodScheme eNewCoSc)
{
	/* Always set up plot. TODO: only set up plot if constellation
	   scheme has changed */
	InitCharType = SDC_CONSTELLATION;
	SetupSDCConst(eNewCoSc);

	removeMarkers();
	SetData(veccData);
	replot();
}

void CDRMPlot::SetupMSCConst(const ECodScheme eNewCoSc)
{
	/* Init chart for MSC constellation */
	setTitle(tr("MSC Constellation"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale (8 / sqrt(42)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.2344, (double) 1.2344);
	setAxisScale(QwtPlot::yLeft, (double) -1.2344, (double) 1.2344);

	/* Insert grid */
	clear();
	if (eNewCoSc == CS_2_SM)
		SetQAM16Grid();
	else
		SetQAM64Grid();

	/* Set marker symbol */
	MarkerSym1.setStyle(QwtSymbol::Ellipse);
	MarkerSym1.setSize(2);
	MarkerSym1.setPen(QPen(MainPenColorConst));
	MarkerSym1.setBrush(QBrush(MainPenColorConst));
}

void CDRMPlot::SetMSCConst(CVector<_COMPLEX>& veccData, ECodScheme eNewCoSc)
{
	/* Always set up plot. TODO: only set up plot if constellation
	   scheme has changed */
	InitCharType = MSC_CONSTELLATION;
	SetupMSCConst(eNewCoSc);

	removeMarkers();
	SetData(veccData);
	replot();
}

void CDRMPlot::SetupAllConst()
{
	/* Init chart for constellation */
	setTitle(tr("MSC / SDC / FAC Constellation"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	enableAxis(QwtPlot::yLeft, TRUE);
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom, (double) -1.5, (double) 1.5);
	setAxisScale(QwtPlot::yLeft, (double) -1.5, (double) 1.5);

	/* Set marker symbols */
	/* MSC */
	MarkerSym1.setStyle(QwtSymbol::Rect);
	MarkerSym1.setSize(2);
	MarkerSym1.setPen(QPen(MainPenColorConst));
	MarkerSym1.setBrush(QBrush(MainPenColorConst));

	/* SDC */
	MarkerSym2.setStyle(QwtSymbol::XCross);
	MarkerSym2.setSize(4);
	MarkerSym2.setPen(QPen(SpecLine1ColorPlot));
	MarkerSym2.setBrush(QBrush(SpecLine1ColorPlot));

	/* FAC */
	MarkerSym3.setStyle(QwtSymbol::Ellipse);
	MarkerSym3.setSize(4);
	MarkerSym3.setPen(QPen(SpecLine2ColorPlot));
	MarkerSym3.setBrush(QBrush(SpecLine2ColorPlot));

	/* Insert "dummy" curves for legend */
	clear();
	curve1 = insertCurve("MSC");
	setCurveSymbol(curve1, MarkerSym1);
	setCurvePen(curve1, QPen(Qt::NoPen));
	enableLegend(TRUE, curve1);

	curve2 = insertCurve("SDC");
	setCurveSymbol(curve2, MarkerSym2);
	setCurvePen(curve2, QPen(Qt::NoPen));
	enableLegend(TRUE, curve2);

	curve3 = insertCurve("FAC");
	setCurveSymbol(curve3, MarkerSym3);
	setCurvePen(curve3, QPen(Qt::NoPen));
	enableLegend(TRUE, curve3);
}

void CDRMPlot::SetAllConst(CVector<_COMPLEX>& veccMSC,
						   CVector<_COMPLEX>& veccSDC,
						   CVector<_COMPLEX>& veccFAC)
{
	/* First check if plot must be set up */
	if (InitCharType != ALL_CONSTELLATION)
	{
		InitCharType = ALL_CONSTELLATION;
		SetupAllConst();
	}

	removeMarkers();

	SetData(veccMSC, veccSDC, veccFAC);

	replot();
}

void CDRMPlot::SetQAM4Grid()
{
	long	curve;
	double	dXMax[2], dYMax[2];
	double	dX[2];

	/* Set scale style */
	QPen ScalePen(MainGridColorPlot, 1, DotLine);

	/* Get bounds of scale */
	dXMax[0] = axisScale(QwtPlot::xBottom)->lBound();
	dXMax[1] = axisScale(QwtPlot::xBottom)->hBound();
	dYMax[0] = axisScale(QwtPlot::yLeft)->lBound();
	dYMax[1] = axisScale(QwtPlot::yLeft)->hBound();

	dX[0] = dX[1] = (double) 0.0;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);
}

void CDRMPlot::SetQAM16Grid()
{
	long	curve;
	double	dXMax[2], dYMax[2];
	double	dX[2];

	/* Set scale style */
	QPen ScalePen(MainGridColorPlot, 1, DotLine);

	/* Get bounds of scale */
	dXMax[0] = axisScale(QwtPlot::xBottom)->lBound();
	dXMax[1] = axisScale(QwtPlot::xBottom)->hBound();
	dYMax[0] = axisScale(QwtPlot::yLeft)->lBound();
	dYMax[1] = axisScale(QwtPlot::yLeft)->hBound();

	dX[0] = dX[1] = (double) 0.0;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) 0.6333;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) -0.6333;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);
}

void CDRMPlot::SetQAM64Grid()
{
	long	curve;
	double	dXMax[2], dYMax[2];
	double	dX[2];

	/* Set scale style */
	QPen ScalePen(MainGridColorPlot, 1, DotLine);

	/* Get bounds of scale */
	dXMax[0] = axisScale(QwtPlot::xBottom)->lBound();
	dXMax[1] = axisScale(QwtPlot::xBottom)->hBound();
	dYMax[0] = axisScale(QwtPlot::yLeft)->lBound();
	dYMax[1] = axisScale(QwtPlot::yLeft)->hBound();

	dX[0] = dX[1] = (double) 0.0;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) 0.3086;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) -0.3086;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) 0.6172;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) -0.6172;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) 0.9258;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) -0.9258;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);
}

void CDRMPlot::OnClicked(const QMouseEvent& e)
{
	/* Get frequency from current cursor position */
	const double dFreq = invTransform(QwtPlot::xBottom, e.x());

	/* Send normalized frequency to receiver */
	const double dMaxxBottom = axisScale(QwtPlot::xBottom)->hBound();

	/* Check if value is valid */
	if (dMaxxBottom != (double) 0.0)
	{
		/* Emit signal containing normalized selected frequency */
		emit xAxisValSet(dFreq / dMaxxBottom);
	}
}

void CDRMPlot::AddWhatsThisHelpChar(const ECharType NCharType)
{
	QString strCurPlotHelp;

	switch (NCharType)
	{
	case AVERAGED_IR:
		/* Impulse Response */
		strCurPlotHelp =
			tr("<b>Impulse Response:</b> This plot shows "
			"the estimated Impulse Response (IR) of the channel based on the "
			"channel estimation. It is the averaged, Hamming Window weighted "
			"Fourier back transformation of the transfer function. The length "
			"of PDS estimation and time synchronization tracking is based on "
			"this function. The two red dashed vertical lines show the "
			"beginning and the end of the guard-interval. The two black dashed "
			"vertical lines show the estimated beginning and end of the PDS of "
			"the channel (derived from the averaged impulse response "
			"estimation). If the \"First Peak\" timing tracking method is "
			"chosen, a bound for peak estimation (horizontal dashed red line) "
			"is shown. Only peaks above this bound are used for timing "
			"estimation.");
		break;

	case TRANSFERFUNCTION:
		/* Transfer Function */
		strCurPlotHelp =
			tr("<b>Transfer Function / Group Delay:</b> "
			"This plot shows the squared magnitude and the group delay of "
			"the estimated channel at each sub-carrier.");
		break;

	case FAC_CONSTELLATION:
	case SDC_CONSTELLATION:
	case MSC_CONSTELLATION:
	case ALL_CONSTELLATION:
		/* Constellations */
		strCurPlotHelp =
			tr("<b>FAC, SDC, MSC:</b> The plots show the "
			"constellations of the FAC, SDC and MSC logical channel of the DRM "
			"stream. Depending on the current transmitter settings, the SDC "
			"and MSC can have 4-QAM, 16-QAM or 64-QAM modulation.");
		break;

	case POWER_SPEC_DENSITY:
		/* Shifted PSD */
		strCurPlotHelp =
			tr("<b>Shifted PSD:</b> This plot shows the "
			"estimated Power Spectrum Density (PSD) of the input signal. The "
			"DC frequency (red dashed vertical line) is fixed at 6 kHz. If "
			"the frequency offset acquisition was successful, the rectangular "
			"DRM spectrum should show up with a center frequency of 6 kHz. "
			"This plot represents the frequency synchronized OFDM spectrum. "
			"If the frequency synchronization was successful, the useful "
			"signal really shows up only inside the actual DRM bandwidth "
			"since the side loops have in this case only energy between the "
			"samples in the frequency domain. On the sample positions outside "
			"the actual DRM spectrum, the DRM signal has zero crossings "
			"because of the orthogonality. Therefore this spectrum represents "
			"NOT the actual spectrum but the \"idealized\" OFDM spectrum.");
		break;

	case SNR_SPECTRUM:
		/* SNR Spectrum (Weighted MER on MSC Cells) */
		strCurPlotHelp =
			tr("<b>SNR Spectrum (Weighted MER on MSC Cells):</b> "
			"This plot shows the Weighted MER on MSC cells for each carrier "
			"separately.");
		break;

	case INPUTSPECTRUM_NO_AV:
		/* Input Spectrum */
		strCurPlotHelp =
			tr("<b>Input Spectrum:</b> This plot shows the "
			"Fast Fourier Transformation (FFT) of the input signal. This plot "
			"is active in both modes, analog and digital. There is no "
			"averaging applied. The screen shot of the Evaluation Dialog shows "
			"the significant shape of a DRM signal (almost rectangular). The "
			"dashed vertical line shows the estimated DC frequency. This line "
			"is very important for the analog AM demodulation. Each time a "
			"new carrier frequency is acquired, the red line shows the "
			"selected AM spectrum. If more than one AM spectrums are within "
			"the sound card frequency range, the strongest signal is chosen.");
		break;

	case INPUT_SIG_PSD:
	case INPUT_SIG_PSD_ANALOG:
		/* Input PSD */
		strCurPlotHelp =
			tr("<b>Input PSD:</b> This plot shows the "
			"estimated power spectral density (PSD) of the input signal. The "
			"PSD is estimated by averaging some Hamming Window weighted "
			"Fourier transformed blocks of the input signal samples. The "
			"dashed vertical line shows the estimated DC frequency.");
		break;

	case AUDIO_SPECTRUM:
		/* Audio Spectrum */
		strCurPlotHelp =
			tr("<b>Audio Spectrum:</b> This plot shows the "
			"averaged audio spectrum of the currently played audio. With this "
			"plot the actual audio bandwidth can easily determined. Since a "
			"linear scale is used for the frequency axis, most of the energy "
			"of the signal is usually concentrated on the far left side of the "
			"spectrum.");
		break;

	case FREQ_SAM_OFFS_HIST:
		/* Frequency Offset / Sample Rate Offset History */
		strCurPlotHelp =
			tr("<b>Frequency Offset / Sample Rate Offset History:"
			"</b> The history "
			"of the values for frequency offset and sample rate offset "
			"estimation is shown. If the frequency offset drift is very small, "
			"this is an indication that the analog front end is of high "
			"quality.");
		break;

	case DOPPLER_DELAY_HIST:
		/* Doppler / Delay History */
		strCurPlotHelp =
			tr("<b>Doppler / Delay History:</b> "
			"The history of the values for the "
			"Doppler and Impulse response length is shown. Large Doppler "
			"values might be responsable for audio drop-outs.");
		break;

	case SNR_AUDIO_HIST:
		/* SNR History */
		strCurPlotHelp =
			tr("<b>SNR History:</b> "
			"The history of the values for the "
			"SNR and correctly decoded audio blocks is shown. The maximum "
			"achievable number of correctly decoded audio blocks per DRM "
			"frame is 10 or 5 depending on the audio sample rate (24 kHz "
			"or 12 kHz AAC core bandwidth).");
		break;

	case INP_SPEC_WATERF:
		/* Waterfall Display of Input Spectrum */
		strCurPlotHelp =
			tr("<b>Waterfall Display of Input Spectrum:</b> "
			"The input spectrum is displayed as a waterfall type. The "
			"different colors represent different levels.");
		break;

	case NONE_OLD:
		break;
	}

	/* Main plot */
	QWhatsThis::add(this, strCurPlotHelp);
}
