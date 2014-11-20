/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
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

#include "AnalogDemDlg.h"
#include "DRMPlot.h"
#include <QMessageBox>
#include <QMenuBar>
#include <QString>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QToolTip>
#include <QButtonGroup>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QSlider>
#include <QLayout>
#include <QProgressBar>
#include <QComboBox>
#include <QWhatsThis>
#include <QDateTime>
#include <QCloseEvent>
#include <QInputDialog>
#include "receivercontroller.h"
#include "ThemeCustomizer.h"

void PhaseGauge::paintEvent(QPaintEvent *)
{
    static const QPoint needle[3] = {
        QPoint(7, 8),
        QPoint(-7, 8),
        QPoint(0, -60)
    };

    QColor needleColor(127, 0, 127);
    int side = qMin(width(), height());
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 200.0, side / 200.0);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    painter.drawEllipse(-100,-100,200,200);
    painter.setPen(Qt::NoPen);
    painter.setBrush(needleColor);
    painter.save();
    painter.rotate(angle);
    painter.drawConvexPolygon(needle, 3);
    painter.restore();
    painter.setPen(needleColor);
   for (int i = 0; i < 8; ++i) {
       painter.drawLine(78, 0, 96, 0);
       painter.rotate(45.0);
   }
}

/* Implementation *************************************************************/
AnalogDemDlg::AnalogDemDlg(ReceiverController* rc, CSettings& Settings,
	CFileMenu* pFileMenu, CSoundCardSelMenu* pSoundCardMenu, QWidget* parent) :
    CWindow(parent, Settings, "AM"),
    controller(rc),
    AMSSDlg(*rc->getReceiver(), Settings, this), MainPlot(NULL),
    pFileMenu(pFileMenu), pSoundCardMenu(pSoundCardMenu)
{
    setupUi(this);

	/* Add file and sound card menu */
    menuBar()->insertMenu(menu_View->menuAction(), pFileMenu);
    menu_Settings->addMenu(pSoundCardMenu);

    connect(action_Stations_Dialog, SIGNAL(triggered()), this, SIGNAL(ViewStationsDlg()));
    connect(action_Live_Schedule_Dialog, SIGNAL(triggered()), this, SIGNAL(ViewLiveScheduleDlg()));
    connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(actionAM, SIGNAL(triggered()), this, SIGNAL(NewAMAcquisition()));
    connect(actionFM, SIGNAL(triggered()), this, SLOT(OnSwitchToFM()));
    connect(actionDRM, SIGNAL(triggered()), this, SLOT(OnSwitchToDRM()));
	connect(pFileMenu, SIGNAL(soundFileChanged(CDRMReceiver::ESFStatus)), this, SLOT(OnSoundFileChanged(CDRMReceiver::ESFStatus)));
	connect(pSoundCardMenu, SIGNAL(sampleRateChanged()), this, SLOT(OnSampleRateChanged()));
    connect(actionAbout_Dream, SIGNAL(triggered()), this, SLOT(OnHelpAbout()));
    connect(actionWhats_This, SIGNAL(triggered()), this, SLOT(OnWhatsThis()));
    SliderBandwidth->setTickPosition(QSlider::TicksBothSides);
    MainPlot = new CDRMPlot(NULL, plot, controller);

	/* Init main plot */
	bool waterfall = getSetting("waterfall", false);
    ButtonWaterfall->setChecked(waterfall);
	MainPlot->SetupChart(waterfall ? CDRMPlot::INP_SPEC_WATERF : CDRMPlot::INPUT_SIG_PSD_ANALOG);

	/* Add tool tip to show the user the possibility of choosing the AM IF */
	QString ptt = tr("Click on the plot to set the demodulation frequency");
	MainPlot->plot->setToolTip(ptt);

	/* Init bandwidth slider */
	UpdateSliderBandwidth();
    SliderBandwidth->setTickPosition(QSlider::TicksBothSides);
    SliderBandwidth->setTickInterval(1000); /* Each kHz a tick */
    SliderBandwidth->setPageStep(1000); /* Hz */

    /* Init PLL phase dial control */
    initPhaseDial();

#ifdef HAVE_SPEEX
    SpinBoxNoiRedLevel->setValue(controller->getReceiver()->GetAMDemod()->GetNoiRedLevel());
#else
    RadioButtonNoiRedSpeex->hide();
    SpinBoxNoiRedLevel->hide();
#endif

	/* Update controls */
	UpdateControls();

	/* Connect controls ----------------------------------------------------- */
    connect(ButtonDRM, SIGNAL(clicked()),
		this, SLOT(OnSwitchToDRM()));
    connect(ButtonAMSS, SIGNAL(clicked()),
		&AMSSDlg, SLOT(show()));
	connect(MainPlot, SIGNAL(xAxisValSet(double)),
		this, SLOT(OnChartxAxisValSet(double)));

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
		this, SLOT(OnTimerPLLPhaseDial()));
	connect(&TimerClose, SIGNAL(timeout()),
		this, SLOT(OnTimerClose()));

    /* Set help text for the controls */
    AddWhatsThisHelp();
    /* Don't activate real-time timers, wait for show event */

    APPLY_CUSTOM_THEME();
}

void AnalogDemDlg::initPhaseDial()
{
    phaseGauge = new PhaseGauge(this);
    phaseGauge->setMinimumHeight(100);
    phaseGauge->setMinimumWidth(100);
    controlsFrame->layout()->addWidget(phaseGauge);
}

void AnalogDemDlg::OnWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

void AnalogDemDlg::OnSwitchToDRM()
{
	emit SwitchMode(RM_DRM);
}

void AnalogDemDlg::OnSwitchToFM()
{
	emit SwitchMode(RM_FM);
}

void AnalogDemDlg::eventUpdate()
{
	/* Put (re)initialization code here for the settings that might have
	   be changed by another top level window. Called on mode switch */
	pFileMenu->UpdateMenu();
	UpdateSliderBandwidth();
}

void AnalogDemDlg::eventShow(QShowEvent*)
{
	OnTimer();
	OnTimerPLLPhaseDial();
	/* Set correct schedule */

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);

	UpdateControls();

    /* Notify the MainPlot of showEvent */
    MainPlot->activate();
}

void AnalogDemDlg::eventHide(QHideEvent*)
{
    /* Notify the MainPlot of hideEvent */
    MainPlot->deactivate();

	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

    bool waterfall = ButtonWaterfall->isChecked();
	putSetting("waterfall", waterfall);
}

void AnalogDemDlg::eventClose(QCloseEvent* ce)
{
	if (!TimerClose.isActive())
	{
		/* Stop real-time timers */
		Timer.stop();
		TimerPLLPhaseDial.stop();

		/* Tell every other window to close too */
		emit Closed();

		/* Set the timer for polling the working thread state */
		TimerClose.start(50);
	}

	/* Stay open until working thread is done */
    if (controller->getReceiver()->GetParameters()->eRunState == CParameter::STOPPED)
	{
        TimerClose.stop();
		ce->accept();
	}
	else
		ce->ignore();
}

void AnalogDemDlg::UpdateControls()
{
	/* Set demodulation type */
    switch (controller->getReceiver()->GetAMDemod()->GetDemodType())
	{
	case CAMDemodulation::DT_AM:
        if (!RadioButtonDemAM->isChecked())
            RadioButtonDemAM->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_LSB:
        if (!RadioButtonDemLSB->isChecked())
            RadioButtonDemLSB->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_USB:
        if (!RadioButtonDemUSB->isChecked())
            RadioButtonDemUSB->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_CW:
        if (!RadioButtonDemCW->isChecked())
            RadioButtonDemCW->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_FM:
        if (!RadioButtonDemFM->isChecked())
            RadioButtonDemFM->setChecked(TRUE);
		break;
	}

	/* Set AGC type */
    switch (controller->getReceiver()->GetAMDemod()->GetAGCType())
	{
	case CAGC::AT_NO_AGC:
        if (!RadioButtonAGCOff->isChecked())
            RadioButtonAGCOff->setChecked(TRUE);
		break;

	case CAGC::AT_SLOW:
        if (!RadioButtonAGCSlow->isChecked())
            RadioButtonAGCSlow->setChecked(TRUE);
		break;

	case CAGC::AT_MEDIUM:
        if (!RadioButtonAGCMed->isChecked())
            RadioButtonAGCMed->setChecked(TRUE);
		break;

	case CAGC::AT_FAST:
        if (!RadioButtonAGCFast->isChecked())
            RadioButtonAGCFast->setChecked(TRUE);
		break;
	}

	/* Set noise reduction type */
    switch (controller->getReceiver()->GetAMDemod()->GetNoiRedType())
	{
	case CAMDemodulation::NR_OFF:
        if (!RadioButtonNoiRedOff->isChecked())
            RadioButtonNoiRedOff->setChecked(TRUE);
		break;

	case CAMDemodulation::NR_LOW:
        if (!RadioButtonNoiRedLow->isChecked())
            RadioButtonNoiRedLow->setChecked(TRUE);
		break;

	case CAMDemodulation::NR_MEDIUM:
        if (!RadioButtonNoiRedMed->isChecked())
            RadioButtonNoiRedMed->setChecked(TRUE);
		break;

	case CAMDemodulation::NR_HIGH:
        if (!RadioButtonNoiRedHigh->isChecked())
            RadioButtonNoiRedHigh->setChecked(TRUE);
		break;

	case CAMDemodulation::NR_SPEEX:
        if (!RadioButtonNoiRedSpeex->isChecked())
            RadioButtonNoiRedSpeex->setChecked(TRUE);
		break;
	}

#ifdef HAVE_SPEEX
	/* Set speex spinbox enable state */
    SpinBoxNoiRedLevel->setEnabled(RadioButtonNoiRedSpeex->isChecked());
#endif

	/* Set filter bandwidth */
    SliderBandwidth->setValue(controller->getReceiver()->GetAMDemod()->GetFilterBW());
    ButtonBandWidth->setText(QString().setNum(
        controller->getReceiver()->GetAMDemod()->GetFilterBW()) +	tr(" Hz"));

	/* Update check boxes */
    CheckBoxMuteAudio->setChecked(controller->getReceiver()->GetWriteData()->GetMuteAudio());
    CheckBoxSaveAudioWave->
        setChecked(controller->getReceiver()->GetWriteData()->GetIsWriteWaveFile());

    CheckBoxAutoFreqAcq->
        setChecked(controller->getReceiver()->GetAMDemod()->AutoFreqAcqEnabled());

    CheckBoxPLL->setChecked(controller->getReceiver()->GetAMDemod()->PLLEnabled());
}

void AnalogDemDlg::UpdateSliderBandwidth()
{
    SliderBandwidth->setRange(0, controller->getReceiver()->GetParameters()->GetSigSampleRate() / 2);
}

void AnalogDemDlg::UpdatePlotStyle(int iPlotstyle)
{
	/* Update main plot window */
	MainPlot->SetPlotStyle(iPlotstyle);
}

void AnalogDemDlg::OnSampleRateChanged()
{
	UpdateSliderBandwidth();
}

void AnalogDemDlg::OnSoundFileChanged(CDRMReceiver::ESFStatus)
{
	UpdateSliderBandwidth();
}

void AnalogDemDlg::OnTimer()
{
    switch(controller->getReceiver()->GetReceiverMode())
	{
	case RM_AM:
		/* Carrier frequency of AM signal */
        ButtonFreqOffset->setText(QString().setNum(
            controller->getReceiver()->GetReceiveData()->ConvertFrequency(
                controller->getReceiver()->GetAMDemod()->GetCurMixFreqOffs())
			, 'f', 2) + " Hz");
		break;
	case RM_DRM:
	case RM_FM:
	case RM_NONE:
		break;
	}
}

void AnalogDemDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

    if (controller->getReceiver()->GetAMDemod()->GetPLLPhase(rCurPLLPhase) == TRUE)
	{
		/* Set current PLL phase (convert radiant in degree) */
        phaseGauge->setValue(rCurPLLPhase * (CReal) 360.0 / (2 * crPi));

		/* Check if control is enabled */
        if (!phaseGauge->isEnabled())
            phaseGauge->setEnabled(true);
	}
	else
	{
		/* Reset dial */
        double d = 360.0*double(qrand())/double(RAND_MAX);
        phaseGauge->setValue(d);

		/* Check if control is disabled */
        if (phaseGauge->isEnabled())
            phaseGauge->setEnabled(false);
	}
}

void AnalogDemDlg::OnTimerClose()
{
    if(controller->getReceiver()->GetParameters()->eRunState == CParameter::STOPPED)
		close();
}

void AnalogDemDlg::on_ButtonGroupDemodulation_buttonClicked(int iID)
{
	iID = -iID - 2; // TODO understand why
	/* DRMReceiver takes care of setting appropriate filter BW */
	switch (iID)
	{
	case 0:
        controller->getReceiver()->SetAMDemodType(CAMDemodulation::DT_AM);
		break;

	case 1:
        controller->getReceiver()->SetAMDemodType(CAMDemodulation::DT_LSB);
		break;

	case 2:
        controller->getReceiver()->SetAMDemodType(CAMDemodulation::DT_USB);
		break;

	case 3:
        controller->getReceiver()->SetAMDemodType(CAMDemodulation::DT_CW);
		break;

	case 4:
        controller->getReceiver()->SetAMDemodType(CAMDemodulation::DT_FM);
		break;
	}

	/* Update controls */
	UpdateControls();
}

void AnalogDemDlg::on_ButtonGroupAGC_buttonClicked(int iID)
{
	iID = -iID - 2; // TODO understand why
	switch (iID)
	{
	case 0:
        controller->getReceiver()->GetAMDemod()->SetAGCType(CAGC::AT_NO_AGC);
		break;

	case 1:
        controller->getReceiver()->GetAMDemod()->SetAGCType(CAGC::AT_SLOW);
		break;

	case 2:
        controller->getReceiver()->GetAMDemod()->SetAGCType(CAGC::AT_MEDIUM);
		break;

	case 3:
        controller->getReceiver()->GetAMDemod()->SetAGCType(CAGC::AT_FAST);
		break;
	}
}

void AnalogDemDlg::on_ButtonGroupNoiseReduction_buttonClicked(int iID)
{
	iID = -iID - 2; // TODO understand why
	switch (iID)
	{
	case 0:
        controller->getReceiver()->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_OFF);
		break;

	case 1:
        controller->getReceiver()->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_LOW);
		break;

	case 2:
        controller->getReceiver()->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_MEDIUM);
		break;

	case 3:
        controller->getReceiver()->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_HIGH);
		break;

#ifdef HAVE_SPEEX
	case 4:
        controller->getReceiver()->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_SPEEX);
		break;
#endif
	}

#ifdef HAVE_SPEEX
	/* Set speex spinbox enable state */
    SpinBoxNoiRedLevel->setEnabled(RadioButtonNoiRedSpeex->isChecked());
#endif
}

void AnalogDemDlg::on_SliderBandwidth_valueChanged(int value)
{
	/* Set new filter in processing module */
    controller->getReceiver()->SetAMFilterBW(value);
    ButtonBandWidth->setText(QString().setNum(value) + tr(" Hz"));

	/* Update chart */
	MainPlot->UpdateAnalogBWMarker();
}

void AnalogDemDlg::on_CheckBoxAutoFreqAcq_clicked(bool checked)
{
	/* Set parameter in working thread module */
    controller->getReceiver()->GetAMDemod()->EnableAutoFreqAcq(checked);
}

void AnalogDemDlg::on_CheckBoxPLL_clicked(bool checked)
{
	/* Set parameter in working thread module */
    controller->getReceiver()->GetAMDemod()->EnablePLL(checked);
}

void AnalogDemDlg::on_CheckBoxMuteAudio_clicked(bool checked)
{
	/* Set parameter in working thread module */
    controller->getReceiver()->GetWriteData()->MuteAudio(checked);
}

void AnalogDemDlg::on_CheckBoxSaveAudioWave_clicked(bool checked)
{
/*
	This code is copied in systemevalDlg.cpp. If you do changes here, you should
	apply the changes in the other file, too
*/
	if (checked)
	{
		/* Show "save file" dialog */
		QString strFileName =
			QFileDialog::getSaveFileName(this, tr("Save Audio"), "DreamOut.wav", tr("Wav (*.wav)"));
		/* Check if user not hit the cancel button */
		if (!strFileName.isEmpty())
		{
            controller->getReceiver()->GetWriteData()->
				StartWriteWaveFile(strFileName.toLocal8Bit().constData());
		}
		else
		{
			/* User hit the cancel button, uncheck the button */
            CheckBoxSaveAudioWave->setChecked(false);
		}
	}
	else
        controller->getReceiver()->GetWriteData()->StopWriteWaveFile();
}

void AnalogDemDlg::OnChartxAxisValSet(double dVal)
{
	/* Perform range check */
	if (dVal < 0.0)
		dVal = 0.0;
	else if (dVal > 1.0)
		dVal = 1.0;

	/* Set new frequency in receiver module */
    controller->getReceiver()->SetAMDemodAcq(dVal);

	/* Update chart */
	MainPlot->UpdateAnalogBWMarker();
}

void AnalogDemDlg::on_ButtonWaterfall_clicked(bool checked)
{
    /* Toggle between normal spectrum plot and waterfall spectrum plot */
	if (checked)
		MainPlot->SetupChart(CDRMPlot::INP_SPEC_WATERF);
	else
		MainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD_ANALOG);
}

/* Manual carrier frequency input box */
void AnalogDemDlg::on_ButtonFreqOffset_clicked(bool)
{
	bool ok = false;
	const double prev_freq =
		controller->getReceiver()->GetReceiveData()->ConvertFrequency(
			controller->getReceiver()->GetAMDemod()->GetCurMixFreqOffs());
	const double new_freq = QInputDialog::getDouble(this, this->windowTitle(),
        LabelCarrierFrequency->text(), prev_freq, -1e6, 1e6, 2, &ok);
	if (ok)
	{
		const _REAL conv_freq =
			controller->getReceiver()->GetReceiveData()->ConvertFrequency(new_freq, TRUE);
		const double dVal = conv_freq /
			(controller->getReceiver()->GetParameters()->GetSigSampleRate() / 2);
		OnChartxAxisValSet(dVal);
	}
}

/* Manual band width input box */
void AnalogDemDlg::on_ButtonBandWidth_clicked(bool)
{
	bool ok = false;
	const int sr2 = controller->getReceiver()->GetParameters()->GetSigSampleRate() / 2;
	const int prev_bw = controller->getReceiver()->GetAMDemod()->GetFilterBW();
	const int new_bw = QInputDialog::getInt(this, this->windowTitle(),
        groupBoxBW->title(), prev_bw, 0, sr2, 2, &ok);
	if (ok)
	{
		controller->getReceiver()->GetAMDemod()->SetFilterBW(new_bw);
        SliderBandwidth->setValue(controller->getReceiver()->GetAMDemod()->GetFilterBW());
	}
}

void AnalogDemDlg::on_SpinBoxNoiRedLevel_valueChanged(int value)
{
    controller->getReceiver()->GetAMDemod()->SetNoiRedLevel(value);
}

void AnalogDemDlg::AddWhatsThisHelp()
{
	/* Noise Reduction */
	const QString strNoiseReduction =
		tr("<b>Noise Reduction:</b> The noise suppression is a frequency "
		"domain optimal filter design based algorithm. The noise PSD is "
		"estimated utilizing a minimum statistic. A problem of this type of "
		"algorithm is that it produces the so called \"musical tones\". The "
		"noise becomes coloured and sounds a bit strange. At the same time, "
		"the useful signal (which might be speech or music) is also "
		"distorted by the algorithm. By selecting the level of noise "
		"reduction, a compromise between distortion of the useful signal "
		"and actual noise reduction can be made.");

	/* Automatic Gain Control */
	const QString strAGC =
		tr("<b>AGC (Automatic Gain Control):</b> Input signals can have a "
		"large variation in power due to channel impairments. To compensate "
		"for that, an automatic gain control can be applied. The AGC has "
		"four settings: Off, Slow, Medium and Fast.");

	/* Filter Bandwidth */
	const QString strFilterBW =
		tr("<b>Filter Bandwidth:</b> A band-pass filter is applied before "
		"the actual demodulation process. With this filter, adjacent signals "
		"are attenuated. The bandwidth of this filter can be chosen in steps "
		"of 1 Hz by using the slider bar. Clicking on the right or left side "
		"of the slider leveler will increase/decrease the bandwidth by 1 kHz. "
		"<br>The current filter bandwidth is indicated in the spectrum plot "
		"by a selection bar.");

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

	/* Mute Audio (same as in systemevaldlg.cpp!) */
    QString strCheckBoxMuteAudio =
		tr("<b>Mute Audio:</b> The audio can be muted by "
		"checking this box. The reaction of checking or unchecking this box "
		"is delayed by approx. 1 second due to the audio buffers.");

	/* Save audio as wave (same as in systemevaldlg.cpp!) */
    QString strCheckBoxSaveAudioWave =
		tr("<b>Save Audio as WAV:</b> Save the audio signal "
		"as stereo, 16-bit, 48 kHz sample rate PCM wave file. Checking this "
		"box will let the user choose a file name for the recording.");

	/* Carrier Frequency */
	QString strTextFreqOffset =
		tr("<b>Carrier Frequency:</b> The (estimated) carrier frequency of the "
		"analog signal is shown. (The estimation of this parameter can be done "
		"by the Autom Frequency Acquisition which uses the estimated PSD of "
		"the input signal and applies a maximum search.)");

	/* Phase lock loop */
	const QString strPLL =
		tr("<b>PLL:</b> The Phase-Lock-Loop (PLL) tracks the carrier of the "
		"modulated received signal. The resulting phase offset between the "
		"reference oscillator and the received carrier is displayed in "
		"a dial control. If the pointer is almost steady, the PLL is locked. "
		"If the pointer of the dial control turns quickly, the PLL is "
		"out of lock. To get the PLL locked, the frequency offset to "
		"the true carrier frequency must not exceed a few Hz.");

	/* Auto frequency acquisition */
	const QString strAutoFreqAcqu =
		tr("<b>Auto Frequency Acquisition:</b> Clicking on the "
		"input spectrum plot changes the mixing frequency for demodulation. "
		"If the Auto Frequency Acquisition is enabled, the largest peak "
		"near the curser is selected.");

    RadioButtonNoiRedOff->setWhatsThis(strNoiseReduction);
    RadioButtonNoiRedLow->setWhatsThis(strNoiseReduction);
    RadioButtonNoiRedMed->setWhatsThis(strNoiseReduction);
    RadioButtonNoiRedHigh->setWhatsThis(strNoiseReduction);
    RadioButtonAGCOff->setWhatsThis(strAGC);
    RadioButtonAGCSlow->setWhatsThis(strAGC);
    RadioButtonAGCMed->setWhatsThis(strAGC);
    RadioButtonAGCFast->setWhatsThis(strAGC);
    ButtonBandWidth->setWhatsThis(strFilterBW);
    SliderBandwidth->setWhatsThis(strFilterBW);
    RadioButtonDemAM->setWhatsThis(strDemodType);
    RadioButtonDemLSB->setWhatsThis(strDemodType);
    RadioButtonDemUSB->setWhatsThis(strDemodType);
    RadioButtonDemCW->setWhatsThis(strDemodType);
    RadioButtonDemFM->setWhatsThis(strDemodType);
    GroupBoxAutoFreqAcq->setWhatsThis(strAutoFreqAcqu);
    CheckBoxAutoFreqAcq->setWhatsThis(strAutoFreqAcqu);
    CheckBoxMuteAudio->setWhatsThis(strCheckBoxMuteAudio);
    GroupBoxPLL->setWhatsThis(strPLL);
    CheckBoxPLL->setWhatsThis(strPLL);
    phaseGauge->setWhatsThis(strPLL);
    TextLabelPhaseOffset->setWhatsThis(strPLL);
    ButtonFreqOffset->setWhatsThis(strTextFreqOffset);
    ButtonFreqOffset->setWhatsThis(strTextFreqOffset);
    CheckBoxSaveAudioWave->setWhatsThis(strCheckBoxSaveAudioWave);
    groupBoxNoiseReduction->setWhatsThis(strNoiseReduction);
    groupBoxAGC->setWhatsThis(strAGC);
    groupBoxDemodulation->setWhatsThis(strDemodType);
    groupBoxBW->setWhatsThis(strFilterBW);
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
CAMSSDlg::CAMSSDlg(CDRMReceiver& NDRMR, CSettings& Settings, QWidget* parent) :
	CWindow(parent, Settings, "AMSS"),
    DRMReceiver(NDRMR)
{
    setupUi(this);

    /* Init AMSS PLL phase dial control */
    phaseGauge = new PhaseGauge(this);
    phaseGauge->setMinimumHeight(100);
    phaseGauge->setMinimumWidth(100);
    phaseFrame->layout()->addWidget(phaseGauge);

    /* Set help text for the controls */
    AddWhatsThisHelp();


    TextAMSSServiceLabel->setText("");
    TextAMSSCountryCode->setText("");
    TextAMSSTimeDate->setText("");
    TextAMSSLanguage->setText("");
    TextAMSSServiceID->setText("");
    TextAMSSAMCarrierMode->setText("");
    TextAMSSInfo->setText("");

    ListBoxAMSSAFSList->setEnabled(FALSE);


	/* Connect controls ----------------------------------------------------- */
	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
		this, SLOT(OnTimerPLLPhaseDial()));
	/* Buttons */
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));

    APPLY_CUSTOM_THEME();
}

void CAMSSDlg::eventHide(QHideEvent*)
{
	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();
}

void CAMSSDlg::eventShow(QShowEvent*)
{
	OnTimer();
	OnTimerPLLPhaseDial();

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);
}

void CAMSSDlg::OnTimer()
{
	int j;

    CParameter& Parameters = *DRMReceiver.GetParameters();
	Parameters.Lock();

	/* Show label if available */
	if ((Parameters.Service[0].IsActive()) && (Parameters.Service[0].strLabel != ""))
	{
		/* Service label (UTF-8 encoded string -> convert) */
        TextAMSSServiceLabel->setText(QString().fromUtf8(
			Parameters.Service[0].strLabel.c_str()
		));
	}
	else
        TextAMSSServiceLabel->setText(tr(""));

	/* Country code */
	const string strCntryCode = Parameters.Service[0].strCountryCode; /* must be of 2 lowercase chars */

	if ((Parameters.Service[0].IsActive()) && (!strCntryCode.empty()) && (strCntryCode != "--"))
	{
        TextAMSSCountryCode->
			setText(QString(GetISOCountryName(strCntryCode).c_str()));
	}
	else
        TextAMSSCountryCode->setText("");

	/* SDC Language code */

	if (Parameters.Service[0].IsActive())
	{
		const string strLangCode = Parameters.Service[0].strLanguageCode; /* must be of 3 lowercase chars */

		if ((!strLangCode.empty()) && (strLangCode != "---"))
             TextAMSSLanguage->
				setText(QString(GetISOLanguageName(strLangCode).c_str()));
		else
            TextAMSSLanguage->setText(QString(strTableLanguageCode[Parameters.Service[0].iLanguage].c_str()));
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
	const size_t iNumAltServices = Parameters.AltFreqSign.vecOtherServices.size();

	if (iNumAltServices != 0)
	{
		QString val = QString().setNum((long) iNumAltServices);
        ListBoxAMSSAFSList->insertItem(10, val);

        ListBoxAMSSAFSList->clear();
        ListBoxAMSSAFSList->setEnabled(TRUE);

		QString freqEntry;

		for (size_t i = 0; i < iNumAltServices; i++)
		{
			switch (Parameters.AltFreqSign.vecOtherServices[i].iSystemID)
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

			const int iNumAltFreqs = Parameters.AltFreqSign.vecOtherServices[i].veciFrequencies.size();

            const int iSystemID = Parameters.AltFreqSign.vecOtherServices[i].iSystemID;

			switch (iSystemID)
			{
			case 0:
			case 1:
			case 2:
				/* AM or DRM, freq in kHz */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						veciFrequencies[j], 10);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " kHz";


				if (iSystemID == 0 || iSystemID == 1)
				{
					freqEntry += " ID:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
				}
				break;

			case 3:
			case 4:
			case 5:
				/* 'FM1 frequency' - 87.5 to 107.9 MHz (100 kHz steps) */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
                        QString().setNum((float) (87.5 + 0.1 * DRMReceiver.
						GetParameters()->AltFreqSign.
						vecOtherServices[i].veciFrequencies[j]), 'f', 1);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " MHz";

				if (iSystemID == 3)
				{
					freqEntry += " ECC+PI:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
				}

				if (iSystemID == 4)
				{
					freqEntry += " PI:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
				}
				break;

			case 6:
			case 7:
			case 8:
				/* 'FM2 frequency'- 76.0 to 90.0 MHz (100 kHz steps) */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
                        QString().setNum((float) (76.0 + 0.1 * DRMReceiver.
						GetParameters()->AltFreqSign.
						vecOtherServices[i].veciFrequencies[j]), 'f', 1);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " MHz";

				if (iSystemID == 6)
				{
					freqEntry += " ECC+PI:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
				}

				if (iSystemID == 7)
				{
					freqEntry += " PI:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
				}
				break;

			default:
				freqEntry = "DAB";
				break;
			}

			if (Parameters.AltFreqSign.
				vecOtherServices[i].bSameService)
			{
				freqEntry += " (same service)";
			}
			else
			{
				freqEntry += " (alt service)";
			}
            ListBoxAMSSAFSList->insertItem(0, freqEntry);
		}
	}
	else
	{
        ListBoxAMSSAFSList->clear();
        ListBoxAMSSAFSList->setEnabled(FALSE);
	}

    TextAMSSServiceID->setText("");
    TextAMSSAMCarrierMode->setText("");

    if (DRMReceiver.GetAMSSDecode()->GetLockStatus() == CAMSSDecode::NO_SYNC
	|| Parameters.Service[0].iServiceID == SERV_ID_NOT_USED
	)
	{
        TextAMSSInfo->setText(tr("No AMSS detected"));
	}
	else
	{
        TextAMSSInfo->setText(tr("Awaiting AMSS data..."));

		/* Display 'block 1' info */
        if (DRMReceiver.GetAMSSDecode()->GetBlock1Status())
		{
            TextAMSSInfo->setText("");

            TextAMSSLanguage->setText(QString(strTableLanguageCode[DRMReceiver.
				GetParameters()->Service[0].iLanguage].c_str()));

            TextAMSSServiceID->setText("ID:" + QString().setNum(
				(long) Parameters.Service[0].iServiceID, 16).toUpper());

            TextAMSSAMCarrierMode->setText(QString(strTableAMSSCarrierMode[DRMReceiver.
				GetParameters()->iAMSSCarrierMode].c_str()));
		}
	}

    TextDataEntityGroupStatus->setText(DRMReceiver.GetAMSSDecode()->
		GetDataEntityGroupStatus());

    TextCurrentBlock->setText(QString().setNum(DRMReceiver.GetAMSSDecode()->
		GetCurrentBlock(), 10));

    TextBlockBits->setText(DRMReceiver.GetAMSSDecode()->GetCurrentBlockBits());

    int val = DRMReceiver.GetAMSSDecode()->GetPercentageDataEntityGroupComplete();
	Parameters.Unlock();
    ProgressBarAMSS->setValue(val);
}

void CAMSSDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

    if (DRMReceiver.GetAMSSPhaseDemod()->GetPLLPhase(rCurPLLPhase) == TRUE)
	{
		/* Set current PLL phase (convert radiant in degree) */
        phaseGauge->setValue(rCurPLLPhase * (CReal) 360.0 / (2 * crPi));

		/* Check if control is enabled */
        if (!phaseGauge->isEnabled())
            phaseGauge->setEnabled(true);
    }
	else
	{
		/* Reset dial */
        phaseGauge->setValue((CReal) 0.0);

		/* Check if control is disabled */
        if (phaseGauge->isEnabled())
            phaseGauge->setEnabled(false);
	}
}

void CAMSSDlg::AddWhatsThisHelp()
{
	// TODO
}
