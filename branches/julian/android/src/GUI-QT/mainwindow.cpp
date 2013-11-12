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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QWhatsThis>
#include <QHideEvent>
#include <QEvent>
#include <QShowEvent>
#include <QCloseEvent>
#include <QTableWidget>
#include "stationselector.h"
#include "SlideShowViewer.h"
#include "journalineviewer.h"
#include "rfwidget.h"
#ifdef QT_WEBKIT_LIB
# include "bwsviewerwidget.h"
#endif
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
#endif
#include "../Scheduler.h"
#include "../util-QT/Util.h"

// Simone's values
// static _REAL WMERSteps[] = {8.0, 12.0, 16.0, 20.0, 24.0};
// David's values
static _REAL WMERSteps[] = {6.0, 12.0, 18.0, 24.0, 30.0};

#ifdef HAVE_LIBHAMLIB
MainWindow::MainWindow(CDRMReceiver& NDRMR, CSettings& Settings, CRig& rig,
                       QWidget* parent)
#else
MainWindow::MainWindow(CDRMReceiver& NDRMR, CSettings& Settings,
                       QWidget* parent)
#endif
    :
    CWindow(parent, Settings, "DRM"),
    ui(new Ui::MainWindow),
    DRMReceiver(NDRMR),
    bEngineering(false),
    pLogging(NULL),
    pSysTray(NULL), pCurrentWindow(this),
    pScheduler(NULL), pScheduleTimer(NULL),
    serviceWidgets(),engineeringWidgets()
{
    ui->setupUi(this);
    for(int i=0; i<4; i++)
    {
        engineeringWidgets[i] = NULL;
    }
    for(int i=0; i<MAX_NUM_SERVICES; i++)
    {
        serviceWidgets[i].audio = NULL;
        serviceWidgets[i].data = NULL;
    }

    /* Set help text for the controls */
    AddWhatsThisHelp();

    CParameter& Parameters = *DRMReceiver.GetParameters();

    pLogging = new CLogging(Parameters);
    pLogging->LoadSettings(Settings);

    /* Creation of file and sound card menu */
    pFileMenu = new CFileMenu(DRMReceiver, this);
    pSoundCardMenu = new CSoundCardSelMenu(DRMReceiver, pFileMenu, this);
    ui->menu_Settings->addMenu(pSoundCardMenu);

    /* TODO add logging menu item and connect to pLogging
    connect(pRF, SIGNAL(startLogging()), pLogging, SLOT(start()));
    connect(pRF, SIGNAL(stopLogging()), pLogging, SLOT(stop()));
    */

    /* Analog demodulation window */
    pAnalogDemDlg = new AnalogDemDlg(DRMReceiver, Settings, pSoundCardMenu);

    /* FM window */
    pFMDlg = new FMDialog(DRMReceiver, Settings, pSoundCardMenu);

    /* general settings window */
    pGeneralSettingsDlg = new GeneralSettingsDlg(Parameters, Settings, this);

    /* Multimedia settings window */
    pMultSettingsDlg = new MultSettingsDlg(Parameters, Settings, this);
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
    connect(pAnalogDemDlg, SIGNAL(SwitchMode(int)), this, SLOT(OnSwitchMode(int)));
    connect(pAnalogDemDlg, SIGNAL(NewAMAcquisition()), this, SLOT(OnNewAcquisition()));
    //connect(pAnalogDemDlg, SIGNAL(ViewStationsDlg()), pStationsDlg, SLOT(show()));
    //connect(pAnalogDemDlg, SIGNAL(ViewLiveScheduleDlg()), pLiveScheduleDlg, SLOT(show()));
    connect(pAnalogDemDlg, SIGNAL(Closed()), this, SLOT(close()));

    connect(pFMDlg, SIGNAL(SwitchMode(int)), this, SLOT(OnSwitchMode(int)));
    connect(pFMDlg, SIGNAL(Closed()), this, SLOT(close()));
    //connect(pFMDlg, SIGNAL(ViewStationsDlg()), pStationsDlg, SLOT(show()));
    //connect(pFMDlg, SIGNAL(ViewLiveScheduleDlg()), pLiveScheduleDlg, SLOT(show()));

    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));
    connect(&TimerClose, SIGNAL(timeout()), this, SLOT(OnTimerClose()));

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

    ui->serviceTabs->clear();

    show();
}

MainWindow::~MainWindow()
{
    delete ui;
    /* Destroying logger */
    delete pLogging;
    /* Destroying top level windows, children are automaticaly destroyed */
    delete pAnalogDemDlg;
    delete pFMDlg;
}
//    TODO putSetting("plotstyle", iPlotStyle, true);

void MainWindow::on_action_Programme_Guide_toggled(bool checked)
{
    if(checked)
    {
        CParameter& Parameters = *DRMReceiver.GetParameters();
        Parameters.Lock();
        pEpg = new EPGDlg(Parameters.GetDataDirectory(), Parameters.ServiceInformation);
        Parameters.Unlock();
        pEpg->showButtons(false);
        int n = ui->serviceTabs->count();
        ui->serviceTabs->insertTab(n++, pEpg, "EPG");
    }
    else
    {
        delete pEpg; pEpg=NULL;
    }
}

void MainWindow::on_actionAlt_Frequencies_toggled(bool checked)
{
    if(checked)
    {
        pAltFreq = new LiveScheduleDlg(DRMReceiver, this);
        int n = ui->serviceTabs->count();
        ui->serviceTabs->insertTab(n++, pAltFreq, "Alt Freq");
    }
    else
    {
        delete pAltFreq; pAltFreq=0;
    }
}

void MainWindow::on_action_Transmissions_toggled(bool checked)
{
    if(checked)
    {
        pTx = new StationSelector();
        int n = ui->serviceTabs->count();
        ui->serviceTabs->insertTab(n++, pTx, "Transmissions");
        connect(this, SIGNAL(Frequency(int)), pTx, SLOT(on_newFrequency(int)));
        connect(pTx, SIGNAL(tuningRequest(int)), this, SLOT(OnTuningRequest(int)));
    }
    else
    {
        disconnect(this, SIGNAL(Frequency(int)), pTx, SLOT(on_newFrequency(int)));
        disconnect(pTx, SIGNAL(tuningRequest(int)), this, SLOT(OnTuningRequest(int)));
        delete pTx; pTx=0;
    }
}

void MainWindow::on_actionEngineering_toggled(bool checked)
{
        bEngineering = checked;
        for(int i=0; i<MAX_NUM_SERVICES; i++)
        {
            if(serviceWidgets[i].audio)
                serviceWidgets[i].audio->setEngineering(bEngineering);
            // TODO serviceWidgets[i].data->setEngineering(bEngineering);
        }

        if(bEngineering)
        {
            if(engineeringWidgets[0]==NULL)
            {
                int iPlotStyle = 0;// TODO set from menu
                RFWidget* pRf = new RFWidget(&DRMReceiver);
                pRf->setPlotStyle(iPlotStyle);
                connect(this, SIGNAL(plotStyleChanged(int)), pRf, SLOT(setPlotStyle(int)));
                engineeringWidgets[0] = pRf;
                engineeringWidgets[1] = new QLabel("");
                engineeringWidgets[2] = new QLabel("");
                engineeringWidgets[3] = new QLabel("");
            }

            int n = ui->serviceTabs->count();
            ui->serviceTabs->insertTab(n++, engineeringWidgets[0], "Channel");
            ui->serviceTabs->insertTab(n++, engineeringWidgets[1], "Streams");
            ui->serviceTabs->insertTab(n++, engineeringWidgets[2], "AFS");
            ui->serviceTabs->insertTab(n++, engineeringWidgets[3], "GPS");
        }
        else
        {
            if(engineeringWidgets[0]!=NULL)
            {
                for(int i=0; i<4; i++)
                {
                    //ui->serviceTabs->removeTab();
                    delete engineeringWidgets[i];
                    engineeringWidgets[i] = NULL;
                }
            }
        }
}

void MainWindow::on_serviceTabs_currentChanged(int)
{
    RFWidget* pRFWidget = (RFWidget*)engineeringWidgets[0];
    if(pRFWidget)
    {
        if(ui->serviceTabs->currentWidget()==pRFWidget)
            pRFWidget->setActive(true);
        else
            pRFWidget->setActive(false);
    }
}

void MainWindow::OnSysTrayActivated(QSystemTrayIcon::ActivationReason reason)
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

void MainWindow::OnSysTrayTimer()
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

void MainWindow::OnWhatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::OnSwitchToFM()
{
    OnSwitchMode(RM_FM);
}

void MainWindow::OnSwitchToAM()
{
    OnSwitchMode(RM_AM);
}

void MainWindow::OnScheduleTimer()
{
    CScheduler::SEvent e;
    e = pScheduler->front();
    if (e.frequency != -1)
    {
        DRMReceiver.SetFrequency(e.frequency);
        if(!pLogging->enabled())
        {
            pLogging->start();
        }
    }
    else
    {
        pLogging->stop();
    }
    if(pScheduler->empty())
    {
        pLogging->stop();
    }
    else
    {
        e = pScheduler->pop();
        time_t now = time(NULL);
        pScheduleTimer->start(1000*(e.time-now));
    }
}

void MainWindow::UpdateChannel(CParameter& Parameters)
{
    RFWidget* pRFWidget = (RFWidget*)engineeringWidgets[0];
    if(pRFWidget==NULL)
        return;

    if(pRFWidget != ui->serviceTabs->currentWidget())
        return;

    Parameters.Lock();

    pRFWidget->setLEDFAC(Parameters.ReceiveStatus.FAC.GetStatus());
    pRFWidget->setLEDSDC(Parameters.ReceiveStatus.SDC.GetStatus());
    //pRFWidget->setLEDMSC(Parameters.ReceiveStatus.MSC.GetStatus());
    pRFWidget->setLEDFrameSync(Parameters.ReceiveStatus.FSync.GetStatus());
    pRFWidget->setLEDTimeSync(Parameters.ReceiveStatus.TSync.GetStatus());
    ETypeRxStatus soundCardStatusI = Parameters.ReceiveStatus.InterfaceI.GetStatus(); /* Input */
    ETypeRxStatus soundCardStatusO = Parameters.ReceiveStatus.InterfaceO.GetStatus(); /* Output */
    pRFWidget->setLEDIOInterface(soundCardStatusO == NOT_PRESENT || (soundCardStatusI != NOT_PRESENT && soundCardStatusI != RX_OK) ? soundCardStatusI : soundCardStatusO);

    /* Show SNR if receiver is in tracking mode */
    if (DRMReceiver.GetAcquiState() == AS_WITH_SIGNAL)
    {
        /* Get a consistant snapshot */

        /* We only get SNR from a local DREAM Front-End */
        pRFWidget->setSNR(Parameters.GetSNR());
        pRFWidget->setMER(Parameters.rMER, Parameters.rWMERMSC);

        pRFWidget->setDelay_Doppler(Parameters.rSigmaEstimate,  Parameters.rMinDelay);

        /* Sample frequency offset estimation */
        pRFWidget->setSampleFrequencyOffset(Parameters.rResampleOffset, Parameters.GetSigSampleRate());

    }
    else
    {
        pRFWidget->setSNR(-1.0);
        pRFWidget->setMER(-1.0, 0);
        pRFWidget->setDelay_Doppler(-1.0, 0);
        pRFWidget->setSampleFrequencyOffset(-1.0, 0);
    }

    /* DC frequency */
    pRFWidget->setFrequencyOffset(DRMReceiver.GetReceiveData()->
                    ConvertFrequency(Parameters.GetDCFrequency()));


    pRFWidget->setChannel(Parameters.GetWaveMode(), Parameters.GetSpectrumOccup(),
            Parameters.eSymbolInterlMode,
            Parameters.eSDCCodingScheme, Parameters.eMSCCodingScheme);

    pRFWidget->setCodeRate(Parameters.MSCPrLe.iPartB, Parameters.MSCPrLe.iPartA);

    pRFWidget->setNumIterations(DRMReceiver.GetMSCMLC()->GetInitNumIterations());
    pRFWidget->setTimeInt(DRMReceiver.GetTimeInt());
    pRFWidget->setFreqInt(DRMReceiver.GetFreqInt());
    pRFWidget->setTiSyncTrac(DRMReceiver.GetTiSyncTracType());
    pRFWidget->setRecFilterEnabled(DRMReceiver.GetFreqSyncAcq()->GetRecFilter());
    pRFWidget->setIntConsEnabled(DRMReceiver.GetIntCons());
    pRFWidget->setFlipSpectrumEnabled(DRMReceiver.GetReceiveData()->GetFlippedSpectrum());


    Parameters.Unlock();
}

void MainWindow::OnSoundFileChanged(CDRMReceiver::ESFStatus)
{
    UpdateDisplay();
}

void MainWindow::OnTimer()
{
    ERecMode eNewReceiverMode = DRMReceiver.GetReceiverMode();
    switch(eNewReceiverMode)
    {
    case RM_DRM:
        UpdateDisplay();
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

    int f = DRMReceiver.GetFrequency();
    if(f != iFrequency)
        emit Frequency(f);
    iFrequency = f;


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
                    pLogging->start();
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
            pLogging->start();  // because its normal, non-scheduled logging ? TODO - check!
    }
}

void MainWindow::OnTimerClose()
{
    if (DRMReceiver.GetParameters()->eRunState == CParameter::STOPPED)
        close();
}

QString
MainWindow::audioServiceDescription(const CService &service)
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
    if (service.AudioParam.rBitrate > (_REAL) 0.0)
    {
        text += " (" + QString().setNum(service.AudioParam.rBitrate, 'f', 2) + " kbps)";
    }

    /* Report missing codec */
    if (!service.AudioParam.bCanDecode)
        text += tr(" [no codec available]");

    return text;
}

QString
MainWindow::dataServiceDescription(const CService &service)
{
    QString text;
    if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
    {
        if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
        {
            switch (service.DataParam.iUserAppIdent)
            {
            case AT_EPG:
                text = tr("Programme Guide");
                break;
            case AT_BROADCASTWEBSITE:
                text = tr("Web Site");
                break;
            case AT_JOURNALINE:
                text = tr("News");
                break;
            case AT_MOTSLIDESHOW:
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

void MainWindow::UpdateDisplay()
{
    CParameter& Parameters = *(DRMReceiver.GetParameters());
    Parameters.Lock();
    ui->MainDisplay->setLevel(Parameters.GetIFSignalLevel());
    ui->MainDisplay->showReceptionStatus(
                Parameters.ReceiveStatus.FAC.GetStatus(),
                Parameters.ReceiveStatus.SDC.GetStatus(),
                Parameters.ReceiveStatus.MSC.GetStatus()
    );

    Parameters.Unlock();

    UpdateChannel(Parameters);

    /* Check if receiver does receive a signal */
    if(DRMReceiver.GetAcquiState() == AS_WITH_SIGNAL) {

        Parameters.Lock();
        for(int shortId=0; shortId<MAX_NUM_SERVICES; shortId++) {

            CService service = Parameters.Service[shortId];
            const _REAL rDataBitRate = Parameters.GetBitRateKbps(shortId, TRUE);
            QString label = QString::fromUtf8(service.strLabel.c_str());

            bool isValidAudioService = service.IsActive()
                    && (service.eAudDataFlag == CService::SF_AUDIO)
                    && (service.AudioParam.iStreamID != STREAM_ID_NOT_USED);

            if (isValidAudioService)
            {
                int index;
                if(label == "")
                    label = QString("%1A").arg(shortId+1);

                if(serviceWidgets[shortId].audio==NULL)
                {
                    AudioDetailWidget* adw = new AudioDetailWidget(&DRMReceiver);
                    serviceWidgets[shortId].audio = adw;
                    index = ui->serviceTabs->addTab(adw, label);
                    connect(adw, SIGNAL(listen(int)), this, SLOT(OnSelectAudioService(int)));
                    connect(this, SIGNAL(plotStyleChanged(int)), adw, SLOT(setPlotStyle(int)));
                }
                else
                {
                    index = ui->serviceTabs->indexOf(serviceWidgets[shortId].audio);
                    ui->serviceTabs->setTabText(index, label);
                }
                serviceWidgets[shortId].audio->updateDisplay(shortId, service);
                serviceWidgets[shortId].audio->setDescription(audioServiceDescription(service));
                // try and order the tabs by shortId
                ui->serviceTabs->tabBar()->moveTab(index, shortId);
            }

            bool isValidDataService = service.IsActive()
                    && (service.DataParam.iStreamID != STREAM_ID_NOT_USED);

            if (isValidDataService)
            {
                int index;
                QString desc = dataServiceDescription(service);
                QString detail =  QString("%1 %2 %3 kbit/s")
                        .arg(desc)
                        .arg(service.iServiceID, 6, 16)
                        .arg(rDataBitRate);

                if(label == "")
                    label = QString("%1 (%2)").arg(shortId+1).arg(desc);
                else
                {
                    if(service.eAudDataFlag == CService::SF_AUDIO)
                    {
                        label = QString("%1 (%2)").arg(label).arg(desc);
                    }
                }

                if(serviceWidgets[shortId].data==NULL)
                {
                    QWidget* w;
                    if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
                    {
                        if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
                        {
                            PacketApplication* pApp = DABApplications::createDecoder(service.DataParam.iUserAppIdent);
                            DRMReceiver.GetDataDecoder()->setApplication(service.DataParam.iPacketID, pApp);
                            switch (service.DataParam.iUserAppIdent)
                            {
                            case AT_EPG:
                                w = new QLabel(detail);
                                break;
                            case AT_BROADCASTWEBSITE:
    #ifdef QT_WEBKIT_LIB
                                w = new BWSViewerWidget(DRMReceiver, (CMOTDABDec*)pApp, Settings, shortId);
                                connect(w, SIGNAL(activated(int)), this, SLOT(OnSelectDataService(int)));
    #endif
                                break;
                            case AT_JOURNALINE:
                                w = new JournalineViewer(Parameters, (CJournaline*)pApp, Settings, shortId);
                                connect(w, SIGNAL(activated(int)), this, SLOT(OnSelectDataService(int)));
                                break;
                            case AT_MOTSLIDESHOW:
                                w = new QLabel(detail);
                                break;
                             default:
                                w = new QLabel(detail);
                            }
                        }
                        else
                            w = new QLabel(detail);
                    }
                    else
                        w = new QLabel(detail);
                    serviceWidgets[shortId].data = w;
                    index = ui->serviceTabs->addTab(w, label);
                }
                else
                {
                    index = ui->serviceTabs->indexOf(serviceWidgets[shortId].data);
                    ui->serviceTabs->setTabText(index, label);
                    QLabel* l = dynamic_cast<QLabel*>(serviceWidgets[shortId].data);
                    if(l)
                        l->setText(detail);
                }
            }
        }

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

        Parameters.Lock();

        ui->MainDisplay->setLevel(Parameters.GetIFSignalLevel());
        ui->MainDisplay->showReceptionStatus(
                    Parameters.ReceiveStatus.FAC.GetStatus(),
                    Parameters.ReceiveStatus.SDC.GetStatus(),
                    Parameters.ReceiveStatus.MSC.GetStatus()
        );

        Parameters.Unlock();
    }
    else {
        /* Main text labels */
        ui->MainDisplay->clearDisplay(tr("Scanning..."));
        ui->MainDisplay->showTextMessage("");
        for(int i=0; i<MAX_NUM_SERVICES; i++)
        {
            if(serviceWidgets[i].audio)
            {
                delete serviceWidgets[i].audio;
                serviceWidgets[i].audio = NULL; // in case called twice

            }
            if(serviceWidgets[i].data)
                delete serviceWidgets[i].data;
                serviceWidgets[i].data = NULL; // in case called twice
        }
    }
}

void MainWindow::ChangeGUIModeToDRM()
{
    CSysTray::Start(pSysTray);
}

void MainWindow::ChangeGUIModeToAM()
{
    emit Mode(RM_AM);
}

void MainWindow::ChangeGUIModeToFM()
{
    emit Mode(RM_FM);
}

void MainWindow::eventUpdate()
{
    /* Put (re)initialization code here for the settings that might have
       be changed by another top level window. Called on mode switch */
    //pFileMenu->UpdateMenu();
    ui->MainDisplay->SetDisplayColor(CRGBConversion::int2RGB(getSetting("colorscheme", 0xff0000, true)));
}

void MainWindow::eventShow(QShowEvent*)
{
    /* Set timer for real-time controls */
    OnTimer();
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void MainWindow::eventHide(QHideEvent*)
{
    /* Deactivate real-time timers */
    Timer.stop();
}

void MainWindow::OnNewAcquisition()
{
    DRMReceiver.RequestNewAcquisition();
}

void MainWindow::OnTuningRequest(int val)
{
    DRMReceiver.SetFrequency(val);
}

void MainWindow::OnSwitchMode(int newMode)
{
    DRMReceiver.SetReceiverMode(ERecMode(newMode));
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void MainWindow::OnSelectAudioService(int shortId)
{
    CParameter& Parameters = *DRMReceiver.GetParameters();

    Parameters.Lock();

    Parameters.SetCurSelAudioService(shortId);

    Parameters.Unlock();
}

void MainWindow::OnSelectDataService(int shortId)
{
    CParameter& Parameters = *DRMReceiver.GetParameters();

    Parameters.Lock();

    Parameters.SetCurSelDataService(shortId);

    Parameters.Unlock();
}

void MainWindow::OnMenuSetDisplayColor()
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

void MainWindow::eventClose(QCloseEvent* ce)
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

void MainWindow::AddWhatsThisHelp()
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
