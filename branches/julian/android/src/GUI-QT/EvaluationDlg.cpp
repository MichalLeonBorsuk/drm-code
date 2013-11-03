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

#include "EvaluationDlg.h"
#include "DialogUtil.h"
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
#endif
#include <QMessageBox>
#include <QLayout>
#include <QDateTime>
#include <QFileDialog>
#include <QHideEvent>
#include <QShowEvent>

/* Implementation *************************************************************/
systemevalDlg::systemevalDlg(CDRMReceiver& NDRMR, CSettings& Settings,
                             QWidget* parent) :
    CWindow(parent, Settings, "System Evaluation"),
    ui(new Ui::SystemEvaluationWindow()),
    DRMReceiver(NDRMR),
    eNewCharType(CDRMPlot::NONE_OLD)
{
    ui->setupUi(this);

    /* Set help text for the controls */
    AddWhatsThisHelp();

    /* Init controls -------------------------------------------------------- */

    /* Init main plot */
    iPlotStyle = getSetting("plotstyle", 0, true);
    putSetting("plotstyle", iPlotStyle, true);
    MainPlot = new CDRMPlot(NULL, ui->plot);
    MainPlot->SetRecObj(&DRMReceiver);
    MainPlot->SetPlotStyle(iPlotStyle);

    //ui->SliderNoOfIterations->
    //    setValue(DRMReceiver.GetMSCMLC()->GetInitNumIterations());
    //ui->TextNumOfIterations->setText(tr("MLC: Number of Iterations: ") +
    //                             QString().setNum(DRMReceiver.GetMSCMLC()->GetInitNumIterations()));



    /* Update controls */
    UpdateControls();

    /* Set the Char Type of each selectable item */
    QTreeWidgetItemIterator it(ui->chartSelector, QTreeWidgetItemIterator::NoChildren);
    for (; *it; it++)
    {
        if ((*it)->text(0) == tr("SNR Spectrum"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::SNR_SPECTRUM);
        if ((*it)->text(0) == tr("Audio Spectrum"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::AUDIO_SPECTRUM);
        if ((*it)->text(0) == tr("Shifted PSD"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::POWER_SPEC_DENSITY);
        if ((*it)->text(0) == tr("Waterfall Input Spectrum"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::INP_SPEC_WATERF);
        if ((*it)->text(0) == tr("Input Spectrum"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::INPUTSPECTRUM_NO_AV);
        if ((*it)->text(0) == tr("Input PSD"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::INPUT_SIG_PSD);
        if ((*it)->text(0) == tr("MSC"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::MSC_CONSTELLATION);
        if ((*it)->text(0) == tr("SDC"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::SDC_CONSTELLATION);
        if ((*it)->text(0) == tr("FAC"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::FAC_CONSTELLATION);
        if ((*it)->text(0) == tr("FAC / SDC / MSC"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::ALL_CONSTELLATION);
        if ((*it)->text(0) == tr("Frequency / Sample Rate"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::FREQ_SAM_OFFS_HIST);
        if ((*it)->text(0) == tr("Delay / Doppler"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::DOPPLER_DELAY_HIST);
        if ((*it)->text(0) == tr("SNR / Audio"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::SNR_AUDIO_HIST);
        if ((*it)->text(0) == tr("Transfer Function"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::TRANSFERFUNCTION);
        if ((*it)->text(0) == tr("Impulse Response"))
            (*it)->setData(0,  Qt::UserRole, CDRMPlot::AVERAGED_IR);
    }

    /* Expand all items */
    ui->chartSelector->expandAll();

    /* Load saved main plot type */
    eCurCharType = PlotNameToECharType(string(getSetting("plottype", QString()).toLocal8Bit()));

    /* If MDI in is enabled, disable some of the controls and use different
       initialization for the chart and chart selector */
    if (DRMReceiver.GetRSIIn()->GetInEnabled() == TRUE)
    {
        ui->drmOptions->setRSCIModeEnabled(true);

        /* Only audio spectrum makes sence for MDI in */
        eCurCharType = CDRMPlot::AUDIO_SPECTRUM;
    }

    /* Init context menu for tree widget */
    pTreeWidgetContextMenu = new QMenu(tr("Chart Selector context menu"), this);
    pTreeWidgetContextMenu->addAction(tr("&Open in separate window"),
            this, SLOT(OnTreeWidgetContMenu(bool)));

    /* Char selector list view */
    ui->chartSelector->setContextMenuPolicy(Qt::CustomContextMenu);

    /* Timer */
    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

    /* Select chart type */
    ui->chartSelector->setCurrentItem(FindItemByECharType(eCurCharType), 0);

    /* Force update */
    OnTimer();
}

systemevalDlg::~systemevalDlg()
{
    if(DRMReceiver.GetWriteData()->GetIsWriteWaveFile())
        DRMReceiver.GetWriteData()->StopWriteWaveFile();
    delete MainPlot;
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
        const CDRMPlot::ECharType eNewType = PlotNameToECharType(Settings.Get(s.str(), "plottype", string()));

        /* get window geometry data */
        CWinGeom c;
        Settings.Get(s.str(), c);
        const QRect WinGeom(c.iXPos, c.iYPos, c.iWSize, c.iHSize);

        /* Open the new chart window */
        CDRMPlot* pNewChartWin = OpenChartWin(eNewType);

        /* and restore its geometry */
        if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
            pNewChartWin->setGeometry(WinGeom);

        /* Add window pointer in vector (needed for closing the windows) */
        vecpDRMPlots.push_back(pNewChartWin);

		/* Show new window */
		pNewChartWin->show();
    }

    /* Update controls */
    UpdateControls();

    /* Activate real-time timer */
    Timer.start(GUI_CONTROL_UPDATE_TIME);

    /* Notify the MainPlot of showEvent */
    MainPlot->activate();
}

void systemevalDlg::eventHide(QHideEvent*)
{
    /* Notify the MainPlot of hideEvent */
    MainPlot->deactivate();

    /* Stop the real-time timer */
    Timer.stop();

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
            Settings.Put(s.str(), "plottype", ECharTypeToPlotName(vecpDRMPlots[i]->GetChartType()));

            iNumOpenCharts++;
        }
        /* Close window afterwards */
        vecpDRMPlots[i]->close();
    }
    putSetting("numchartwin", iNumOpenCharts);

    /* We do not need the pointers anymore, reset vector */
    vecpDRMPlots.clear();

    /* Store current plot type */
    putSetting("plottype", QString::fromLocal8Bit(ECharTypeToPlotName(eCurCharType).c_str()));
}

void systemevalDlg::UpdatePlotStyle(int iPlotStyle)
{
    /* Save the new style */
    putSetting("plotstyle", iPlotStyle, true);
    this->iPlotStyle = iPlotStyle;

    /* Update chart windows */
    for (size_t i = 0; i < vecpDRMPlots.size(); i++)
        vecpDRMPlots[i]->SetPlotStyle(iPlotStyle);

    /* Update main plot window */
    MainPlot->SetPlotStyle(iPlotStyle);
}

void systemevalDlg::on_chartSelector_currentItemChanged(QTreeWidgetItem *curr)
{
    /* Make sure we have a non root item */
    if (curr && curr->parent())
    {
        /* Get chart type from selected item */
        eCurCharType = CDRMPlot::ECharType(curr->data(0, Qt::UserRole).toInt());
        /* Setup chart */
        MainPlot->SetupChart(eCurCharType);
    }
}

void systemevalDlg::OnTreeWidgetContMenu(bool)
{
    if (eNewCharType != CDRMPlot::NONE_OLD)
    {
        /* Open the new chart */
		CDRMPlot* pNewChartWin = OpenChartWin(eNewCharType);
        vecpDRMPlots.push_back(pNewChartWin);

		/* Show new window */
		pNewChartWin->show();

        eNewCharType = CDRMPlot::NONE_OLD;
    }
}

void systemevalDlg::on_chartSelector_customContextMenuRequested(const QPoint& p)
{
    QModelIndex index = ui->chartSelector->indexAt(p);
    /* Make sure we have a non root item */
    if (index.parent() != QModelIndex())
    {
        /* Popup the context menu */
        eNewCharType = CDRMPlot::ECharType(index.data(Qt::UserRole).toInt());
        pTreeWidgetContextMenu->exec(QCursor::pos());
    }
}

CDRMPlot* systemevalDlg::OpenChartWin(CDRMPlot::ECharType eNewType)
{
    /* Create new chart window */
    CDRMPlot* pNewChartWin = new CDRMPlot(this, NULL);
    pNewChartWin->setCaption(tr("Chart Window"));

    /* Set correct icon (use the same as this dialog) */
    const QIcon& icon = windowIcon();
    pNewChartWin->setIcon(icon);

    /* Set receiver object and correct chart type */
    pNewChartWin->SetRecObj(&DRMReceiver);
    pNewChartWin->SetupChart(eNewType);

    /* Set plot style*/
    pNewChartWin->SetPlotStyle(iPlotStyle);

    return pNewChartWin;
}

QTreeWidgetItem* systemevalDlg::FindItemByECharType(CDRMPlot::ECharType eCharType)
{
    for (int i = 0;; i++)
    {
        QTreeWidgetItem* item = ui->chartSelector->topLevelItem(i);
        if (item == NULL)
            return NULL;
        for (int j = 0; j < item->childCount(); j++)
        {
            QTreeWidgetItem* subitem = item->child(j);
            CDRMPlot::ECharType eCurCharType = CDRMPlot::ECharType(subitem->data(0, Qt::UserRole).toInt());
            if (eCurCharType == eCharType)
                return subitem;
        }
    }
}

CDRMPlot::ECharType systemevalDlg::PlotNameToECharType(const string& PlotName)
{
    QString plotName(PlotName.c_str());
    for (int i = 0;; i++)
    {
        QTreeWidgetItem* item = ui->chartSelector->topLevelItem(i);
        if (item == NULL)
            return CDRMPlot::AUDIO_SPECTRUM; /* safe value */
        for (int j = 0; j < item->childCount(); j++)
        {
            QTreeWidgetItem* subitem = item->child(j);
            if (plotName == subitem->text(0))
                return CDRMPlot::ECharType(subitem->data(0, Qt::UserRole).toInt());
        }
    }
}

string systemevalDlg::ECharTypeToPlotName(CDRMPlot::ECharType eCharType)
{
    QTreeWidgetItem* item = FindItemByECharType(eCharType);
    if (item != NULL)
        return item->text(0).toStdString();
    return string();
}

void systemevalDlg::OnTimer()
{
    CParameter& Parameters = *(DRMReceiver.GetParameters());

    Parameters.Lock();

    // TODO ui->drmDetail->updateDisplay();
    UpdateGPS(Parameters);

    UpdateControls();

    ui->CheckBoxReverb->setChecked(DRMReceiver.GetAudSorceDec()->GetReverbEffect());
    ui->CheckBoxMuteAudio->setChecked(DRMReceiver.GetWriteData()->GetMuteAudio());
    ui->CheckBoxSaveAudioWave->setChecked(DRMReceiver.GetWriteData()->GetIsWriteWaveFile());

    Parameters.Unlock();
}

void systemevalDlg::UpdateControls()
{
    ui->drmOptions->setNumIterations(DRMReceiver.GetMSCMLC()->GetInitNumIterations());
    ui->drmOptions->setTimeInt(DRMReceiver.GetTimeInt());
    ui->drmOptions->setFreqInt(DRMReceiver.GetFreqInt());
    ui->drmOptions->setTiSyncTrac(DRMReceiver.GetTiSyncTracType());
    ui->drmOptions->setRecFilterEnabled(DRMReceiver.GetFreqSyncAcq()->GetRecFilter());
    ui->drmOptions->setIntConsEnabled(DRMReceiver.GetIntCons());
    ui->drmOptions->setFlipSpectrumEnabled(DRMReceiver.GetReceiveData()->GetFlippedSpectrum());
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
//      Wrong char on Qt 5
//      qStrPosition = QString(tr("Lat: %1\260  Long: %2\260")).arg(gps.fix.latitude, 0, 'f', 4).arg(gps.fix.longitude,0, 'f',4);
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

void systemevalDlg::on_drmOptions_TimeLinear()
{
    if (DRMReceiver.GetTimeInt() != CChannelEstimation::TLINEAR)
        DRMReceiver.SetTimeInt(CChannelEstimation::TLINEAR);
}

void systemevalDlg::on_drmOptions_TimeWiener()
{
    if (DRMReceiver.GetTimeInt() != CChannelEstimation::TWIENER)
        DRMReceiver.SetTimeInt(CChannelEstimation::TWIENER);
}

void systemevalDlg::on_drmOptions_FrequencyLinear()
{
    if (DRMReceiver.GetFreqInt() != CChannelEstimation::FLINEAR)
        DRMReceiver.SetFreqInt(CChannelEstimation::FLINEAR);
}

void systemevalDlg::on_drmOptions_FrequencyDft()
{
    if (DRMReceiver.GetFreqInt() != CChannelEstimation::FDFTFILTER)
        DRMReceiver.SetFreqInt(CChannelEstimation::FDFTFILTER);
}

void systemevalDlg::on_drmOptions_FrequencyWiener()
{
    if (DRMReceiver.GetFreqInt() != CChannelEstimation::FWIENER)
        DRMReceiver.SetFreqInt(CChannelEstimation::FWIENER);
}

void systemevalDlg::on_drmOptions_TiSyncFirstPeak()
{
    if (DRMReceiver.GetTiSyncTracType() !=
            CTimeSyncTrack::TSFIRSTPEAK)
    {
        DRMReceiver.SetTiSyncTracType(CTimeSyncTrack::TSFIRSTPEAK);
    }
}

void systemevalDlg::on_drmOptions_TiSyncEnergy()
{
    if (DRMReceiver.GetTiSyncTracType() !=
            CTimeSyncTrack::TSENERGY)
    {
        DRMReceiver.SetTiSyncTracType(CTimeSyncTrack::TSENERGY);
    }
}

void systemevalDlg::on_drmOptions_noOfIterationsChanged(int value)
{
    /* Set new value in working thread module */
    DRMReceiver.GetMSCMLC()->SetNumIterations(value);
}

void systemevalDlg::on_drmOptions_FlipSpectrum(int state)
{
    /* Set parameter in working thread module */
    DRMReceiver.GetReceiveData()->SetFlippedSpectrum(state == Qt::Checked);
}

void systemevalDlg::on_drmOptions_RecFilter(int state)
{
    /* Set parameter in working thread module */
    DRMReceiver.GetFreqSyncAcq()->SetRecFilter(state == Qt::Checked);

    /* If filter status is changed, a new aquisition is necessary */
    DRMReceiver.RequestNewAcquisition();
}

void systemevalDlg::on_drmOptions_ModiMetric(int state)
{
    /* Set parameter in working thread module */
    DRMReceiver.SetIntCons(state == Qt::Checked);
}

void systemevalDlg::on_CheckBoxMuteAudio_stateChanged(int state)
{
    /* Set parameter in working thread module */
    DRMReceiver.GetWriteData()->MuteAudio(state == Qt::Checked);
}

void systemevalDlg::on_CheckBoxReverb_stateChanged(int state)
{
    /* Set parameter in working thread module */
    DRMReceiver.GetAudSorceDec()->SetReverbEffect(state == Qt::Checked);
}

void systemevalDlg::on_CheckBoxSaveAudioWave_stateChanged(int state)
{
    /*
    	This code is copied in AnalogDemDlg.cpp. If you do changes here, you should
    	apply the changes in the other file, too
    */
    if (state == Qt::Checked)
    {
        /* Show "save file" dialog */
        QString strFileName =
            QFileDialog::getSaveFileName(this, "*.wav", tr("DreamOut.wav"));

        /* Check if user not hit the cancel button */
        if (!strFileName.isEmpty())
        {
			string s = strFileName.toUtf8().constData();
            DRMReceiver.GetWriteData()->StartWriteWaveFile(s);
        }
        else
        {
            /* User hit the cancel button, uncheck the button */
            ui->CheckBoxSaveAudioWave->setChecked(FALSE);
        }
    }
    else
        DRMReceiver.GetWriteData()->StopWriteWaveFile();
}


void systemevalDlg::on_CheckBoxWriteLog_stateChanged(int state)
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

void systemevalDlg::setLogging(bool enabled)
{
    ui->CheckBoxWriteLog->setChecked(enabled);
}

void systemevalDlg::AddWhatsThisHelp()
{

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

    /* Save audio as wave */
    ui->CheckBoxSaveAudioWave->setWhatsThis(
                     tr("<b>Save Audio as WAV:</b> Save the audio signal "
                        "as stereo, 16-bit, 48 kHz sample rate PCM wave file. Checking this "
                        "box will let the user choose a file name for the recording."));

}
