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
#include <QDir>
#include <QWhatsThis>
#include <QHideEvent>
#include <QEvent>
#include <QShowEvent>
#include <QCloseEvent>
#include <QMessageBox>
#include <QColorDialog>
#include <QDoubleValidator>

#include "ui_DRMMainWindow.h"

#include "dreamtabwidget.h"
#include "engineeringtabwidget.h"
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
#include "../util-QT/EPG.h"
#include "EvaluationDlg.h"
#include "SoundCardSelMenu.h"
#include "DialogUtil.h"
#include "StationsDlg.h"
#include "LiveScheduleDlg.h"
#include "EPGDlg.h"
#include "AnalogDemDlg.h"
#include "MultSettingsDlg.h"
#include "GeneralSettingsDlg.h"
#include "MultColorLED.h"
#include "Logging.h"
#include "../datadecoding/DataDecoder.h"
#include "serviceselector.h"

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
    ui(new Ui::DRMMainWindow),
    DRMReceiver(NDRMR),
    controller(new ReceiverController(&NDRMR, Settings, this)),
    TimerClose(),
    pLogging(NULL),pSysEvalDlg(NULL),pBWSDlg(NULL),
    pSysTray(NULL),
    pScheduler(NULL), pScheduleTimer(NULL),iCurrentFrequency(-1),
    pServiceSelector(NULL),pServiceTabs(NULL),pEngineeringTabs(NULL),
    pMultimediaWindow(NULL),
    baseWindowTitle(tr("Dream DRM Receivvr"))
{
    ui->setupUi(this);
    ui->lineEditFrequency->setVisible(false);
    QDoubleValidator* fvalid = new QDoubleValidator(this);
    ui->lineEditFrequency->setValidator(fvalid);
    connect(ui->lineEditFrequency, SIGNAL(returnPressed ()), this, SLOT(tune()));

    pAboutDlg = new CAboutDlg(this);

    /* Set help text for the controls */
    AddWhatsThisHelp();

    CParameter& Parameters = *DRMReceiver.GetParameters();

    pLogging = new CLogging(Parameters);
    pLogging->LoadSettings(Settings);

    /* Creation of file and sound card ui->menu */
    pFileMenu = new CFileMenu(DRMReceiver, this, ui->menu_View);
    pSoundCardMenu = new CSoundCardSelMenu(DRMReceiver, pFileMenu, this);
    ui->menu_Settings->addMenu(pSoundCardMenu);
    connect(pFileMenu, SIGNAL(soundFileChanged(CDRMReceiver::ESFStatus)), this, SLOT(OnSoundFileChanged(CDRMReceiver::ESFStatus)));

    /* Analog demodulation window */
    pAnalogDemDlg = new AnalogDemDlg(controller, Settings, pFileMenu, pSoundCardMenu);

    /* Parent list for Stations and Live Schedule window */
	QMap <QWidget*,QString> parents;
	parents[this] = "drm";
	parents[pAnalogDemDlg] = "analog";

    /* Stations window */
#ifdef HAVE_LIBHAMLIB
    pStationsDlg = new StationsDlg(Settings, rig, parents);
#else
    pStationsDlg = new StationsDlg(Settings, parents);
#endif

    /* Live Schedule window */
    pLiveScheduleDlg = new LiveScheduleDlg(Settings, parents);

    /* Journaline viewer window */
    pJLDlg = new JLViewer(Settings, this);

    /* MOT slide show window */
    pSlideShowDlg = new SlideShowViewer(Settings, this);

    /* Programme Guide Window */
    string sDataFilesDirectory = Settings.Get(
                "Receiver", "datafilesdirectory", string(DEFAULT_DATA_FILES_DIRECTORY));

    pEPGDlg = new EPGDlg(Settings, this);
    pEPGDlg->setDecoder(new EPG(Parameters, sDataFilesDirectory));

    /* Evaluation window */
    pSysEvalDlg = new systemevalDlg(controller, Settings, this);

    /* general settings window */
    pGeneralSettingsDlg = new GeneralSettingsDlg(Settings, this);

    /* Multimedia settings window */
    pMultSettingsDlg = new MultSettingsDlg(Parameters, Settings, this);

    connect(ui->action_Evaluation_Dialog, SIGNAL(triggered()), pSysEvalDlg, SLOT(show()));
    connect(ui->action_Stations_Dialog, SIGNAL(triggered()), pStationsDlg, SLOT(show()));
    connect(ui->action_Live_Schedule_Dialog, SIGNAL(triggered()), pLiveScheduleDlg, SLOT(show()));
    connect(ui->action_Programme_Guide_Dialog, SIGNAL(triggered()), pEPGDlg, SLOT(show()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    ui->action_Multimedia_Dialog->setEnabled(false);

    connect(ui->actionMultimediaSettings, SIGNAL(triggered()), pMultSettingsDlg, SLOT(show()));

    connect(ui->actionAM, SIGNAL(triggered()), this, SLOT(OnSwitchToAM()));
    connect(ui->actionDRM, SIGNAL(triggered()), this, SLOT(OnSwitchToDRM()));
    connect(ui->actionFM, SIGNAL(triggered()), this, SLOT(OnSwitchToFM()));

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

    connect(pAnalogDemDlg, SIGNAL(About()), this, SLOT(OnHelpAbout()));

    /* Init progress bar for input signal level */
#if QWT_VERSION < 0x060100
    ui->ProgrInputLevel->setRange(-50.0, 0.0);
    ui->ProgrInputLevel->setOrientation(Qt::Vertical, QwtThermo::LeftScale);
#else
    ui->ProgrInputLevel->setScale(-50.0, 0.0);
    ui->ProgrInputLevel->setOrientation(Qt::Vertical);
    ui->ProgrInputLevel->setScalePosition(QwtThermo::TrailingScale);
#endif
    ui->ProgrInputLevel->setAlarmLevel(-12.5);
    QColor alarmColor(QColor(255, 0, 0));
    QColor fillColor(QColor(0, 190, 0));
#if QWT_VERSION < 0x060000
    ui->ProgrInputLevel->setAlarmColor(alarmColor);
    ui->ProgrInputLevel->setFillColor(fillColor);
#else
    QPalette newPalette = ui->FrameMainDisplay->palette();
    newPalette.setColor(QPalette::Base, newPalette.color(QPalette::Window));
    newPalette.setColor(QPalette::ButtonText, fillColor);
    newPalette.setColor(QPalette::Highlight, alarmColor);
    ui->ProgrInputLevel->setPalette(newPalette);
#endif

#ifdef HAVE_LIBHAMLIB
    connect(pStationsDlg, SIGNAL(subscribeRig()), &rig, SLOT(subscribe()));
    connect(pStationsDlg, SIGNAL(unsubscribeRig()), &rig, SLOT(unsubscribe()));
    connect(&rig, SIGNAL(sigstr(double)), pStationsDlg, SLOT(OnSigStr(double)));
    connect(pLogging, SIGNAL(subscribeRig()), &rig, SLOT(subscribe()));
    connect(pLogging, SIGNAL(unsubscribeRig()), &rig, SLOT(unsubscribe()));
#endif
    // Evaluation Dialog
    connect(pSysEvalDlg, SIGNAL(startLogging()), pLogging, SLOT(start()));
    connect(pSysEvalDlg, SIGNAL(stopLogging()), pLogging, SLOT(stop()));
    pSysEvalDlg->connectController(controller);

    /* Update times for color LEDs */
    ui->CLED_FAC->SetUpdateTime(1500);
    ui->CLED_SDC->SetUpdateTime(1500);
    ui->CLED_MSC->SetUpdateTime(600);

    connectController();

    connect(pAnalogDemDlg, SIGNAL(ViewStationsDlg()), pStationsDlg, SLOT(show()));
    connect(pAnalogDemDlg, SIGNAL(ViewLiveScheduleDlg()), pLiveScheduleDlg, SLOT(show()));
    connect(pAnalogDemDlg, SIGNAL(Closed()), this, SLOT(close()));
    connect(pAnalogDemDlg, SIGNAL(SwitchMode(int)), pStationsDlg, SLOT(OnSwitchMode(int)));

    connect(&TimerClose, SIGNAL(timeout()), this, SLOT(OnTimerClose()));

    // clear display
    on_signalLost();

    /* System tray setup */
    pSysTray = CSysTray::Create(
        this,
        SLOT(OnSysTrayActivated(QSystemTrayIcon::ActivationReason)),
        SLOT(OnSysTrayTimer()),
        ":/icons/MainIcon.svg");
    CSysTray::AddAction(pSysTray, tr("&New Acquisition"), controller, SLOT(triggerNewAcquisition()));
    CSysTray::AddSeparator(pSysTray);
    CSysTray::AddAction(pSysTray, tr("&Exit"), this, SLOT(close()));

	/* clear signal strenght */
	setBars(0);

	/* Activate real-time timers */
    controller->start(GUI_CONTROL_UPDATE_TIME);
    show();
}

FDRMDialog::~FDRMDialog()
{
    /* Destroying logger */
    delete pLogging;
    /* Destroying top level windows, children are automaticaly destroyed */
    delete pAnalogDemDlg;
}

void FDRMDialog::connectController()
{
    // ui
    connect(controller, SIGNAL(setAFS(bool)), ui->labelAFS, SLOT(setEnabled(bool)));
    connect(controller, SIGNAL(setAFS(bool)), ui->action_Live_Schedule_Dialog, SLOT(setEnabled(bool)));
    connect(ui->actionReset, SIGNAL(triggered()), controller, SLOT(triggerNewAcquisition()));

    // mainwindow
    connect(controller, SIGNAL(MSCChanged(ETypeRxStatus)), this, SLOT(on_MSCChanged(ETypeRxStatus)));
    connect(controller, SIGNAL(SDCChanged(ETypeRxStatus)), this, SLOT(on_SDCChanged(ETypeRxStatus)));
    connect(controller, SIGNAL(FACChanged(ETypeRxStatus)), this, SLOT(on_FACChanged(ETypeRxStatus)));
    connect(controller, SIGNAL(channelReceptionChanged(Reception)), this, SLOT(on_channelReceptionChanged(Reception)));
    connect(controller, SIGNAL(InputSignalLevelChanged(double)), this, SLOT(on_InputSignalLevelChanged(double)));
    connect(controller, SIGNAL(serviceChanged(int, const CService&)), this, SLOT(on_serviceChanged(int, const CService&)));
    connect(controller, SIGNAL(signalLost()), this, SLOT(on_signalLost()));
    connect(controller, SIGNAL(mode(int)), this, SLOT(on_modeChanged(int)));
    connect(controller, SIGNAL(frequencyChanged(int)), this, SLOT(on_frequencyChanged(int)));
    connect(this, SIGNAL(frequencyChanged(int)), controller, SLOT(setFrequency(int)));

    // stations
    connect(pStationsDlg, SIGNAL(frequencyChanged(int)), controller, SLOT(setFrequency(int)));
    connect(controller, SIGNAL(mode(int)), pStationsDlg, SLOT(OnSwitchMode(int)));

    // AFS
    connect(controller, SIGNAL(frequencyChanged(int)), pStationsDlg, SLOT(SetFrequency(int)));
    connect(controller, SIGNAL(serviceChanged(int, const CService&)), pLiveScheduleDlg, SLOT(setService(int, const CService&)));
    connect(controller, SIGNAL(position(double,double)), pLiveScheduleDlg, SLOT(setLocation(double, double)));
    connect(controller, SIGNAL(position(double,double)), pLiveScheduleDlg, SLOT(setLocation(double, double)));
    connect(controller, SIGNAL(AFS(const CAltFreqSign&)), pLiveScheduleDlg, SLOT(setAFS(const CAltFreqSign&)));
    connect(controller, SIGNAL(serviceInformation(const map <uint32_t,CServiceInformation>)),
            pLiveScheduleDlg, SLOT(setServiceInformation(const map <uint32_t,CServiceInformation>)));

    // AM
    connect(pAnalogDemDlg, SIGNAL(SwitchMode(int)), controller, SLOT(setMode(int)));
    connect(controller, SIGNAL(mode(int)), pAnalogDemDlg, SLOT(on_modeChanged(int)));
    connect(pAnalogDemDlg, SIGNAL(NewAMAcquisition()), controller, SLOT(triggerNewAcquisition()));
}

void FDRMDialog::setBars(int bars)
{
    ui->onebar->setAutoFillBackground(bars>0);
    ui->twobars->setAutoFillBackground(bars>1);
    ui->threebars->setAutoFillBackground(bars>2);
    ui->fourbars->setAutoFillBackground(bars>3);
    ui->fivebars->setAutoFillBackground(bars>4);
}

void FDRMDialog::on_actionEngineering_Mode_triggered(bool checked)
{
    if(checked)
    {
        if(pEngineeringTabs==NULL)
        {
            pEngineeringTabs = new EngineeringTabWidget(controller);
            ui->verticalLayout->addWidget(pEngineeringTabs);
        }
        pEngineeringTabs->show();
    }
    else
    {
        if(pEngineeringTabs)
            pEngineeringTabs->hide();
    }
    Settings.Put("GUI", "engineering", checked);
    QApplication::processEvents();
    resize(0,0);
}

void FDRMDialog::on_actionSingle_Window_Mode_triggered(bool checked)
{
    if(checked)
    {
        if(pServiceTabs==NULL)
        {
            pServiceTabs = new DreamTabWidget(controller, this);
            ui->verticalLayout->addWidget(pServiceTabs);
            connect(pServiceTabs, SIGNAL(audioServiceSelected(int)), controller, SLOT(selectAudioService(int)));
            connect(pServiceTabs, SIGNAL(dataServiceSelected(int)), this, SLOT(OnSelectDataService(int)));
            connect(controller, SIGNAL(serviceChanged(int, const CService&)), pServiceTabs, SLOT(onServiceChanged(int, const CService&)));
            connect(controller, SIGNAL(textMessageChanged(int, QString)), pServiceTabs, SLOT(setText(int, QString)));
        }
        if(pServiceSelector)
            pServiceSelector->hide();
        ui->TextTextMessage->hide();
        pServiceTabs->show();
        ui->menu_View->menuAction()->setVisible(false);
        ui->action_Multimedia_Dialog->setEnabled(false);
        //============ try this ========================
        QWidget* widget = pServiceTabs->currentWidget();
        if(widget)
        {
            widget->parentWidget()->layout()->invalidate();
            QWidget *parent = widget->parentWidget();
            while (parent) {
                parent->adjustSize();
                parent = parent->parentWidget();
            }
        }
    }
    else
    {
        if(pServiceSelector==NULL)
        {
            pServiceSelector = new ServiceSelector(this);
            ui->verticalLayout->addWidget(pServiceSelector);
            connect(pServiceSelector, SIGNAL(audioServiceSelected(int)), controller, SLOT(selectAudioService(int)));
            connect(pServiceSelector, SIGNAL(dataServiceSelected(int)), this, SLOT(OnSelectDataService(int)));
            connect(controller, SIGNAL(serviceChanged(int,const CService&)), pServiceSelector, SLOT(onServiceChanged(int, const CService&)));
            connect(controller, SIGNAL(textMessageChanged(int, QString)), this, SLOT(on_textMessageChanged(int, QString)));
        }
        if(pServiceTabs)
            pServiceTabs->hide();
        pServiceSelector->show();
        ui->TextTextMessage->show();
        ui->menu_View->menuAction()->setVisible(true);
        // TODO enable multimedia menu option if there is a data service we can decode
    }
    Settings.Put("GUI", "singlewindow", checked);
    QApplication::processEvents();
    resize(0,0);
}

void FDRMDialog::on_actionGeneralSettings_triggered()
{
    CParameter& Parameters = *DRMReceiver.GetParameters();
    Parameters.Lock();
    pGeneralSettingsDlg->onPosition(Parameters.gps_data.fix.latitude, Parameters.gps_data.fix.longitude);
    QString gpsd = QString("%1:%2").arg(Parameters.gps_host.c_str()).arg(Parameters.gps_port.c_str());
    pGeneralSettingsDlg->onGPSd(gpsd, false);
    Parameters.Unlock();
    pGeneralSettingsDlg->show();
}

void FDRMDialog::onUserEnteredPosition(double lat, double lng)
{
    CParameter& Parameters = *DRMReceiver.GetParameters();
    Parameters.Lock();
    Parameters.gps_data.fix.latitude = lat;
    Parameters.gps_data.fix.longitude = lng;
    Parameters.Unlock();
}

void FDRMDialog::onUseGPSd(const QString& s)
{
    CParameter& Parameters = *DRMReceiver.GetParameters();
    Parameters.Lock();
    QStringList l = s.split(":");
    string host=l[0].toUtf8().constData();
    if(Parameters.gps_host != host)
    {
        Parameters.restart_gpsd = true;
        Parameters.gps_host=host;
    }
    string port=l[0].toUtf8().constData();
    if(Parameters.gps_port != port)
    {
        Parameters.restart_gpsd = true;
        Parameters.gps_port=port;
    }
    Parameters.Unlock();
}

void FDRMDialog::OnSysTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger
#if QT_VERSION < 0x050000
        || reason == QSystemTrayIcon::DoubleClick
#endif
    )
    {
        const Qt::WindowStates ws = windowState();
        if (ws & Qt::WindowMinimized)
            setWindowState((ws & ~Qt::WindowMinimized) | Qt::WindowActive);
        else
            toggleVisibility();
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
        Parameters.Unlock();
    }
	else {
        Message = tr("Scanning...");
	}
    CSysTray::SetToolTip(pSysTray, Title, Message);
}

void FDRMDialog::OnWhatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

// this is just a signal mapper
void FDRMDialog::OnSwitchToAM()
{
    controller->setMode(RM_AM);
}

// this is just a signal mapper
void FDRMDialog::OnSwitchToDRM()
{
    controller->setMode(RM_DRM);
}

// this is just a signal mapper
void FDRMDialog::OnSwitchToFM()
{
    controller->setMode(RM_FM);
}

void FDRMDialog::on_SDCChanged(ETypeRxStatus s)
{
    SetStatus(ui->CLED_SDC, s);
}

void FDRMDialog::on_FACChanged(ETypeRxStatus s)
{
    SetStatus(ui->CLED_FAC, s);
}

void FDRMDialog::on_MSCChanged(ETypeRxStatus s)
{
    SetStatus(ui->CLED_MSC, s);
}

void FDRMDialog::on_channelReceptionChanged(Reception r)
{
    if(r.wmer>WMERSteps[4])
        setBars(5);
    else if(r.wmer>WMERSteps[3])
        setBars(4);
    else if(r.wmer>WMERSteps[2])
        setBars(3);
    else if(r.wmer>WMERSteps[1])
        setBars(2);
    else if(r.wmer>WMERSteps[0])
        setBars(1);
    else
        setBars(0);
}

void FDRMDialog::on_InputSignalLevelChanged(double d)
{
    ui->ProgrInputLevel->setValue(d);
}
#if 0
void FDRMDialog::UpdateDRM_GUI()
{
    if (isVisible() == false)
        ChangeGUIModeToDRM();

}
#endif
void FDRMDialog::startLogging()
{
    pSysEvalDlg->CheckBoxWriteLog->setChecked(true);
}

void FDRMDialog::stopLogging()
{
    pSysEvalDlg->CheckBoxWriteLog->setChecked(false);
}

void FDRMDialog::OnScheduleTimer()
{
    CScheduler::SEvent e;
    e = pScheduler->front();
    if (e.frequency != -1)
    {
        controller->setFrequency(e.frequency);
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

void FDRMDialog::on_serviceChanged(int short_id, const CService& service)
{
    if(short_id==DRMReceiver.GetParameters()->GetCurSelAudioService())
    {
        if(service.IsActive())
        {
            /* Service label (UTF-8 encoded string -> convert) */
            QString ServiceLabel(QString::fromUtf8(service.strLabel.c_str()));
            ui->LabelServiceLabel->setText(ServiceLabel);
            /* Service ID (plot number in hexadecimal format) */
            const long iServiceID = (long) service.iServiceID;

            if (iServiceID != 0)
            {
                ui->LabelServiceID->setText(QString("ID:%1").arg(iServiceID,4,16).toUpper());
            }
            else
                ui->LabelServiceID->setText("");

            /* Codec label */
            ui->LabelCodec->setText(GetCodecString(service));

            /* Type (Mono / Stereo) label */
            ui->LabelStereoMono->setText(GetTypeString(service));

            /* Language and program type labels (only for audio service) */
            if (service.eAudDataFlag == CService::SF_AUDIO)
            {
                /* SDC Language */
                const string strLangCode = service.strLanguageCode;

                if ((!strLangCode.empty()) && (strLangCode != "---"))
                {
                    ui->LabelLanguage->
                    setText(QString(GetISOLanguageName(strLangCode).c_str()));
                }
                else
                {
                    /* FAC Language */
                    const int iLanguageID = service.iLanguage;

                    if ((iLanguageID > 0) &&
                            (iLanguageID < LEN_TABLE_LANGUAGE_CODE))
                    {
                        ui->LabelLanguage->setText(
                            strTableLanguageCode[iLanguageID].c_str());
                    }
                    else
                        ui->LabelLanguage->setText("");
                }

                /* Program type */
                const int iProgrammTypeID = service.iServiceDescr;

                if ((iProgrammTypeID > 0) &&
                        (iProgrammTypeID < LEN_TABLE_PROG_TYPE_CODE))
                {
                    ui->LabelProgrType->setText(
                        strTableProgTypCod[iProgrammTypeID].c_str());
                }
                else
                    ui->LabelProgrType->setText("");
            }

            /* Country code */
            const string strCntryCode = service.strCountryCode;

            if ((!strCntryCode.empty()) && (strCntryCode != "--"))
            {
                ui->LabelCountryCode->
                setText(QString(GetISOCountryName(strCntryCode).c_str()));
            }
            else
                ui->LabelCountryCode->setText("");

            _REAL rPartABLenRat;
            {
                CParameter& Parameters = *DRMReceiver.GetParameters();
                Parameters.Lock();
                rPartABLenRat = Parameters.PartABLenRatio(short_id);
                Parameters.Unlock();
            }

            /* Bit-rate */
            QString strBitrate = QString().setNum(service.AudioParam.rBitRate, 'f', 2) + tr(" kbps");

            /* Equal or unequal error protection */
            if (rPartABLenRat != (_REAL) 0.0)
            {
                /* Print out the percentage of part A length to total length */
                strBitrate += " UEP (" +
                              QString().setNum(rPartABLenRat * 100, 'f', 1) + " %)";
            }
            else
            {
                /* If part A is zero, equal error protection (EEP) is used */
                strBitrate += " EEP";
            }
            ui->LabelBitrate->setText(strBitrate);
        }
        else
        {
            ui->LabelServiceLabel->setText(tr("No Service"));
            ui->LabelBitrate->setText("");
            ui->LabelCodec->setText("");
            ui->LabelStereoMono->setText("");
            ui->LabelProgrType->setText("");
            ui->LabelLanguage->setText("");
            ui->LabelCountryCode->setText("");
            ui->LabelServiceID->setText("");
        }
    }

    if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
    {
        if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
        {
            switch (service.DataParam.iUserAppIdent)
            {
            case DAB_AT_EPG:
                ui->action_Multimedia_Dialog->setEnabled(false); // has its own menu
                break;
            case DAB_AT_BROADCASTWEBSITE:
            case DAB_AT_JOURNALINE:
            case DAB_AT_MOTSLIDESHOW:
                if(ui->actionSingle_Window_Mode->isChecked()==false)
                    ui->action_Multimedia_Dialog->setEnabled(true);
                break;
            default:
                ui->action_Multimedia_Dialog->setEnabled(false);
                ;
            }
        }
    }
}

void FDRMDialog::on_modeChanged(int value)
{
    switch(ERecMode(value))
    {
    case RM_DRM:
        baseWindowTitle = tr("Dream DRM Receiver");
        ui->actionDRM->setVisible(false);
        ui->actionFM->setVisible(true);
        ui->lineEditFrequency->setVisible(false);
        ui->LabelServiceLabel->setVisible(true);
        on_actionSingle_Window_Mode_triggered(
                    ui->actionSingle_Window_Mode->isChecked()
                    );
        if(isVisible())
            update();
        else
            show();
        CSysTray::Start(pSysTray);
        break;
    case RM_AM:
        baseWindowTitle = tr("Analog Demodulation");
        hide();
        // for later:
        ((QDoubleValidator*)ui->lineEditFrequency->validator())->setRange(0.1,26.1, 3);
        CSysTray::Stop(pSysTray, tr("Dream AM"));
        break;
    case RM_FM:
        baseWindowTitle = tr("Dream FM Receiver");
        ui->actionDRM->setVisible(true);
        ui->actionFM->setVisible(false);
        ((QDoubleValidator*)ui->lineEditFrequency->validator())->setRange(65.8,108.0, 1);
        ui->lineEditFrequency->setVisible(true);
        ui->LabelServiceLabel->setVisible(false); // until we have RDS
        if(pServiceSelector)
            pServiceSelector->hide();
        if(pServiceTabs)
            pServiceTabs->hide();
        // TODO shrink window to min
        CSysTray::Stop(pSysTray, tr("Dream FM"));
        break;
    case RM_NONE: // wait until working thread starts operating
        break;
    }
    UpdateWindowTitle();

    // do this once only
    if(pScheduler==NULL)
        initialiseSchedule();
}

void FDRMDialog::initialiseSchedule()
{
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

void FDRMDialog::on_textMessageChanged(int short_id, QString text)
{
    if(short_id==DRMReceiver.GetParameters()->GetCurSelAudioService())
    {
        ui->TextTextMessage->setEnabled(text!="");
        ui->TextTextMessage->setText(formatTextMessage(text));
    }
}

QString FDRMDialog::formatTextMessage(const QString& textMessage) const
{
    /* Activate text window */
    ui->TextTextMessage->setEnabled(TRUE);

    QString formattedMessage = "";
    for (int i = 0; i < (int)textMessage.length(); i++)
    {
        switch (textMessage.at(i).unicode())
        {
        case 0x0A:
            /* Code 0x0A may be inserted to indicate a preferred
               line break */
        case 0x1F:
            /* Code 0x1F (hex) may be inserted to indicate a
               preferred word break. This code may be used to
            	   display long words comprehensibly */
            formattedMessage += "<br>";
            break;

        case 0x0B:
            /* End of a headline */
            formattedMessage = "<b><u>"
                               + formattedMessage
                               + "</u></b></center><br><center>";
            break;

        case '<':
            formattedMessage += "&lt;";
            break;

        case '>':
            formattedMessage += "&gt;";
            break;

        default:
            formattedMessage += textMessage[int(i)];
        }
    }
    Linkify(formattedMessage);
    return "<center>" + formattedMessage + "</center>";
}

void FDRMDialog::tune()
{
    double f = ui->lineEditFrequency->text().toDouble();
    emit frequencyChanged(floor(1000.0*f));
}

void FDRMDialog::on_frequencyChanged(int freq)
{
    // TODO - add a MHz suffix without messing up editing
    QString fs = QString("%1").arg(double(freq)/1000.0, 5, 'f', 1);
    ui->lineEditFrequency->setText(fs);
}

void FDRMDialog::on_signalLost()
{
    /* No signal is currently received ---------------------------------- */
    /* Disable service buttons and associated labels */
    if(pServiceSelector)
        pServiceSelector->disableAll();
    if(pServiceTabs)
        pServiceTabs->clear();

    /* Main text labels */
    ui->LabelBitrate->setText("");
    ui->LabelCodec->setText("");
    ui->LabelStereoMono->setText("");
    ui->LabelProgrType->setText("");
    ui->LabelLanguage->setText("");
    ui->LabelCountryCode->setText("");
    ui->LabelServiceID->setText("");

    /* Hide text message label */
    ui->TextTextMessage->setEnabled(FALSE);
    ui->TextTextMessage->setText("");

    ui->LabelServiceLabel->setText(tr("Scanning..."));
    setBars(0);
}

void FDRMDialog::UpdateWindowTitle()
{
    QString windowName, fileName(QString::fromLocal8Bit(DRMReceiver.GetInputFileName().c_str()));
    const bool bInputFile = fileName != "";
    fileName = bInputFile ? QDir(fileName).dirName() : "";
    windowName = baseWindowTitle;
    windowName = bInputFile ? fileName + " - " + windowName : windowName;
    setWindowTitle(windowName);
    pAnalogDemDlg->setWindowTitle(windowName);
}

void FDRMDialog::eventUpdate()
{
    /* Put (re)initialization code here for the settings that might have
       be changed by another top level window. Called on mode switch */
    pFileMenu->UpdateMenu();
    SetDisplayColor(CRGBConversion::int2RGB(getSetting("colorscheme", 0xff0000, true)));
}

void FDRMDialog::eventShow(QShowEvent*)
{
    bool b = Settings.Get("GUI", "singlewindow", false);
    ui->actionSingle_Window_Mode->setChecked(b); // does not emit trigger signal
    on_actionSingle_Window_Mode_triggered(b);
    b = Settings.Get("GUI", "engineering", false);
    ui->actionEngineering_Mode->setChecked(b); // does not emit trigger signal
    on_actionEngineering_Mode_triggered(b);
}

void FDRMDialog::eventHide(QHideEvent*)
{
}

void FDRMDialog::OnSelectDataService(int shortId)
{
    if(pMultimediaWindow)
    {
        disconnect(controller, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), pMultimediaWindow, SLOT(setStatus(int, ETypeRxStatus)));
        pMultimediaWindow = NULL;
    }
    if(ui->action_Multimedia_Dialog->isEnabled()) // must be multi-window
        on_action_Multimedia_Dialog_triggered();
    controller->selectDataService(shortId);
}

void FDRMDialog::on_action_Multimedia_Dialog_triggered()
{
    // TODO - run this on startup if the window was already open
    CParameter& Parameters = *DRMReceiver.GetParameters();

    CDataDecoder* DataDecoder = DRMReceiver.GetDataDecoder();

    Parameters.Lock();

    const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
    const uint32_t iAudioServiceID = Parameters.Service[iCurSelAudioServ].iServiceID;

    /* Get current data service */
    int shortID = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[shortID];

    int iAppIdent = Parameters.Service[shortID].DataParam.iUserAppIdent;

    if(pMultimediaWindow)
    {
        disconnect(this, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), pMultimediaWindow, SLOT(setStatus(int, ETypeRxStatus)));
        pMultimediaWindow = NULL;
    }

    switch(iAppIdent)
    {
    case DAB_AT_EPG:
        pEPGDlg->setServiceInformation(Parameters.ServiceInformation, iAudioServiceID);
        break;
    case DAB_AT_BROADCASTWEBSITE:
#ifdef QT_WEBKIT_LIB
        if(pBWSDlg==NULL)
            pBWSDlg = new BWSViewer(DRMReceiver, Settings, this);
        //pBWSDlg->setDecoder(DataDecoder);
        //pBWSDlg->setServiceInformation(service);
        pMultimediaWindow = pBWSDlg;
#endif
        break;
    case DAB_AT_JOURNALINE:
        pJLDlg->setServiceInformation(service, iAudioServiceID);
        pJLDlg->setDecoder(DataDecoder);
        pMultimediaWindow = pJLDlg;
        break;
    case DAB_AT_MOTSLIDESHOW:
        pSlideShowDlg->setServiceInformation(shortID, service);
        pMultimediaWindow = pSlideShowDlg;
        break;
    }

    Parameters.Unlock();

    if(pMultimediaWindow != NULL)
    {
        connect(controller, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), pMultimediaWindow, SLOT(setStatus(int, ETypeRxStatus)));
        pMultimediaWindow->show();
    }
    else
    {
        if(iAppIdent!=0) // might not be set yet.
            QMessageBox::information(this, "Dream", tr("unsupported data application"));
    }
}

void FDRMDialog::OnMenuSetDisplayColor()
{
    const QColor color = CRGBConversion::int2RGB(getSetting("colorscheme", 0xff0000, true));
    const QColor newColor = QColorDialog::getColor(color, this);
    if (newColor.isValid())
    {
        /* Store new color and update display */
        SetDisplayColor(newColor);
        putSetting("colorscheme", CRGBConversion::RGB2int(newColor), true);
    }
}

void FDRMDialog::eventClose(QCloseEvent* ce)
{
    /* The close event has been ui->actioned and we want to shut
     * down, but the main window should be the last thing to
     * close so that the user knows the program has completed
     * when the window closes
     */
    if (!TimerClose.isActive())
    {
        /* Request that the working thread stops */
        DRMReceiver.Stop();

        /* Stop real-time timer */
        //Timer.stop();

        pLogging->SaveSettings(Settings);

        /* Set the timer for polling the working thread state */
        TimerClose.start(50);

        CSysTray::Destroy(&pSysTray);
    }

    /* Wait indefinitely until the working thread is stopped,
     * so if the window never close it mean there is a bug
     * somewhere, a fix is needed
     */
    if (DRMReceiver.GetParameters()->eRunState == CParameter::STOPPED)
    {
        TimerClose.stop();
        pAboutDlg->close();
        pAnalogDemDlg->close();
        ce->accept();
    }
    else
        ce->ignore();
}

void FDRMDialog::SetDisplayColor(const QColor newColor)
{
    /* Collect pointers to the desired controls in a vector */
    vector<QWidget*> vecpWidgets;
    vecpWidgets.push_back(ui->TextTextMessage);
    vecpWidgets.push_back(ui->LabelBitrate);
    vecpWidgets.push_back(ui->LabelCodec);
    vecpWidgets.push_back(ui->LabelStereoMono);
    vecpWidgets.push_back(ui->FrameAudioDataParams);
    vecpWidgets.push_back(ui->LabelProgrType);
    vecpWidgets.push_back(ui->LabelLanguage);
    vecpWidgets.push_back(ui->LabelCountryCode);
    vecpWidgets.push_back(ui->LabelServiceID);
    vecpWidgets.push_back(ui->TextLabelInputLevel);
    vecpWidgets.push_back(ui->ProgrInputLevel);
    vecpWidgets.push_back(ui->CLED_FAC);
    vecpWidgets.push_back(ui->CLED_SDC);
    vecpWidgets.push_back(ui->CLED_MSC);
    vecpWidgets.push_back(ui->FrameMainDisplay);

    for (size_t i = 0; i < vecpWidgets.size(); i++)
    {
        /* Request old palette */
        QPalette CurPal(vecpWidgets[i]->palette());

        /* Change colors */
        if (vecpWidgets[i] != ui->TextTextMessage)
        {
            CurPal.setColor(QPalette::Active, QPalette::Text, newColor);
            CurPal.setColor(QPalette::Active, QPalette::Foreground, newColor);
            CurPal.setColor(QPalette::Inactive, QPalette::Text, newColor);
            CurPal.setColor(QPalette::Inactive, QPalette::Foreground, newColor);
        }
        CurPal.setColor(QPalette::Active, QPalette::Button, newColor);
        CurPal.setColor(QPalette::Active, QPalette::Light, newColor);
        CurPal.setColor(QPalette::Active, QPalette::Dark, newColor);

        CurPal.setColor(QPalette::Inactive, QPalette::Button, newColor);
        CurPal.setColor(QPalette::Inactive, QPalette::Light, newColor);
        CurPal.setColor(QPalette::Inactive, QPalette::Dark, newColor);

        /* Special treatment for text message window */
        if (vecpWidgets[i] == ui->TextTextMessage)
        {
            /* We need to specify special color for disabled */
            CurPal.setColor(QPalette::Disabled, QPalette::Light, Qt::black);
            CurPal.setColor(QPalette::Disabled, QPalette::Dark, Qt::black);
        }

        /* Set new palette */
        vecpWidgets[i]->setPalette(CurPal);
    }
}

void FDRMDialog::AddWhatsThisHelp()
{
    /*
    	This text was taken from the only documentation of Dream software
    */
    /* Text Message */
    QString strTextMessage =
        tr("<b>Text Message:</b> On the top right the text "
           "message label is shown. This label only appears when an actual text "
           "message is transmitted. If the current service does not transmit a "
           "text message, the label will be disabled.");

    /* Input Level */
    const QString strInputLevel =
        tr("<b>Input Level:</b> The input level meter shows "
           "the relative input signal peak level in dB. If the level is too high, "
           "the meter turns from green to red. The red region should be avoided "
           "since overload causes distortions which degrade the reception "
           "performance. Too low levels should be avoided too, since in this case "
           "the Signal-to-Noise Ratio (SNR) degrades.");


    /* Status LEDs */
    const QString strStatusLEDS =
        tr("<b>Status LEDs:</b> The three status LEDs show "
           "the current CRC status of the three logical channels of a DRM stream. "
           "These LEDs are the same as the top LEDs on the Evaluation Dialog.");


    /* Station ui->Label and Info Display */
    const QString strStationLabelOther =
        tr("<b>Station ui->Label and Info Display:</b> In the "
           "big label with the black background the station label and some other "
           "information about the current selected service is displayed. The "
           "magenta text on the top shows the bit-rate of the current selected "
           "service (The abbreviations EEP and "
           "UEP stand for Equal Error Protection and Unequal Error Protection. "
           "UEP is a feature of DRM for a graceful degradation of the decoded "
           "audio signal in case of a bad reception situation. UEP means that "
           "some parts of the audio is higher protected and some parts are lower "
           "protected (the ratio of higher protected part length to total length "
           "is shown in the brackets)), the audio compression format "
           "(e.g. AAC), if SBR is used and what audio mode is used (Mono, Stereo, "
           "P-Stereo -> low-complexity or parametric stereo). In case SBR is "
           "used, the actual sample rate is twice the sample rate of the core AAC "
           "decoder. The next two types of information are the language and the "
           "program type of the service (e.g. German / News).<br>The big "
           "turquoise text in the middle is the station label. This label may "
           "appear later than the magenta text since this information is "
           "transmitted in a different logical channel of a DRM stream. On the "
           "right, the ID number connected with this service is shown.");

    ui->TextTextMessage->setWhatsThis(strTextMessage);
    ui->TextLabelInputLevel->setWhatsThis(strInputLevel);
    ui->ProgrInputLevel->setWhatsThis(strInputLevel);
    ui->CLED_MSC->setWhatsThis(strStatusLEDS);
    ui->CLED_SDC->setWhatsThis(strStatusLEDS);
    ui->CLED_FAC->setWhatsThis(strStatusLEDS);
    ui->LabelBitrate->setWhatsThis(strStationLabelOther);
    ui->LabelCodec->setWhatsThis(strStationLabelOther);
    ui->LabelStereoMono->setWhatsThis(strStationLabelOther);
    ui->LabelServiceLabel->setWhatsThis(strStationLabelOther);
    ui->LabelProgrType->setWhatsThis(strStationLabelOther);
    ui->LabelServiceID->setWhatsThis(strStationLabelOther);
    ui->LabelLanguage->setWhatsThis(strStationLabelOther);
    ui->LabelCountryCode->setWhatsThis(strStationLabelOther);
    ui->FrameAudioDataParams->setWhatsThis(strStationLabelOther);

    const QString strBars = tr("from 1 to 5 bars indicates WMER in the range %1 to %2 dB")
            .arg(floor(WMERSteps[0]))
            .arg(floor(WMERSteps[4]));
    ui->onebar->setWhatsThis(strBars);
    ui->twobars->setWhatsThis(strBars);
    ui->threebars->setWhatsThis(strBars);
    ui->fourbars->setWhatsThis(strBars);
    ui->fivebars->setWhatsThis(strBars);
}

void FDRMDialog::OnHelpAbout()
{
    pAboutDlg->show();
}

void FDRMDialog::OnSoundFileChanged(CDRMReceiver::ESFStatus)
{
    UpdateWindowTitle();
    on_signalLost();
}
