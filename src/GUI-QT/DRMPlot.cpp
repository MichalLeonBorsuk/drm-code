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
#include "../util/Settings.h"
#include "../matlib/Matlib.h"
#include "../chanest/ChannelEstimation.h"
#include <limits>
#include <algorithm>
#include <functional>
#include <qwt_scale_engine.h>
#include <qwt_legend.h>
#include <qwt_color_map.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_picker.h>
#include <QIcon>
#include <QFrame>
#include <QLayout>
#include <iostream>

/* Length of the history for synchronization parameters (used for the plot) */
#define LEN_HIST_PLOT_SYNC_PARMS		2250

/* TODO - see if we have lost any dynamic rescaling


Simone: - waterfall spectrum: could be improved, can´t see much of the signal itself,
 it´s all blue
- plotstyle: does not work from settings menu, but is OK from command line


 */

/* Implementation *************************************************************/


class WaterfallColorMap: public QwtColorMap
{
public:
    WaterfallColorMap();
    WaterfallColorMap(const QwtLinearColorMap &);
    virtual ~WaterfallColorMap();
    WaterfallColorMap &operator=(const WaterfallColorMap &);
    virtual QwtColorMap *copy() const;
virtual QRgb rgb(const QwtDoubleInterval &, double value) const;
virtual unsigned char colorIndex(const QwtDoubleInterval &, double value) const;
private:
};

WaterfallColorMap::WaterfallColorMap():QwtColorMap()
{
}

WaterfallColorMap::WaterfallColorMap(const QwtLinearColorMap &m)
:QwtColorMap(m)
{
}

WaterfallColorMap::~WaterfallColorMap()
{
}

WaterfallColorMap& WaterfallColorMap::operator=(const WaterfallColorMap &m)
{
    *(QwtColorMap*)this = *(QwtColorMap*)&m;
	return *this;
}

QwtColorMap *WaterfallColorMap::copy() const
{
    return new WaterfallColorMap;
}

QRgb WaterfallColorMap::rgb(const QwtDoubleInterval &, double value) const
{
    const int iMaxHue = 359; /* Range of "Hue" is 0-359 */
    const int iMaxSat = 255; /* Range of saturation is 0-255 */

    /* Translate dB-values into colors */
    const int iCurCol =
	(int) Round((value - MIN_VAL_INP_SPEC_Y_AXIS_DB) /
	(MAX_VAL_INP_SPEC_Y_AXIS_DB - MIN_VAL_INP_SPEC_Y_AXIS_DB) *
	iMaxHue);

		/* Reverse colors and add some offset (to make it look a bit nicer) */
		const int iColOffset = 60;
		int iFinalCol = iMaxHue - iColOffset - iCurCol;
		if (iFinalCol < 0) /* Prevent from out-of-range */
			iFinalCol = 0;

		/* Also change saturation to get dark colors when low level */
		const int iCurSat = (int) ((1 - (_REAL) iFinalCol / iMaxHue) * iMaxSat);

		return QColor::fromHsv(iFinalCol, iCurSat, iCurSat).rgb();
}

unsigned char WaterfallColorMap::colorIndex(const QwtDoubleInterval&, double) const
{
    return 0;
}


class scaleGen
{
public:
  scaleGen(double step = 1.0, double start=0.0):
    current(start), interval(step) {}
  double operator() () { return current+=interval; }
private:
  double current;
  double interval;
};

template<typename T>
void boundValues(const vector<T>& vec, T& min, T&max)
{
	max = - numeric_limits<T>::max();
	min = numeric_limits<T>::max();

	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] > max)
			max = vec[i];

		if (vec[i] < min)
			min = vec[i];
	}
}

class TransferFunction : unary_function<CComplex,double>
{
public:
    double operator() (CComplex) const;
};

double TransferFunction::operator()(CComplex val) const
{
    const _REAL rNormData = (_REAL) numeric_limits<_SAMPLE>::max() * numeric_limits<_SAMPLE>::max();
    const _REAL rNormSqMagChanEst = SqMag(val) / rNormData;

    if (rNormSqMagChanEst > 0)
	return (_REAL) 10.0 * Log10(rNormSqMagChanEst);
    else
	return RET_VAL_LOG_0;
}

class GroupDelay : unary_function<CComplex,double>
{
public:
    GroupDelay(CComplex init, CReal tu):rOldPhase(Angle(init)),rTu(tu){}
    double operator() (CComplex);
private:
    CReal rOldPhase;
    CReal rTu;
};

double GroupDelay::operator()(CComplex val)
{
    CReal rCurphase = Angle(val);
    CReal rDiffPhase = rCurphase - rOldPhase;

    /* Store phase */
    rOldPhase = rCurphase;

    /* Take care of wrap around of angle() function */
    if (rDiffPhase > WRAP_AROUND_BOUND_GRP_DLY)
	rDiffPhase -= 2.0 * crPi;
    if (rDiffPhase < -WRAP_AROUND_BOUND_GRP_DLY)
	rDiffPhase += 2.0 * crPi;

    /* Apply normalization */
    return rDiffPhase * rTu * 1000.0 /* ms */;
}

CDRMPlot::CDRMPlot(QwtPlot *p, CParameter* param) : QObject(),
	MainPenColorPlot(),MainPenColorConst(),MainGridColorPlot(),
	SpecLine1ColorPlot(),SpecLine2ColorPlot(),
	PassBandColorPlot(),BckgrdColorPlot(),
	leftTitle(), rightTitle(), bottomTitle(),LastCanvasSize(),
	eCurSDCCodingScheme(),eCurMSCCodingScheme(),
	main1curve(NULL), main2curve(NULL),
	DCCarrierCurve(NULL), BandwidthMarkerCurve(NULL),
	curve1(NULL), curve2(NULL), curve3(NULL),
	curve4(NULL), curve5(NULL), curve6(NULL),
	MarkerSymFAC(), MarkerSymSDC(), MarkerSymMSC(),
	grid(NULL), spectrogram(NULL), spectrogramData(),
	CurrentChartType(NONE_OLD),WantedChartType(NONE_OLD),
	bOnTimerCharMutexFlag(false),TimerChart(),Parameters(*param),
	plot(p),styleId(),plotDetails()
{
	grid = new QwtPlotGrid();

	/* Grid defaults */
	grid->enableXMin(false);
	grid->enableYMin(false);
	grid->attach(plot);

	/* Fonts */
	QFont axisfont;
	axisfont.setPointSize(8);
	axisfont.setStyleHint(QFont::SansSerif, QFont::PreferOutline);
	QFont titlefont(axisfont);
	titlefont.setWeight(QFont::Bold);

	plot->setAxisFont(QwtPlot::xBottom, axisfont);
	plot->setAxisFont(QwtPlot::yLeft, axisfont);
	plot->setAxisFont(QwtPlot::yRight, axisfont);
	QwtText title;
	title.setFont(titlefont);
	plot->setTitle(title);

    /* axis titles */
    bottomTitle.setFont(axisfont);
    plot->setAxisTitle(QwtPlot::xBottom, bottomTitle);

    leftTitle.setFont(axisfont);
    plot->setAxisTitle(QwtPlot::yLeft, leftTitle);

    rightTitle.setFont(axisfont);
    plot->setAxisTitle(QwtPlot::yRight, rightTitle);

	/* Global frame */
	plot->setFrameStyle(QFrame::Panel|QFrame::Sunken);
	plot->setLineWidth(2);
	plot->setMargin(10);

	/* Canvas */
	plot->setCanvasLineWidth(0);
	plot->canvas()->setBackgroundRole(QPalette::Window);

	/* Set default style */
	SetPlotStyle(0);

	/* Set marker symbols */
	/* MSC */
	MarkerSymMSC.setStyle(QwtSymbol::Rect);
	MarkerSymMSC.setSize(2);
	MarkerSymMSC.setPen(QPen(MainPenColorConst));
	MarkerSymMSC.setBrush(QBrush(MainPenColorConst));

	/* SDC */
	MarkerSymSDC.setStyle(QwtSymbol::XCross);
	MarkerSymSDC.setSize(4);
	MarkerSymSDC.setPen(QPen(SpecLine1ColorPlot));
	MarkerSymSDC.setBrush(QBrush(SpecLine1ColorPlot));

	/* FAC */
	MarkerSymFAC.setStyle(QwtSymbol::Ellipse);
	MarkerSymFAC.setSize(4);
	MarkerSymFAC.setPen(QPen(SpecLine2ColorPlot));
	MarkerSymFAC.setBrush(QBrush(SpecLine2ColorPlot));

    /* Legend */
    plot->insertLegend(new QwtLegend(), QwtPlot::RightLegend);

    /* For torn off plots */
	plot->setWindowTitle(tr("Chart Window"));
    plot->setWindowIcon(QIcon(":/icons/MainIcon.png"));

	/* Connections */
	QwtPlotPicker* picker = new QwtPlotPicker(
	QwtPlot::xBottom, QwtPlot::yLeft, QwtPicker::PointSelection,
	QwtPicker::NoRubberBand, QwtPicker::AlwaysOff, plot->canvas()
    );
	connect(picker, SIGNAL(selected(const QwtDoublePoint&)), this, SLOT(OnClicked(const QwtDoublePoint&)));
	connect(&TimerChart, SIGNAL(timeout()), this, SLOT(OnTimerChart()));

	TimerChart.stop();

	PlotDetails pd;
	pd.aboutText = 	tr("<b>Input PSD:</b> This plot shows the "
			"estimated power spectral density (PSD) of the input signal. The "
			"PSD is estimated by averaging some Hamming Window weighted "
			"Fourier transformed blocks of the input signal samples. The "
			"dashed vertical line shows the estimated DC frequency.");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME_FAST;
	plotDetails[INPUT_SIG_PSD] = pd;
	plotDetails[INPUT_SIG_PSD_ANALOG] = pd;

	pd.aboutText = 	tr("<b>Waterfall Display of Input Spectrum:</b> "
			"The input spectrum is displayed as a waterfall type. The "
			"different colors represent different levels.");
	pd.timerInterval = GUI_CONTROL_UPDATE_WATERFALL;
	plotDetails[INP_SPEC_WATERF] = pd;

	pd.aboutText = 	tr("<b>SNR Spectrum (Weighted MER on MSC Cells):</b> "
			"This plot shows the Weighted MER on MSC cells for each carrier "
			"separately.");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME_FAST;
	plotDetails[SNR_SPECTRUM] = pd;

	pd.aboutText = 	tr("<b>Shifted PSD:</b> This plot shows the "
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
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME_FAST;
	plotDetails[POWER_SPEC_DENSITY] = pd;

	pd.aboutText = 	tr("<b>Impulse Response:</b> This plot shows "
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
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME_FAST;
	plotDetails[AVERAGED_IR] = pd;

	pd.aboutText = 	tr("<b>Transfer Function / Group Delay:</b> "
			"This plot shows the squared magnitude and the group delay of "
			"the estimated channel at each sub-carrier.");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME_FAST;
	plotDetails[TRANSFERFUNCTION] = pd;

	pd.aboutText = 	tr("<b>Input Spectrum:</b> This plot shows the "
			"Fast Fourier Transformation (FFT) of the input signal. This plot "
			"is active in both modes, analog and digital. There is no "
			"averaging applied. The screen shot of the Evaluation Dialog shows "
			"the significant shape of a DRM signal (almost rectangular). The "
			"dashed vertical line shows the estimated DC frequency. This line "
			"is very important for the analog AM demodulation. Each time a "
			"new carrier frequency is acquired, the red line shows the "
			"selected AM spectrum. If more than one AM spectrums are within "
			"the sound card frequency range, the strongest signal is chosen.");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME;
	plotDetails[INPUTSPECTRUM_NO_AV] = pd;

	pd.aboutText = 	tr("<b>Audio Spectrum:</b> This plot shows the "
			"averaged audio spectrum of the currently played audio. With this "
			"plot the actual audio bandwidth can easily determined. Since a "
			"linear scale is used for the frequency axis, most of the energy "
			"of the signal is usually concentrated on the far left side of the "
			"spectrum.");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME;
	plotDetails[AUDIO_SPECTRUM] = pd;

	pd.aboutText = 	tr("<b>SNR History:</b> "
			"The history of the values for the "
			"SNR and correctly decoded audio blocks is shown. The maximum "
			"achievable number of correctly decoded audio blocks per DRM "
			"frame is 10 or 5 depending on the audio sample rate (24 kHz "
			"or 12 kHz AAC core bandwidth).");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME;
	plotDetails[SNR_AUDIO_HIST] = pd;

	pd.aboutText = 	tr("<b>Frequency Offset / Sample Rate Offset History:"
			"</b> The history "
			"of the values for frequency offset and sample rate offset "
			"estimation is shown. If the frequency offset drift is very small, "
			"this is an indication that the analog front end is of high "
			"quality.");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME;
	plotDetails[FREQ_SAM_OFFS_HIST] = pd;

	pd.aboutText = 	tr("<b>Doppler / Delay History:</b> "
			"The history of the values for the "
			"Doppler and Impulse response length is shown. Large Doppler "
			"values might be responsable for audio drop-outs.");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME;
	plotDetails[DOPPLER_DELAY_HIST] = pd;

	pd.aboutText = 	tr("<b>FAC, SDC, MSC:</b> The plots show the "
			"constellations of the FAC, SDC and MSC logical channel of the DRM "
			"stream. Depending on the current transmitter settings, the SDC "
			"and MSC can have 4-QAM, 16-QAM or 64-QAM modulation.");
	pd.timerInterval = GUI_CONTROL_UPDATE_TIME;
	plotDetails[FAC_CONSTELLATION] = pd;
	plotDetails[SDC_CONSTELLATION] = pd;
	plotDetails[MSC_CONSTELLATION] = pd;
	plotDetails[ALL_CONSTELLATION] = pd;
}

CDRMPlot::~CDRMPlot()
{
    // TODO - do we need stuff here or does QT do it all ?
}

void CDRMPlot::start()
{
    startPlot(CurrentChartType);
    TimerChart.start();
}

void CDRMPlot::stop()
{
    TimerChart.stop();
    endPlot(CurrentChartType);
}

void CDRMPlot::load(const CSettings& s, const string& section)
{
	CWinGeom g;
	s.Get(section, g);
	const QRect WinGeom(g.iXPos, g.iYPos, g.iWSize, g.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
	{
	plot->setGeometry(WinGeom);
	}
	SetPlotStyle(s.Get(section, "plotstyle", 0));
	SetupChart(EPlotType(s.Get(section, "plottype", 0)));
}

bool CDRMPlot::save(CSettings& s, const string& section)
{
    /* Check, if window wasn't closed by the user */
    if (plot->isVisible())
    {
	CWinGeom c;
	const QRect CWGeom = plot->geometry();

	/* Set parameters */
	c.iXPos = CWGeom.x();
	c.iYPos = CWGeom.y();
	c.iHSize = CWGeom.height();
	c.iWSize = CWGeom.width();

	s.Put(section, c);
	/* Convert plot type into an integer type. TODO: better solution */
	s.Put(section, "plottype", (int) GetChartType());
	s.Put(section, "plotstyle", styleId);
	// if its a free standing window, close it
	if(plot->parent() == NULL)
	    plot->close();
	return true;
    }
    return false;
}

void CDRMPlot::SetupChart(const EPlotType eNewType)
{
    /* Set internal variable */
    WantedChartType = eNewType;
}

void CDRMPlot::OnTimerChart()
{
	/* In some cases, if the user moves the mouse very fast over the chart
	   selection list view, this function is called by two different threads.
	   Somehow, using QMutex does not help. Therefore we introduce a flag for
	   doing this job. This solution is a work-around. TODO: better solution */
	if (bOnTimerCharMutexFlag == true)
		return;

	bOnTimerCharMutexFlag = true;

	bool chartChangeNeeded = false;

    ECodScheme e = Parameters.Channel.eSDCmode;
    if(e!=eCurSDCCodingScheme)
    {
	eCurSDCCodingScheme = e;
	chartChangeNeeded = true;
    }

    e = Parameters.Channel.eMSCmode;
    if(e!=eCurMSCCodingScheme)
    {
	eCurMSCCodingScheme = e;
	chartChangeNeeded = true;
    }

    if(CurrentChartType != WantedChartType)
    {
	chartChangeNeeded = true;
    }

    if(chartChangeNeeded)
    {
	SetupChartNow();
	CurrentChartType = WantedChartType;
    }
    UpdateChartNow();
	/* "Unlock" mutex flag */
	bOnTimerCharMutexFlag = false;
}

void CDRMPlot::SetupChartNow()
{
    endPlot(CurrentChartType);
    startPlot(WantedChartType);

	plot->setWhatsThis(plotDetails[WantedChartType].aboutText);
    TimerChart.setInterval(plotDetails[WantedChartType].timerInterval);

    plot->detachItems();

	switch (WantedChartType)
	{
	case AVERAGED_IR:
		SetAvIR();
		break;

	case TRANSFERFUNCTION:
		SetTranFct();
		break;

	case POWER_SPEC_DENSITY:
		SetPSD();
		break;

	case SNR_SPECTRUM:
		SetSNRSpectrum();
		break;

	case INPUTSPECTRUM_NO_AV:
		SetInpSpectrum();
		break;

	case INP_SPEC_WATERF:
		SetInpSpecWaterf();
		break;

	case INPUT_SIG_PSD:
	case INPUT_SIG_PSD_ANALOG:
		SetInpPSD();
		break;

	case AUDIO_SPECTRUM:
		SetAudioSpectrum();
		break;

	case FREQ_SAM_OFFS_HIST:
		SetFreqSamOffsHist();
		break;

	case DOPPLER_DELAY_HIST:
		SetDopplerDelayHist();
		break;

	case SNR_AUDIO_HIST:
		SetSNRAudHist();
		break;

	case FAC_CONSTELLATION:
		SetFACConst();
		break;

	case SDC_CONSTELLATION:
		SetSDCConst();
		break;

	case MSC_CONSTELLATION:
		SetMSCConst();
		break;

	case ALL_CONSTELLATION:
		SetAllConst();
		break;

	case NONE_OLD:
		break;
	}
	CurrentChartType = WantedChartType;
}

void CDRMPlot::UpdateChartNow()
{
	switch (CurrentChartType)
	{
	case AVERAGED_IR:
		UpdateAvIR();
		break;

	case TRANSFERFUNCTION:
		UpdateTranFct();
		break;

	case POWER_SPEC_DENSITY:
		UpdatePSD();
		break;

	case SNR_SPECTRUM:
		UpdateSNRSpectrum();
		break;

	case INPUTSPECTRUM_NO_AV:
		UpdateInpSpectrum();
		break;

	case INP_SPEC_WATERF:
		UpdateInpSpecWaterf();
		break;

	case INPUT_SIG_PSD:
	case INPUT_SIG_PSD_ANALOG:
		UpdateInpPSD();
		break;

	case AUDIO_SPECTRUM:
		UpdateAudioSpectrum();
		break;

	case FREQ_SAM_OFFS_HIST:
		UpdateFreqSamOffsHist();
		break;

	case DOPPLER_DELAY_HIST:
		UpdateDopplerDelayHist();
		break;

	case SNR_AUDIO_HIST:
		UpdateSNRAudHist();
		break;

	case FAC_CONSTELLATION:
		UpdateFACConst();
		break;

	case SDC_CONSTELLATION:
		UpdateSDCConst();
		break;

	case MSC_CONSTELLATION:
		UpdateMSCConst();
		break;

	case ALL_CONSTELLATION:
		UpdateAllConst();
		break;

	case NONE_OLD:
		break;
	}
    plot->replot();
}

void CDRMPlot::startPlot(EPlotType e)
{
    Parameters.Lock();
    const _REAL rDRMFrameDur = GetFrameDuration();

	switch (e)
	{
	case AVERAGED_IR:
	Parameters.Measurements.PIR.subscribe();
		break;
	case TRANSFERFUNCTION:
	Parameters.Measurements.ChannelEstimate.subscribe();
		break;
	case POWER_SPEC_DENSITY:
	Parameters.Measurements.PowerDensitySpectrum.subscribe();
		break;
	case SNR_SPECTRUM:
	Parameters.Measurements.SNRProfile.subscribe();
		break;
	case INPUTSPECTRUM_NO_AV:
	Parameters.Measurements.InputSpectrum.subscribe();
		break;
	case INP_SPEC_WATERF:
	Parameters.Measurements.InputSpectrum.subscribe();
		break;
	case INPUT_SIG_PSD:
	case INPUT_SIG_PSD_ANALOG:
	Parameters.Measurements.PSD.subscribe();
		break;
	case AUDIO_SPECTRUM:
	Parameters.Measurements.AudioSpectrum.subscribe();
		break;
	case FREQ_SAM_OFFS_HIST:
	Parameters.Measurements.FrequencySyncValue.subscribe();
	Parameters.Measurements.SampleFrequencyOffset.subscribe();
	Parameters.Measurements.FrequencySyncValue.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
	Parameters.Measurements.SampleFrequencyOffset.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
		break;
	case DOPPLER_DELAY_HIST:
	Parameters.Measurements.Doppler.subscribe();
	//Parameters.Measurements.Doppler.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
	Parameters.Measurements.Delay.subscribe();
	//Parameters.Measurements.Delay.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
		break;
	case SNR_AUDIO_HIST:
	Parameters.Measurements.SNRHist.subscribe();
	Parameters.Measurements.SNRHist.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
	Parameters.Measurements.CDAudHist.subscribe();
	Parameters.Measurements.CDAudHist.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
		break;
	case FAC_CONSTELLATION:
	Parameters.Measurements.FACVectorSpace.subscribe();
		break;
	case SDC_CONSTELLATION:
	Parameters.Measurements.SDCVectorSpace.subscribe();
		break;
	case MSC_CONSTELLATION:
	Parameters.Measurements.MSCVectorSpace.subscribe();
		break;
	case ALL_CONSTELLATION:
	Parameters.Measurements.FACVectorSpace.subscribe();
	Parameters.Measurements.SDCVectorSpace.subscribe();
	Parameters.Measurements.MSCVectorSpace.subscribe();
		break;
	case NONE_OLD:
		break;
	}
    Parameters.Unlock();
}

void CDRMPlot::endPlot(EPlotType e)
{
    Parameters.Lock();
	switch (e)
	{
	case AVERAGED_IR:
	Parameters.Measurements.PIR.unsubscribe();
		break;
	case TRANSFERFUNCTION:
	Parameters.Measurements.ChannelEstimate.unsubscribe();
		break;
	case POWER_SPEC_DENSITY:
	Parameters.Measurements.PowerDensitySpectrum.unsubscribe();
		break;
	case SNR_SPECTRUM:
	Parameters.Measurements.SNRProfile.unsubscribe();
		break;
	case INPUTSPECTRUM_NO_AV:
	Parameters.Measurements.InputSpectrum.unsubscribe();
		break;
	case INP_SPEC_WATERF:
	Parameters.Measurements.InputSpectrum.unsubscribe();
	spectrogram->detach();
		break;
	case INPUT_SIG_PSD:
	case INPUT_SIG_PSD_ANALOG:
	Parameters.Measurements.PSD.unsubscribe();
		break;
	case AUDIO_SPECTRUM:
	Parameters.Measurements.AudioSpectrum.unsubscribe();
		break;
	case FREQ_SAM_OFFS_HIST:
	Parameters.Measurements.FrequencySyncValue.unsubscribe();
	Parameters.Measurements.SampleFrequencyOffset.unsubscribe();
		break;
	case DOPPLER_DELAY_HIST:
	Parameters.Measurements.Doppler.unsubscribe();
	Parameters.Measurements.Delay.unsubscribe();
		break;
	case SNR_AUDIO_HIST:
	Parameters.Measurements.SNRHist.unsubscribe();
	Parameters.Measurements.CDAudHist.unsubscribe();
		break;
	case FAC_CONSTELLATION:
	Parameters.Measurements.FACVectorSpace.unsubscribe();
		break;
	case SDC_CONSTELLATION:
	Parameters.Measurements.SDCVectorSpace.unsubscribe();
		break;
	case MSC_CONSTELLATION:
	Parameters.Measurements.MSCVectorSpace.unsubscribe();
		break;
	case ALL_CONSTELLATION:
	Parameters.Measurements.FACVectorSpace.unsubscribe();
	Parameters.Measurements.SDCVectorSpace.unsubscribe();
	Parameters.Measurements.MSCVectorSpace.unsubscribe();
		break;
	case NONE_OLD:
		break;
	}
    Parameters.Unlock();
}

void CDRMPlot::SetPlotStyle(const int iNewStyleID)
{
	QColor BckgrdColorPlot;

    styleId = iNewStyleID;

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
	grid->setMajPen(QPen(MainGridColorPlot, 0, Qt::DotLine));
	grid->setMinPen(QPen(MainGridColorPlot, 0, Qt::DotLine));
	plot->setCanvasBackground(BckgrdColorPlot);

	/* Make sure that plot are being initialized again */
	WantedChartType = NONE_OLD;
}

/* Duration of OFDM symbol */
_REAL CDRMPlot::GetSymbolDuration()
{
     return _REAL(Parameters.CellMappingTable.iFFTSizeN + Parameters.CellMappingTable.iGuardSize)
	/
	_REAL(SOUNDCRD_SAMPLE_RATE);
}

/* Duration of DRM frame */
_REAL CDRMPlot::GetFrameDuration()
{
	return _REAL(Parameters.CellMappingTable.iNumSymPerFrame) * GetSymbolDuration();
}

void CDRMPlot::SetData(QwtPlotCurve* curve, vector<_COMPLEX>& veccData)
{
	const int iPoints = veccData.size();
	/* Copy data from vector into a temporary array */
	double *pdX = new double[iPoints];
	double *pdY = new double[iPoints];
	for (int i = 0; i < iPoints; i++)
	{
		pdX[i] = veccData[i].real();
		pdY[i] = veccData[i].imag();
	}
	curve->setData(pdX, pdY, iPoints);
	delete[] pdX;
	delete[] pdY;
}

void CDRMPlot::SetAvIR()
{
    plot->setTitle(tr("Channel Impulse Response"));
    plot->enableAxis(QwtPlot::yRight, false);

    grid = new QwtPlotGrid();
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [ms]"));

    /* Fixed vertical scale  */
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("IR [dB]"));
    plot->setAxisScale(QwtPlot::yLeft, -20.0, 40.0);

    /* Insert curves */
    curve1 = new QwtPlotCurve(tr("Guard-interval beginning"));
    curve2 = new QwtPlotCurve(tr("Guard-interval end"));
    curve3 = new QwtPlotCurve(tr("Estimated begin of impulse response"));
    curve4 = new QwtPlotCurve(tr("Estimated end of impulse response"));
    curve1->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve2->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve3->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve4->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));

    curve1->setItemAttribute(QwtPlotItem::Legend, false);
    curve2->setItemAttribute(QwtPlotItem::Legend, false);
    curve3->setItemAttribute(QwtPlotItem::Legend, false);
    curve4->setItemAttribute(QwtPlotItem::Legend, false);

    curve1->attach(plot);
    curve2->attach(plot);
    curve3->attach(plot);
    curve4->attach(plot);

    curve5 = new QwtPlotCurve(tr("Higher Bound"));
    curve5->setItemAttribute(QwtPlotItem::Legend, false);
#ifdef _DEBUG_
    curve6 = new QwtPlotCurve(tr("Lower bound"));
    curve5->setPen(QPen(SpecLine1ColorPlot));
    curve6->setPen(QPen(SpecLine2ColorPlot));
    curve6->setItemAttribute(QwtPlotItem::Legend, false);
    curve5->attach(plot);
    curve6->attach(plot);
#else
    curve5->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve5->attach(plot);
#endif

    /* Add main curve */
    main1curve = new QwtPlotCurve(tr("Channel Impulse Response"));

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main1curve->setItemAttribute(QwtPlotItem::Legend, false);
    main1curve->attach(plot);
}

void CDRMPlot::UpdateAvIR()
{
    CMeasurements::CPIR pir;
    /* Get data from module */
    Parameters.Lock();
    bool b = Parameters.Measurements.PIR.get(pir);
    Parameters.Unlock();
    if(!b)
    {
	return; // data not being generated yet
    }

	if(pir.data.size() == 0)
	return;

    /* Vertical bounds -------------------------------------------------- */
    double dX[2], dY[2];

    dY[0] = plot->axisScaleDiv(QwtPlot::yLeft)->lowerBound();
    dY[1] = plot->axisScaleDiv(QwtPlot::yLeft)->upperBound();

    /* These bounds show the beginning and end of the guard-interval */

    /* Left bound */
    dX[0] = dX[1] = pir.rStartGuard;
    curve1->setData(dX, dY, 2);

    /* Right bound */
    dX[0] = dX[1] = pir.rEndGuard;
    curve2->setData(dX, dY, 2);

    /* Estimated begin of impulse response */
    dX[0] = dX[1] = pir.rPDSBegin;
    curve3->setData(dX, dY, 2);

    /* Estimated end of impulse response */
    dX[0] = dX[1] = pir.rPDSEnd;
    curve4->setData(dX, dY, 2);

    /* Data for the actual impulse response curve */
    vector<_REAL> vecrScale(pir.data.size());
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(pir.rStep, pir.rStart));
    main1curve->setData(&vecrScale[0], &pir.data[0], vecrScale.size());

    /* Horizontal bounds ------------------------------------------------ */
    /* These bounds show the peak detection bound from timing tracking */
    dX[0] = vecrScale[0];
    dX[1] = vecrScale[vecrScale.size() - 1];

#ifdef _DEBUG_
    /* Lower bound */
    dY[0] = dY[1] = rLowerB;
    setCurveData(curve6, dX, dY, 2);

    /* Higher bound */
    dY[0] = dY[1] = rHigherB;
#else
    /* Only include highest bound */
    dY[0] = dY[1] = max(pir.rHigherBound, pir.rLowerBound);
#endif
    curve5->setData(dX, dY, 2);

    /* Adjust scale for x-axis */
    plot->setAxisScale(QwtPlot::xBottom, vecrScale[0], vecrScale[vecrScale.size() - 1]);
}

void CDRMPlot::SetTranFct()
{
    plot->setTitle(tr("Channel Transfer Function / Group Delay"));

    grid = new QwtPlotGrid();
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

    plot->setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));

    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("TF [dB]"));
    plot->setAxisScale(QwtPlot::yLeft,  -85.0,  -35.0);

    plot->enableAxis(QwtPlot::yRight);
    plot->setAxisTitle(QwtPlot::yRight, tr("Group Delay [ms]"));
    plot->setAxisScale(QwtPlot::yRight, -50.0, 50.0);

    /* Add curves */

    /* TODO - check that its group delay that should be scaled to right axis!! */
    main1curve = new QwtPlotCurve(tr("Transf. Fct."));
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    main2curve = new QwtPlotCurve(tr("Group Del."));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setYAxis(QwtPlot::yRight);

    main1curve->attach(plot);
    main2curve->attach(plot);
}

void CDRMPlot::UpdateTranFct()
{
    vector<double> transferFunc, groupDelay, scale;
	vector<_COMPLEX> veccChanEst;

    Parameters.Lock();
    bool b = Parameters.Measurements.ChannelEstimate.get(veccChanEst);
    int iFFTSizeN = Parameters.CellMappingTable.iFFTSizeN;
    Parameters.Unlock();

    if(b==false || veccChanEst.size()==0)
	return; // not running yet

    int iNumCarrier = veccChanEst.size();

    transferFunc.resize(iNumCarrier);
    transform(
	veccChanEst.begin(), veccChanEst.end(),
	transferFunc.begin(),
	TransferFunction()
    );

	groupDelay.resize(iNumCarrier);
    transform(veccChanEst.begin(), veccChanEst.end(),
	groupDelay.begin(),
	GroupDelay(veccChanEst[0], CReal(iFFTSizeN) / CReal(SOUNDCRD_SAMPLE_RATE))
    );

	scale.resize(transferFunc.size());
    generate(scale.begin(), scale.end(), scaleGen());

	plot->setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) scale.size());

	main1curve->setData(&scale[0], &transferFunc[0], scale.size());
	main2curve->setData(&scale[0], &groupDelay[0], scale.size());
}

void CDRMPlot::SetAudioSpectrum()
{
    plot->setTitle(tr("Audio Spectrum"));
    plot->enableAxis(QwtPlot::yRight, false);

    grid = new QwtPlotGrid();
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, "AS [dB]");

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::yLeft, (double) -90.0, (double) -20.0);
    double dBandwidth = (double) SOUNDCRD_SAMPLE_RATE / 2400; /* 20.0 for 48 kHz */
    if (dBandwidth < 20.0)
	dBandwidth = (double) 20.0;

    plot->setAxisScale(QwtPlot::xBottom, (double) 0.0, dBandwidth);

    /* Add main curve */
    main1curve = new QwtPlotCurve(tr("Audio Spectrum"));
    main1curve->setItemAttribute(QwtPlotItem::Legend, false);
    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main1curve->attach(plot);
}

void CDRMPlot::UpdateAudioSpectrum()
{
    /* Get data from module */
    vector<_REAL> vecrData;
    Parameters.Lock();
    bool b = Parameters.Measurements.AudioSpectrum.get(vecrData);
    Parameters.Unlock();
    if(b)
    {
	vector<_REAL> vecrScale(vecrData.size());
	const _REAL rFactorScale = (_REAL)SOUNDCRD_SAMPLE_RATE/vecrData.size()/2000.0;
	generate(vecrScale.begin(), vecrScale.end(), scaleGen(rFactorScale));
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
    }
}


void CDRMPlot::SetFreqSamOffsHist()
{
    plot->setTitle(tr("Rel. Frequency Offset / Sample Rate Offset History"));
    plot->enableAxis(QwtPlot::yRight);

    grid = new QwtPlotGrid();
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [s]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yRight, tr("Sample Rate Offset [Hz]"));
    plot->enableAxis(QwtPlot::yLeft, true);

    /* Add main curves */
    main1curve = new QwtPlotCurve(tr("Freq."));
    main2curve = new QwtPlotCurve(tr("Samp."));
    main2curve->setYAxis(QwtPlot::yRight);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    main1curve->attach(plot);
    main2curve->attach(plot);

    // initial values
    plot->setAxisScale(QwtPlot::yLeft, -10.0, 10.0);
    plot->setAxisScale(QwtPlot::yRight, -5.0, 5.0);

    // fixed X-axis
	Parameters.Lock();
	/* Duration of OFDM symbol */
	//const _REAL rTs = GetSymbolDuration(); TODO ?
	Parameters.Unlock();
}

// TODO - get starting point at right of chart
void CDRMPlot::UpdateFreqSamOffsHist()
{

    vector<double> vecrFreqOffs, vecrSamOffs;

	Parameters.Lock();
	/* Duration of OFDM symbol */
	const _REAL rTs = GetSymbolDuration();
	/* Value from frequency acquisition */
	const _REAL rFreqOffsetAcqui = Parameters.rFreqOffsetAcqui * SOUNDCRD_SAMPLE_RATE;

    Parameters.Measurements.FrequencySyncValue.get(vecrFreqOffs);
    Parameters.Measurements.SampleFrequencyOffset.get(vecrSamOffs);

	Parameters.Unlock();

	plot->setAxisTitle(QwtPlot::yLeft, tr("Freq. Offset [Hz] rel. to ")
	+QString().setNum(rFreqOffsetAcqui)+" Hz");

	vector<double> vecrScale(LEN_HIST_PLOT_SYNC_PARMS, 0.0);
	/* Calculate time scale */
#if 1
	const _REAL rLeft = rTs*(1- LEN_HIST_PLOT_SYNC_PARMS);
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rTs, rLeft));
#else
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rTs;
#endif
	plot->setAxisScale(QwtPlot::xBottom, vecrScale[0], 0.0);

	/* Customized auto-scaling. We adjust the y scale so that it
	 is not larger than rMinScaleRange"  */
	const _REAL rMinScaleRange = 1.0; /* Hz */

    if(vecrFreqOffs.size()>0)
    {

	/* Get maximum and minimum values */
	double MaxFreq = -numeric_limits<double>::max();
	double MinFreq = numeric_limits<double>::max();

	for (size_t i = 0; i < vecrFreqOffs.size(); i++)
	{
	    if (vecrFreqOffs[i] > MaxFreq)
		MaxFreq = vecrFreqOffs[i];

	    if (vecrFreqOffs[i] < MinFreq)
		MinFreq = vecrFreqOffs[i];
	}
	/* Apply scale to plot */
	plot->setAxisScale(QwtPlot::yLeft,
		floor(MinFreq / rMinScaleRange), ceil(MaxFreq / rMinScaleRange));
	main1curve->setData(&vecrScale[vecrScale.size()-vecrFreqOffs.size()],
				&vecrFreqOffs[0], vecrFreqOffs.size());
    }

    if(vecrSamOffs.size()>0)
    {
	double MaxSam = -numeric_limits<double>::max();
	double MinSam = numeric_limits<double>::max();
	for (size_t i = 0; i < vecrSamOffs.size(); i++)
	{
	    if (vecrSamOffs[i] > MaxSam)
		MaxSam = vecrSamOffs[i];

	    if (vecrSamOffs[i] < MinSam)
		MinSam = vecrSamOffs[i];
	}
	plot->setAxisScale(QwtPlot::yRight,
		floor(MinSam / rMinScaleRange), ceil(MaxSam / rMinScaleRange));
	main2curve->setData(&vecrScale[vecrScale.size()-vecrSamOffs.size()],
				&vecrSamOffs[0], vecrSamOffs.size());
    }
}

void CDRMPlot::SetDopplerDelayHist()
{
    plot->setTitle(tr("Delay / Doppler History"));
    plot->enableAxis(QwtPlot::yRight);

    grid = new QwtPlotGrid();
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Delay [ms]"));
    plot->setAxisTitle(QwtPlot::yRight, tr("Doppler [Hz]"));

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::yLeft, (double) 0.0, (double) 10.0);
    plot->setAxisScale(QwtPlot::yRight, (double) 0.0, (double) 4.0);

    /* Add main curves */
    main1curve = new QwtPlotCurve(tr("Delay"));
    main2curve = new QwtPlotCurve(tr("Doppler"));
    main2curve->setYAxis(QwtPlot::yRight);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    main1curve->attach(plot);
    main2curve->attach(plot);
}

void CDRMPlot::UpdateDopplerDelayHist()
{
    vector<double> vecrDelay, vecrDoppler, vecrScale;
	Parameters.Lock();
	_REAL rStep = GetFrameDuration()/60.0;
	Parameters.Measurements.Delay.get(vecrDelay);
	Parameters.Measurements.Doppler.get(vecrDoppler);
	Parameters.Unlock();

	_REAL rStart = -(rStep*_REAL(LEN_HIST_PLOT_SYNC_PARMS-1));
    plot->setAxisScale(QwtPlot::xBottom, rStart, 0.0);
	vecrScale.resize(vecrDelay.size());
	_REAL rLeft = -_REAL(vecrDelay.size()-1)*rStep;
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
	main1curve->setData(&vecrScale[0], &vecrDelay[0], vecrDelay.size());
	vecrScale.resize(vecrDoppler.size());
    rLeft = -_REAL(vecrDoppler.size()-1)*rStep;
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
	main2curve->setData(&vecrScale[0], &vecrDoppler[0], vecrDoppler.size());
}

void CDRMPlot::SetSNRAudHist()
{
    plot->setTitle(tr("SNR / Correctly Decoded Audio History"));
    plot->enableAxis(QwtPlot::yRight);

    grid = new QwtPlotGrid();
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("SNR [dB]"));
    plot->setAxisTitle(QwtPlot::yRight, tr("Corr. Dec. Audio / DRM-Frame"));

    /* Add main curves */
    main1curve = new QwtPlotCurve(tr("SNR"));
    main2curve = new QwtPlotCurve(tr("Audio"));
    main2curve->setYAxis(QwtPlot::yRight);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    main1curve->attach(plot);
    main2curve->attach(plot);
}

void CDRMPlot::UpdateSNRAudHist()
{
    vector<double> vecrSNR, vecrAudio, vecrScale;
	/* Duration of DRM frame */
	Parameters.Lock();
	_REAL rStep = GetFrameDuration()/60.0;
    Parameters.Measurements.SNRHist.get(vecrSNR);
    vector<int> v;
    Parameters.Measurements.CDAudHist.get(v);
    vecrAudio.resize(v.size());
    vecrAudio.assign(v.begin(), v.end());
	Parameters.Unlock();

	/* Customized auto-scaling. We adjust the y scale maximum so that it
	   is not more than "rMaxDisToMax" to the curve */
	const int iMaxDisToMax = 5; /* dB */
	const int iMinValueSNRYScale = 15; /* dB */

	/* Get maximum value */
	_REAL MaxSNR = numeric_limits<_REAL>::min();
	for (size_t i = 0; i < vecrSNR.size(); i++)
	{
		if (vecrSNR[i] > MaxSNR)
			MaxSNR = vecrSNR[i];
	}

	/* Quantize scale to a multiple of "iMaxDisToMax" */
	double dMaxYScaleSNR = ceil(MaxSNR / _REAL(iMaxDisToMax)) * _REAL(iMaxDisToMax);

	/* Bound at the minimum allowed value */
	if (dMaxYScaleSNR < double(iMinValueSNRYScale))
		dMaxYScaleSNR = double(iMinValueSNRYScale);

	/* Ratio between the maximum values for audio and SNR. The ratio should be
	   chosen so that the audio curve is not in the same range as the SNR curve
	   under "normal" conditions to increase readability of curves.
	   Since at very low SNRs, no audio can received anyway so we do not have to
	   check whether the audio y-scale is in range of the curve */
	const double rRatioAudSNR = 1.5;
	const double dMaxYScaleAudio = dMaxYScaleSNR * rRatioAudSNR;

	_REAL rStart = -(rStep*_REAL(LEN_HIST_PLOT_SYNC_PARMS-1));

	/* Apply scale to plot */
	plot->setAxisScale(QwtPlot::yLeft, 0.0, dMaxYScaleSNR);
	plot->setAxisScale(QwtPlot::yRight, 0.0, dMaxYScaleAudio);
	plot->setAxisScale(QwtPlot::xBottom, rStart, 0.0);

    if(vecrSNR.size()>0)
    {
	vecrScale.resize(vecrSNR.size());
	_REAL rLeft = -_REAL(vecrSNR.size()-1)*rStep;
	generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
	main1curve->setData(&vecrScale[0], &vecrSNR[0], vecrSNR.size());
    }
    if(vecrAudio.size()>0)
    {
	vecrScale.resize(vecrAudio.size());
	_REAL rLeft = -_REAL(vecrAudio.size()-1)*rStep;
	generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
	main2curve->setData(&vecrScale[0], &vecrAudio[0], vecrAudio.size());
    }
}

void CDRMPlot::SpectrumPlotDefaults(
    const QString& title, const QString& axistitle, uint penwidth)
{
	plot->setTitle(title);
	plot->enableAxis(QwtPlot::yRight, false);

	grid = new QwtPlotGrid();
	grid->enableXMin(false);
	grid->enableYMin(false);
	grid->enableX(true);
	grid->enableY(true);
	grid->attach(plot);

	plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	plot->enableAxis(QwtPlot::yLeft, true);
	plot->setAxisTitle(QwtPlot::yLeft, axistitle+" [dB]");

	/* Fixed scale */
	plot->setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	plot->setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
		MAX_VAL_INP_SPEC_Y_AXIS_DB);

	/* Insert line for DC carrier */
	DCCarrierCurve = new QwtPlotCurve(tr("DC carrier"));
	DCCarrierCurve->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
	DCCarrierCurve->setItemAttribute(QwtPlotItem::Legend, false);
	DCCarrierCurve->attach(plot);

	/* Add main curve */
	main1curve = new QwtPlotCurve(axistitle);
	main1curve->setPen(QPen(MainPenColorPlot, penwidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	main1curve->setItemAttribute(QwtPlotItem::Legend, false);
	main1curve->attach(plot);
}

void CDRMPlot::SetDCCarrier(double dVal)
{
	double dX[2], dY[2];
	dX[0] = dX[1] = dVal / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
	dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB;

	DCCarrierCurve->setData(dX, dY, 2);
}

void CDRMPlot::SetPSD()
{
    /* Init chart for power spectral density estimation */
    SpectrumPlotDefaults(
	tr("Shifted Power Spectral Density of Input Signal"),
	tr("Shifted PSD"), 1);

    /* fixed values for DC Carrier line */
    SetDCCarrier(VIRTUAL_INTERMED_FREQ);
}

void CDRMPlot::UpdatePSD()
{
    vector<_REAL> vecrData;
    Parameters.Lock();
    bool b = Parameters.Measurements.PowerDensitySpectrum.get(vecrData);
    Parameters.Unlock();
    if(b)
    {
	vector<_REAL> vecrScale(vecrData.size());
	_REAL rFactorScale = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(vecrData.size()) / 2000.0;
	generate(vecrScale.begin(), vecrScale.end(), scaleGen(rFactorScale));
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
    }
}

void CDRMPlot::SetSNRSpectrum()
{
    plot->setTitle(tr("SNR Spectrum (Weighted MER on MSC Cells)"));
    plot->enableAxis(QwtPlot::yRight, false);

    grid = new QwtPlotGrid();
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

    plot->setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("WMER [dB]"));

    /* Add main curve */
    main1curve = new QwtPlotCurve(tr("SNR Spectrum"));

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main1curve->setItemAttribute(QwtPlotItem::Legend, false);

    main1curve->attach(plot);
}

void CDRMPlot::UpdateSNRSpectrum()
{
    vector<_REAL> vecrData;
    Parameters.Lock();
    bool b = Parameters.Measurements.SNRProfile.get(vecrData);
    Parameters.Unlock();
    if(b == false)
	return;

	const int iSize = vecrData.size();

    vector<_REAL> vecrScale(iSize);
    // TODO - do we want to count excluded carriers ?
    generate(vecrScale.begin(), vecrScale.end(), scaleGen());

	/* Fixed scale for x-axis */
	plot->setAxisScale(QwtPlot::xBottom, 0.0, double(iSize));

	/* Fixed / variable scale (if SNR is in range, use fixed scale otherwise
	   enlarge scale) */
	/* Get maximum value */
	_REAL rMaxSNR = numeric_limits<_REAL>::min();
	for (int i = 0; i < iSize; i++)
	{
		if (vecrData[i] > rMaxSNR)
			rMaxSNR = vecrData[i];
	}

	double dMaxScaleYAxis = MAX_VAL_SNR_SPEC_Y_AXIS_DB;

	if (rMaxSNR > dMaxScaleYAxis)
	{
		const double rEnlareStep = 10.0; /* dB */
		dMaxScaleYAxis = ceil(rMaxSNR / rEnlareStep) * rEnlareStep;
	}

	/* Set scale */
	plot->setAxisScale(QwtPlot::yLeft, MIN_VAL_SNR_SPEC_Y_AXIS_DB, dMaxScaleYAxis);

	/* Set actual data */
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
}


void CDRMPlot::SetInpSpectrum()
{

    SpectrumPlotDefaults(tr("Input Spectrum"), tr("Input Spectrum"), 1);
}

void CDRMPlot::UpdateInpSpectrum()
{
    vector<_REAL> vecrData, vecrScale;
    SetDCCarrier(Parameters.GetDCFrequency());
    Parameters.Lock();
    Parameters.Measurements.InputSpectrum.get(vecrData);
    Parameters.Unlock();
    _REAL rStep = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(vecrData.size()) / 2000.0;
	vecrScale.resize(vecrData.size());
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep));
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
}

void CDRMPlot::SetInpPSD()
{
    if(WantedChartType==INPUT_SIG_PSD_ANALOG)
    {
	/* Insert line for bandwidth marker behind main curves */
	BandwidthMarkerCurve = new QwtPlotCurve(tr("Filter bandwidth"));

	BandwidthMarkerCurve->setBrush(PassBandColorPlot);
	BandwidthMarkerCurve->setItemAttribute(QwtPlotItem::Legend, false);
	BandwidthMarkerCurve->attach(plot);
    }
    SpectrumPlotDefaults(tr("Input PSD"), tr("Input PSD"), 2);
}

void CDRMPlot::UpdateInpPSD()
{
    _REAL rStart, rStep;
    vector<_REAL> vecrData;
    // read it from the parameter structure
    Parameters.Lock();
    bool psdOk = Parameters.Measurements.PSD.get(vecrData);
    bool etsi = Parameters.Measurements.bETSIPSD;
    Parameters.Unlock();
    if(psdOk==false)
	return;
    if(etsi) // if the RSCI output is turned on we display that version
    {
	// starting frequency and frequency step as defined in TS 102 349
	// plot expects the scale values in kHz
	rStart = _REAL(-7.875) + VIRTUAL_INTERMED_FREQ/_REAL(1000.0);
	rStep =_REAL(0.1875);

    }
    else // Traditional Dream values
    {
	rStart = 0.0;
	rStep = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(vecrData.size()) / 2000.0;
    }
    vector<_REAL> vecrScale(vecrData.size());
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rStart));

    if(WantedChartType==INPUT_SIG_PSD_ANALOG)
    {
	_REAL r;
	Parameters.Lock();
	bool b = Parameters.Measurements.AnalogCurMixFreqOffs.get(r);

	/* Insert marker for filter bandwidth if required */
	_REAL rBWCenter, rBWWidth;
	Parameters.Measurements.AnalogBW.get(rBWWidth);
	Parameters.Measurements.AnalogCenterFreq.get(rBWCenter);
	Parameters.Unlock();
	if(b)
	    SetDCCarrier(r);
	if (rBWWidth != (_REAL) 0.0)
	{
	    double dX[4], dY[4];
	    dX[0] = dX[1] = (rBWCenter - rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;
	    dX[2] = dX[3] = (rBWCenter + rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;

	    /* Take the min-max values from scale to get vertical line */
	    dY[0] = MAX_VAL_INP_SPEC_Y_AXIS_DB;
	    dY[1] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
	    dY[2] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
	    dY[3] = MAX_VAL_INP_SPEC_Y_AXIS_DB;
	    BandwidthMarkerCurve->setData(dX, dY, 4);
	}
	else
	    BandwidthMarkerCurve->setData(NULL, NULL, 0);
    }
    else
    {
	SetDCCarrier(Parameters.GetDCFrequency());
    }
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
}

QwtRasterData *SpectrogramData::copy() const
{
    SpectrogramData* c = new SpectrogramData();
    c->height = height;
    c->data = data;
    c->scale = scale;
    return c;
}

double SpectrogramData::value(double x, double y) const
{
    size_t row, col;
    // need to interpolate ?
    // first map the y value to index the row
    row = height - size_t(y);
    // then see if we have data for it
    if(row>=data.size())
	return MIN_VAL_INP_SPEC_Y_AXIS_DB; // lowest possible dB value
    // then find the x value in scale
    const vector<double>& rowdata = data.at(row);
    //col = size_t(x/boundingRect().width()*double(rowdata.size()));
    col = size_t(x*170.0);

    if(col>=rowdata.size())
	return rowdata.at(rowdata.size()-1);
    // return the value
    return rowdata.at(col);
}

void SpectrogramData::setHeight(size_t h)
{
    height = h;
    // left top width height
    setBoundingRect(QwtDoubleRect(0.0, 0.0, _REAL(SOUNDCRD_SAMPLE_RATE) / 2000.0, h));
}

void SpectrogramData::setData(vector<double>& row)
{
    data.push_front(row);
    if(data.size()>height)
	data.pop_back();
}

void CDRMPlot::SetInpSpecWaterf()
{
    const QSize CanvSize = plot->canvas()->size();

    LastCanvasSize = CanvSize;

    plot->setTitle(tr("Waterfall Input Spectrum"));
    plot->enableAxis(QwtPlot::yRight, false);
    plot->enableAxis(QwtPlot::yLeft, false);
    plot->enableAxis(QwtPlot::xBottom, true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    // spectrogram uses scales to determine value range
    // each pixel represents one sample (400 ms ?)
    plot->setAxisScale(QwtPlot::yLeft, 0.0, CanvSize.height());
    plot->setAxisScale(QwtPlot::xBottom, 0.0, _REAL(SOUNDCRD_SAMPLE_RATE) / 2000.0);
    //grid->enableX(false);
    //grid->enableY(false);

    // TODO check with cvs plot->Layout()->setAlignCanvasToScales(true);
    spectrogramData.setHeight(CanvSize.height());
    if(spectrogram == NULL)
	spectrogram = new QwtPlotSpectrogram();

    spectrogram->setColorMap(*new WaterfallColorMap);
    spectrogram->attach(plot);
}

void CDRMPlot::UpdateInpSpecWaterf()
{
    vector<_REAL> vecrData;
    Parameters.Lock();
    Parameters.Measurements.InputSpectrum.get(vecrData);
    Parameters.Unlock();
    if(vecrData.size()==0)
    {
	return;
    }
    _REAL rStart = 0.0;
    _REAL rStep = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(vecrData.size()) / 2000.0;
    vector<_REAL> vecrScale(vecrData.size());
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rStart));
    spectrogramData.setData(vecrData);
    spectrogram->setData(spectrogramData);
}

void CDRMPlot::ConstellationPlotDefaults(const QString& title, double lim, int n)
{
    double limit = int(10.0*lim)/10.0; // one decimal place
    plot->setTitle(title+" "+tr("Constellation"));
    plot->enableAxis(QwtPlot::yRight, false);

    grid = new QwtPlotGrid();
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(plot);

	plot->setAxisTitle(QwtPlot::xBottom, tr("Real"));
	plot->enableAxis(QwtPlot::yLeft, true);
	plot->setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));
	double step = limit/n;
	plot->setAxisScale(QwtPlot::xBottom, -limit, limit, step);
	plot->setAxisScale(QwtPlot::yLeft, -limit, limit, step);
}

QwtPlotCurve* CDRMPlot::ScatterCurve(const QString& title, const QwtSymbol& s)
{
    QwtPlotCurve* curve = new QwtPlotCurve(title);
    curve->setSymbol(s);
    curve->setPen(QPen(Qt::NoPen));
    curve->setItemAttribute(QwtPlotItem::Legend, false);
    curve->attach(plot);
    return curve;
}

void CDRMPlot::SetFACConst()
{
    ConstellationPlotDefaults(tr("FAC"), 2.0 / sqrt(2.0), 1);
    curve1 = ScatterCurve("FAC", MarkerSymFAC);
}

void CDRMPlot::UpdateFACConst()
{
    vector<_COMPLEX> veccData;
    Parameters.Lock();
    bool b = Parameters.Measurements.FACVectorSpace.get(veccData);
    Parameters.Unlock();
    if(b)
	SetData(curve1, veccData);
}

void CDRMPlot::SetSDCConst()
{
    ConstellationPlotDefaults("SDC", 4.0 / sqrt(10.0), (eCurSDCCodingScheme == CS_1_SM)?1:2);
    curve2 = ScatterCurve("SDC", MarkerSymSDC);
}

void CDRMPlot::UpdateSDCConst()
{
    vector<_COMPLEX> veccData;
    Parameters.Lock();
    bool b = Parameters.Measurements.SDCVectorSpace.get(veccData);
    Parameters.Unlock();
    if(b)
	SetData(curve2, veccData);
}

void CDRMPlot::SetMSCConst()
{
    /* Fixed scale (8 / sqrt(42)) */
    ConstellationPlotDefaults("MSC", 8.0 / sqrt(42.0), (eCurMSCCodingScheme == CS_2_SM)?2:4);
    curve3 = ScatterCurve("MSC", MarkerSymMSC);
}

void CDRMPlot::UpdateMSCConst()
{
    vector<_COMPLEX> veccData;
    Parameters.Lock();
    bool b = Parameters.Measurements.MSCVectorSpace.get(veccData);
    Parameters.Unlock();
    if(b)
	SetData(curve3, veccData);
}

void CDRMPlot::SetAllConst()
{
    ConstellationPlotDefaults(tr("MSC / SDC / FAC"), 1.5, 4);
    curve1 = ScatterCurve("FAC", MarkerSymFAC);
    curve2 = ScatterCurve("SDC", MarkerSymSDC);
    curve3 = ScatterCurve("MSC", MarkerSymMSC);
    curve1->setItemAttribute(QwtPlotItem::Legend, true);
    curve2->setItemAttribute(QwtPlotItem::Legend, true);
    curve3->setItemAttribute(QwtPlotItem::Legend, true);
}

void CDRMPlot::UpdateAllConst()
{
    UpdateFACConst();
    UpdateSDCConst();
    UpdateMSCConst();
}

void CDRMPlot::OnClicked(const QwtDoublePoint& p)
{
	/* Get frequency from current cursor position */
	const double dFreq = p.x();

	/* Send normalized frequency to receiver */
	const double dMaxxBottom = plot->axisScaleDiv(QwtPlot::xBottom)->upperBound();
	/* Check if value is valid */
	if (dMaxxBottom != (double) 0.0)
	{
		/* Emit signal containing normalized selected frequency */
		emit xAxisValSet(dFreq / dMaxxBottom);
	}
}
