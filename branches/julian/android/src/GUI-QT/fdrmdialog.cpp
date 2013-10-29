/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo
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

#include "fdrmdialog.h"
#include <iostream>
#include <QWhatsThis>
#include <QHideEvent>
#include <QEvent>
#include <QShowEvent>
#include <QCloseEvent>
#include <QTableWidget>
#include "SlideShowViewer.h"
#include "JLViewer.h"
#ifdef QT_WEBKIT_LIB
# include "BWSViewer.h"
#endif
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
#endif
#include "../Scheduler.h"
#include "../util-QT/Util.h"
#include "audiodetailwidget.h"

// Simone's values
// static _REAL WMERSteps[] = {8.0, 12.0, 16.0, 20.0, 24.0};
// David's values
static _REAL WMERSteps[] = {6.0, 12.0, 18.0, 24.0, 30.0};

/* Implementation *************************************************************/
#ifdef HAVE_LIBHAMLIB
FDRMDialog::FDRMDialog(CDRMReceiver& NDRMR, CSettings& Settings, CRig& rig,
                       QWidget* parent)
#else
FDRMDialog::FDRMDialog(CDRMReceiver& NDRMR, CSettings& Settings, 
                       QWidget* parent)
#endif
    :
    CWindow(parent, Settings, "DRM"),
    ui(new Ui_MainWindow()),
    DRMReceiver(NDRMR),
    pLogging(NULL),
    pSysTray(NULL), pCurrentWindow(this),
    iMultimediaServiceBit(0),
    iLastMultimediaServiceSelected(-1),
    pScheduler(NULL), pScheduleTimer(NULL)
{
    ui->setupUi(this);

    /* Set help text for the controls */
    AddWhatsThisHelp();

    CParameter& Parameters = *DRMReceiver.GetParameters();

    pLogging = new CLogging(Parameters);
    pLogging->LoadSettings(Settings);

    /* Creation of file and sound card menu */
    pFileMenu = new CFileMenu(DRMReceiver, this);
    pSoundCardMenu = new CSoundCardSelMenu(DRMReceiver, pFileMenu, this);
    ui->menu_Settings->addMenu(pSoundCardMenu);

    /* Analog demodulation window */
    pAnalogDemDlg = new AnalogDemDlg(DRMReceiver, Settings, pSoundCardMenu);

    /* FM window */
    pFMDlg = new FMDialog(DRMReceiver, Settings, pSoundCardMenu);

    /* Parent list for Stations and Live Schedule window */
	QMap <QWidget*,QString> parents;
	parents[this] = "drm";
	parents[pAnalogDemDlg] = "analog";

    /* Stations window */
#ifdef HAVE_LIBHAMLIB
    pStationsDlg = new StationsDlg(DRMReceiver, Settings, rig, parents);
#else
    pStationsDlg = new StationsDlg(DRMReceiver, Settings, parents);
#endif

    /* Live Schedule window */
    pLiveScheduleDlg = new LiveScheduleDlg(DRMReceiver, Settings, parents);

    /* MOT broadcast website viewer window */
#ifdef QT_WEBKIT_LIB
    pBWSDlg = new BWSViewer(DRMReceiver, Settings, this);
#endif

    /* Journaline viewer window */
    pJLDlg = new JLViewer(DRMReceiver, Settings, this);

    /* MOT slide show window */
    pSlideShowDlg = new SlideShowViewer(DRMReceiver, Settings, this);

    /* Programme Guide Window */
    pEPGDlg = new EPGDlg(DRMReceiver, Settings, this);

    /* Evaluation window */
    pSysEvalDlg = new systemevalDlg(DRMReceiver, Settings, this);

    /* general settings window */
    pGeneralSettingsDlg = new GeneralSettingsDlg(Parameters, Settings, this);

    /* Multimedia settings window */
    pMultSettingsDlg = new MultSettingsDlg(Parameters, Settings, this);

    connect(ui->action_Evaluation_Dialog, SIGNAL(triggered()), pSysEvalDlg, SLOT(show()));
    connect(ui->action_Multimedia_Dialog, SIGNAL(triggered()), this, SLOT(OnViewMultimediaDlg()));
    connect(ui->action_Stations_Dialog, SIGNAL(triggered()), pStationsDlg, SLOT(show()));
    connect(ui->action_Live_Schedule_Dialog, SIGNAL(triggered()), pLiveScheduleDlg, SLOT(show()));
    connect(ui->action_Programme_Guide_Dialog, SIGNAL(triggered()), pEPGDlg, SLOT(show()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));

    ui->action_Multimedia_Dialog->setEnabled(false);

    connect(ui->actionMultimediaSettings, SIGNAL(triggered()), pMultSettingsDlg, SLOT(show()));
    connect(ui->actionGeneralSettings, SIGNAL(triggered()), pGeneralSettingsDlg, SLOT(show()));

    connect(ui->actionAM, SIGNAL(triggered()), this, SLOT(OnSwitchToAM()));
    connect(ui->actionFM, SIGNAL(triggered()), this, SLOT(OnSwitchToFM()));
    connect(ui->actionDRM, SIGNAL(triggered()), this, SLOT(OnNewAcquisition()));

    connect(ui->actionDisplayColor, SIGNAL(triggered()), this, SLOT(OnMenuSetDisplayColor()));

    /* Plot style settings */
    QSignalMapper* plotStyleMapper = new QSignalMapper(this);
    QActionGroup* plotStyleGroup = new QActionGroup(this);
    plotStyleGroup->addAction(ui->actionBlueWhite);
    plotStyleGroup->addAction(ui->actionGreenBlack);
    plotStyleGroup->addAction(ui->actionBlackGrey);
    plotStyleMapper->setMapping(ui->actionBlueWhite, 0);
    plotStyleMapper->setMapping(ui->actionGreenBlack, 1);
    plotStyleMapper->setMapping(ui->actionBlackGrey, 2);
    connect(ui->actionBlueWhite, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(ui->actionGreenBlack, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(ui->actionBlackGrey, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(plotStyleMapper, SIGNAL(mapped(int)), this, SIGNAL(plotStyleChanged(int)));
    switch(getSetting("plotstyle", int(0), true))
    {
    case 0:
        ui->actionBlueWhite->setChecked(true);
        break;
    case 1:
        ui->actionGreenBlack->setChecked(true);
        break;
    case 2:
        ui->actionBlackGrey->setChecked(true);
        break;
    }

    connect(ui->actionAbout_Dream, SIGNAL(triggered()), this, SLOT(OnHelpAbout()));
    connect(ui->actionWhats_This, SIGNAL(triggered()), this, SLOT(OnWhatsThis()));

    connect(this, SIGNAL(plotStyleChanged(int)), pSysEvalDlg, SLOT(UpdatePlotStyle(int)));
    connect(this, SIGNAL(plotStyleChanged(int)), pAnalogDemDlg, SLOT(UpdatePlotStyle(int)));

    connect(pFMDlg, SIGNAL(About()), this, SLOT(OnHelpAbout()));
    connect(pAnalogDemDlg, SIGNAL(About()), this, SLOT(OnHelpAbout()));


#ifdef HAVE_LIBHAMLIB
    connect(pStationsDlg, SIGNAL(subscribeRig()), &rig, SLOT(subscribe()));
    connect(pStationsDlg, SIGNAL(unsubscribeRig()), &rig, SLOT(unsubscribe()));
    connect(&rig, SIGNAL(sigstr(double)), pStationsDlg, SLOT(OnSigStr(double)));
    connect(pLogging, SIGNAL(subscribeRig()), &rig, SLOT(subscribe()));
    connect(pLogging, SIGNAL(unsubscribeRig()), &rig, SLOT(unsubscribe()));
#endif
    connect(pSysEvalDlg, SIGNAL(startLogging()), pLogging, SLOT(start()));
    connect(pSysEvalDlg, SIGNAL(stopLogging()), pLogging, SLOT(stop()));
    connect(pAnalogDemDlg, SIGNAL(SwitchMode(int)), this, SLOT(OnSwitchMode(int)));
    connect(pAnalogDemDlg, SIGNAL(NewAMAcquisition()), this, SLOT(OnNewAcquisition()));
    connect(pAnalogDemDlg, SIGNAL(ViewStationsDlg()), pStationsDlg, SLOT(show()));
    connect(pAnalogDemDlg, SIGNAL(ViewLiveScheduleDlg()), pLiveScheduleDlg, SLOT(show()));
    connect(pAnalogDemDlg, SIGNAL(Closed()), this, SLOT(close()));

    connect(pFMDlg, SIGNAL(SwitchMode(int)), this, SLOT(OnSwitchMode(int)));
    connect(pFMDlg, SIGNAL(Closed()), this, SLOT(close()));
    connect(pFMDlg, SIGNAL(ViewStationsDlg()), pStationsDlg, SLOT(show()));
    connect(pFMDlg, SIGNAL(ViewLiveScheduleDlg()), pLiveScheduleDlg, SLOT(show()));

    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));
    connect(&TimerClose, SIGNAL(timeout()), this, SLOT(OnTimerClose()));

    ClearDisplay();

    /* System tray setup */
    pSysTray = CSysTray::Create(
        this,
        SLOT(OnSysTrayActivated(QSystemTrayIcon::ActivationReason)),
        SLOT(OnSysTrayTimer()),
        ":/icons/MainIcon.svg");
    CSysTray::AddAction(pSysTray, tr("&New Acquisition"), this, SLOT(OnNewAcquisition()));
    CSysTray::AddSeparator(pSysTray);
    CSysTray::AddAction(pSysTray, tr("&Exit"), this, SLOT(close()));

	/* clear signal strenght */
    ui->MainDisplay->setBars(0);

	/* Activate real-time timers */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

FDRMDialog::~FDRMDialog()
{
    /* Destroying logger */
    delete pLogging;
    /* Destroying top level windows, children are automaticaly destroyed */
    delete pAnalogDemDlg;
    delete pFMDlg;
}

void FDRMDialog::OnSysTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger
#if QT_VERSION < 0x050000
        || reason == QSystemTrayIcon::DoubleClick
#endif
    )
    {
        const Qt::WindowStates ws = pCurrentWindow->windowState();
        if (ws & Qt::WindowMinimized)
            pCurrentWindow->setWindowState((ws & ~Qt::WindowMinimized) | Qt::WindowActive);
        else
            pCurrentWindow->toggleVisibility();
    }
}

void FDRMDialog::OnSysTrayTimer()
{
    QString Title, Message;
    if (DRMReceiver.GetAcquiState() == AS_WITH_SIGNAL)
    {
        CParameter& Parameters = *(DRMReceiver.GetParameters());
        Parameters.Lock();
            const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
            CService audioService = Parameters.Service[iCurSelAudioServ];
            const bool bServiceIsValid = audioService.IsActive()
                                   && (audioService.AudioParam.iStreamID != STREAM_ID_NOT_USED)
                                   && (audioService.eAudDataFlag == CService::SF_AUDIO);
            if (bServiceIsValid)
            {
                /* Service label (UTF-8 encoded string -> convert) */
                Title = QString::fromUtf8(audioService.strLabel.c_str());
                // Text message of current selected audio service (UTF-8 decoding)
                Message = QString::fromUtf8(audioService.AudioParam.strTextMessage.c_str());
            }
		if(Parameters.rWMERMSC>WMERSteps[4])
            ui->MainDisplay->setBars(5);
		else if(Parameters.rWMERMSC>WMERSteps[3])
            ui->MainDisplay->setBars(4);
		else if(Parameters.rWMERMSC>WMERSteps[2])
            ui->MainDisplay->setBars(3);
		else if(Parameters.rWMERMSC>WMERSteps[1])
            ui->MainDisplay->setBars(2);
		else if(Parameters.rWMERMSC>WMERSteps[0])
            ui->MainDisplay->setBars(1);
		else
            ui->MainDisplay->setBars(0);
        Parameters.Unlock();
    }
	else {
        Message = tr("Scanning...");
        ui->MainDisplay->setBars(0);
	}
    CSysTray::SetToolTip(pSysTray, Title, Message);
}

void FDRMDialog::OnWhatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void FDRMDialog::OnSwitchToFM()
{
    OnSwitchMode(RM_FM);
}

void FDRMDialog::OnSwitchToAM()
{
    OnSwitchMode(RM_AM);
}


void FDRMDialog::UpdateDRM_GUI()
{
    _BOOLEAN bMultimediaServiceAvailable;
    CParameter& Parameters = *DRMReceiver.GetParameters();

    if (isVisible() == false)
        ChangeGUIModeToDRM();

    Parameters.Lock();

    ui->MainDisplay->setLevel(Parameters.GetIFSignalLevel());
    int iShortID = Parameters.GetCurSelAudioService();
    ETypeRxStatus msc_status;
    if(Parameters.Service[iShortID].eAudDataFlag == CService::SF_AUDIO)
        msc_status = Parameters.AudioComponentStatus[iShortID].GetStatus();
    else
        msc_status = Parameters.DataComponentStatus[iShortID].GetStatus();
    ui->MainDisplay->showReceptionStatus(
                Parameters.ReceiveStatus.FAC.GetStatus(),
                Parameters.ReceiveStatus.SDC.GetStatus(),
                msc_status
    );

    Parameters.Unlock();

    /* Clear the multimedia service bit */
    iMultimediaServiceBit = 0;

    /* Check if receiver does receive a signal */
    if(DRMReceiver.GetAcquiState() == AS_WITH_SIGNAL)
        UpdateDisplay();
    else
    {
        ClearDisplay();
        /* No signal then set the last
           multimedia service selected to none */
        iLastMultimediaServiceSelected = -1;
    }
    /* If multimedia service availability has changed
       then update the menu */
    bMultimediaServiceAvailable = iMultimediaServiceBit != 0;
    if (bMultimediaServiceAvailable != ui->action_Multimedia_Dialog->isEnabled())
        ui->action_Multimedia_Dialog->setEnabled(bMultimediaServiceAvailable);
}

void FDRMDialog::startLogging()
{
    pSysEvalDlg->setLogging(true);
}

void FDRMDialog::stopLogging()
{
    pSysEvalDlg->setLogging(false);
}

void FDRMDialog::OnScheduleTimer()
{
    CScheduler::SEvent e;
    e = pScheduler->front();
    if (e.frequency != -1)
    {
        DRMReceiver.SetFrequency(e.frequency);
        if(!pLogging->enabled())
        {
            startLogging();
        }
    }
    else
    {
        stopLogging();
    }
    if(pScheduler->empty())
    {
        stopLogging();
    }
    else
    {
        e = pScheduler->pop();
        time_t now = time(NULL);
        pScheduleTimer->start(1000*(e.time-now));
    }
}

void FDRMDialog::OnTimer()
{
    ERecMode eNewReceiverMode = DRMReceiver.GetReceiverMode();
    switch(eNewReceiverMode)
    {
    case RM_DRM:
        UpdateDRM_GUI();
        break;
    case RM_AM:
        ChangeGUIModeToAM();
        break;
    case RM_FM:
        ChangeGUIModeToFM();
        break;
    case RM_NONE: // wait until working thread starts operating
        break;
    }

    // do this here so GUI has initialised before we might pop up a message box
    if(pScheduler!=NULL)
        return;

    string schedfile = Settings.Get("command", "schedule", string());
    if(schedfile != "")
    {
        bool testMode = Settings.Get("command", "test", false);
        pScheduler = new CScheduler(testMode);
        if(pScheduler->LoadSchedule(schedfile)) {
            pScheduleTimer = new QTimer(this);
            connect(pScheduleTimer, SIGNAL(timeout()), this, SLOT(OnScheduleTimer()));
            /* Setup the first timeout */
            CScheduler::SEvent e;
            if(!pScheduler->empty()) {
                e = pScheduler->front();
                time_t now = time(NULL);
                time_t next = e.time - now;
                if(next > 0)
                {
                    pScheduleTimer->start(1000*next);
                }
                else // We are late starting
                {
                    startLogging();
                }
            }
        }
        else {
            QMessageBox::information(this, "Dream", tr("Schedule file requested but not found"));
        }
    }
    else
    {
        if(pLogging->enabled())
            startLogging();
    }
}

void FDRMDialog::OnTimerClose()
{
    if (DRMReceiver.GetParameters()->eRunState == CParameter::STOPPED)
        close();
}

QString
FDRMDialog::audioServiceDescription(const CService &service, _REAL rAudioBitRate)
{
    /* Do UTF-8 to string conversion with the label strings */
    QString strLabel = QString().fromUtf8(service.strLabel.c_str());

    /* Label for service selection button (service label, codec
       and Mono / Stereo information) */
    QString strCodec = GetCodecString(service);
    QString strType = GetTypeString(service);
    QString text = strLabel;
    if (!strCodec.isEmpty() || !strType.isEmpty())
        text += "  |   " + strCodec + " " + strType;

    /* Bit-rate (only show if greater than 0) */
    if (rAudioBitRate > (_REAL) 0.0)
    {
        text += " (" + QString().setNum(rAudioBitRate, 'f', 2) + " kbps)";
    }

    /* Report missing codec */
    if (!DRMReceiver.GetAudSorceDec()->CanDecode(service.AudioParam.eAudioCoding))
        text += tr(" [no codec available]");

    return text;
}

QString
FDRMDialog::dataServiceDescription(const CService &service)
{
    QString text;
    if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
    {
        if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
        {
            switch (service.DataParam.iUserAppIdent)
            {
            case DAB_AT_EPG:
                text = tr("Programme Guide");
                break;
            case DAB_AT_BROADCASTWEBSITE:
                text = tr("Web Site");
                break;
            case DAB_AT_JOURNALINE:
                text = tr("News");
                break;
            case DAB_AT_MOTSLIDESHOW:
                text = tr("Slides");
                break;
             default:
                text = tr("Packet Data");
            }
        }
        else
            text = tr("Packet Data");
    }
    else
        text = tr("Stream Data");
    return text;
}

QString FDRMDialog::serviceSelector(CParameter& Parameters, int i)
{
    Parameters.Lock();

    CService service = Parameters.Service[i];
    const _REAL rAudioBitRate = Parameters.GetBitRateKbps(i, FALSE);
    const _REAL rDataBitRate = Parameters.GetBitRateKbps(i, TRUE);

    /* detect if AFS mux information is available TODO - align to service */
    bool bAFS = ((i==0) && (
                     (Parameters.AltFreqSign.vecMultiplexes.size() > 0)
                     || (Parameters.AltFreqSign.vecOtherServices.size() > 0)
                 ));

    Parameters.Unlock();

    QString text;

    /* Check, if service is used */
    if (service.IsActive())
    {
        text = audioServiceDescription(service, rAudioBitRate);
        /* Audio service */
        if ((service.eAudDataFlag == CService::SF_AUDIO))
        {

            /* Show, if a multimedia stream is connected to this service */
            if (service.DataParam.iStreamID != STREAM_ID_NOT_USED)
            {
                if (service.DataParam.iUserAppIdent == DAB_AT_EPG)
                    text += tr(" + EPG"); /* EPG service */
                else
                {
                    text += tr(" + MM"); /* other multimedia service */
                    /* Set multimedia service bit */
                    iMultimediaServiceBit |= 1 << i;
                }

                /* Bit-rate of connected data stream */
                text += " (" + QString().setNum(rDataBitRate, 'f', 2) + " kbps)";
            }
        }
        /* Data service */
        else
        {
            if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
            {
                if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
                {
                    switch (service.DataParam.iUserAppIdent)
                    {
                    case DAB_AT_BROADCASTWEBSITE:
                    case DAB_AT_JOURNALINE:
                    case DAB_AT_MOTSLIDESHOW:
                        /* Set multimedia service bit */
                        iMultimediaServiceBit |= 1 << i;
                        break;
                    }
                }
            }
        }

        if(bAFS)
        {
            text += tr(" + AFS");
        }
    }
    return text;
}

void FDRMDialog::UpdateDisplay()
{
    CParameter& Parameters = *(DRMReceiver.GetParameters());
    Parameters.Lock();
    int n=0;
    for(int i=0; i<MAX_NUM_SERVICES; i++) {

        CService service = Parameters.Service[i];
        const _REAL rAudioBitRate = Parameters.GetBitRateKbps(i, FALSE);

        bool isValidAudioService = service.IsActive()
                && (service.eAudDataFlag == CService::SF_AUDIO)
                && (service.AudioParam.iStreamID != STREAM_ID_NOT_USED);

        if (isValidAudioService)
        {
            QString label = QString::fromUtf8(service.strLabel.c_str());
            if(label == "")
                label = QString("%1A").arg(i+1);
            AudioDetailWidget* adw=NULL;
            if(ui->serviceTabs->count()>n) {
                adw = dynamic_cast<AudioDetailWidget*>(ui->serviceTabs->widget(n));
            }
            if(adw) {
                ui->serviceTabs->setTabText(n, label);
            }
            else {
                if(ui->serviceTabs->widget(n))
                    ui->serviceTabs->removeTab(n);
                adw = new AudioDetailWidget();
                ui->serviceTabs->addTab(adw, label);
                connect(adw, SIGNAL(listen(int)), this, SLOT(OnSelectAudioService(int)));
            }
            adw->updateDisplay(i, service);
             n++;
        }
    }
    for(int i=0; i<MAX_NUM_SERVICES; i++) {
        const _REAL rDataBitRate = Parameters.GetBitRateKbps(i, TRUE);

        const CService& service = Parameters.Service[i];

        bool isValidDataService = service.IsActive()
                && (service.DataParam.iStreamID != STREAM_ID_NOT_USED);

        if (isValidDataService)
        {
            QString label = QString::fromUtf8(service.strLabel.c_str());
            if(label == "")
                label = QString("%1D").arg(i+1);
            QString s = dataServiceDescription(service);
            QLabel* w=NULL;
            if(ui->serviceTabs->count()>n) {
                w = dynamic_cast<QLabel*>(ui->serviceTabs->widget(n));
            }
            if(w) {
                ui->serviceTabs->setTabText(n, label+" "+s);
                w->setText(s+" "+QString("%1 %2 kbit/s").arg(service.iServiceID, 6, 16).arg(rDataBitRate));
            }
            else {
                if(ui->serviceTabs->widget(n))
                    ui->serviceTabs->removeTab(n);
                ui->serviceTabs->addTab(new QLabel(s), QString("%1D").arg(i+1));
            }
             n++;
        }
    }
    Parameters.Unlock();

    Parameters.Lock();
    /*  get current selected services */
    int iCurSelAudioServ = Parameters.GetCurSelAudioService();

    CService audioService = Parameters.Service[iCurSelAudioServ];
    Parameters.Unlock();

    bool bServiceIsValid = audioService.IsActive()
                           && (audioService.AudioParam.iStreamID != STREAM_ID_NOT_USED)
                           && (audioService.eAudDataFlag == CService::SF_AUDIO);

    int i, iFirstAudioService=-1, iFirstDataService=-1;
    for(i=0; i < MAX_NUM_SERVICES; i++)
    {
        QString label = serviceSelector(Parameters, i);
        if (!bServiceIsValid && (iFirstAudioService == -1 || iFirstDataService == -1))
        {
            Parameters.Lock();
            audioService = Parameters.Service[i];
            Parameters.Unlock();
            /* If the current audio service is not valid
            	find the first audio service available */
            if (iFirstAudioService == -1
                    && audioService.IsActive()
                    && (audioService.AudioParam.iStreamID != STREAM_ID_NOT_USED)
                    && (audioService.eAudDataFlag == CService::SF_AUDIO))
            {
                iFirstAudioService = i;
            }
            /* If the current audio service is not valid
            	find the first data service available */
            if (iFirstDataService == -1
                    && audioService.IsActive()
                    && (audioService.eAudDataFlag == CService::SF_DATA))
            {
                iFirstDataService = i;
            }
        }
    }

    /* Select a valid service, priority to audio service */
    if (iFirstAudioService != -1)
    {
        iCurSelAudioServ = iFirstAudioService;
        bServiceIsValid = true;
    }
    else
    {
        if (iFirstDataService != -1)
        {
            iCurSelAudioServ = iFirstDataService;
            bServiceIsValid = true;
        }
    }

    if(bServiceIsValid)
    {
        /* Get selected service */
        Parameters.Lock();
        audioService = Parameters.Service[iCurSelAudioServ];
        Parameters.Unlock();

        /* If we have text messages */
        if (audioService.AudioParam.bTextflag == TRUE)
        {
            // Text message of current selected audio service (UTF-8 decoding)
            QString TextMessage(QString::fromUtf8(audioService.AudioParam.strTextMessage.c_str()));
            ui->MainDisplay->showTextMessage(TextMessage);
        }
        else
        {
            ui->MainDisplay->showTextMessage("");
        }

        /* Check whether service parameters were not transmitted yet */
        if (audioService.IsActive())
        {
            ui->MainDisplay->showServiceInfo(audioService);

            Parameters.Lock();
            _REAL rPartABLenRat = Parameters.PartABLenRatio(iCurSelAudioServ);
            _REAL rBitRate = Parameters.GetBitRateKbps(iCurSelAudioServ, FALSE);
            Parameters.Unlock();

            ui->MainDisplay->setBitRate(rBitRate, rPartABLenRat);

        }
        else
        {
            ui->MainDisplay->clearDisplay(tr("No Service"));
        }
    }
}

void FDRMDialog::ClearDisplay()
{
    /* No signal is currently received ---------------------------------- */
    ui->serviceTabs->clear();
    /* Main text labels */
    ui->MainDisplay->clearDisplay(tr("Scanning..."));
    ui->MainDisplay->showTextMessage("");
}

/* change mode is only called when the mode REALLY has changed
 * so no conditionals are needed in this routine
 */

void FDRMDialog::ChangeGUIModeToDRM()
{
    CSysTray::Start(pSysTray);
    pCurrentWindow = this;
    pCurrentWindow->eventUpdate();
    pCurrentWindow->show();
}

void FDRMDialog::ChangeGUIModeToAM()
{
    hide();
    Timer.stop();
    CSysTray::Stop(pSysTray, tr("Dream AM"));
    pCurrentWindow = pAnalogDemDlg;
    pCurrentWindow->eventUpdate();
    pCurrentWindow->show();
}

void FDRMDialog::ChangeGUIModeToFM()
{
    hide();
    Timer.stop();
    CSysTray::Stop(pSysTray, tr("Dream FM"));
    pCurrentWindow = pFMDlg;
    pCurrentWindow->eventUpdate();
    pCurrentWindow->show();
}

void FDRMDialog::eventUpdate()
{
    /* Put (re)initialization code here for the settings that might have
       be changed by another top level window. Called on mode switch */
    //pFileMenu->UpdateMenu();
    ui->MainDisplay->SetDisplayColor(CRGBConversion::int2RGB(getSetting("colorscheme", 0xff0000, true)));
}

void FDRMDialog::eventShow(QShowEvent*)
{
    /* Set timer for real-time controls */
    OnTimer();
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FDRMDialog::eventHide(QHideEvent*)
{
    /* Deactivate real-time timers */
    Timer.stop();
}

void FDRMDialog::OnNewAcquisition()
{
    DRMReceiver.RequestNewAcquisition();
}

void FDRMDialog::OnSwitchMode(int newMode)
{
    DRMReceiver.SetReceiverMode(ERecMode(newMode));
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FDRMDialog::OnSelectAudioService(int shortId)
{
    CParameter& Parameters = *DRMReceiver.GetParameters();

    Parameters.Lock();

    Parameters.SetCurSelAudioService(shortId);

    Parameters.Unlock();
}

void FDRMDialog::OnSelectDataService(int shortId)
{
    CParameter& Parameters = *DRMReceiver.GetParameters();
    QWidget* pDlg = NULL;

    Parameters.Lock();

    int iAppIdent = Parameters.Service[shortId].DataParam.iUserAppIdent;

    switch(iAppIdent)
    {
    case DAB_AT_EPG:
        pDlg = pEPGDlg;
        break;
    case DAB_AT_BROADCASTWEBSITE:
#ifdef QT_WEBKIT_LIB
        pDlg = pBWSDlg;
#endif
        break;
    case DAB_AT_JOURNALINE:
        pDlg = pJLDlg;
        break;
    case DAB_AT_MOTSLIDESHOW:
        pDlg = pSlideShowDlg;
        break;
    }

    if(pDlg != NULL)
        Parameters.SetCurSelDataService(shortId);

    CService::ETyOServ eAudDataFlag = Parameters.Service[shortId].eAudDataFlag;

    Parameters.Unlock();

    if(pDlg != NULL)
    {
        if (pDlg != pEPGDlg)
            iLastMultimediaServiceSelected = shortId;
        pDlg->show();
    }
    else
    {
        if (eAudDataFlag == CService::SF_DATA)
        {
            QMessageBox::information(this, "Dream", tr("unsupported data application"));
        }
    }
}

void FDRMDialog::OnViewMultimediaDlg()
{
    /* Check if multimedia service is available */
    if (iMultimediaServiceBit != 0)
    {
        /* Initialize the multimedia service to show, -1 mean none */
        int iMultimediaServiceToShow = -1;

        /* Check the validity of iLastMultimediaServiceSelected,
           if invalid set it to none */
        if (((1<<iLastMultimediaServiceSelected) & iMultimediaServiceBit) == 0)
            iLastMultimediaServiceSelected = -1;

        /* Simply set to the last selected multimedia if any */
        if (iLastMultimediaServiceSelected != -1)
            iMultimediaServiceToShow = iLastMultimediaServiceSelected;

        else
        {
            /* Select the first multimedia found */
            for (int i = 0; i < MAX_NUM_SERVICES; i++)
            {
                /* A bit is set when a service is available,
                   the position of the bit is the service number */
                if ((1<<i) & iMultimediaServiceBit)
                {
                    /* Service found! */
                    iMultimediaServiceToShow = i;
                    break;
                }
            }
        }
        /* If we have a service then show it */
        if (iMultimediaServiceToShow != -1)
            OnSelectDataService(iMultimediaServiceToShow);
    }
}

void FDRMDialog::OnMenuSetDisplayColor()
{
    const QColor color = CRGBConversion::int2RGB(getSetting("colorscheme", 0xff0000, true));
    const QColor newColor = QColorDialog::getColor(color, this);
    if (newColor.isValid())
    {
        /* Store new color and update display */
        ui->MainDisplay->SetDisplayColor(newColor);
        putSetting("colorscheme", CRGBConversion::RGB2int(newColor), true);
    }
}

void FDRMDialog::eventClose(QCloseEvent* ce)
{
    /* The close event has been actioned and we want to shut
     * down, but the main window should be the last thing to
     * close so that the user knows the program has completed
     * when the window closes
     */
    if (!TimerClose.isActive())
    {
        /* Request that the working thread stops */
        DRMReceiver.Stop();

        /* Stop real-time timer */
        Timer.stop();

        pLogging->SaveSettings(Settings);

        /* Set the timer for polling the working thread state */
        TimerClose.start(50);
    }

    /* Wait indefinitely until the working thread is stopped,
     * so if the window never close it mean there is a bug
     * somewhere, a fix is needed
     */
    if (DRMReceiver.GetParameters()->eRunState == CParameter::STOPPED)
    {
        TimerClose.stop();
        AboutDlg.close();
        pAnalogDemDlg->close();
        pFMDlg->close();
        CSysTray::Destroy(&pSysTray);
        ce->accept();
    }
    else
        ce->ignore();
}

void FDRMDialog::AddWhatsThisHelp()
{
    /* Service Selectors */
    const QString strServiceSel =
        tr("<b>Service Selectors:</b> In a DRM stream up to "
           "four services can be carried. The service can be an audio service, "
           "a data service or an audio service with data. "
           "Audio services can have associated text messages, in addition to any data component. "
           "If a Multimedia data service is selected, the Multimedia Dialog will automatically show up. "
           "On the right of each service selection button a short description of the service is shown. "
           "If an audio service has associated Multimedia data, \"+ MM\" is added to this text. "
           "If such a service is selected, opening the Multimedia Dialog will allow the data to be viewed "
           "while the audio is still playing. If the data component of a service is not Multimedia, "
           "but an EPG (Electronic Programme Guide) \"+ EPG\" is added to the description. "
           "The accumulated Programme Guides for all stations can be viewed by opening the Programme Guide Dialog. "
           "The selected channel in the Programme Guide Dialog defaults to the station being received. "
           "If Alternative Frequency Signalling is available, \"+ AFS\" is added to the description. "
           "In this case the alternative frequencies can be viewed by opening the Live Schedule Dialog."
          );

    ui->serviceTabs->setWhatsThis(strServiceSel);
}
