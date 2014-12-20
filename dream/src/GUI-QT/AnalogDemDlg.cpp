/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Volker Fischer, Andrew Murphy
 *
 * Description:
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *  - Additional widgets for displaying AMSS information
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
#include "receivercontroller.h"
#include "ThemeCustomizer.h"
#include <QPainter>
#include <QWhatsThis>
#include <QFileDialog>
#include <QInputDialog>
#include <QDateTime>

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
    CWindow(parent, Settings, "AM"), ui(new Ui::AMMainWindow),
    controller(rc),
    AMSSDlg(*rc->getReceiver(), Settings, NULL),
    pFileMenu(pFileMenu), pSoundCardMenu(pSoundCardMenu),subSampleCount(0),
    MainPlot(NULL)
{
    ui->setupUi(this);

    /* Add file and sound card menu */
    menuBar()->insertMenu(ui->menu_View->menuAction(), pFileMenu);
    ui->menu_Settings->addMenu(pSoundCardMenu);

    connect(ui->action_Stations_Dialog, SIGNAL(triggered()), this, SIGNAL(ViewStationsDlg()));
    connect(ui->action_Live_Schedule_Dialog, SIGNAL(triggered()), this, SIGNAL(ViewLiveScheduleDlg()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionAM, SIGNAL(triggered()), this, SIGNAL(NewAMAcquisition()));
    connect(ui->actionFM, SIGNAL(triggered()), this, SLOT(OnSwitchToFM()));
    connect(ui->actionDRM, SIGNAL(triggered()), this, SLOT(OnSwitchToDRM()));
    connect(pFileMenu, SIGNAL(soundFileChanged(CDRMReceiver::ESFStatus)), this, SLOT(OnSoundFileChanged(CDRMReceiver::ESFStatus)));
    connect(pSoundCardMenu, SIGNAL(sampleRateChanged()), this, SLOT(OnSampleRateChanged()));
    connect(ui->actionAbout_Dream, SIGNAL(triggered()), this, SLOT(OnHelpAbout()));
    connect(ui->actionWhats_This, SIGNAL(triggered()), this, SLOT(OnWhatsThis()));

    /* Init main plot */

    MainPlot = new CDRMPlot();
    ui->plotLayout->addWidget(MainPlot->widget());
    connect(MainPlot, SIGNAL(plotClicked(double)), this, SLOT(OnPlotClicked(double)));

    bool waterfall = getSetting("waterfall", false);
    ui->checkBoxWaterfall->setChecked(waterfall);

    /* Init bandwidth slider */
    UpdateSliderBandwidth();

    /* Init PLL phase dial control */
    initPhaseDial();

#ifdef HAVE_SPEEX
    ui->SpinBoxNoiRedLevel->setValue(controller->getReceiver()->GetAMDemod()->GetNoiRedLevel());
#else
    ui->RadioButtonNoiRedSpeex->hide();
    ui->SpinBoxNoiRedLevel->hide();
#endif

    /* Update controls */
    UpdateControls();

    /* Connect controls ----------------------------------------------------- */
    connect(ui->ButtonDRM, SIGNAL(clicked()),
            this, SLOT(OnSwitchToDRM()));
    connect(ui->ButtonAMSS, SIGNAL(clicked()),
            &AMSSDlg, SLOT(show()));

    connect(controller, SIGNAL(dataAvailable()), this, SLOT(on_new_data()));

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
    ui->controlsFrame->layout()->addWidget(phaseGauge);
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
    connect(controller, SIGNAL(dataAvailable()),this, SLOT(OnTimer()));
    UpdateControls();
    on_checkBoxWaterfall_toggled(ui->checkBoxWaterfall->isChecked());
}

void AnalogDemDlg::eventHide(QHideEvent*)
{
    /* Notify the MainPlot of hideEvent */
    /* stop real-time timers */
    disconnect(controller, SIGNAL(dataAvailable()),this, SLOT(OnTimer()));
    bool waterfall = ui->checkBoxWaterfall->isChecked();
    putSetting("waterfall", waterfall);
}

void AnalogDemDlg::eventClose(QCloseEvent* ce)
{
    if (!TimerClose.isActive())
    {
        /* Stop real-time timers */
        //Timer.stop();
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
        if (!ui->RadioButtonDemAM->isChecked())
            ui->RadioButtonDemAM->setChecked(true);
        break;

    case CAMDemodulation::DT_LSB:
        if (!ui->RadioButtonDemLSB->isChecked())
            ui->RadioButtonDemLSB->setChecked(true);
        break;

    case CAMDemodulation::DT_USB:
        if (!ui->RadioButtonDemUSB->isChecked())
            ui->RadioButtonDemUSB->setChecked(true);
        break;

    case CAMDemodulation::DT_CW:
        if (!ui->RadioButtonDemCW->isChecked())
            ui->RadioButtonDemCW->setChecked(true);
        break;

    case CAMDemodulation::DT_FM:
        if (!ui->RadioButtonDemFM->isChecked())
            ui->RadioButtonDemFM->setChecked(true);
        break;
    }

    /* Set AGC type */
    switch (controller->getReceiver()->GetAMDemod()->GetAGCType())
    {
    case CAGC::AT_NO_AGC:
        if (!ui->RadioButtonAGCOff->isChecked())
            ui->RadioButtonAGCOff->setChecked(true);
        break;

    case CAGC::AT_SLOW:
        if (!ui->RadioButtonAGCSlow->isChecked())
            ui->RadioButtonAGCSlow->setChecked(true);
        break;

    case CAGC::AT_MEDIUM:
        if (!ui->RadioButtonAGCMed->isChecked())
            ui->RadioButtonAGCMed->setChecked(true);
        break;

    case CAGC::AT_FAST:
        if (!ui->RadioButtonAGCFast->isChecked())
            ui->RadioButtonAGCFast->setChecked(true);
        break;
    }

    /* Set noise reduction type */
    switch (controller->getReceiver()->GetAMDemod()->GetNoiRedType())
    {
    case CAMDemodulation::NR_OFF:
        if (!ui->RadioButtonNoiRedOff->isChecked())
            ui->RadioButtonNoiRedOff->setChecked(true);
        break;

    case CAMDemodulation::NR_LOW:
        if (!ui->RadioButtonNoiRedLow->isChecked())
            ui->RadioButtonNoiRedLow->setChecked(true);
        break;

    case CAMDemodulation::NR_MEDIUM:
        if (!ui->RadioButtonNoiRedMed->isChecked())
            ui->RadioButtonNoiRedMed->setChecked(true);
        break;

    case CAMDemodulation::NR_HIGH:
        if (!ui->RadioButtonNoiRedHigh->isChecked())
            ui->RadioButtonNoiRedHigh->setChecked(true);
        break;

    case CAMDemodulation::NR_SPEEX:
        if (!ui->RadioButtonNoiRedSpeex->isChecked())
            ui->RadioButtonNoiRedSpeex->setChecked(true);
        break;
    }

#ifdef HAVE_SPEEX
    /* Set speex spinbox enable state */
    ui->SpinBoxNoiRedLevel->setEnabled(ui->RadioButtonNoiRedSpeex->isChecked());
#endif

    /* Set filter bandwidth */

    int fbw = controller->getReceiver()->GetAMDemod()->GetFilterBW();
    ui->SliderBandwidth->setValue(fbw);
    ui->ButtonBandWidth->setText(tr("%1 Hz").arg(fbw));

    /* Update check boxes */
    ui->CheckBoxMuteAudio->setChecked(controller->getReceiver()->GetWriteData()->GetMuteAudio());
    ui->CheckBoxSaveAudioWave->
    setChecked(controller->getReceiver()->GetWriteData()->GetIsWriteWaveFile());

    ui->CheckBoxAutoFreqAcq->
    setChecked(controller->getReceiver()->GetAMDemod()->AutoFreqAcqEnabled());

    ui->CheckBoxPLL->setChecked(controller->getReceiver()->GetAMDemod()->PLLEnabled());
}

void AnalogDemDlg::UpdateSliderBandwidth()
{
    ui->SliderBandwidth->setRange(0, controller->getReceiver()->GetParameters()->GetSigSampleRate() / 2);
}

void AnalogDemDlg::UpdatePlotStyle(int iPlotstyle)
{
    MainPlot->SetPlotStyle(iPlotstyle);
}

void AnalogDemDlg::OnSampleRateChanged()
{
    UpdateSliderBandwidth();
    on_checkBoxWaterfall_toggled(ui->checkBoxWaterfall->isChecked());
}

void AnalogDemDlg::OnSoundFileChanged(CDRMReceiver::ESFStatus)
{
    UpdateSliderBandwidth();
    on_checkBoxWaterfall_toggled(ui->checkBoxWaterfall->isChecked());
}

void AnalogDemDlg::on_new_data()
{
    MainPlot->update(controller);
    UpdateControls();
}

void AnalogDemDlg::OnTimer()
{
    if(subSampleCount==0)
    {
        subSampleCount=20;
    }
    else
    {
        subSampleCount--;
    }

    if(controller->getReceiver()->GetReceiverMode() != RM_AM)
        return;

    double rDCFreq = controller->getReceiver()->GetAMDemod()->GetCurMixFreqOffs();
    double f = controller->getReceiver()->GetReceiveData()->ConvertFrequency(rDCFreq);
    CVector<_REAL>  vecrData, vecrScale;

    if(subSampleCount == 0) {
        /* Carrier frequency of AM signal */
        ui->ButtonFreqOffset->setText(QString().setNum(f, 'f', 2) + " Hz");
    }
    if(subSampleCount==0 || subSampleCount == 10) { // Fast Update
        OnTimerPLLPhaseDial();
    }
}

void AnalogDemDlg::OnTimerPLLPhaseDial()
{
    CReal rCurPLLPhase;

    if (controller->getReceiver()->GetAMDemod()->GetPLLPhase(rCurPLLPhase))
    {
        /* Set current PLL phase (convert radiant in degree) */
        phaseGauge->setValue(rCurPLLPhase * (CReal) 360.0 / (2 * crPi));
    }
    else
    {
        /* Reset dial */
        double d = 0.0; // 360.0*double(qrand())/double(RAND_MAX);
        phaseGauge->setValue(d);
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
    CAMDemodulation::EDemodType n[] = {CAMDemodulation::DT_AM,CAMDemodulation::DT_LSB,CAMDemodulation::DT_USB,
                                       CAMDemodulation::DT_CW,CAMDemodulation::DT_FM
                                      };
    controller->setAnalogModulation(n[iID]);

    UpdateControls();
}

void AnalogDemDlg::on_ButtonGroupAGC_buttonClicked(int iID)
{
    iID = -iID - 2; // TODO understand why
    const CAGC::EType n[] = { CAGC::AT_NO_AGC,CAGC::AT_SLOW,CAGC::AT_MEDIUM,CAGC::AT_FAST};
    controller->setAnalogAGC(n[iID]);
}

void AnalogDemDlg::on_ButtonGroupNoiseReduction_buttonClicked(int iID)
{
    iID = -iID - 2; // TODO understand why
    const CAMDemodulation::ENoiRedType n[] = { CAMDemodulation::NR_OFF,CAMDemodulation::NR_LOW,
                                               CAMDemodulation::NR_MEDIUM,CAMDemodulation::NR_HIGH,
                                               CAMDemodulation::NR_SPEEX
                                             };
    controller->setAnalogNoiseReduction(n[iID]);

#ifdef HAVE_SPEEX
    /* Set speex spinbox enable state */
    ui->SpinBoxNoiRedLevel->setEnabled(ui->RadioButtonNoiRedSpeex->isChecked());
#endif
}

void AnalogDemDlg::on_SliderBandwidth_valueChanged(int value)
{
    /* Set new filter in processing module */
    controller->setAMFilterBW(value);
    ui->ButtonBandWidth->setText(tr("%1 Hz").arg(value));
}

void AnalogDemDlg::on_CheckBoxAutoFreqAcq_clicked(bool checked)
{
    /* Set parameter in working thread module */
    controller->setEnableAutoFreqAcq(checked);
}

void AnalogDemDlg::on_CheckBoxPLL_clicked(bool checked)
{
    /* Set parameter in working thread module */
    controller->setEnablePLL(checked);
}

void AnalogDemDlg::on_CheckBoxMuteAudio_clicked(bool checked)
{
    /* Set parameter in working thread module */
    controller->muteAudio(checked);
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
            controller->setSaveAudio(strFileName.toLocal8Bit().constData());
        }
        else
        {
            /* User hit the cancel button, uncheck the button */
            ui->CheckBoxSaveAudioWave->setChecked(false);
        }
    }
    else
        controller->setSaveAudio("");
}

void AnalogDemDlg::on_checkBoxWaterfall_toggled(bool checked)
{
    int sr = controller->getReceiver()->GetParameters()->GetSigSampleRate();
    /* Toggle between normal spectrum plot and waterfall spectrum plot */
    MainPlot->SetupChart(checked?INP_SPEC_WATERF:INPUT_SIG_PSD_ANALOG, sr);
}

/* Manual carrier frequency input box */
void AnalogDemDlg::on_ButtonFreqOffset_clicked(bool)
{
    bool ok = false;
    const double prev_freq =
        controller->getReceiver()->GetReceiveData()->ConvertFrequency(
            controller->getReceiver()->GetAMDemod()->GetCurMixFreqOffs());
    const double new_freq = QInputDialog::getDouble(this, this->windowTitle(),
                            ui->LabelCarrierFrequency->text(), prev_freq, -1e6, 1e6, 2, &ok);
    if (ok)
    {
        OnPlotClicked(new_freq);
    }
}

void AnalogDemDlg::OnPlotClicked(double d)
{
    const _REAL conv_freq =
        controller->getReceiver()->GetReceiveData()->ConvertFrequency(d, true);
    double dVal = conv_freq /
                  (controller->getReceiver()->GetParameters()->GetSigSampleRate() / 2);
    if (dVal < 0.0)
        dVal = 0.0;
    else if (dVal > 1.0)
        dVal = 1.0;
    /* Set new frequency in receiver module */
    controller->setAMDemodAcq(dVal);
}

/* Manual band width input box */
void AnalogDemDlg::on_ButtonBandWidth_clicked(bool)
{
    bool ok = false;
    const int sr2 = controller->getReceiver()->GetParameters()->GetSigSampleRate() / 2;
    const int prev_bw = controller->getReceiver()->GetAMDemod()->GetFilterBW();
    const int new_bw = QInputDialog::getInt(this, this->windowTitle(),
                                            ui->groupBoxBW->title(), prev_bw, 0, sr2, 2, &ok);
    if (ok)
    {
        controller->setAMFilterBW(new_bw);
    }
}

void AnalogDemDlg::on_SpinBoxNoiRedLevel_valueChanged(int value)
{
    controller->setAnalogNoiseReduction(value);
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

    ui->RadioButtonNoiRedOff->setWhatsThis(strNoiseReduction);
    ui->RadioButtonNoiRedLow->setWhatsThis(strNoiseReduction);
    ui->RadioButtonNoiRedMed->setWhatsThis(strNoiseReduction);
    ui->RadioButtonNoiRedHigh->setWhatsThis(strNoiseReduction);
    ui->RadioButtonAGCOff->setWhatsThis(strAGC);
    ui->RadioButtonAGCSlow->setWhatsThis(strAGC);
    ui->RadioButtonAGCMed->setWhatsThis(strAGC);
    ui->RadioButtonAGCFast->setWhatsThis(strAGC);
    ui->ButtonBandWidth->setWhatsThis(strFilterBW);
    ui->SliderBandwidth->setWhatsThis(strFilterBW);
    ui->RadioButtonDemAM->setWhatsThis(strDemodType);
    ui->RadioButtonDemLSB->setWhatsThis(strDemodType);
    ui->RadioButtonDemUSB->setWhatsThis(strDemodType);
    ui->RadioButtonDemCW->setWhatsThis(strDemodType);
    ui->RadioButtonDemFM->setWhatsThis(strDemodType);
    ui->GroupBoxAutoFreqAcq->setWhatsThis(strAutoFreqAcqu);
    ui->CheckBoxAutoFreqAcq->setWhatsThis(strAutoFreqAcqu);
    ui->CheckBoxMuteAudio->setWhatsThis(strCheckBoxMuteAudio);
    ui->GroupBoxPLL->setWhatsThis(strPLL);
    ui->CheckBoxPLL->setWhatsThis(strPLL);
    phaseGauge->setWhatsThis(strPLL);
    ui->TextLabelPhaseOffset->setWhatsThis(strPLL);
    ui->ButtonFreqOffset->setWhatsThis(strTextFreqOffset);
    ui->ButtonFreqOffset->setWhatsThis(strTextFreqOffset);
    ui->CheckBoxSaveAudioWave->setWhatsThis(strCheckBoxSaveAudioWave);
    ui->groupBoxNoiseReduction->setWhatsThis(strNoiseReduction);
    ui->groupBoxAGC->setWhatsThis(strAGC);
    ui->groupBoxDemodulation->setWhatsThis(strDemodType);
    ui->groupBoxBW->setWhatsThis(strFilterBW);
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
    CWindow(parent, Settings, "AMSS"), ui(new Ui::CAMSSDlgBase),
    DRMReceiver(NDRMR)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window); // window was not decorated.

    /* Init AMSS PLL phase dial control */
    phaseGauge = new PhaseGauge(this);
    phaseGauge->setMinimumHeight(100);
    phaseGauge->setMinimumWidth(100);
    ui->phaseFrame->layout()->addWidget(phaseGauge);

    /* Set help text for the controls */
    AddWhatsThisHelp();


    ui->TextAMSSServiceLabel->setText("");
    ui->TextAMSSCountryCode->setText("");
    ui->TextAMSSTimeDate->setText("");
    ui->TextAMSSLanguage->setText("");
    ui->TextAMSSServiceID->setText("");
    ui->TextAMSSAMCarrierMode->setText("");
    ui->TextAMSSInfo->setText("");

    ui->ListBoxAMSSAFSList->setEnabled(false);


    /* Connect controls ----------------------------------------------------- */
    /* Timers */
    connect(&Timer, SIGNAL(timeout()),this, SLOT(OnTimer()));
    connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
            this, SLOT(OnTimerPLLPhaseDial()));
    /* Buttons */
    connect(ui->buttonOk, SIGNAL(clicked()), this, SLOT(close()));

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
    //Timer.start(GUI_CONTROL_UPDATE_TIME);
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
        ui->TextAMSSServiceLabel->setText(QString().fromUtf8(
                                              Parameters.Service[0].strLabel.c_str()
                                          ));
    }
    else
        ui->TextAMSSServiceLabel->setText(tr(""));

    /* Country code */
    const string strCntryCode = Parameters.Service[0].strCountryCode; /* must be of 2 lowercase chars */

    if ((Parameters.Service[0].IsActive()) && (!strCntryCode.empty()) && (strCntryCode != "--"))
    {
        ui->TextAMSSCountryCode->
        setText(QString(GetISOCountryName(strCntryCode).c_str()));
    }
    else
        ui->TextAMSSCountryCode->setText("");

    /* SDC Language code */

    if (Parameters.Service[0].IsActive())
    {
        const string strLangCode = Parameters.Service[0].strLanguageCode; /* must be of 3 lowercase chars */

        if ((!strLangCode.empty()) && (strLangCode != "---"))
            ui->TextAMSSLanguage->
            setText(QString(GetISOLanguageName(strLangCode).c_str()));
        else
            ui->TextAMSSLanguage->setText(QString(strTableLanguageCode[Parameters.Service[0].iLanguage].c_str()));
    }
    else
        ui->TextAMSSLanguage->setText("");

    /* Time, date */
    if ((Parameters.iUTCHour == 0) &&
            (Parameters.iUTCMin == 0) &&
            (Parameters.iDay == 0) &&
            (Parameters.iMonth == 0) &&
            (Parameters.iYear == 0))
    {
        /* No time service available */
        ui->TextAMSSTimeDate->setText("");
    }
    else
    {
        QDateTime DateTime;
        DateTime.setDate(QDate(Parameters.iYear, Parameters.iMonth, Parameters.iDay));
        DateTime.setTime(QTime(Parameters.iUTCHour, Parameters.iUTCMin));

        ui->TextAMSSTimeDate->setText(DateTime.toString());
    }

    /* Get number of alternative services */
    const size_t iNumAltServices = Parameters.AltFreqSign.vecOtherServices.size();

    if (iNumAltServices != 0)
    {
        QString val = QString().setNum((long) iNumAltServices);
        ui->ListBoxAMSSAFSList->insertItem(10, val);

        ui->ListBoxAMSSAFSList->clear();
        ui->ListBoxAMSSAFSList->setEnabled(true);

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
            ui->ListBoxAMSSAFSList->insertItem(0, freqEntry);
        }
    }
    else
    {
        ui->ListBoxAMSSAFSList->clear();
        ui->ListBoxAMSSAFSList->setEnabled(false);
    }

    ui->TextAMSSServiceID->setText("");
    ui->TextAMSSAMCarrierMode->setText("");

    if (DRMReceiver.GetAMSSDecode()->GetLockStatus() == CAMSSDecode::NO_SYNC
            || Parameters.Service[0].iServiceID == SERV_ID_NOT_USED
       )
    {
        ui->TextAMSSInfo->setText(tr("No AMSS detected"));
    }
    else
    {
        ui->TextAMSSInfo->setText(tr("Awaiting AMSS data..."));

        /* Display 'block 1' info */
        if (DRMReceiver.GetAMSSDecode()->GetBlock1Status())
        {
            ui->TextAMSSInfo->setText("");

            ui->TextAMSSLanguage->setText(QString(strTableLanguageCode[DRMReceiver.
                                                  GetParameters()->Service[0].iLanguage].c_str()));

            ui->TextAMSSServiceID->setText("ID:" + QString().setNum(
                                               (long) Parameters.Service[0].iServiceID, 16).toUpper());

            ui->TextAMSSAMCarrierMode->setText(QString(strTableAMSSCarrierMode[DRMReceiver.
                                               GetParameters()->iAMSSCarrierMode].c_str()));
        }
    }

    ui->TextDataEntityGroupStatus->setText(DRMReceiver.GetAMSSDecode()->
                                           GetDataEntityGroupStatus());

    ui->TextCurrentBlock->setText(QString().setNum(DRMReceiver.GetAMSSDecode()->
                                  GetCurrentBlock(), 10));

    ui->TextBlockBits->setText(DRMReceiver.GetAMSSDecode()->GetCurrentBlockBits());

    int val = DRMReceiver.GetAMSSDecode()->GetPercentageDataEntityGroupComplete();
    Parameters.Unlock();
    ui->ProgressBarAMSS->setValue(val);
}

void CAMSSDlg::OnTimerPLLPhaseDial()
{
    CReal rCurPLLPhase;

    if (DRMReceiver.GetAMSSPhaseDemod()->GetPLLPhase(rCurPLLPhase))
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
