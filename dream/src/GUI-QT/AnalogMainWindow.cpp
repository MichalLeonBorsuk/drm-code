/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additional widgets for displaying AMSS information
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

#include "AnalogMainWindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QMotifStyle>
#include <QStringListModel>
#include <qwt_dial_needle.h>

#include "ReceiverSettingsDlg.h"
#include "StationsDlg.h"
#include "LiveScheduleDlg.h"
#include "DRMPlot.h"
#include "../AMSSDemodulation.h"

/* Implementation *************************************************************/
AnalogMainWindow::AnalogMainWindow(ReceiverInterface& NDRMR, CSettings& NSettings,
	QWidget* parent, Qt::WFlags f):
    QMainWindow(parent, f), Ui_AnalogMainWindow(),
	Receiver(NDRMR), Settings(NSettings),
	pReceiverSettingsDlg(NULL), stationsDlg(NULL), liveScheduleDlg(NULL),
	plot(NULL),Timer(),TimerPLLPhaseDial(),
	AMSSDlg(NDRMR, Settings, this, f), quitWanted(true),
	bgDemod(), bgAGC(), bgNoiseRed()
{
    setupUi(this);

	/* Set help text for the controls */
	AddWhatsThisHelp();

    pReceiverSettingsDlg = new ReceiverSettingsDlg(Receiver, Settings, this);

	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

	/* Settings menu  ------------------------------------------------------- */
	connect(actionFM, SIGNAL(triggered()), this, SLOT(OnSwitchToFM()));
	connect(actionDRM, SIGNAL(triggered()), this, SLOT(OnSwitchToDRM()));
	connect(actionAM, SIGNAL(triggered()), this, SLOT(OnNewAMAcquisition()));
	connect(actionReceiver_Settings, SIGNAL(triggered()), pReceiverSettingsDlg, SLOT(show()));

	/* Stations window */
	stationsDlg = new StationsDlg(Receiver, Settings, false, this, "", false, Qt::WindowMinMaxButtonsHint);
	connect(actionStations, SIGNAL(triggered()), stationsDlg, SLOT(show()));

	/* Live Schedule window */
	liveScheduleDlg = new LiveScheduleDlg(Receiver, Settings, this, "", false, Qt::WindowMinMaxButtonsHint);
	connect(actionAFS, SIGNAL(triggered()), liveScheduleDlg, SLOT(show()));

	/* Init main plot */
	plot = new CDRMPlot(SpectrumPlot, Receiver.GetAnalogParameters());
	plot->SetPlotStyle(Settings.Get("System Evaluation Dialog", "plotstyle", 0));
	plot->SetupChart(CDRMPlot::INPUT_SIG_PSD_ANALOG);
	connect(plot, SIGNAL(xAxisValSet(double)), this, SLOT(OnChartxAxisValSet(double)));

	SliderBandwidth->setRange(0, SOUNDCRD_SAMPLE_RATE / 2);
	SliderBandwidth->setTickPosition(QSlider::TicksBothSides);
	SliderBandwidth->setTickInterval(1000); /* Each kHz a tick */
	SliderBandwidth->setPageStep(1000); /* Hz */

	/* Init PLL phase dial control */
	PhaseDial->setMode(QwtDial::RotateNeedle);
	PhaseDial->setWrapping(true);
	PhaseDial->setReadOnly(true);
	PhaseDial->setScale(0, 360, 45); /* Degrees */
	PhaseDial->setOrigin(270);
	PhaseDial->setNeedle(new QwtDialSimpleNeedle(QwtDialSimpleNeedle::Arrow));
	PhaseDial->setFrameShadow(QwtDial::Plain);
	PhaseDial->setScaleOptions(QwtDial::ScaleTicks);

	/* Update controls */
	UpdateControls();

	/* Connect controls ----------------------------------------------------- */
	connect(pushButtonFM, SIGNAL(clicked()), this, SLOT(OnSwitchToFM()));
	connect(ButtonDRM, SIGNAL(clicked()), this, SLOT(OnSwitchToDRM()));
	connect(ButtonAMSS, SIGNAL(clicked()), this, SLOT(OnButtonAMSS()));
	connect(ButtonWaterfall, SIGNAL(clicked()), this, SLOT(OnButtonWaterfall()));

	/* Button groups */
	bgDemod.addButton(RadioButtonDemNBFM, int(NBFM));
	bgDemod.addButton(RadioButtonDemCW, int(CW));
	bgDemod.addButton(RadioButtonDemUSB, int(USB));
	bgDemod.addButton(RadioButtonDemLSB, int(LSB));
	bgDemod.addButton(RadioButtonDemAM, int(AM));
	bgAGC.addButton(RadioButtonAGCOff, int(AT_NO_AGC));
	bgAGC.addButton(RadioButtonAGCSlow, int(AT_SLOW));
	bgAGC.addButton(RadioButtonAGCMed, int(AT_MEDIUM));
	bgAGC.addButton(RadioButtonAGCFast, int(AT_FAST));
	bgNoiseRed.addButton(RadioButtonNoiRedOff, int(NR_OFF));
	bgNoiseRed.addButton(RadioButtonNoiRedLow, int(NR_LOW));
	bgNoiseRed.addButton(RadioButtonNoiRedMed, int(NR_MEDIUM));
	bgNoiseRed.addButton(RadioButtonNoiRedHigh, int(NR_HIGH));
	connect(&bgDemod, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioDemodulation(int)));
	connect(&bgAGC, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioAGC(int)));
	connect(&bgNoiseRed, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioNoiRed(int)));

	/* Slider */
	connect(SliderBandwidth, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderBWChange(int)));

	/* Check boxes */
	connect(CheckBoxMuteAudio, SIGNAL(clicked()), this, SLOT(OnCheckBoxMuteAudio()));
	connect(CheckBoxReverb, SIGNAL(clicked()), this, SLOT(OnCheckBoxReverb()));
	connect(CheckBoxSaveAudioWave, SIGNAL(clicked()), this, SLOT(OnCheckSaveAudioWav()));
	connect(CheckBoxAutoFreqAcq, SIGNAL(clicked()), this, SLOT(OnCheckAutoFreqAcq()));

	connect(PLLButton, SIGNAL(clicked ()), this, SLOT(OnCheckPLL()));

	/* Timers */
	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()), this, SLOT(OnTimerPLLPhaseDial()));

	/* Don't activate real-time timers, wait for show event */
    Timer.stop();
    TimerPLLPhaseDial.stop();
    plot->stop();
}

void AnalogMainWindow::showEvent(QShowEvent*)
{
	CWinGeom s;
	Settings.Get("AnalogGUI", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	OnTimer();

	/* default close action is to exit */
    quitWanted = true;

	/* Activate real-time timer */
	Timer.start(GUI_CONTROL_UPDATE_TIME);

	/* Open AMSS window */
	if (Settings.Get("AnalogGUI", "AMSSvisible", false) == true)
		AMSSDlg.show();
	else
		AMSSDlg.hide();

	if(Settings.Get("AnalogGUI", "Stationsvisible", false))
	stationsDlg->show();

	if(Settings.Get("AnalogGUI", "AFSvisible", false))
	liveScheduleDlg->show();

    PLLButton->setChecked(Settings.Get("AnalogGUI", "pll", true));

    try {
 	plot->start();
    } catch(const char* e)
    {
	QMessageBox::information(this, "Problem", e);
    }

	UpdateControls();
}

void AnalogMainWindow::hideEvent(QHideEvent*)
{
	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

	/* Close windows */
	Settings.Put("AnalogGUI", "AMSSvisible", AMSSDlg.isVisible());
	AMSSDlg.hide();
	Settings.Put("AnalogGUI", "Stationsvisible", stationsDlg->isVisible());
	stationsDlg->hide();
	Settings.Put("AnalogGUI", "AFSvisible", liveScheduleDlg->isVisible());
	liveScheduleDlg->hide();

    Settings.Put("AnalogGUI", "pll", PLLButton->isChecked());

    plot->stop();

	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("AnalogGUI", s);
}

void AnalogMainWindow::closeEvent(QCloseEvent* ce)
{
    if(quitWanted)
    {
	if(!Receiver.End())
	{
	    QMessageBox::critical(this, "Dream", tr("Exit"), tr("Termination of working thread failed"));
	}
	qApp->quit();
    }
	ce->accept();
}

void AnalogMainWindow::OnSwitchToDRM()
{
    CParameter& Parameter = *Receiver.GetAnalogParameters();
    Parameter.Lock();
    Parameter.eModulation = DRM;
    Parameter.RxEvent = ChannelReconfiguration;
    Parameter.Unlock();
}

void AnalogMainWindow::OnSwitchToFM()
{
    CParameter& Parameter = *Receiver.GetAnalogParameters();
    Parameter.Lock();
    Parameter.eModulation = WBFM;
    Parameter.RxEvent = ChannelReconfiguration;
    Parameter.Unlock();
}

void AnalogMainWindow::UpdateControls()
{
    CParameter& Parameter = *Receiver.GetAnalogParameters();
    Parameter.Lock();
    EModulationType eModulation = Parameter.eModulation;
    Parameter.Unlock();
	/* Set demodulation type */
	switch (eModulation)
	{
	case AM:
		if (!RadioButtonDemAM->isChecked())
			RadioButtonDemAM->setChecked(true);
		break;

	case LSB:
		if (!RadioButtonDemLSB->isChecked())
			RadioButtonDemLSB->setChecked(true);
		break;

	case USB:
		if (!RadioButtonDemUSB->isChecked())
			RadioButtonDemUSB->setChecked(true);
		break;

	case CW:
		if (!RadioButtonDemCW->isChecked())
			RadioButtonDemCW->setChecked(true);
		break;

	case NBFM:
		if (!RadioButtonDemNBFM->isChecked())
			RadioButtonDemNBFM->setChecked(true);
		break;

	case WBFM:
	case DRM: case NONE:
		break;
	}

	/* Set AGC type */
	switch (Receiver.GetAnalogAGCType())
	{
	case AT_NO_AGC:
		if (!RadioButtonAGCOff->isChecked())
			RadioButtonAGCOff->setChecked(true);
		break;

	case AT_SLOW:
		if (!RadioButtonAGCSlow->isChecked())
			RadioButtonAGCSlow->setChecked(true);
		break;

	case AT_MEDIUM:
		if (!RadioButtonAGCMed->isChecked())
			RadioButtonAGCMed->setChecked(true);
		break;

	case AT_FAST:
		if (!RadioButtonAGCFast->isChecked())
			RadioButtonAGCFast->setChecked(true);
		break;
	}

	/* Set noise reduction type */
	switch (Receiver.GetAnalogNoiseReductionType())
	{
	case NR_OFF:
		if (!RadioButtonNoiRedOff->isChecked())
			RadioButtonNoiRedOff->setChecked(true);
		break;

	case NR_LOW:
		if (!RadioButtonNoiRedLow->isChecked())
			RadioButtonNoiRedLow->setChecked(true);
		break;

	case NR_MEDIUM:
		if (!RadioButtonNoiRedMed->isChecked())
			RadioButtonNoiRedMed->setChecked(true);
		break;

	case NR_HIGH:
		if (!RadioButtonNoiRedHigh->isChecked())
			RadioButtonNoiRedHigh->setChecked(true);
		break;
	}

	/* Set filter bandwidth */
	SliderBandwidth->setValue(Receiver.GetAnalogFilterBWHz());
	TextLabelBandWidth->setText(QString().setNum(Receiver.GetAnalogFilterBWHz())+tr(" Hz"));

	/* Update check boxes */
	CheckBoxMuteAudio->setChecked(Receiver.GetMuteAudio());
	CheckBoxSaveAudioWave->setChecked(Receiver.GetIsWriteWaveFile());

	CheckBoxAutoFreqAcq->
		setChecked(Receiver.AnalogAutoFreqAcqEnabled());

    PLLButton->setChecked(Receiver.AnalogPLLEnabled());
}

void AnalogMainWindow::UpdatePlotStyle()
{
	/* Update main plot window */
	plot->SetPlotStyle(Settings.Get("System Evaluation Dialog", "plotstyle", 0));
}

void AnalogMainWindow::OnTimer()
{
	bool b;
	_REAL r;

    CParameter& Parameter = *Receiver.GetAnalogParameters();
    Parameter.Lock();
    EModulationType eModulation = Parameter.eModulation;
    Parameter.Unlock();
	switch(eModulation)
	{
	case DRM:
	    quitWanted = false;
	    close();
		    break;
	    case AM: case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
		    /* Carrier frequency of AM signal */
	    b = Receiver.GetAnalogParameters()->Measurements.AnalogCurMixFreqOffs.get(r);
	    if(b)
	    {
		TextFreqOffset->setText(tr("Carrier<br>Frequency:<br><b>")
		+ QString().setNum(r, 'f', 2) + " Hz</b>");
	    }
	    else
	    {
		TextFreqOffset->setText(tr("Carrier Frequency not available"));
	    }
	    UpdateControls();
	    break;
	case NONE:
		    break;
	}
	if(!AMSSDlg.isVisible())
	ButtonAMSS->setChecked(false);
}

void AnalogMainWindow::OnTimerPLLPhaseDial()
{
    if(!Receiver.AnalogPLLEnabled()) // probablly not needed - maybe dangerous ?
    {
	PLLButton->setChecked(false);
	return;
    }

	CReal rCurPLLPhase;

	if (Receiver.GetAnalogPLLPhase(rCurPLLPhase) == true)
	{
		/* Set current PLL phase (convert radiant in degree) */
		PhaseDial->setValue(rCurPLLPhase * (CReal) 360.0 / (2 * crPi));
	PhaseDial->setEnabled(true);
	}
	else
	{
		/* Reset dial */
		PhaseDial->setValue((CReal) 0.0);
	PhaseDial->setEnabled(false);
	}
}

void AnalogMainWindow::OnRadioDemodulation(int iID)
{
	/* Receiver takes care of setting appropriate filter BW */
    CParameter& Parameter = *Receiver.GetAnalogParameters();
    Parameter.Lock();
    Parameter.eModulation = EModulationType(iID);
    Parameter.RxEvent = ChannelReconfiguration;
    Parameter.Unlock();

	/* Update controls */
	UpdateControls();
}

void AnalogMainWindow::OnRadioAGC(int iID)
{
	Receiver.SetAnalogAGCType(EType(iID));
}

void AnalogMainWindow::OnRadioNoiRed(int iID)
{
	Receiver.SetAnalogNoiseReductionType(ENoiRedType(iID));
}

void AnalogMainWindow::OnSliderBWChange(int value)
{
	/* Set new filter in processing module */
	Receiver.SetAnalogFilterBWHz(value);
	TextLabelBandWidth->setText(QString().setNum(value) + tr(" Hz"));
}

void AnalogMainWindow::OnCheckAutoFreqAcq()
{
	/* Set parameter in working thread module */
	Receiver.EnableAnalogAutoFreqAcq(CheckBoxAutoFreqAcq->isChecked());
}

void AnalogMainWindow::OnCheckPLL()
{
	/* Set parameter in working thread module */
	if(PLLButton->isChecked())
	{
	Receiver.EnableAnalogPLL(true);
	PhaseDial->setEnabled(true);
	OnTimerPLLPhaseDial();
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);
	}
	else
	{
	Receiver.EnableAnalogPLL(false);
	PhaseDial->setEnabled(false);
	TimerPLLPhaseDial.stop();
	}

}

void AnalogMainWindow::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	Receiver.MuteAudio(CheckBoxMuteAudio->isChecked());
}

void AnalogMainWindow::OnCheckBoxReverb()
{
	/* Set parameter in working thread module */
	Receiver.SetReverbEffect(CheckBoxReverb->isChecked());
}

void AnalogMainWindow::OnCheckSaveAudioWav()
{
	OnSaveAudio(this, CheckBoxSaveAudioWave, Receiver);
}

void AnalogMainWindow::OnChartxAxisValSet(double dVal)
{
	/* Set new frequency in receiver module */
	Receiver.SetAnalogDemodAcq(dVal);
}

void AnalogMainWindow::OnButtonWaterfall()
{
	/* Toggle between normal spectrum plot and waterfall spectrum plots */
	if (ButtonWaterfall->isChecked())
		plot->SetupChart(CDRMPlot::INP_SPEC_WATERF);
	else
		plot->SetupChart(CDRMPlot::INPUT_SIG_PSD_ANALOG);
}

void AnalogMainWindow::OnButtonAMSS()
{
    if(ButtonAMSS->isChecked())
    {
	/* Open AMSS window and set in foreground */
	AMSSDlg.show();
	AMSSDlg.raise();
    }
    else
    {
	AMSSDlg.hide();
    }
}

void AnalogMainWindow::OnNewAMAcquisition()
{
    CParameter& Parameter = *Receiver.GetAnalogParameters();
    Parameter.Lock();
    Parameter.RxEvent = Reinitialise;
    Parameter.Unlock();
}

void AnalogMainWindow::AddWhatsThisHelp()
{
	/* Noise Reduction */
	const QString strNoiseReduction =
		tr("<b>Noise Reduction:</b> The noise suppression is a frequency "
		"domain optimal filter design based algorithm. The noise PSD is "
		"estimated utilizing a minimum statistic. A problem of this type of "
		"algorithm is that it produces the so called \"musical tones\". The "
		"noise becomes colored and sounds a bit strange. At the same time, "
		"the useful signal (which might be speech or music) is also "
		"distorted by the algorithm. By selecting the level of noise "
		"reduction, a compromise between distortion of the useful signal "
		"and actual noise reduction can be made.");

	ButtonGroupNoiseReduction->setWhatsThis(strNoiseReduction);
	RadioButtonNoiRedOff->setWhatsThis(strNoiseReduction);
	RadioButtonNoiRedLow->setWhatsThis(strNoiseReduction);
	RadioButtonNoiRedMed->setWhatsThis(strNoiseReduction);
	RadioButtonNoiRedHigh->setWhatsThis(strNoiseReduction);

	/* Automatic Gain Control */
	const QString strAGC =
		tr("<b>AGC (Automatic Gain Control):</b> Input signals can have a "
		"large variation in power due to channel impairments. To compensate "
		"for that, an automatic gain control can be applied. The AGC has "
		"four settings: Off, Slow, Medium and Fast.");

	ButtonGroupAGC->setWhatsThis(strAGC);
	RadioButtonAGCOff->setWhatsThis(strAGC);
	RadioButtonAGCSlow->setWhatsThis(strAGC);
	RadioButtonAGCMed->setWhatsThis(strAGC);
	RadioButtonAGCFast->setWhatsThis(strAGC);

	/* Filter Bandwidth */
	const QString strFilterBW =
		tr("<b>Filter Bandwidth:</b> A band-pass filter is applied before "
		"the actual demodulation process. With this filter, adjacent signals "
		"are attenuated. The bandwidth of this filter can be chosen in steps "
		"of 1 Hz by using the slider bar. Clicking on the right or left side "
		"of the slider leveler will increase/decrease the bandwidth by 1 kHz. "
		"<br>The current filter bandwidth is indicated in the spectrum plot "
		"by a selection bar.");

	ButtonGroupBW->setWhatsThis(strFilterBW);
	TextLabelBandWidth->setWhatsThis(strFilterBW);
	SliderBandwidth->setWhatsThis(strFilterBW);

	/* Demodulation type */
	const QString strDemodType =
		tr("<b>Demodulation Type:</b> The following analog "
		"demodulation types are available:<ul>"
		"<li><b>AM:</b> This analog demodulation type is used in most of "
		"the hardware radios. The envelope of the complex base-band signal "
		"is used followed by a high-pass filter to remove the DC offset. "
		"Additionally, a low pass filter with the same bandwidth as the "
		"pass-band filter is applied to reduce the noise caused by "
		"non-linear distortions.</li>"
		"<li><b>LSB / USB:</b> These are single-side-band (SSB) demodulation "
		"types. Only one side of the spectrum is evaluated, the upper side "
		"band is used in USB and the lower side band with LSB. It is "
		"important for SSB demodulation that the DC frequency of the analog "
		"signal is known to get satisfactory results. The DC frequency is "
		"automatically estimated by starting a new acquisition or by "
		"clicking on the plot.</li>"
		"<li><b>CW:</b> This demodulation type can be used to receive "
		"CW signals. Only a narrow frequency band in a fixed distance "
		"to the mixing frequency is used. By clicking on the spectrum "
		"plot, the center position of the band pass filter can be set.</li>"
		"<li><b>FM:</b> This is a narrow band frequency demodulation.</li>"
		"</ul>");

	ButtonGroupDemodulation->setWhatsThis(strDemodType);
	RadioButtonDemAM->setWhatsThis(strDemodType);
	RadioButtonDemLSB->setWhatsThis(strDemodType);
	RadioButtonDemUSB->setWhatsThis(strDemodType);
	RadioButtonDemCW->setWhatsThis(strDemodType);
	RadioButtonDemNBFM->setWhatsThis(strDemodType);

	/* Mute Audio (same as in systemevaldlg.cpp!) */
	CheckBoxMuteAudio->setWhatsThis(
		tr("<b>Mute Audio:</b> The audio can be muted by "
		"checking this box. The reaction of checking or unchecking this box "
		"is delayed by approx. 1 second due to the audio buffers."));

	/* Save audio as wave (same as in systemevaldlg.cpp!) */
	CheckBoxSaveAudioWave->setWhatsThis(
		tr("<b>Save Audio as WAV:</b> Save the audio signal "
		"as stereo, 16-bit, 48 kHz sample rate PCM wave file. Checking this "
		"box will let the user choose a file name for the recording."));

	/* Carrier Frequency */
	TextFreqOffset->setWhatsThis(
		tr("<b>Carrier Frequency:</b> The (estimated) carrier frequency of the "
		"analog signal is shown. (The estimation of this parameter can be done "
		"by the Autom Frequency Acquisition which uses the estimated PSD of "
		"the input signal and applies a maximum search.)"));

	/* Phase lock loop */
	const QString strPLL =
		tr("<b>PLL:</b> The Phase-Lock-Loop (PLL) tracks the carrier of the "
		"modulated received signal. The resulting phase offset between the "
		"reference oscillator and the received carrier is displayed in "
		"a dial control. If the pointer is almost steady, the PLL is locked. "
		"If the pointer of the dial control turns quickly, the PLL is "
		"out of lock. To get the PLL locked, the frequency offset to "
		"the true carrier frequency must not exceed a few Hz.");

	PhaseDial->setWhatsThis(strPLL);
	PLLButton->setWhatsThis(strPLL);

	/* Auto frequency acquisition */
	const QString strAutoFreqAcqu =
		tr("<b>Auto Frequency Acquisition:</b> Clicking on the "
		"input spectrum plot changes the mixing frequency for demodulation. "
		"If the Auto Frequency Acquisition is enabled, the largest peak "
		"near the cursor is selected.");

	GroupBoxAutoFreqAcq->setWhatsThis(strAutoFreqAcqu);
	CheckBoxAutoFreqAcq->setWhatsThis(strAutoFreqAcqu);

}

void AnalogMainWindow::OnHelpWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

/******************************************************************************\
* AMSS controls                                                                *
\******************************************************************************/
/*
	Added by Andrew Murphy, BBC Research & Development, 2005

	Additional widgets have been added to display the AMSS service label,
	language etc. in in a similar style to that used for DRM reception.
	A display has also been added to show the status of the AMSS decoding.
	Everytime an AMSS CRC passes (for block or block 2) the 47 decoded
	bits are displayed. Note this could also include 'false' passes.

	The percentage of the current data entity group or 'SDC' is displayed
	along with which parts of the data entity group have been decoded. A
	'#' indicates that a data entity gruop segment is yet to be received
	whilst a 'c' or 'C' indicates a CRC pass for the block 2 carrying that
	particular segment.

	Added phase offset display for AMSS demodulation loop.
*/
CAMSSDlg::CAMSSDlg(AnalogReceiverInterface& R, CSettings& NSettings,
	QWidget* parent, Qt::WFlags f) : QDialog(parent, f), Ui_AMSSDlg(),
	Receiver(R), Settings(NSettings)
{
    setupUi(this);

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Init AMSS PLL phase dial control */
	PhaseDialAMSS->setMode(QwtDial::RotateNeedle);
	PhaseDialAMSS->setWrapping(true);
	PhaseDialAMSS->setReadOnly(true);
	PhaseDialAMSS->setScale(0, 360, 45); /* Degrees */
	PhaseDialAMSS->setOrigin(270);
	PhaseDialAMSS->setNeedle(new QwtDialSimpleNeedle(QwtDialSimpleNeedle::Arrow));
	PhaseDialAMSS->setFrameShadow(QwtDial::Plain);
	PhaseDialAMSS->setScaleOptions(QwtDial::ScaleTicks);

	TextAMSSServiceLabel->setText("");
	TextAMSSCountryCode->setText("");
	TextAMSSTimeDate->setText("");
	TextAMSSLanguage->setText("");
	TextAMSSServiceID->setText("");
	TextAMSSAMCarrierMode->setText("");
	TextAMSSInfo->setText("");

	ListWidgetAMSSAFSList->setEnabled(false);


	/* Connect controls ----------------------------------------------------- */
	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
		this, SLOT(OnTimerPLLPhaseDial()));

	/* set the progress bar style */
	ProgressBarAMSS->setStyle( new QMotifStyle() );

}

void CAMSSDlg::hideEvent(QHideEvent*)
{
	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("AMSS Dialog", s);
}

void CAMSSDlg::showEvent(QShowEvent*)
{
	CWinGeom s;
	Settings.Get("AMSS Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	OnTimer();
	OnTimerPLLPhaseDial();

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);
}

void CAMSSDlg::OnTimer()
{
	CParameter& Parameters = *Receiver.GetAnalogParameters();
	Parameters.Lock();
	const CService& Service = Parameters.Service[0];

	/* Show label if available */
	if ((Service.IsActive()) && (Service.strLabel != ""))
	{
		/* Service label (UTF-8 encoded string -> convert) */
		TextAMSSServiceLabel->setText(QString().fromUtf8(
	    Service.strLabel.c_str()));
	}
	else
		TextAMSSServiceLabel->setText(tr(""));

	/* Country code */
	const string strCntryCode = Service.strCountryCode; /* must be of 2 lowercase chars */

	if ((Service.IsActive()) && (!strCntryCode.empty()) && (strCntryCode != "--"))
	{
		TextAMSSCountryCode->
			setText(QString(GetISOCountryName(strCntryCode).c_str()));
	}
	else
		TextAMSSCountryCode->setText("");

	/* SDC Language code */

	if (Service.IsActive())
	{
		const string strLangCode = Service.strLanguageCode; /* must be of 3 lowercase chars */

		if ((!strLangCode.empty()) && (strLangCode != "---"))
			 TextAMSSLanguage->
				setText(QString(GetISOLanguageName(strLangCode).c_str()));
		else
			TextAMSSLanguage->setText(QString(strTableLanguageCode[Service.iLanguage].c_str()));
	}
	else
		TextAMSSLanguage->setText("");

	/* Time, date */
	if ((Parameters.iUTCHour == 0) &&
		(Parameters.iUTCMin == 0) &&
		(Parameters.iDay == 0) &&
		(Parameters.iMonth == 0) &&
		(Parameters.iYear == 0))
	{
		/* No time service available */
		TextAMSSTimeDate->setText("");
	}
	else
	{
		QDateTime DateTime;
		DateTime.setDate(QDate(Parameters.iYear, Parameters.iMonth, Parameters.iDay));
		DateTime.setTime(QTime(Parameters.iUTCHour, Parameters.iUTCMin));

		TextAMSSTimeDate->setText(DateTime.toString());
	}

	/* Get number of alternative services */
	const uint32_t sid = Parameters.Service[0].iServiceID;
	const CAltFreqSign AltFreqSign = Parameters.ServiceInformation[sid].AltFreqSign;

	const size_t iNumAltServices = AltFreqSign.vecOtherServices.size();

	if (iNumAltServices != 0)
	{
		ListWidgetAMSSAFSList->insertItem(10, QString().setNum((long) iNumAltServices));

		ListWidgetAMSSAFSList->clear();
		ListWidgetAMSSAFSList->setEnabled(true);

		QString freqEntry;

		for (size_t i = 0; i < iNumAltServices; i++)
		{
			const COtherService& OtherService = AltFreqSign.vecOtherServices[i];
			switch (OtherService.iSystemID)
			{
			case 0:
				freqEntry = "DRM:";
				break;

			case 1:
			case 2:
				freqEntry = "AM:   ";
				break;

			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				freqEntry = "FM:   ";
				break;

			default:
				freqEntry = "";
				break;
			}

			const int iNumAltFreqs = OtherService.veciFrequencies.size();

			switch (OtherService.iSystemID)
			{
			case 0:
			case 1:
			case 2:
				/* AM or DRM, freq in kHz */
				for (int j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((long)
						OtherService.
						veciFrequencies[j], 10);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " kHz";

				if (OtherService.iSystemID == 0 || OtherService.iSystemID == 1)
				{
					freqEntry += " ID:";
					freqEntry +=
						QString().setNum((long)
						OtherService.
						iServiceID, 16).toUpper();
				}
				break;

			case 3:
			case 4:
			case 5:
				/* 'FM1 frequency' - 87.5 to 107.9 MHz (100 kHz steps) */
				for (int j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((float) (87.5 + 0.1 * OtherService.veciFrequencies[j]), 'f', 1);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " MHz";

				if (OtherService.iSystemID == 3)
				{
					freqEntry += " ECC+PI:";
					freqEntry +=
						QString().setNum((long)
						OtherService.
						iServiceID, 16).toUpper();
				}

				if (OtherService.iSystemID == 4)
				{
					freqEntry += " PI:";
					freqEntry +=
						QString().setNum((long)
						OtherService.
						iServiceID, 16).toUpper();
				}
				break;

			case 6:
			case 7:
			case 8:
				/* 'FM2 frequency'- 76.0 to 90.0 MHz (100 kHz steps) */
				for (int j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((float) (76.0 + 0.1 *
							OtherService.veciFrequencies[j]), 'f', 1);
					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " MHz";

				if (OtherService.iSystemID == 6)
				{
					freqEntry += " ECC+PI:";
					freqEntry +=
						QString().setNum((long) OtherService.iServiceID, 16).toUpper();
				}

				if (OtherService.iSystemID == 7)
				{
					freqEntry += " PI:";
					freqEntry +=
						QString().setNum((long) OtherService.iServiceID, 16).toUpper();
				}
				break;

			default:
				freqEntry = "DAB";
				break;
			}

			if (OtherService.bSameService)
			{
				freqEntry += " (same service)";
			}
			else
			{
				freqEntry += " (alt service)";
			}

			ListWidgetAMSSAFSList->insertItem(0, freqEntry);
		}
	}
	else
	{
		ListWidgetAMSSAFSList->clear();
		ListWidgetAMSSAFSList->setEnabled(false);
	}

	TextAMSSServiceID->setText("");
	TextAMSSAMCarrierMode->setText("");

	if (Receiver.GetAMSSLockStatus() == NO_SYNC
	|| Service.iServiceID == SERV_ID_NOT_USED
	)
	{
		TextAMSSInfo->setText(tr("No AMSS detected"));
	}
	else
	{
		TextAMSSInfo->setText(tr("Awaiting AMSS data..."));

		/* Display 'block 1' info */
		if (Receiver.GetAMSSBlock1Status())
		{
			TextAMSSInfo->setText("");

			TextAMSSLanguage->setText(QString(strTableLanguageCode[Service.iLanguage].c_str()));

			TextAMSSServiceID->setText("ID:" + QString().setNum(
				(long) Service.iServiceID, 16).toUpper());

			TextAMSSAMCarrierMode->setText(QString(strTableAMSSCarrierMode[Parameters.iAMSSCarrierMode].c_str()));
		}
	}

	Parameters.Unlock();

	TextDataEntityGroupStatus->setText(Receiver.GetAMSSDataEntityGroupStatus());

	TextCurrentBlock->setText(QString().setNum(Receiver.GetCurrentAMSSBlock(), 10));

	TextBlockBits->setText(Receiver.GetCurrentAMSSBlockBits());

	ProgressBarAMSS->setValue(Receiver.GetPercentageAMSSDataEntityGroupComplete());

}

void CAMSSDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

	if (Receiver.GetAMSSPLLPhase(rCurPLLPhase) == true)
	{
		/* Set current PLL phase (convert radiant in degree) */
		PhaseDialAMSS->setValue(rCurPLLPhase * (CReal) 360.0 / (2 * crPi));

		/* Check if control is enabled */
		if (!PhaseDialAMSS->isEnabled())
			PhaseDialAMSS->setEnabled(true);
	}
	else
	{
		/* Reset dial */
		PhaseDialAMSS->setValue((CReal) 0.0);

		/* Check if control is disabled */
		if (PhaseDialAMSS->isEnabled())
			PhaseDialAMSS->setEnabled(false);
	}
}

void CAMSSDlg::AddWhatsThisHelp()
{
	// TODO
}
