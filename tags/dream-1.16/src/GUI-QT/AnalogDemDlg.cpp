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

#include "AnalogDemDlg.h"
#include <qmessagebox.h>
#include <qmenubar.h>
#include <qwt_dial.h>
#include <qwt_dial_needle.h>
#include <qstring.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qtooltip.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qfiledialog.h>
#include <qslider.h>
#include <qlayout.h>
#include <qprogressbar.h>
#include <qcombobox.h>

#if QT_VERSION < 0x040000
# include "qt2/DRMPlot.h"
# include <qlistbox.h>
  /* This include is for setting the progress bar style */
# include <qmotifstyle.h>
#else
# include "DRMPlot.h"
# include "SoundCardSelMenu.h"
# include <QWhatsThis>
# include <QDateTime>
# include <QCloseEvent>
# include <qwt_plot_layout.h>
#endif

/* Implementation *************************************************************/
AnalogDemDlg::AnalogDemDlg(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f):
	AnalogDemDlgBase(parent, name, modal, f),
	DRMReceiver(NDRMR), Settings(NSettings), AMSSDlg(NDRMR, Settings, parent, name, modal, f)
{
	CWinGeom s;
	Settings.Get("AM Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	/* Set help text for the controls */
	AddWhatsThisHelp();

#if QT_VERSION < 0x040000
	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	QPopupMenu* EvalWinMenu = new QPopupMenu(this);
	CHECK_PTR(EvalWinMenu);
	EvalWinMenu->insertItem(tr("S&tations Dialog..."), this,
		SIGNAL(ViewStationsDlg()), CTRL+Key_T);
	EvalWinMenu->insertItem(tr("&Live Schedule Dialog..."), this,
		SIGNAL(ViewLiveScheduleDlg()), CTRL+Key_L);
	EvalWinMenu->insertSeparator();
	EvalWinMenu->insertItem(tr("E&xit"), this, SLOT(close()), CTRL+Key_Q);

	/* Settings menu  ------------------------------------------------------- */
	QPopupMenu* pSettingsMenu = new QPopupMenu(this);
	CHECK_PTR(pSettingsMenu);
	pSettingsMenu->insertItem(tr("&Sound Card Selection"),
		new CSoundCardSelMenu(DRMReceiver.GetSoundInInterface(), DRMReceiver.GetSoundOutInterface(), this));
	pSettingsMenu->insertItem(tr("&DRM (digital)"), this,
		SLOT(OnSwitchToDRM()), CTRL+Key_D);
	pSettingsMenu->insertItem(tr("&FM (analog)"), this,
		SLOT(OnSwitchToFM()), CTRL+Key_F);
	pSettingsMenu->insertItem(tr("New &AM Acquisition"), this,
		SIGNAL(NewAMAcquisition()), CTRL+Key_A);


	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&View"), EvalWinMenu);
	pMenu->insertItem(tr("&Settings"), pSettingsMenu);
	pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	AnalogDemDlgBaseLayout->setMenuBar(pMenu);
#else
    connect(action_Stations_Dialog, SIGNAL(triggered()), this, SIGNAL(ViewStationsDlg()));
    connect(action_Live_Schedule_Dialog, SIGNAL(triggered()), this, SIGNAL(ViewLiveScheduleDlg()));
    connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(actionAM, SIGNAL(triggered()), this, SIGNAL(NewAMAcquisition()));
    connect(actionFM, SIGNAL(triggered()), this, SLOT(OnSwitchToFM()));
    connect(actionDRM, SIGNAL(triggered()), this, SLOT(OnSwitchToDRM()));
    menu_Settings->addMenu( new CSoundCardSelMenu(
		DRMReceiver.GetSoundInInterface(),
		DRMReceiver.GetSoundOutInterface(),
	this));
    connect(actionAbout_Dream, SIGNAL(triggered()), this, SLOT(OnHelpAbout()));
    connect(actionWhats_This, SIGNAL(triggered()), this, SLOT(on_actionWhats_This()));
    SliderBandwidth->setTickPosition(QSlider::TicksBothSides);
    MainPlot = new CDRMPlot(NULL, plot);
#endif

	/* Init main plot */
	bool waterfall = Settings.Get("AM Dialog", "waterfall", false);
#if QT_VERSION < 0x040000
	ButtonWaterfall->setOn(waterfall);
#else
	ButtonWaterfall->setChecked(waterfall);
#endif
	if(MainPlot)
	{
		MainPlot->SetRecObj(&DRMReceiver);
		MainPlot->SetPlotStyle(Settings.Get("System Evaluation Dialog", "plotstyle", 0));
		MainPlot->SetupChart(waterfall?CDRMPlot::INP_SPEC_WATERF:CDRMPlot::INPUT_SIG_PSD_ANALOG);
	}

	/* Add tool tip to show the user the possibility of choosing the AM IF */
        QString ptt = tr("Click on the plot to set the demodulation frequency");
	if(MainPlot)
	{
#if QT_VERSION < 0x040000
		MainPlot->setMargin(1);
        QToolTip::add(MainPlot, ptt);
#else
        MainPlot->plot->setToolTip(ptt);
#endif
	}

	SliderBandwidth->setRange(0, SOUNDCRD_SAMPLE_RATE / 2);
#if QT_VERSION < 0x040000
	SliderBandwidth->setTickmarks(QSlider::Both);
#else
	SliderBandwidth->setTickPosition(QSlider::TicksBothSides);
#endif
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
#if QWT_VERSION < 0x060000
	PhaseDial->setScaleOptions(QwtDial::ScaleTicks);
#else
	PhaseDial->setScaleComponents(QwtAbstractScaleDraw::Ticks);
#endif


	/* Update controls */
	UpdateControls();


	/* Connect controls ----------------------------------------------------- */
	connect(ButtonDRM, SIGNAL(clicked()),
		this, SLOT(OnSwitchToDRM()));
	connect(ButtonAMSS, SIGNAL(clicked()),
		this, SLOT(OnButtonAMSS()));
	connect(ButtonWaterfall, SIGNAL(clicked()),
		this, SLOT(OnButtonWaterfall()));
	connect(MainPlot, SIGNAL(xAxisValSet(double)),
		this, SLOT(OnChartxAxisValSet(double)));

	/* Button groups */
#if QT_VERSION < 0x040000
	connect(ButtonGroupDemodulation, SIGNAL(clicked(int)),
		this, SLOT(OnRadioDemodulation(int)));
	connect(ButtonGroupAGC, SIGNAL(clicked(int)),
		this, SLOT(OnRadioAGC(int)));
	connect(ButtonGroupNoiseReduction, SIGNAL(clicked(int)),
		this, SLOT(OnRadioNoiRed(int)));
#else
	connect(ButtonGroupDemodulation, SIGNAL(buttonClicked(int)),
		this, SLOT(OnRadioDemodulation(int)));
	connect(ButtonGroupAGC, SIGNAL(buttonClicked(int)),
		this, SLOT(OnRadioAGC(int)));
	connect(ButtonGroupNoiseReduction, SIGNAL(buttonClicked(int)),
		this, SLOT(OnRadioNoiRed(int)));
#endif

	/* Slider */
	connect(SliderBandwidth, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderBWChange(int)));

	/* Check boxes */
	connect(CheckBoxMuteAudio, SIGNAL(clicked()),
		this, SLOT(OnCheckBoxMuteAudio()));
	connect(CheckBoxSaveAudioWave, SIGNAL(clicked()),
		this, SLOT(OnCheckSaveAudioWAV()));
	connect(CheckBoxAutoFreqAcq, SIGNAL(clicked()),
		this, SLOT(OnCheckAutoFreqAcq()));
	connect(CheckBoxPLL, SIGNAL(clicked()),
		this, SLOT(OnCheckPLL()));

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
		this, SLOT(OnTimerPLLPhaseDial()));
	connect(&TimerClose, SIGNAL(timeout()),
		this, SLOT(OnTimerClose()));

	/* Don't activate real-time timers, wait for show event */
}

void AnalogDemDlg::on_actionWhats_This()
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

void AnalogDemDlg::showEvent(QShowEvent* e)
{
	EVENT_FILTER(e);
	OnTimer();
	OnTimerPLLPhaseDial();
	/* Set correct schedule */

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);

	if(Settings.Get("AM Dialog", "Stations Dialog visible", FALSE))
		emit ViewStationsDlg();

	UpdateControls();

	/* Open AMSS window */
	if (Settings.Get("AMSS Dialog", "visible", FALSE) == TRUE)
		AMSSDlg.show();
	else
		AMSSDlg.hide();

#if QT_VERSION >= 0x040000  
    /* Notify the MainPlot of showEvent */
    if(MainPlot) MainPlot->activate();
#endif
}

void AnalogDemDlg::hideEvent(QHideEvent* e)
{
	EVENT_FILTER(e);
#if QT_VERSION >= 0x040000  
    /* Notify the MainPlot of hideEvent */
    if(MainPlot) MainPlot->deactivate();
#endif

	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

	/* Close AMSS window */
	Settings.Put("AMSS Dialog", "visible", AMSSDlg.isVisible());
	AMSSDlg.hide();

	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("AM Dialog", s);
	bool waterfall;
#if QT_VERSION < 0x040000
	waterfall = ButtonWaterfall->state() == QButton::On;
#else
	waterfall = ButtonWaterfall->isChecked();
#endif
	Settings.Put("AM Dialog", "waterfall", waterfall);
}

void AnalogDemDlg::closeEvent(QCloseEvent* ce)
{
	if (!TimerClose.isActive())
	{
		/* Stop real-time timers */
		Timer.stop();
		TimerPLLPhaseDial.stop();

		/* Close AMSS window */
		Settings.Put("AMSS Dialog", "visible", AMSSDlg.isVisible());
		AMSSDlg.hide();

		/* Save window geometry data */
		CWinGeom s;
		QRect WinGeom = geometry();
		s.iXPos = WinGeom.x();
		s.iYPos = WinGeom.y();
		s.iHSize = WinGeom.height();
		s.iWSize = WinGeom.width();
		Settings.Put("AM Dialog", s);

		/* Tell every other window to close too */
		emit Closed();

		/* Set the timer for polling the working thread state */
		TimerClose.start(50);
	}

	/* Stay open until working thread is done */
	if (DRMReceiver.GetParameters()->eRunState == CParameter::STOPPED)
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
	switch (DRMReceiver.GetAMDemod()->GetDemodType())
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
	switch (DRMReceiver.GetAMDemod()->GetAGCType())
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
	switch (DRMReceiver.GetAMDemod()->GetNoiRedType())
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
	}

	/* Set filter bandwidth */
	SliderBandwidth->setValue(DRMReceiver.GetAMDemod()->GetFilterBW());
	TextLabelBandWidth->setText(QString().setNum(
		DRMReceiver.GetAMDemod()->GetFilterBW()) +	tr(" Hz"));

	/* Update check boxes */
	CheckBoxMuteAudio->setChecked(DRMReceiver.GetWriteData()->GetMuteAudio());
	CheckBoxSaveAudioWave->
		setChecked(DRMReceiver.GetWriteData()->GetIsWriteWaveFile());

	CheckBoxAutoFreqAcq->
		setChecked(DRMReceiver.GetAMDemod()->AutoFreqAcqEnabled());

	CheckBoxPLL->setChecked(DRMReceiver.GetAMDemod()->PLLEnabled());
}

void AnalogDemDlg::UpdatePlotStyle(int iPlotstyle)
{
    /* Update main plot window */
    if(MainPlot)
        MainPlot->SetPlotStyle(iPlotstyle);
}

void AnalogDemDlg::OnTimer()
{
	switch(DRMReceiver.GetReceiverMode())
	{
	case RM_DRM:
		this->hide();
		break;
	case RM_FM:
		this->hide();
		break;
	case RM_AM:
		/* Carrier frequency of AM signal */
		TextFreqOffset->setText(tr("Carrier<br>Frequency:<br><b>")
		+ QString().setNum(DRMReceiver.GetAMDemod()->GetCurMixFreqOffs(), 'f', 2)
		+ " Hz</b>");
		break;
	case RM_NONE:
		break;
	}
}

void AnalogDemDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

	if (DRMReceiver.GetAMDemod()->GetPLLPhase(rCurPLLPhase) == TRUE)
	{
		/* Set current PLL phase (convert radiant in degree) */
		PhaseDial->setValue(rCurPLLPhase * (CReal) 360.0 / (2 * crPi));

		/* Check if control is enabled */
		if (!PhaseDial->isEnabled())
			PhaseDial->setEnabled(true);
	}
	else
	{
		/* Reset dial */
		PhaseDial->setValue((CReal) 0.0);

		/* Check if control is disabled */
		if (PhaseDial->isEnabled())
			PhaseDial->setEnabled(false);
	}
}

void AnalogDemDlg::OnTimerClose()
{
#if QT_VERSION >= 0x040000
	if(DRMReceiver.GetParameters()->eRunState == CParameter::STOPPED)
		close();
#endif
}

void AnalogDemDlg::OnRadioDemodulation(int iID)
{
#if QT_VERSION >= 0x040000
	iID = -iID - 2; // TODO understand why
#endif
	/* DRMReceiver takes care of setting appropriate filter BW */
	switch (iID)
	{
	case 0:
		DRMReceiver.SetAMDemodType(CAMDemodulation::DT_AM);
		break;

	case 1:
		DRMReceiver.SetAMDemodType(CAMDemodulation::DT_LSB);
		break;

	case 2:
		DRMReceiver.SetAMDemodType(CAMDemodulation::DT_USB);
		break;

	case 3:
		DRMReceiver.SetAMDemodType(CAMDemodulation::DT_CW);
		break;

	case 4:
		DRMReceiver.SetAMDemodType(CAMDemodulation::DT_FM);
		break;
	}

	/* Update controls */
	UpdateControls();
}

void AnalogDemDlg::OnRadioAGC(int iID)
{
#if QT_VERSION >= 0x040000
	iID = -iID - 2; // TODO understand why
#endif
	switch (iID)
	{
	case 0:
		DRMReceiver.GetAMDemod()->SetAGCType(CAGC::AT_NO_AGC);
		break;

	case 1:
		DRMReceiver.GetAMDemod()->SetAGCType(CAGC::AT_SLOW);
		break;

	case 2:
		DRMReceiver.GetAMDemod()->SetAGCType(CAGC::AT_MEDIUM);
		break;

	case 3:
		DRMReceiver.GetAMDemod()->SetAGCType(CAGC::AT_FAST);
		break;
	}
}

void AnalogDemDlg::OnRadioNoiRed(int iID)
{
#if QT_VERSION >= 0x040000
	iID = -iID - 2; // TODO understand why
#endif
	switch (iID)
	{
	case 0:
		DRMReceiver.GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_OFF);
		break;

	case 1:
		DRMReceiver.GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_LOW);
		break;

	case 2:
		DRMReceiver.GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_MEDIUM);
		break;

	case 3:
		DRMReceiver.GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_HIGH);
		break;
	}
}

void AnalogDemDlg::OnSliderBWChange(int value)
{
	/* Set new filter in processing module */
	DRMReceiver.SetAMFilterBW(value);
	TextLabelBandWidth->setText(QString().setNum(value) + tr(" Hz"));

	/* Update chart */
	if(MainPlot) MainPlot->Update();
}

void AnalogDemDlg::OnCheckAutoFreqAcq()
{
	/* Set parameter in working thread module */
	DRMReceiver.GetAMDemod()->EnableAutoFreqAcq(CheckBoxAutoFreqAcq->isChecked());
}

void AnalogDemDlg::OnCheckPLL()
{
	/* Set parameter in working thread module */
	DRMReceiver.GetAMDemod()->EnablePLL(CheckBoxPLL->isChecked());
}

void AnalogDemDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	DRMReceiver.GetWriteData()->MuteAudio(CheckBoxMuteAudio->isChecked());
}

void AnalogDemDlg::OnCheckSaveAudioWAV()
{
/*
	This code is copied in systemevalDlg.cpp. If you do changes here, you should
	apply the changes in the other file, too
*/
	if (CheckBoxSaveAudioWave->isChecked() == TRUE)
	{
		/* Show "save file" dialog */
		QString strFileName =
#if QT_VERSION < 0x040000
			QFileDialog::getSaveFileName("DreamOut.wav", "*.wav", this);
#else
			QFileDialog::getSaveFileName(this, tr("Save Audio"), "DreamOut.wav", tr("Wav (*.wav)"));
#endif
		/* Check if user not hit the cancel button */
		if (!strFileName.isEmpty())
		{
#if QT_VERSION < 0x040000
			DRMReceiver.GetWriteData()->
				StartWriteWaveFile(strFileName.latin1());
#else
			DRMReceiver.GetWriteData()->
				StartWriteWaveFile(strFileName.toLatin1().data());
#endif
		}
		else
		{
			/* User hit the cancel button, uncheck the button */
			CheckBoxSaveAudioWave->setChecked(FALSE);
		}
	}
	else
		DRMReceiver.GetWriteData()->StopWriteWaveFile();
}

void AnalogDemDlg::OnChartxAxisValSet(double dVal)
{
	/* Set new frequency in receiver module */
	DRMReceiver.SetAMDemodAcq(dVal);

	/* Update chart */
	if(MainPlot) MainPlot->Update();
}

void AnalogDemDlg::OnButtonWaterfall()
{
	/* Toggle between normal spectrum plot and waterfall spectrum plot */
#if QT_VERSION < 0x040000
	if (MainPlot && ButtonWaterfall->state() == QButton::On)
#else
	if (MainPlot && ButtonWaterfall->isChecked())
#endif
		MainPlot->SetupChart(CDRMPlot::INP_SPEC_WATERF);
	else
		MainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD_ANALOG);
}

void AnalogDemDlg::OnButtonAMSS()
{
	/* Open AMSS window and set in foreground */
	AMSSDlg.show();
	AMSSDlg.raise();
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

#if QT_VERSION < 0x040000
	QWhatsThis::add(ButtonGroupNoiseReduction, strNoiseReduction);
	QWhatsThis::add(RadioButtonNoiRedOff, strNoiseReduction);
	QWhatsThis::add(RadioButtonNoiRedLow, strNoiseReduction);
	QWhatsThis::add(RadioButtonNoiRedMed, strNoiseReduction);
	QWhatsThis::add(RadioButtonNoiRedHigh, strNoiseReduction);

	QWhatsThis::add(ButtonGroupAGC, strAGC);
	QWhatsThis::add(RadioButtonAGCOff, strAGC);
	QWhatsThis::add(RadioButtonAGCSlow, strAGC);
	QWhatsThis::add(RadioButtonAGCMed, strAGC);
	QWhatsThis::add(RadioButtonAGCFast, strAGC);

	QWhatsThis::add(ButtonGroupBW, strFilterBW);
	QWhatsThis::add(TextLabelBandWidth, strFilterBW);
	QWhatsThis::add(SliderBandwidth, strFilterBW);

	QWhatsThis::add(ButtonGroupDemodulation, strDemodType);
	QWhatsThis::add(RadioButtonDemAM, strDemodType);
	QWhatsThis::add(RadioButtonDemLSB, strDemodType);
	QWhatsThis::add(RadioButtonDemUSB, strDemodType);
	QWhatsThis::add(RadioButtonDemCW, strDemodType);
	QWhatsThis::add(RadioButtonDemFM, strDemodType);

	QWhatsThis::add(CheckBoxMuteAudio, strCheckBoxMuteAudio);

	QWhatsThis::add(CheckBoxSaveAudioWave, strCheckBoxSaveAudioWave);

	QWhatsThis::add(TextFreqOffset, strTextFreqOffset);

	QWhatsThis::add(GroupBoxPLL, strPLL);
	QWhatsThis::add(CheckBoxPLL, strPLL);
	QWhatsThis::add(PhaseDial, strPLL);
	QWhatsThis::add(TextLabelPhaseOffset, strPLL);

	QWhatsThis::add(GroupBoxAutoFreqAcq, strAutoFreqAcqu);
	QWhatsThis::add(CheckBoxAutoFreqAcq, strAutoFreqAcqu);
#else
    RadioButtonNoiRedOff->setWhatsThis(strNoiseReduction);
    RadioButtonNoiRedLow->setWhatsThis(strNoiseReduction);
    RadioButtonNoiRedMed->setWhatsThis(strNoiseReduction);
    RadioButtonNoiRedHigh->setWhatsThis(strNoiseReduction);
    RadioButtonAGCOff->setWhatsThis(strAGC);
    RadioButtonAGCSlow->setWhatsThis(strAGC);
    RadioButtonAGCMed->setWhatsThis(strAGC);
    RadioButtonAGCFast->setWhatsThis(strAGC);
    TextLabelBandWidth->setWhatsThis(strFilterBW);
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
    PhaseDial->setWhatsThis(strPLL);
    TextLabelPhaseOffset->setWhatsThis(strPLL);
    TextFreqOffset->setWhatsThis(strTextFreqOffset);
    CheckBoxSaveAudioWave->setWhatsThis(strCheckBoxSaveAudioWave);
    groupBoxNoiseReduction->setWhatsThis(strNoiseReduction);
    groupBoxAGC->setWhatsThis(strAGC);
    groupBoxDemodulation->setWhatsThis(strDemodType);
    groupBoxBW->setWhatsThis(strFilterBW);
#endif
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
CAMSSDlg::CAMSSDlg(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f) :
	CAMSSDlgBase(parent, name, modal, f),
	DRMReceiver(NDRMR),
	Settings(NSettings)
{
	CWinGeom s;
	Settings.Get("AMSS Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

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
#if QWT_VERSION < 0x060000
	PhaseDialAMSS->setScaleOptions(QwtDial::ScaleTicks);
#else
	PhaseDialAMSS->setScaleComponents(QwtAbstractScaleDraw::Ticks);
#endif

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

	/* set the progress bar style */
#if QT_VERSION < 0x040000
	ProgressBarAMSS->setStyle( new QMotifStyle() );
#endif

}

void CAMSSDlg::hideEvent(QHideEvent* e)
{
	EVENT_FILTER(e);
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

void CAMSSDlg::showEvent(QShowEvent* e)
{
	EVENT_FILTER(e);
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
#if QT_VERSION < 0x040000
			QCString(Parameters.Service[0].strLabel.c_str())
#else
			Parameters.Service[0].strLabel.c_str()
#endif
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
#if QT_VERSION < 0x040000
		ListBoxAMSSAFSList->insertItem(val, 10);
#else
		ListBoxAMSSAFSList->insertItem(10, val);
#endif

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
#if QT_VERSION < 0x040000
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
#else
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
#endif
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
#if QT_VERSION < 0x040000
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
#else
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
#endif
				}

				if (iSystemID == 4)
				{
					freqEntry += " PI:";
					freqEntry +=
#if QT_VERSION < 0x040000
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
#else
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
#endif
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
#if QT_VERSION < 0x040000
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
#else
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
#endif
				}

				if (iSystemID == 7)
				{
					freqEntry += " PI:";
					freqEntry +=
#if QT_VERSION < 0x040000
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
#else
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).toUpper();
#endif
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

#if QT_VERSION < 0x040000
			ListBoxAMSSAFSList->insertItem(freqEntry, 0);
#else
			ListBoxAMSSAFSList->insertItem(0, freqEntry);
#endif
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

#if QT_VERSION < 0x040000
			TextAMSSServiceID->setText("ID:" + QString().setNum(
				(long) Parameters.Service[0].iServiceID, 16).upper());
#else
			TextAMSSServiceID->setText("ID:" + QString().setNum(
				(long) Parameters.Service[0].iServiceID, 16).toUpper());
#endif

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
#if QT_VERSION < 0x040000
	ProgressBarAMSS->setProgress(val);
#else
	ProgressBarAMSS->setValue(val);
#endif
}

void CAMSSDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

	if (DRMReceiver.GetAMSSPhaseDemod()->GetPLLPhase(rCurPLLPhase) == TRUE)
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
