/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Volker Fischer
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

#include "EvaluationDlg.h"
#include "DialogUtil.h"
#include "receivercontroller.h"
#include "../util-QT/Util.h"
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
#endif
#include <QMessageBox>
#include <QLayout>
#include <QDateTime>
#include <QFileDialog>
#include <QHideEvent>
#include <QShowEvent>
#include <QTreeWidgetItemIterator>
#include "ThemeCustomizer.h"
#include "DialogUtil.h"
#include "drmdetail.h"

/* Implementation *************************************************************/
systemevalDlg::systemevalDlg(ReceiverController* rc, CSettings& Settings,
                             QWidget* parent) :
    CWindow(parent, Settings, "System Evaluation"),
    ui(new Ui::SystemEvaluationWindow()),
    controller(rc),Parameters(*rc->getReceiver()->GetParameters()),
    eNewCharType(NONE_OLD), iPlotStyle(0)
{
    ui->setupUi(this);

    /* Set help text for the controls */
    AddWhatsThisHelp();

    detail = new DRMDetail(this);
    ui->display->addWidget(detail);

    /* Init controls -------------------------------------------------------- */

    /* Init main plot */
    MainPlot = new CDRMPlot();
    ui->plotLayout->addWidget(MainPlot->widget());

    /* Initialise controls */
    setControls(Settings);

    MainPlot->setupTreeWidget(ui->chartSelector);

    /* Load saved main plot type */
    eCurCharType = PlotNameToECharType(getSetting("plottype", QString()));

    /* If MDI in is enabled, disable some of the controls and use different
       initialization for the chart and chart selector */
    if (controller->getReceiver()->GetRSIIn()->GetInEnabled())
    {
        ui->SliderNoOfIterations->setEnabled(false);

        ui->ButtonGroupChanEstFreqInt->setEnabled(false);
        ui->ButtonGroupChanEstTimeInt->setEnabled(false);
        ui->ButtonGroupTimeSyncTrack->setEnabled(false);
        ui->CheckBoxFlipSpec->setEnabled(false);
        ui->GroupBoxInterfRej->setEnabled(false);

        /* Only audio spectrum makes sence for MDI in */
        eCurCharType = AUDIO_SPECTRUM;
    }

    /* Init context menu for tree widget */
    pTreeWidgetContextMenu = new QMenu(tr("Chart Selector context menu"), this);
    pTreeWidgetContextMenu->addAction(tr("&Open in separate window"),
                                      this, SLOT(OnTreeWidgetContMenu(bool)));

    /* Connect controls ----------------------------------------------------- */
    connect(ui->SliderNoOfIterations, SIGNAL(valueChanged(int)),
            this, SLOT(OnSliderIterChange(int)));

    /* Radio buttons */
    connect(ui->RadioButtonTiLinear, SIGNAL(clicked()),
            this, SLOT(OnRadioTimeLinear()));
    connect(ui->RadioButtonTiWiener, SIGNAL(clicked()),
            this, SLOT(OnRadioTimeWiener()));
    connect(ui->RadioButtonFreqLinear, SIGNAL(clicked()),
            this, SLOT(OnRadioFrequencyLinear()));
    connect(ui->RadioButtonFreqDFT, SIGNAL(clicked()),
            this, SLOT(OnRadioFrequencyDft()));
    connect(ui->RadioButtonFreqWiener, SIGNAL(clicked()),
            this, SLOT(OnRadioFrequencyWiener()));
    connect(ui->RadioButtonTiSyncEnergy, SIGNAL(clicked()),
            this, SLOT(OnRadioTiSyncEnergy()));
    connect(ui->RadioButtonTiSyncFirstPeak, SIGNAL(clicked()),
            this, SLOT(OnRadioTiSyncFirstPeak()));

    /* Char selector list view */
    connect(ui->chartSelector, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(OnListSelChanged( QTreeWidgetItem *, QTreeWidgetItem *)));
    ui->chartSelector->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->chartSelector, SIGNAL(customContextMenuRequested ( const QPoint&  )),
            this, SLOT(OnCustomContextMenuRequested(const QPoint&)));

    /* Buttons */
    connect(ui->buttonOk, SIGNAL(clicked()), this, SLOT(close()));

    /* Check boxes */
    connect(ui->CheckBoxFlipSpec, SIGNAL(clicked()),
            this, SLOT(OnCheckFlipSpectrum()));
    connect(ui->CheckBoxMuteAudio, SIGNAL(clicked()),
            this, SLOT(OnCheckBoxMuteAudio()));
    connect(ui->CheckBoxWriteLog, SIGNAL(stateChanged(int)),
            this, SLOT(OnCheckWriteLog(int)));
    connect(ui->CheckBoxSaveAudioWave, SIGNAL(clicked()),
            this, SLOT(OnCheckSaveAudioWAV()));
    connect(ui->CheckBoxRecFilter, SIGNAL(clicked()),
            this, SLOT(OnCheckRecFilter()));
    connect(ui->CheckBoxModiMetric, SIGNAL(clicked()),
            this, SLOT(OnCheckModiMetric()));
    connect(ui->CheckBoxReverb, SIGNAL(clicked()),
            this, SLOT(OnCheckBoxReverb()));

    APPLY_CUSTOM_THEME();
}

void systemevalDlg::connectController(ReceiverController* controller)
{
    connect(controller, SIGNAL(FACChanged(ETypeRxStatus)), detail, SLOT(setLEDFAC(ETypeRxStatus)));
    connect(controller, SIGNAL(SDCChanged(ETypeRxStatus)), detail, SLOT(setLEDSDC(ETypeRxStatus)));
    connect(controller, SIGNAL(MSCChanged(ETypeRxStatus)), detail, SLOT(setLEDMSC(ETypeRxStatus)));
    connect(controller, SIGNAL(FSyncChanged(ETypeRxStatus)), detail, SLOT(setLEDFrameSync(ETypeRxStatus)));
    connect(controller, SIGNAL(TSyncChanged(ETypeRxStatus)), detail, SLOT(setLEDTimeSync(ETypeRxStatus)));
    connect(controller, SIGNAL(InputStatusChanged(ETypeRxStatus)), detail, SLOT(setLEDIOInterface(ETypeRxStatus)));
    connect(controller, SIGNAL(channelReceptionChanged(Reception&)), this, SLOT(onReception(Reception&)));
    connect(controller, SIGNAL(channelConfigurationChanged(ChannelConfiguration&)), this, SLOT(onChannel(ChannelConfiguration&)));

    connect(this, SIGNAL(saveAudio(const string&)), controller, SLOT(setSaveAudio(const string&)));
    connect(this, SIGNAL(muteAudio(bool)), controller, SLOT(muteAudio(bool)));
    connect(this, SIGNAL(setReverbEffect(bool)), controller, SLOT(setReverbEffect(bool)));
    connect(this, SIGNAL(setRecFilter(bool)), controller, SLOT(setRecFilter(bool)));
    connect(this, SIGNAL(setFlippedSpectrum(bool)), controller, SLOT(setFlippedSpectrum(bool)));
    connect(this, SIGNAL(setIntCons(bool)), controller, SLOT(setIntCons(bool)));
    connect(this, SIGNAL(setNumMSCMLCIterations(int)), controller, SLOT(setNumMSCMLCIterations(int)));
    connect(this, SIGNAL(setTimeInt(int)), controller, SLOT(setTimeInt(int)));
    connect(this, SIGNAL(setFreqInt(int)), controller, SLOT(setFreqInt(int)));
    connect(this, SIGNAL(setTiSyncTracType(int)), controller, SLOT(setTiSyncTracType(int)));
}

systemevalDlg::~systemevalDlg()
{
    // close wav file
    if(ui->CheckBoxSaveAudioWave->isChecked())
        emit saveAudio("");
    delete MainPlot;
}

void systemevalDlg::onChannel(ChannelConfiguration& c)
{
    detail->setChannel(ERobMode(c.robm), ESpecOcc(c.mode), ESymIntMod(c.interl),
               ECodScheme(c.sdcConst), ECodScheme(c.mscConst));
    detail->setCodeRate(c.protLev.iPartA, c.protLev.iPartB);
    detail->setNumServices(c.nAudio, c.nData);
}

void systemevalDlg::onReception(Reception& r)
{
    detail->setSNR(r.snr);
    detail->setMER(r.mer, r.wmer);
    detail->setDelay_Doppler(r.sigmaEstimate, r.minDelay);
    detail->setSampleFrequencyOffset(r.sampleOffset, r.sampleRate);
    detail->setFrequencyOffset(r.dcOffset);
}

void systemevalDlg::setControls(CSettings& s)
{
    /* ui->Slider for MLC number of iterations */
    const int iNumIt = s.Get("Receiver", "mlciter", 1);
    if (ui->SliderNoOfIterations->value() != iNumIt)
    {
        /* Update slider and label */
        ui->SliderNoOfIterations->setValue(iNumIt);
        ui->TextNumOfIterations->setText(tr("MLC Iterations: ") +
                                     QString().setNum(iNumIt));
    }

    /* Update for channel estimation and time sync switches */
    switch (s.Get("Receiver", "timeint", 1))
    {
    case CChannelEstimation::TLINEAR:
        if (!ui->RadioButtonTiLinear->isChecked())
            ui->RadioButtonTiLinear->setChecked(true);
        break;

    case CChannelEstimation::TWIENER:
        if (!ui->RadioButtonTiWiener->isChecked())
            ui->RadioButtonTiWiener->setChecked(true);
        break;
    }

    switch (s.Get("Receiver", "freqint", 1))
    {
    case CChannelEstimation::FLINEAR:
        if (!ui->RadioButtonFreqLinear->isChecked())
            ui->RadioButtonFreqLinear->setChecked(true);
        break;

    case CChannelEstimation::FDFTFILTER:
        if (!ui->RadioButtonFreqDFT->isChecked())
            ui->RadioButtonFreqDFT->setChecked(true);
        break;

    case CChannelEstimation::FWIENER:
        if (!ui->RadioButtonFreqWiener->isChecked())
            ui->RadioButtonFreqWiener->setChecked(true);
        break;
    }

    switch (s.Get("Receiver", "timesync", 1))
    {
    case CTimeSyncTrack::TSFIRSTPEAK:
        if (!ui->RadioButtonTiSyncFirstPeak->isChecked())
            ui->RadioButtonTiSyncFirstPeak->setChecked(true);
        break;

    case CTimeSyncTrack::TSENERGY:
        if (!ui->RadioButtonTiSyncEnergy->isChecked())
            ui->RadioButtonTiSyncEnergy->setChecked(true);
        break;
    }

    /* Update settings checkbuttons */
    ui->CheckBoxReverb->setChecked(s.Get("Receiver", "reverb", false));
    ui->CheckBoxRecFilter->setChecked(s.Get("Receiver", "filter", false));
    ui->CheckBoxModiMetric->setChecked(s.Get("Receiver", "modemetric", false));
    ui->CheckBoxMuteAudio->setChecked(s.Get("Receiver", "muteaudio", false));
    ui->CheckBoxFlipSpec->setChecked(s.Get("Receiver", "flipspectrum", false));

    ui->CheckBoxSaveAudioWave->setChecked(s.Get("command", "writewav")!="");
}

void systemevalDlg::eventShow(QShowEvent*)
{
    /* Restore chart windows */
    const int iNumChartWin = getSetting("numchartwin", 0);
    for (int i = 0; i < iNumChartWin; i++)
    {
        stringstream s;

        /* create the section key for this window */
        s << "Chart Window " << i;

        /* get the chart type */
        const ECharType eNewType = PlotNameToECharType(QString(Settings.Get(s.str(), "plottype", string()).c_str()));

        /* get window geometry data */
        CWinGeom c;
        Settings.Get(s.str(), c);
        const QRect WinGeom(c.iXPos, c.iYPos, c.iWSize, c.iHSize);

        /* Open the new chart window */
        ChartDialog* pNewChartWin = OpenChartWin(eNewType);

        /* and restore its geometry */
        if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
            pNewChartWin->setGeometry(WinGeom);

        /* Add window pointer in vector (needed for closing the windows) */
        vecpDRMPlots.push_back(pNewChartWin);

        /* Show new window */
        pNewChartWin->show();
    }

    // pass data available events on to plots
    connect(controller, SIGNAL(dataAvailable()), this, SLOT(OnDataAvailable()));

    selectChart(eCurCharType);
}

void systemevalDlg::eventHide(QHideEvent*)
{
    // stop passing data available events on to plots
    disconnect(controller, SIGNAL(dataAvailable()), this, SLOT(OnDataAvailable()));

    /* Store size and position of all additional chart windows */
    int iNumOpenCharts = 0;

    for (size_t i = 0; i < vecpDRMPlots.size(); i++)
    {
        /* Check, if window wasn't closed by the user */
        if (vecpDRMPlots[i]->isVisible())
        {
            stringstream s;
            CWinGeom c;
            const QRect CWGeom = vecpDRMPlots[i]->geometry();

            /* Set parameters */
            c.iXPos = CWGeom.x();
            c.iYPos = CWGeom.y();
            c.iHSize = CWGeom.height();
            c.iWSize = CWGeom.width();

            s << "Chart Window " << iNumOpenCharts;
            Settings.Put(s.str(), c);
            Settings.Put(s.str(), "plottype", ECharTypeToPlotName(vecpDRMPlots[i]->getChartType()).toStdString());

            iNumOpenCharts++;
        }
        /* Close window afterwards */
        vecpDRMPlots[i]->close();
    }
    putSetting("numchartwin", iNumOpenCharts);

    /* We do not need the pointers anymore, reset vector */
    vecpDRMPlots.clear();

    /* Store current plot type */
    putSetting("plottype", ECharTypeToPlotName(eCurCharType));
}

void systemevalDlg::UpdatePlotStyle(int iPlotStyle)
{
    this->iPlotStyle = iPlotStyle;

    /* Update chart windows */
    for (size_t i = 0; i < vecpDRMPlots.size(); i++)
        vecpDRMPlots[i]->SetPlotStyle(iPlotStyle);

    /* Update main plot window */
    MainPlot->SetPlotStyle(iPlotStyle);
}

void systemevalDlg::OnTreeWidgetContMenu(bool)
{
    if (eNewCharType != NONE_OLD)
    {
        /* Open the new chart */
        ChartDialog* pNewChartWin = OpenChartWin(eNewCharType);
        vecpDRMPlots.push_back(pNewChartWin);

        /* Show new window */
        pNewChartWin->show();

        eNewCharType = NONE_OLD;
    }
}

void systemevalDlg::OnCustomContextMenuRequested(const QPoint& p)
{
    QModelIndex index = ui->chartSelector->indexAt(p);
    /* Make sure we have a non root item */
    if (index.parent() != QModelIndex())
    {
        /* Popup the context menu */
        eNewCharType = ECharType(index.data(Qt::UserRole).toInt());
        pTreeWidgetContextMenu->exec(QCursor::pos());
    }
}

ChartDialog *systemevalDlg::OpenChartWin(ECharType eNewType)
{
    /* Create new chart window */
    ChartDialog* pNewChartWin = new ChartDialog(NULL);
    pNewChartWin->setWindowTitle(tr("Chart Window"));

    /* Set correct icon (use the same as this dialog) */
    const QIcon& icon = windowIcon();
    pNewChartWin->setWindowIcon(icon);

    /* Set receiver object and correct chart type */
    pNewChartWin->SetupChart(eNewType, controller->getReceiver()->GetParameters()->GetSigSampleRate());

    /* Set plot style*/
    pNewChartWin->SetPlotStyle(iPlotStyle);

    connect(this, SIGNAL(dataAvailable(ReceiverController*)), pNewChartWin, SLOT(update(ReceiverController*)));

    return pNewChartWin;
}

ECharType systemevalDlg::PlotNameToECharType(const QString& plotName)
{
    QTreeWidgetItemIterator it(ui->chartSelector, QTreeWidgetItemIterator::NoChildren);
    while (*it) {
        if (plotName == (*it)->text(0))
            return ECharType((*it)->data(0, Qt::UserRole).toInt());
        ++it;
    }
    return AUDIO_SPECTRUM; /* safe value */
}

QString systemevalDlg::ECharTypeToPlotName(ECharType eCharType)
{
    QTreeWidgetItemIterator it(ui->chartSelector, QTreeWidgetItemIterator::NoChildren);
    while (*it) {
        ECharType eCurCharType = ECharType((*it)->data(0, Qt::UserRole).toInt());
        if (eCurCharType == eCharType)
            return (*it)->text(0);
        ++it;
    }
    return QString();
}

void systemevalDlg::selectChart(ECharType eCharType)
{
    QTreeWidgetItemIterator it(ui->chartSelector, QTreeWidgetItemIterator::NoChildren);
    while (*it) {
        ECharType eCurCharType = ECharType((*it)->data(0, Qt::UserRole).toInt());
        if (eCurCharType == eCharType) {
            ui->chartSelector->setCurrentItem(*it); // this version generates the signal we want
        }
        ++it;
    }
}

void systemevalDlg::OnDataAvailable()
{
    MainPlot->update(controller);
    emit dataAvailable(controller);
}

void systemevalDlg::UpdateGPS(CParameter& Parameters)
{
    gps_data_t& gps = Parameters.gps_data;

    if((gps.set&STATUS_SET)==0) {
        ui->LEDGPS->SetLight(CMultColorLED::RL_RED);
    } else {

        if(gps.status==0)
            ui->LEDGPS->SetLight(CMultColorLED::RL_YELLOW);
        else
            ui->LEDGPS->SetLight(CMultColorLED::RL_GREEN);
    }

    QString qStrPosition;
    if (gps.set&LATLON_SET)
        qStrPosition = QString(trUtf8("Lat: %1°  Long: %2°")).arg(gps.fix.latitude, 0, 'f', 4).arg(gps.fix.longitude,0, 'f',4);
    else
        qStrPosition = tr("Lat: ?  Long: ?");

    QString qStrAltitude;
    if (gps.set&ALTITUDE_SET)
        qStrAltitude = QString(tr("  Alt: %1 m")).arg(gps.fix.altitude, 0, 'f', 0);
    else
        qStrAltitude = tr("  Alt: ?");
    QString qStrSpeed;
    if (gps.set&SPEED_SET)
        qStrSpeed = QString(tr("Speed: %1 m/s")).arg(gps.fix.speed, 0, 'f', 1);
    else
        qStrSpeed = tr("Speed: ?");
    QString qStrTrack;
    if (gps.set&TRACK_SET)
        qStrTrack =  QString(tr("  Track: %1\260")).arg(gps.fix.track);
    else
        qStrTrack =  tr("  Track: ?");
    QString qStrTime;
    if (gps.set&TIME_SET)
    {
        struct tm * p_ts;
        time_t tt = time_t(gps.fix.time);
        p_ts = gmtime(&tt);
        QChar fill('0');
        qStrTime = QString("UTC: %1/%2/%3 %4:%5:%6  ")
                   .arg(1900 + p_ts->tm_year)
                   .arg(1 + p_ts->tm_mon, 2, 10, fill)
                   .arg(p_ts->tm_mday, 2, 10, fill)
                   .arg(p_ts->tm_hour, 2, 10, fill)
                   .arg(p_ts->tm_min, 2, 10, fill)
                   .arg(p_ts->tm_sec,2, 10, fill);
    }
    else
        qStrTime = "UTC: ?";
    QString qStrSat;
    if (gps.set&SATELLITE_SET)
        qStrSat = tr("  Satellites: ") + QString().setNum(gps.satellites_used);
    else
        qStrSat = tr("  Satellites: ?");

    ui->TextLabelGPSPosition->setText(qStrPosition+qStrAltitude);
    ui->TextLabelGPSSpeedHeading->setText(qStrSpeed+qStrTrack);
    ui->TextLabelGPSTime->setText(qStrTime+qStrSat);
}


void systemevalDlg::OnRadioTimeLinear()
{
    emit setTimeInt(CChannelEstimation::TLINEAR);
}

void systemevalDlg::OnRadioTimeWiener()
{
    emit setTimeInt(CChannelEstimation::TWIENER);
}

void systemevalDlg::OnRadioFrequencyLinear()
{
    emit setFreqInt(CChannelEstimation::FLINEAR);
}

void systemevalDlg::OnRadioFrequencyDft()
{
    emit setFreqInt(CChannelEstimation::FDFTFILTER);
}

void systemevalDlg::OnRadioFrequencyWiener()
{
    emit setFreqInt(CChannelEstimation::FWIENER);
}

void systemevalDlg::OnRadioTiSyncFirstPeak()
{
    emit setTiSyncTracType(CTimeSyncTrack::TSFIRSTPEAK);
}

void systemevalDlg::OnRadioTiSyncEnergy()
{
    emit setTiSyncTracType(CTimeSyncTrack::TSENERGY);
}

void systemevalDlg::OnSliderIterChange(int value)
{
    emit setNumMSCMLCIterations(value);
    /* Show the new value in the label control */
    ui->TextNumOfIterations->setText(tr("MLC Iterations: ") +
                                 QString().setNum(value));
}

void systemevalDlg::OnListSelChanged(QTreeWidgetItem *curr, QTreeWidgetItem *)
{
    /* Make sure we have a non root item */
    if (curr && curr->parent() != NULL)
    {
        /* Get chart type from selected item */
        eCurCharType = ECharType(curr->data(0, Qt::UserRole).toInt());
        /* Setup chart */
        MainPlot->SetupChart(eCurCharType, controller->getReceiver()->GetParameters()->GetSigSampleRate());
    }
}

void systemevalDlg::OnCheckFlipSpectrum()
{
    emit setFlippedSpectrum(ui->CheckBoxFlipSpec->isChecked());
}

void systemevalDlg::OnCheckRecFilter()
{
    emit setRecFilter(ui->CheckBoxRecFilter->isChecked());
}

void systemevalDlg::OnCheckModiMetric()
{
    emit setIntCons(ui->CheckBoxModiMetric->isChecked());
}

void systemevalDlg::OnCheckBoxMuteAudio()
{
    emit muteAudio(ui->CheckBoxMuteAudio->isChecked());
}

void systemevalDlg::OnCheckBoxReverb()
{
    emit setReverbEffect(ui->CheckBoxReverb->isChecked());
}

void systemevalDlg::OnCheckSaveAudioWAV()
{
    /*
        This code is copied in AnalogDemDlg.cpp. If you do changes here, you should
        apply the changes in the other file, too
    */
    if (ui->CheckBoxSaveAudioWave->isChecked())
    {
        /* Show "save file" dialog */
        QString strFileName =
            QFileDialog::getSaveFileName(this, "*.wav", tr("DreamOut.wav"));

        /* Check if user not hit the cancel button */
        if (!strFileName.isEmpty())
        {
            string s = strFileName.toUtf8().constData();
            emit saveAudio(s);
        }
        else
        {
            /* User hit the cancel button, uncheck the button */
            ui->CheckBoxSaveAudioWave->setChecked(false);
        }
    }
    else
        emit saveAudio("");
}


void systemevalDlg::OnCheckWriteLog(int state)
{
    if (state == Qt::Checked)
    {
        emit startLogging();
    }
    else
    {
        emit stopLogging();
    }
}

QString systemevalDlg::GetRobModeStr(ERobMode v)
{
    switch (v)
    {
    case RM_ROBUSTNESS_MODE_A:
        return "A";
        break;

    case RM_ROBUSTNESS_MODE_B:
        return "B";
        break;

    case RM_ROBUSTNESS_MODE_C:
        return "C";
        break;

    case RM_ROBUSTNESS_MODE_D:
        return "D";
        break;

    case RM_ROBUSTNESS_MODE_E:
        return "E";
        break;

    default:
        return "?";
    }
}

QString systemevalDlg::GetSpecOccStr(ESpecOcc v)
{
    switch (v)
    {
    case SO_0:
        return "4.5 kHz";
        break;

    case SO_1:
        return "5 kHz";
        break;

    case SO_2:
        return "9 kHz";
        break;

    case SO_3:
        return "10 kHz";
        break;

    case SO_4:
        return "18 kHz";
        break;

    case SO_5:
        return "20 kHz";
        break;

    default:
        return "?";
    }
}

void systemevalDlg::AddWhatsThisHelp()
{
    const QString strNumOfIterations =
        tr("<b>MLC, Number of Iterations:</b> In DRM, a "
           "multilevel channel coder is used. With this code it is possible to "
           "iterate the decoding process in the decoder to improve the decoding "
           "result. The more iterations are used the better the result will be. "
           "But switching to more iterations will increase the CPU load. "
           "Simulations showed that the first iteration (number of "
           "iterations = 1) gives the most improvement (approx. 1.5 dB at a "
           "BER of 10-4 on a Gaussian channel, Mode A, 10 kHz bandwidth). The "
           "improvement of the second iteration will be as small as 0.3 dB."
           "<br>The recommended number of iterations given in the DRM "
           "standard is one iteration (number of iterations = 1).");

    ui->TextNumOfIterations->setWhatsThis(strNumOfIterations);
    ui->SliderNoOfIterations->setWhatsThis(strNumOfIterations);

    /* Flip Input Spectrum */
    ui->CheckBoxFlipSpec->setWhatsThis(
        tr("<b>Flip Input Spectrum:</b> Checking this box "
           "will flip or invert the input spectrum. This is necessary if the "
           "mixer in the front-end uses the lower side band."));

    /* Mute Audio */
    ui->CheckBoxMuteAudio->setWhatsThis(
        tr("<b>Mute Audio:</b> The audio can be muted by "
           "checking this box. The reaction of checking or unchecking this box "
           "is delayed by approx. 1 second due to the audio buffers."));

    /* Reverberation Effect */
    ui->CheckBoxReverb->setWhatsThis(
        tr("<b>Reverberation Effect:</b> If this check box is checked, a "
           "reverberation effect is applied each time an audio drop-out occurs. "
           "With this effect it is possible to mask short drop-outs."));

    /* Log File */
    ui->CheckBoxWriteLog->setWhatsThis(
        tr("<b>Log File:</b> Checking this box brings the "
           "Dream software to write a log file about the current reception. "
           "Every minute the average SNR, number of correct decoded FAC and "
           "number of correct decoded MSC blocks are logged including some "
           "additional information, e.g. the station label and bit-rate. The "
           "log mechanism works only for audio services using AAC source coding. "
#ifdef _WIN32
           "During the logging no Dream windows "
           "should be moved or re-sized. This can lead to incorrect log files "
           "(problem with QT timer implementation under Windows). This problem "
           "does not exist in the Linux version of Dream."
#endif
           "<br>The log file will be "
           "written in the directory were the Dream application was started and "
           "the name of this file is always DreamLog.txt"));

    /* Wiener */
    const QString strWienerChanEst =
        tr("<b>Channel Estimation Settings:</b> With these "
           "settings, the channel estimation method in time and frequency "
           "direction can be selected. The default values use the most powerful "
           "algorithms. For more detailed information about the estimation "
           "algorithms there are a lot of papers and books available.<br>"
           "<b>Wiener:</b> Wiener interpolation method "
           "uses estimation of the statistics of the channel to design an optimal "
           "filter for noise reduction.");

    ui->RadioButtonFreqWiener->setWhatsThis(strWienerChanEst);
    ui->RadioButtonTiWiener->setWhatsThis(strWienerChanEst);

    /* Linear */
    const QString strLinearChanEst =
        tr("<b>Channel Estimation Settings:</b> With these "
           "settings, the channel estimation method in time and frequency "
           "direction can be selected. The default values use the most powerful "
           "algorithms. For more detailed information about the estimation "
           "algorithms there are a lot of papers and books available.<br>"
           "<b>Linear:</b> Simple linear interpolation "
           "method to get the channel estimate. The real and imaginary parts "
           "of the estimated channel at the pilot positions are linearly "
           "interpolated. This algorithm causes the lowest CPU load but "
           "performs much worse than the Wiener interpolation at low SNRs.");

    ui->RadioButtonFreqLinear->setWhatsThis(strLinearChanEst);
    ui->RadioButtonTiLinear->setWhatsThis(strLinearChanEst);

    /* DFT Zero Pad */
    ui->RadioButtonFreqDFT->setWhatsThis(
        tr("<b>Channel Estimation Settings:</b> With these "
           "settings, the channel estimation method in time and frequency "
           "direction can be selected. The default values use the most powerful "
           "algorithms. For more detailed information about the estimation "
           "algorithms there are a lot of papers and books available.<br>"
           "<b>DFT Zero Pad:</b> Channel estimation method "
           "for the frequency direction using Discrete Fourier Transformation "
           "(DFT) to transform the channel estimation at the pilot positions to "
           "the time domain. There, a zero padding is applied to get a higher "
           "resolution in the frequency domain -> estimates at the data cells. "
           "This algorithm is very speed efficient but has problems at the edges "
           "of the OFDM spectrum due to the leakage effect."));

    /* Guard Energy */
    ui->RadioButtonTiSyncEnergy->setWhatsThis(
        tr("<b>Guard Energy:</b> Time synchronization "
           "tracking algorithm utilizes the estimation of the impulse response. "
           "This method tries to maximize the energy in the guard-interval to set "
           "the correct timing."));

    /* First Peak */
    ui->RadioButtonTiSyncFirstPeak->setWhatsThis(
        tr("<b>First Peak:</b> This algorithms searches for "
           "the first peak in the estimated impulse response and moves this peak "
           "to the beginning of the guard-interval (timing tracking algorithm)."));


    /* Save audio as wave */
    ui->CheckBoxSaveAudioWave->setWhatsThis(
        tr("<b>Save Audio as WAV:</b> Save the audio signal "
           "as stereo, 16-bit, 48 kHz sample rate PCM wave file. Checking this "
           "box will let the user choose a file name for the recording."));

    /* Interferer Rejection */
    const QString strInterfRej =
        tr("<b>Interferer Rejection:</b> There are two "
           "algorithms available to reject interferers:<ul>"
           "<li><b>Bandpass Filter (BP-Filter):</b>"
           " The bandpass filter is designed to have the same bandwidth as "
           "the DRM signal. If, e.g., a strong signal is close to the border "
           "of the actual DRM signal, under some conditions this signal will "
           "produce interference in the useful bandwidth of the DRM signal "
           "although it is not on the same frequency as the DRM signal. "
           "The reason for that behaviour lies in the way the OFDM "
           "demodulation is done. Since OFDM demodulation is a block-wise "
           "operation, a windowing has to be applied (which is rectangular "
           "in case of OFDM). As a result, the spectrum of a signal is "
           "convoluted with a Sinc function in the frequency domain. If a "
           "sinusoidal signal close to the border of the DRM signal is "
           "considered, its spectrum will not be a distinct peak but a "
           "shifted Sinc function. So its spectrum is broadened caused by "
           "the windowing. Thus, it will spread in the DRM spectrum and "
           "act as an in-band interferer.<br>"
           "There is a special case if the sinusoidal signal is in a "
           "distance of a multiple of the carrier spacing of the DRM signal. "
           "Since the Sinc function has zeros at certain positions it happens "
           "that in this case the zeros are exactly at the sub-carrier "
           "frequencies of the DRM signal. In this case, no interference takes "
           "place. If the sinusoidal signal is in a distance of a multiple of "
           "the carrier spacing plus half of the carrier spacing away from the "
           "DRM signal, the interference reaches its maximum.<br>"
           "As a result, if only one DRM signal is present in the 20 kHz "
           "bandwidth, bandpass filtering has no effect. Also,  if the "
           "interferer is far away from the DRM signal, filtering will not "
           "give much improvement since the squared magnitude of the spectrum "
           "of the Sinc function is approx -15 dB down at 1 1/2 carrier "
           "spacing (approx 70 Hz with DRM mode B) and goes down to approx "
           "-30 dB at 10 times the carrier spacing plus 1 / 2 of the carrier "
           "spacing (approx 525 Hz with DRM mode B). The bandpass filter must "
           "have very sharp edges otherwise the gain in performance will be "
           "very small.</li>"
           "<li><b>Modified Metrics:</b> Based on the "
           "information from the SNR versus sub-carrier estimation, the metrics "
           "for the Viterbi decoder can be modified so that sub-carriers with "
           "high noise are attenuated and do not contribute too much to the "
           "decoding result. That can improve reception under bad conditions but "
           "may worsen the reception in situations where a lot of fading happens "
           "and no interferer are present since the SNR estimation may be "
           "not correct.</li></ul>");

    ui->GroupBoxInterfRej->setWhatsThis(strInterfRej);
    ui->CheckBoxRecFilter->setWhatsThis(strInterfRej);
    ui->CheckBoxModiMetric->setWhatsThis(strInterfRej);
}
