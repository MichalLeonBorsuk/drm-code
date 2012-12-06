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
#if QT_VERSION < 0x040000
# include <qpopupmenu.h>
# include <qbuttongroup.h>
# include <qwhatsthis.h>
# include <qcstring.h>
# include "MultimediaDlg.h"
# define toUpper(s) s.upper()
#else
# include <QWhatsThis>
# include <QHideEvent>
# include <QEvent>
# include <QShowEvent>
# include <QCloseEvent>
# include "BWSViewer.h"
# include "SlideShowViewer.h"
# include "JLViewer.h"
# define CHECK_PTR(x) Q_CHECK_PTR(x)
# define toUpper(s) s.toUpper()
#endif
#include "Rig.h"
#include "../Scheduler.h"

inline QString str2qstr(const string& s) {
#if QT_VERSION < 0x040000
    return QString().fromUtf8(QCString(s.c_str()));
#else
    return QString().fromUtf8(s.c_str());
#endif
}

/* Implementation *************************************************************/
FDRMDialog::FDRMDialog(CDRMReceiver& NDRMR, CSettings& NSettings, CRig& rig,
                       QWidget* parent, const char* name, bool modal, Qt::WFlags f)
    :
    FDRMDialogBase(parent, name, modal, f),
    DRMReceiver(NDRMR),Settings(NSettings),
    Timer(),serviceLabels(4),pLogging(NULL),
    iMultimediaServiceBit(0),
    iLastMultimediaServiceSelected(-1),
    pScheduler(NULL), pScheduleTimer(NULL)
{
    /* Recover window size and position */
    CWinGeom s;
    Settings.Get("DRM Dialog", s);
    const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
    if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
        setGeometry(WinGeom);

    /* Set help text for the controls */
    AddWhatsThisHelp();

    CParameter& Parameters = *DRMReceiver.GetParameters();

    pLogging = new CLogging(Parameters);
    pLogging->LoadSettings(Settings);
    pLogging->reStart();

#if QT_VERSION < 0x040000
    /* Analog demodulation window */
    pAnalogDemDlg = new AnalogDemDlg(DRMReceiver, Settings, NULL, "", FALSE, Qt::WStyle_MinMax);
    SetDialogCaption(pAnalogDemDlg, tr("Analog Demodulation"));

    /* FM window */
    pFMDlg = new FMDialog(DRMReceiver, Settings, rig, NULL, "", FALSE, Qt::WStyle_MinMax);
    SetDialogCaption(pFMDlg, tr("FM Receiver"));

    /* Multimedia window */
    pMultiMediaDlg = new MultimediaDlg(DRMReceiver, Settings, NULL, "", FALSE, Qt::WStyle_MinMax);
    SetDialogCaption(pMultiMediaDlg, tr("Multimedia"));

    /* Stations window */
    pStationsDlg = new StationsDlg(DRMReceiver, Settings, rig, this, "", FALSE, Qt::WStyle_MinMax);
    SetDialogCaption(pStationsDlg, tr("Stations"));

    /* Live Schedule window */
    pLiveScheduleDlg = new LiveScheduleDlg(DRMReceiver, Settings, this, "", FALSE, Qt::WStyle_MinMax);
    SetDialogCaption(pLiveScheduleDlg, tr("Live Schedule"));

    /* Programme Guide Window */
    pEPGDlg = new EPGDlg(DRMReceiver, Settings, this, "", FALSE, Qt::WStyle_MinMax);
    SetDialogCaption(pEPGDlg, tr("Programme Guide"));

    /* Evaluation window */
    pSysEvalDlg = new systemevalDlg(DRMReceiver, Settings, this, "", FALSE, Qt::WStyle_MinMax);
    SetDialogCaption(pSysEvalDlg, tr("System Evaluation"));

    /* General settings window */
    pGeneralSettingsDlg = new GeneralSettingsDlg(Parameters, Settings, this, "", TRUE, Qt::WStyle_Dialog);
    SetDialogCaption(pGeneralSettingsDlg, tr("General settings"));

    /* Multimedia settings window */
    pMultSettingsDlg = new MultSettingsDlg(Parameters, Settings, this, "", TRUE, Qt::WStyle_Dialog);
    SetDialogCaption(pMultSettingsDlg, tr("Multimedia settings"));

    /* Set Menu ***************************************************************/
    /* View menu ------------------------------------------------------------ */
    QPopupMenu* EvalWinMenu = new QPopupMenu(this);
    CHECK_PTR(EvalWinMenu);
    EvalWinMenu->insertItem(tr("&Evaluation Dialog..."), pSysEvalDlg,
                            SLOT(show()), Qt::CTRL+Qt::Key_E, 0);
    EvalWinMenu->insertItem(tr("M&ultimedia Dialog..."), pMultiMediaDlg,
                            SLOT(show()), Qt::CTRL+Qt::Key_U, 1);
    EvalWinMenu->insertItem(tr("S&tations Dialog..."), pStationsDlg,
                            SLOT(show()), Qt::CTRL+Qt::Key_T, 2);
    EvalWinMenu->insertItem(tr("&Live Schedule Dialog..."), pLiveScheduleDlg,
                            SLOT(show()), Qt::CTRL+Qt::Key_L, 3);
    EvalWinMenu->insertItem(tr("&Programme Guide..."), pEPGDlg,
                            SLOT(show()), Qt::CTRL+Qt::Key_P, 4);
    EvalWinMenu->insertSeparator();
    EvalWinMenu->insertItem(tr("E&xit"), this, SLOT(close()), Qt::CTRL+Qt::Key_Q, 5);

    /* Settings menu  ------------------------------------------------------- */
    pSettingsMenu = new QPopupMenu(this);
    CHECK_PTR(pSettingsMenu);

    pSettingsMenu->insertItem(tr("&AM (analog)"), this,
                              SLOT(OnSwitchToAM()), Qt::CTRL+Qt::Key_A);
    pSettingsMenu->insertItem(tr("&FM (analog)"), this,
                              SLOT(OnSwitchToFM()), Qt::CTRL+Qt::Key_F);
    pSettingsMenu->insertItem(tr("New &DRM Acquisition"), this,
                              SLOT(OnNewAcquisition()), Qt::CTRL+Qt::Key_D);
    pSettingsMenu->insertSeparator();
    pSettingsMenu->insertItem(tr("Set D&isplay Color..."), this,
                              SLOT(OnMenuSetDisplayColor()));

    /* Plot style settings */
    pPlotStyleMenu = new QPopupMenu(this);
    pPlotStyleMenu->insertItem(tr("&Blue / White"), this, SLOT(OnMenuPlotStyle(int)), 0, 0);
    pPlotStyleMenu->insertItem(tr("&Green / Black"), this, SLOT(OnMenuPlotStyle(int)), 0, 1);
    pPlotStyleMenu->insertItem(tr("B&lack / Grey"), this, SLOT(OnMenuPlotStyle(int)), 0, 2);
    pSettingsMenu->insertItem(tr("&Plot Style"), pPlotStyleMenu);

    /* Set check */
    pPlotStyleMenu->setItemChecked(Settings.Get("System Evaluation Dialog", "plotstyle", 0), TRUE);

    /* Multimedia settings */
    pSettingsMenu->insertSeparator();
    pSettingsMenu->insertItem(tr("&Multimedia settings..."), pMultSettingsDlg,
                              SLOT(show()));

    pSettingsMenu->insertItem(tr("&General settings..."), pGeneralSettingsDlg,
                              SLOT(show()));

    /* Sound Card */
    pSettingsMenu->insertItem(tr("&Sound Card Selection"),
                              new CSoundCardSelMenu(DRMReceiver.GetSoundInInterface(),
                                      DRMReceiver.GetSoundOutInterface(), this));

    /* Main menu bar -------------------------------------------------------- */
    pMenu = new QMenuBar(this);
    CHECK_PTR(pMenu);
    pMenu->insertItem(tr("&View"), EvalWinMenu);
    pMenu->insertItem(tr("&Settings"), pSettingsMenu);
    pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
    pMenu->setSeparator(QMenuBar::InWindowsStyle);

    /* Now tell the layout about the menu */
    FDRMDialogBaseLayout->setMenuBar(pMenu);

    connect(this, SIGNAL(plotStyleChanged(int)), pSysEvalDlg, SLOT(UpdatePlotStyle(int)));
    connect(this, SIGNAL(plotStyleChanged(int)), pAnalogDemDlg, SLOT(UpdatePlotStyle(int)));

    /* Digi controls */
    /* Set display color */
    SetDisplayColor(CRGBConversion::int2RGB(Settings.Get("DRM Dialog", "colorscheme", 0xff0000)));

    pButtonGroup = new QButtonGroup(this);
    pButtonGroup->hide();
    pButtonGroup->setExclusive(true);
    pButtonGroup->insert(PushButtonService1, 0);
    pButtonGroup->insert(PushButtonService2, 1);
    pButtonGroup->insert(PushButtonService3, 2);
    pButtonGroup->insert(PushButtonService4, 3);
    connect(pButtonGroup, SIGNAL(clicked(int)), this, SLOT(OnSelectAudioService(int)));
    connect(pButtonGroup, SIGNAL(clicked(int)), this, SLOT(OnSelectDataService(int)));
#else
    /* Analog demodulation window */
    pAnalogDemDlg = new AnalogDemDlg(DRMReceiver, Settings);

    /* FM window */
    pFMDlg = new FMDialog(DRMReceiver, Settings, rig);

    /* MOT broadcast website viewer window */
    pBWSDlg = new BWSViewer(DRMReceiver, Settings, NULL);

    /* Journaline viewer window */
    pJLDlg = new JLViewer(DRMReceiver, Settings, NULL);

    /* MOT slide show window */
    pSlideShowDlg = new SlideShowViewer(DRMReceiver, Settings, NULL);

    /* Stations window */
    pStationsDlg = new StationsDlg(DRMReceiver, Settings, rig, this);

    /* Live Schedule window */
    pLiveScheduleDlg = new LiveScheduleDlg(DRMReceiver, Settings, this);

    /* Programme Guide Window */
    pEPGDlg = new EPGDlg(DRMReceiver, Settings, this);

    /* Evaluation window */
    pSysEvalDlg = new systemevalDlg(DRMReceiver, Settings, this);

    /* general settings window */
    pGeneralSettingsDlg = new GeneralSettingsDlg(Parameters, Settings, this);

    /* Multimedia settings window */
    pMultSettingsDlg = new MultSettingsDlg(Parameters, Settings, this);

    connect(action_Evaluation_Dialog, SIGNAL(triggered()), pSysEvalDlg, SLOT(show()));
    connect(action_Multimedia_Dialog, SIGNAL(triggered()), this, SLOT(OnViewMultimediaDlg()));
    connect(action_Stations_Dialog, SIGNAL(triggered()), pStationsDlg, SLOT(show()));
    connect(action_Live_Schedule_Dialog, SIGNAL(triggered()), pLiveScheduleDlg, SLOT(show()));
    connect(action_Programme_Guide_Dialog, SIGNAL(triggered()), pEPGDlg, SLOT(show()));
    connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

    action_Multimedia_Dialog->setEnabled(false);

	pFileMenu = new CFileMenu(DRMReceiver, this, menu_View);

	pSoundCardMenu = new CSoundCardSelMenu(&DRMReceiver,
		DRMReceiver.GetSoundInInterface(),
		DRMReceiver.GetSoundOutInterface(),
		pFileMenu, this);
    menu_Settings->addMenu(pSoundCardMenu);

    connect(actionMultimediaSettings, SIGNAL(triggered()), pMultSettingsDlg, SLOT(show()));
    connect(actionGeneralSettings, SIGNAL(triggered()), pGeneralSettingsDlg, SLOT(show()));

    connect(actionAM, SIGNAL(triggered()), this, SLOT(OnSwitchToAM()));
    connect(actionFM, SIGNAL(triggered()), this, SLOT(OnSwitchToFM()));
    connect(actionDRM, SIGNAL(triggered()), this, SLOT(OnNewAcquisition()));

    connect(actionDisplayColor, SIGNAL(triggered()), this, SLOT(OnMenuSetDisplayColor()));

    /* Plot style settings */
    plotStyleMapper = new QSignalMapper(this);
    plotStyleGroup = new QActionGroup(this);
    plotStyleGroup->addAction(actionBlueWhite);
    plotStyleGroup->addAction(actionGreenBlack);
    plotStyleGroup->addAction(actionBlackGrey);
    plotStyleMapper->setMapping(actionBlueWhite, 0);
    plotStyleMapper->setMapping(actionGreenBlack, 1);
    plotStyleMapper->setMapping(actionBlackGrey, 2);
    connect(actionBlueWhite, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(actionGreenBlack, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(actionBlackGrey, SIGNAL(triggered()), plotStyleMapper, SLOT(map()));
    connect(plotStyleMapper, SIGNAL(mapped(int)), this, SIGNAL(plotStyleChanged(int)));
    switch(Settings.Get("System Evaluation Dialog", "plotstyle", int(0)))
    {
    case 0:
        actionBlueWhite->setChecked(true);
        break;
    case 1:
        actionGreenBlack->setChecked(true);
        break;
    case 2:
        actionBlackGrey->setChecked(true);
        break;
    }

    connect(actionAbout_Dream, SIGNAL(triggered()), this, SLOT(OnHelpAbout()));
    connect(actionWhats_This, SIGNAL(triggered()), this, SLOT(on_actionWhats_This()));

    connect(this, SIGNAL(plotStyleChanged(int)), pSysEvalDlg, SLOT(UpdatePlotStyle(int)));
    connect(this, SIGNAL(plotStyleChanged(int)), pAnalogDemDlg, SLOT(UpdatePlotStyle(int)));

    /* Digi controls */
    /* Set display color */
    SetDisplayColor(CRGBConversion::int2RGB(Settings.Get("DRM Dialog", "colorscheme", 0xff0000)));

    pButtonGroup = new QButtonGroup(this);
    pButtonGroup->setExclusive(true);
    pButtonGroup->addButton(PushButtonService1, 0);
    pButtonGroup->addButton(PushButtonService2, 1);
    pButtonGroup->addButton(PushButtonService3, 2);
    pButtonGroup->addButton(PushButtonService4, 3);
    connect(pButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnSelectAudioService(int)));
    connect(pButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnSelectDataService(int)));

#endif

    connect(pFMDlg, SIGNAL(About()), this, SLOT(OnHelpAbout()));
    connect(pAnalogDemDlg, SIGNAL(About()), this, SLOT(OnHelpAbout()));

    /* Init progress bar for input signal level */
    ProgrInputLevel->setRange(-50.0, 0.0);
    ProgrInputLevel->setAlarmLevel(-12.5);
    QColor alarmColor(QColor(255, 0, 0));
    QColor fillColor(QColor(0, 190, 0));
#if QWT_VERSION < 0x050000
    ProgrInputLevel->setOrientation(QwtThermo::Vertical, QwtThermo::Left);
#else
    ProgrInputLevel->setOrientation(Qt::Vertical, QwtThermo::LeftScale);
#endif
#if QWT_VERSION < 0x060000
    ProgrInputLevel->setAlarmColor(alarmColor);
    ProgrInputLevel->setFillColor(fillColor);
#else
    QPalette newPalette = FrameMainDisplay->palette();
    newPalette.setColor(QPalette::Base, newPalette.color(QPalette::Window));
    newPalette.setColor(QPalette::ButtonText, fillColor);
    newPalette.setColor(QPalette::Highlight, alarmColor);
    ProgrInputLevel->setPalette(newPalette);
#endif

    connect(pStationsDlg, SIGNAL(subscribeRig()), &rig, SLOT(subscribe()));
    connect(pStationsDlg, SIGNAL(unsubscribeRig()), &rig, SLOT(unsubscribe()));
    connect(&rig, SIGNAL(sigstr(double)), pStationsDlg, SLOT(OnSigStr(double)));
    connect(pLogging, SIGNAL(subscribeRig()), &rig, SLOT(subscribe()));
    connect(pLogging, SIGNAL(unsubscribeRig()), &rig, SLOT(unsubscribe()));
    connect(pSysEvalDlg, SIGNAL(startLogging()), pLogging, SLOT(start()));
    connect(pSysEvalDlg, SIGNAL(stopLogging()), pLogging, SLOT(stop()));

    /* Update times for color LEDs */
    CLED_FAC->SetUpdateTime(1500);
    CLED_SDC->SetUpdateTime(1500);
    CLED_MSC->SetUpdateTime(600);

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

    serviceLabels[0] = TextMiniService1;
    serviceLabels[1] = TextMiniService2;
    serviceLabels[2] = TextMiniService3;
    serviceLabels[3] = TextMiniService4;

    ClearDisplay();

    /* Activate real-time timers */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
    string schedfile = Settings.Get("command", "schedule", string());
    if(schedfile != "")
    {
        pScheduler = new CScheduler;
        pScheduler->LoadSchedule(schedfile);
        pScheduleTimer = new QTimer(this);
        connect(pScheduleTimer, SIGNAL(timeout()), this, SLOT(OnScheduleTimer()));
        /* Setup the first timeout */
        CScheduler::SEvent e;
        e = pScheduler->front();
        time_t now = time(NULL);
        pScheduleTimer->start(1000*(e.time-now));
//#if QT_VERSION >= 0x040000
//        pScheduleTimer->start();
//#else
//        // TODO
//#endif
	}
}

FDRMDialog::~FDRMDialog()
{
    delete pLogging;
}

void FDRMDialog::on_actionWhats_This()
{
    QWhatsThis::enterWhatsThisMode();
}

#if QT_VERSION < 0x040000
void FDRMDialog::OnMenuPlotStyle(int value)
{
    /* Set new plot style in other dialogs */
    emit plotStyleChanged(value);
    /* Taking care of the checks */
    for (int i = 0; i < NUM_AVL_COLOR_SCHEMES_PLOT; i++)
    pPlotStyleMenu->setItemChecked(i, i == value);
}
#endif

void FDRMDialog::OnSwitchToFM()
{
    OnSwitchMode(RM_FM);
}

void FDRMDialog::OnSwitchToAM()
{
    OnSwitchMode(RM_AM);
}

void FDRMDialog::SetStatus(CMultColorLED* LED, ETypeRxStatus state)
{
    switch(state)
    {
    case NOT_PRESENT:
        LED->Reset(); /* GREY */
        break;

    case CRC_ERROR:
        LED->SetLight(CMultColorLED::RL_RED);
        break;

    case DATA_ERROR:
        LED->SetLight(CMultColorLED::RL_YELLOW);
        break;

    case RX_OK:
        LED->SetLight(CMultColorLED::RL_GREEN);
        break;
    }
}

void FDRMDialog::UpdateDRM_GUI()
{
#if QT_VERSION >= 0x040000
    _BOOLEAN bMultimediaServiceAvailable;
#endif
    CParameter& Parameters = *DRMReceiver.GetParameters();

    if (isVisible() == false)
        ChangeGUIModeToDRM();

    Parameters.Lock();

    /* Input level meter */
    ProgrInputLevel->setValue(Parameters.GetIFSignalLevel());
    SetStatus(CLED_MSC, Parameters.ReceiveStatus.Audio.GetStatus());
    SetStatus(CLED_SDC, Parameters.ReceiveStatus.SDC.GetStatus());
    SetStatus(CLED_FAC, Parameters.ReceiveStatus.FAC.GetStatus());

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
#if QT_VERSION >= 0x040000
    /* If multimedia service availability has changed
       then update the menu */
    bMultimediaServiceAvailable = iMultimediaServiceBit != 0;
    if (bMultimediaServiceAvailable != action_Multimedia_Dialog->isEnabled())
        action_Multimedia_Dialog->setEnabled(bMultimediaServiceAvailable);
#endif
}

void FDRMDialog::OnScheduleTimer()
{
	CScheduler::SEvent e;
	e = pScheduler->front();
//#if QT_VERSION >= 0x040000
//QDateTime dt;
//dt.setTime_t(e.time);
//qDebug() << dt.toString("yyyy-MM-dd hh:mm:ss") << " " << e.frequency;
//#endif
	if (e.frequency != -1)
	{
		pLogging->LoadSettings(Settings);
		pLogging->reStart();
		DRMReceiver.SetFrequency(e.frequency);
//dprintf("start\n");
	}
	else
	{
		pLogging->stop();
//dprintf("stop\n");
	}
	e = pScheduler->pop();
	time_t now = time(NULL);
	pScheduleTimer->start(1000*(e.time-now));
//#if QT_VERSION >= 0x040000
//	pScheduleTimer->setInterval(1000*(e.time-now));
//#else
//	pScheduleTimer->changeInterval(1000*(e.time-now));
//#endif
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
        pStationsDlg->hide(); // in case open in AM mode - AM dialog can't hide this
        pLiveScheduleDlg->hide(); // in case open in AM mode - AM dialog can't hide this
        ChangeGUIModeToFM();
        break;
    case RM_NONE: // wait until working thread starts operating
        break;
    }
}

void FDRMDialog::OnTimerClose()
{
    if (DRMReceiver.GetParameters()->eRunState == CParameter::STOPPED)
        close();
}

void FDRMDialog::showTextMessage(const QString& textMessage)
{
    /* Activate text window */
    TextTextMessage->setEnabled(TRUE);

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
    formattedMessage = "<center>" + formattedMessage + "</center>";
    TextTextMessage->setText(formattedMessage);

}

void FDRMDialog::showServiceInfo(const CService& service)
{
    /* Service label (UTF-8 encoded string -> convert) */
    LabelServiceLabel->setText(str2qstr(service.strLabel));

    /* Service ID (plot number in hexadecimal format) */
    const long iServiceID = (long) service.iServiceID;

    if (iServiceID != 0)
    {
        LabelServiceID->setText(toUpper(QString("ID:%1").arg(iServiceID,4,16)));
    }
    else
        LabelServiceID->setText("");

    /* Codec label */
    LabelCodec->setText(GetCodecString(service));

    /* Type (Mono / Stereo) label */
    LabelStereoMono->setText(GetTypeString(service));

    /* Language and program type labels (only for audio service) */
    if (service.eAudDataFlag == CService::SF_AUDIO)
    {
        /* SDC Language */
        const string strLangCode = service.strLanguageCode;

        if ((!strLangCode.empty()) && (strLangCode != "---"))
        {
            LabelLanguage->
            setText(QString(GetISOLanguageName(strLangCode).c_str()));
        }
        else
        {
            /* FAC Language */
            const int iLanguageID = service.iLanguage;

            if ((iLanguageID > 0) &&
                    (iLanguageID < LEN_TABLE_LANGUAGE_CODE))
            {
                LabelLanguage->setText(
                    strTableLanguageCode[iLanguageID].c_str());
            }
            else
                LabelLanguage->setText("");
        }

        /* Program type */
        const int iProgrammTypeID = service.iServiceDescr;

        if ((iProgrammTypeID > 0) &&
                (iProgrammTypeID < LEN_TABLE_PROG_TYPE_CODE))
        {
            LabelProgrType->setText(
                strTableProgTypCod[iProgrammTypeID].c_str());
        }
        else
            LabelProgrType->setText("");
    }

    /* Country code */
    const string strCntryCode = service.strCountryCode;

    if ((!strCntryCode.empty()) && (strCntryCode != "--"))
    {
        LabelCountryCode->
        setText(QString(GetISOCountryName(strCntryCode).c_str()));
    }
    else
        LabelCountryCode->setText("");
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
        /* Do UTF-8 to string conversion with the label strings */
        QString strLabel = str2qstr(service.strLabel);

        /* Label for service selection button (service label, codec
           and Mono / Stereo information) */
        text = strLabel + "  |   " + GetCodecString(service) + " " + GetTypeString(service);

        /* Bit-rate (only show if greater than 0) */
        if (rAudioBitRate > (_REAL) 0.0)
        {
            text += " (" + QString().setNum(rAudioBitRate, 'f', 2) + " kbps)";
        }

        /* Audio service */
        if ((service.eAudDataFlag == CService::SF_AUDIO))
        {
            /* Report missing codec */
            if (!DRMReceiver.GetAudSorceDec()->CanDecode(service.AudioParam.eAudioCoding))
                text += tr(" [no codec available]");

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

        switch (i)
        {
        case 0:
            PushButtonService1->setEnabled(TRUE);
            break;

        case 1:
            PushButtonService2->setEnabled(TRUE);
            break;

        case 2:
            PushButtonService3->setEnabled(TRUE);
            break;

        case 3:
            PushButtonService4->setEnabled(TRUE);
            break;
        }
    }
    return text;
}

void FDRMDialog::UpdateDisplay()
{
    CParameter& Parameters = *(DRMReceiver.GetParameters());

    Parameters.Lock();

    /* Receiver does receive a DRM signal ------------------------------- */
    /* First get current selected services */
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
        serviceLabels[i]->setText(label);
#if QT_VERSION < 0x040000
        pButtonGroup->find(i)->setEnabled(label != "");
#else
        pButtonGroup->button(i)->setEnabled(label != "");
#endif
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

#if QT_VERSION < 0x040000
        pButtonGroup->setButton(iCurSelAudioServ);
#else
        pButtonGroup->button(iCurSelAudioServ)->setChecked(true);
#endif

        /* If we have text messages */
        if (audioService.AudioParam.bTextflag == TRUE)
        {
            // Text message of current selected audio service (UTF-8 decoding)
            showTextMessage(str2qstr(audioService.AudioParam.strTextMessage));
        }
        else
        {
            /* Deactivate text window */
            TextTextMessage->setEnabled(FALSE);

            /* Clear Text */
            TextTextMessage->setText("");
        }

        /* Check whether service parameters were not transmitted yet */
        if (audioService.IsActive())
        {
            showServiceInfo(audioService);

            Parameters.Lock();
            _REAL rPartABLenRat = Parameters.PartABLenRatio(iCurSelAudioServ);
            _REAL rBitRate = Parameters.GetBitRateKbps(iCurSelAudioServ, FALSE);
            Parameters.Unlock();

            /* Bit-rate */
            QString strBitrate = QString().setNum(rBitRate, 'f', 2) + tr(" kbps");

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
            LabelBitrate->setText(strBitrate);

        }
        else
        {
            LabelServiceLabel->setText(tr("No Service"));
            LabelBitrate->setText("");
            LabelCodec->setText("");
            LabelStereoMono->setText("");
            LabelProgrType->setText("");
            LabelLanguage->setText("");
            LabelCountryCode->setText("");
            LabelServiceID->setText("");
        }
    }
}

void FDRMDialog::ClearDisplay()
{
    /* No signal is currently received ---------------------------------- */
    /* Disable service buttons and associated labels */
    pButtonGroup->setExclusive(FALSE);
    for(size_t i=0; i<serviceLabels.size(); i++)
    {
#if QT_VERSION < 0x040000
        QPushButton* button = (QPushButton*)pButtonGroup->find(i);
        if (button && button->isEnabled()) button->setEnabled(FALSE);
        if (button && button->isOn())      button->setOn(FALSE);
#else
        QPushButton* button = (QPushButton*)pButtonGroup->button(i);
        if (button && button->isEnabled()) button->setEnabled(FALSE);
        if (button && button->isChecked()) button->setChecked(FALSE);
#endif
        serviceLabels[i]->setText("");
    }
    pButtonGroup->setExclusive(TRUE);

    /* Main text labels */
    LabelBitrate->setText("");
    LabelCodec->setText("");
    LabelStereoMono->setText("");
    LabelProgrType->setText("");
    LabelLanguage->setText("");
    LabelCountryCode->setText("");
    LabelServiceID->setText("");

    /* Hide text message label */
    TextTextMessage->setEnabled(FALSE);
    TextTextMessage->setText("");

    LabelServiceLabel->setText(tr("Scanning..."));
}

/* change mode is only called when the mode REALLY has changed
 * so no conditionals are needed in this routine
 */

void FDRMDialog::ChangeGUIModeToDRM()
{
    switchEvent();
    show();
}

void FDRMDialog::ChangeGUIModeToAM()
{
    hide();
    Timer.stop();
    pAnalogDemDlg->switchEvent();
    pAnalogDemDlg->show();
}

void FDRMDialog::ChangeGUIModeToFM()
{
    hide();
    Timer.stop();
    pFMDlg->switchEvent();
    pFMDlg->show();
}

void FDRMDialog::switchEvent()
{
    /* Put initilization code on mode switch here */
#if QT_VERSION >= 0x040000
    pFileMenu->UpdateMenu();
#endif
}

void FDRMDialog::showEvent(QShowEvent* e)
{
    EVENT_FILTER(e);
    if (Settings.Get("DRM Dialog", "Stations Dialog visible", false))
        pStationsDlg->show();
    else
        pStationsDlg->hide(); // in case AM had it open

    if (Settings.Get("DRM Dialog", "Live Schedule Dialog visible", false))
        pLiveScheduleDlg->show();

    if (Settings.Get("DRM Dialog", "EPG Dialog visible", false))
        pEPGDlg->show();

    if (Settings.Get("DRM Dialog", "System Evaluation Dialog visible", false))
        pSysEvalDlg->show();

#if QT_VERSION < 0x040000
    if (Settings.Get("DRM Dialog", "MultiMedia Dialog visible", false))
        pMultiMediaDlg->show();
#else
    if (Settings.Get("DRM Dialog", "BWS Dialog visible", false))
        pBWSDlg->show();

    if (Settings.Get("DRM Dialog", "SS Dialog visible", false))
        pSlideShowDlg->show();

    if (Settings.Get("DRM Dialog", "JL Dialog visible", false))
        pJLDlg->show();
#endif

    if (Settings.Get("DRM Dialog", "EPG Dialog visible", false))
        pEPGDlg->show();

    /* Set timer for real-time controls */
    OnTimer();
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FDRMDialog::hideEvent(QHideEvent* e)
{
    EVENT_FILTER(e);
    /* Deactivate real-time timers */
    Timer.stop();

    /* remember the state of the windows */
    Settings.Put("DRM Dialog", "Live Schedule Dialog visible", pLiveScheduleDlg->isVisible());
    Settings.Put("DRM Dialog", "System Evaluation Dialog visible", pSysEvalDlg->isVisible());
#if QT_VERSION < 0x040000
    Settings.Put("DRM Dialog", "MultiMedia Dialog visible", pMultiMediaDlg->isVisible());
    pMultiMediaDlg->hide();
    pMultiMediaDlg->SaveSettings(Settings);
#else
    Settings.Put("DRM Dialog", "BWS Dialog visible", pBWSDlg->isVisible());
    Settings.Put("DRM Dialog", "JL Dialog visible", pJLDlg->isVisible());
    Settings.Put("DRM Dialog", "SS Dialog visible", pSlideShowDlg->isVisible());
    pSlideShowDlg->hide();
    pBWSDlg->hide();
    pJLDlg->hide();
#endif
    Settings.Put("DRM Dialog", "EPG Dialog visible", pEPGDlg->isVisible());

    /* now close all the other windows */
    pSysEvalDlg->hide();
    pLiveScheduleDlg->hide();
    pEPGDlg->hide();
    pStationsDlg->hide();

    CWinGeom s;
    QRect WinGeom = geometry();
    s.iXPos = WinGeom.x();
    s.iYPos = WinGeom.y();
    s.iHSize = WinGeom.height();
    s.iWSize = WinGeom.width();
    Settings.Put("DRM Dialog", s);

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

#if QT_VERSION < 0x040000
    switch(iAppIdent)
    {
    case DAB_AT_EPG:
        pDlg = pEPGDlg;
        break;
    case DAB_AT_BROADCASTWEBSITE:
    case DAB_AT_JOURNALINE:
    case DAB_AT_MOTSLIDESHOW:
        pDlg = pMultiMediaDlg;
        break;
    default:
        ;
    }
#else
    switch(iAppIdent)
    {
    case DAB_AT_EPG:
        pDlg = pEPGDlg;
        break;
    case DAB_AT_BROADCASTWEBSITE:
        pDlg = pBWSDlg;
        break;
    case DAB_AT_JOURNALINE:
        pDlg = pJLDlg;
        break;
    case DAB_AT_MOTSLIDESHOW:
        pDlg = pSlideShowDlg;
        break;
    }
#endif

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
    const QColor color = CRGBConversion::int2RGB(Settings.Get("DRM Dialog", "colorscheme", 0xff0000));
    const QColor newColor = QColorDialog::getColor( color, this);
    if (newColor.isValid())
    {
        /* Store new color and update display */
        SetDisplayColor(newColor);
        Settings.Put("DRM Dialog", "colorscheme", CRGBConversion::RGB2int(newColor));
    }
}

void FDRMDialog::closeEvent(QCloseEvent* ce)
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

        pStationsDlg->SaveSettings(Settings);
        pLiveScheduleDlg->SaveSettings(Settings);
        pLogging->SaveSettings(Settings);

        /* Save the station dialog visibility state */
        switch (DRMReceiver.GetReceiverMode())
        {
        case RM_DRM:
            Settings.Put("DRM Dialog", "Stations Dialog visible", pStationsDlg->isVisible());
            break;
        case RM_AM:
            Settings.Put("AM Dialog", "Stations Dialog visible", pStationsDlg->isVisible());
            break;
        default:
            break;
        }

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
#if QT_VERSION >= 0x040000
        AboutDlg.reject();
#endif
        ce->accept();
    }
    else
        ce->ignore();
}

QString FDRMDialog::GetCodecString(const CService& service)
{
    QString strReturn;

    /* First check if it is audio or data service */
    if (service.eAudDataFlag == CService::SF_AUDIO)
    {
        /* Audio service */
        const CAudioParam::EAudSamRat eSamRate = service.AudioParam.eAudioSamplRate;

        /* Audio coding */
        switch (service.AudioParam.eAudioCoding)
        {
        case CAudioParam::AC_AAC:
            /* Only 12 and 24 kHz sample rates are supported for AAC encoding */
            if (eSamRate == CAudioParam::AS_12KHZ)
                strReturn = "aac";
            else
                strReturn = "AAC";
            break;

        case CAudioParam::AC_CELP:
            /* Only 8 and 16 kHz sample rates are supported for CELP encoding */
            if (eSamRate == CAudioParam::AS_8_KHZ)
                strReturn = "celp";
            else
                strReturn = "CELP";
            break;

        case CAudioParam::AC_HVXC:
            strReturn = "HVXC";
            break;
        }

        /* SBR */
        if (service.AudioParam.eSBRFlag == CAudioParam::SB_USED)
        {
            strReturn += "+";
        }
    }
    else
    {
        /* Data service */
        strReturn = "Data:";
    }

    return strReturn;
}

QString FDRMDialog::GetTypeString(const CService& service)
{
    QString strReturn;

    /* First check if it is audio or data service */
    if (service.eAudDataFlag == CService::SF_AUDIO)
    {
        /* Audio service */
        /* Mono-Stereo */
        switch (service.AudioParam.eAudioMode)
        {
        case CAudioParam::AM_MONO:
            strReturn = "Mono";
            break;

        case CAudioParam::AM_P_STEREO:
            strReturn = "P-Stereo";
            break;

        case CAudioParam::AM_STEREO:
            strReturn = "Stereo";
            break;
        }
    }
    else
    {
        /* Data service */
        if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
        {
            if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
            {
                switch (service.DataParam.iUserAppIdent)
                {
                case 1:
                    strReturn = tr("Dynamic labels");
                    break;

                case DAB_AT_MOTSLIDESHOW:
                    strReturn = tr("MOT Slideshow");
                    break;

                case DAB_AT_BROADCASTWEBSITE:
                    strReturn = tr("MOT WebSite");
                    break;

                case DAB_AT_TPEG:
                    strReturn = tr("TPEG");
                    break;

                case DAB_AT_DGPS:
                    strReturn = tr("DGPS");
                    break;

                case DAB_AT_TMC:
                    strReturn = tr("TMC");
                    break;

                case DAB_AT_EPG:
                    strReturn = tr("EPG - Electronic Programme Guide");
                    break;

                case DAB_AT_JAVA:
                    strReturn = tr("Java");
                    break;

                case DAB_AT_JOURNALINE: /* Journaline */
                    strReturn = tr("Journaline");
                    break;
                }
            }
            else
                strReturn = tr("Unknown Service");
        }
        else
            strReturn = tr("Unknown Service");
    }

    return strReturn;
}

void FDRMDialog::SetDisplayColor(const QColor newColor)
{
    /* Collect pointers to the desired controls in a vector */
    vector<QWidget*> vecpWidgets;
    vecpWidgets.push_back(TextTextMessage);
    vecpWidgets.push_back(LabelBitrate);
    vecpWidgets.push_back(LabelCodec);
    vecpWidgets.push_back(LabelStereoMono);
    vecpWidgets.push_back(FrameAudioDataParams);
    vecpWidgets.push_back(LabelProgrType);
    vecpWidgets.push_back(LabelLanguage);
    vecpWidgets.push_back(LabelCountryCode);
    vecpWidgets.push_back(LabelServiceID);
    vecpWidgets.push_back(TextLabelInputLevel);
    vecpWidgets.push_back(ProgrInputLevel);
    vecpWidgets.push_back(CLED_FAC);
    vecpWidgets.push_back(CLED_SDC);
    vecpWidgets.push_back(CLED_MSC);
    vecpWidgets.push_back(FrameMainDisplay);

    for (size_t i = 0; i < vecpWidgets.size(); i++)
    {
        /* Request old palette */
        QPalette CurPal(vecpWidgets[i]->palette());

        /* Change colors */
#if QT_VERSION < 0x040000
        CurPal.setColor(QPalette::Active, QColorGroup::Foreground, newColor);
        CurPal.setColor(QPalette::Active, QColorGroup::Button, newColor);
        CurPal.setColor(QPalette::Active, QColorGroup::Text, newColor);
        CurPal.setColor(QPalette::Active, QColorGroup::Light, newColor);
        CurPal.setColor(QPalette::Active, QColorGroup::Dark, newColor);

        CurPal.setColor(QPalette::Inactive, QColorGroup::Foreground, newColor);
        CurPal.setColor(QPalette::Inactive, QColorGroup::Button, newColor);
        CurPal.setColor(QPalette::Inactive, QColorGroup::Text, newColor);
        CurPal.setColor(QPalette::Inactive, QColorGroup::Light, newColor);
        CurPal.setColor(QPalette::Inactive, QColorGroup::Dark, newColor);

        /* Special treatment for text message window. This should always be
           black color of the text */
        if (vecpWidgets[i] == TextTextMessage)
        {
            CurPal.setColor(QPalette::Active, QColorGroup::Text, Qt::black);
            CurPal.setColor(QPalette::Active, QColorGroup::Foreground, Qt::black);
            CurPal.setColor(QPalette::Inactive, QColorGroup::Text, Qt::black);
            CurPal.setColor(QPalette::Inactive, QColorGroup::Foreground, Qt::black);

            /* We need to specify special color for disabled */
            CurPal.setColor(QPalette::Disabled, QColorGroup::Light, Qt::black);
            CurPal.setColor(QPalette::Disabled, QColorGroup::Dark, Qt::black);
        }
#else
        CurPal.setColor(QPalette::Active, QPalette::Foreground, newColor);
        CurPal.setColor(QPalette::Active, QPalette::Button, newColor);
        CurPal.setColor(QPalette::Active, QPalette::Text, newColor);
        CurPal.setColor(QPalette::Active, QPalette::Light, newColor);
        CurPal.setColor(QPalette::Active, QPalette::Dark, newColor);

        CurPal.setColor(QPalette::Inactive, QPalette::Foreground, newColor);
        CurPal.setColor(QPalette::Inactive, QPalette::Button, newColor);
        CurPal.setColor(QPalette::Inactive, QPalette::Text, newColor);
        CurPal.setColor(QPalette::Inactive, QPalette::Light, newColor);
        CurPal.setColor(QPalette::Inactive, QPalette::Dark, newColor);

        /* Special treatment for text message window. This should always be
           black color of the text */
        if (vecpWidgets[i] == TextTextMessage)
        {
            CurPal.setColor(QPalette::Active, QPalette::Text, Qt::black);
            CurPal.setColor(QPalette::Active, QPalette::Foreground, Qt::black);
            CurPal.setColor(QPalette::Inactive, QPalette::Text, Qt::black);
            CurPal.setColor(QPalette::Inactive, QPalette::Foreground, Qt::black);

            /* We need to specify special color for disabled */
            CurPal.setColor(QPalette::Disabled, QPalette::Light, Qt::black);
            CurPal.setColor(QPalette::Disabled, QPalette::Dark, Qt::black);
        }
#endif
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


    /* Station Label and Info Display */
    const QString strStationLabelOther =
        tr("<b>Station Label and Info Display:</b> In the "
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

#if QT_VERSION < 0x040000
    QWhatsThis::add(TextTextMessage, strTextMessage);
    QWhatsThis::add(TextLabelInputLevel, strInputLevel);
    QWhatsThis::add(ProgrInputLevel, strInputLevel);
    QWhatsThis::add(CLED_MSC, strStatusLEDS);
    QWhatsThis::add(CLED_SDC, strStatusLEDS);
    QWhatsThis::add(CLED_FAC, strStatusLEDS);
    QWhatsThis::add(LabelBitrate, strStationLabelOther);
    QWhatsThis::add(LabelCodec, strStationLabelOther);
    QWhatsThis::add(LabelStereoMono, strStationLabelOther);
    QWhatsThis::add(LabelServiceLabel, strStationLabelOther);
    QWhatsThis::add(LabelProgrType, strStationLabelOther);
    QWhatsThis::add(LabelServiceID, strStationLabelOther);
    QWhatsThis::add(LabelLanguage, strStationLabelOther);
    QWhatsThis::add(LabelCountryCode, strStationLabelOther);
    QWhatsThis::add(FrameAudioDataParams, strStationLabelOther);
    QWhatsThis::add(PushButtonService1, strServiceSel);
    QWhatsThis::add(PushButtonService2, strServiceSel);
    QWhatsThis::add(PushButtonService3, strServiceSel);
    QWhatsThis::add(PushButtonService4, strServiceSel);
    QWhatsThis::add(TextMiniService1, strServiceSel);
    QWhatsThis::add(TextMiniService2, strServiceSel);
    QWhatsThis::add(TextMiniService3, strServiceSel);
    QWhatsThis::add(TextMiniService4, strServiceSel);
#else
    TextTextMessage->setWhatsThis(strTextMessage);
    TextLabelInputLevel->setWhatsThis(strInputLevel);
    ProgrInputLevel->setWhatsThis(strInputLevel);
    CLED_MSC->setWhatsThis(strStatusLEDS);
    CLED_SDC->setWhatsThis(strStatusLEDS);
    CLED_FAC->setWhatsThis(strStatusLEDS);
    LabelBitrate->setWhatsThis(strStationLabelOther);
    LabelCodec->setWhatsThis(strStationLabelOther);
    LabelStereoMono->setWhatsThis(strStationLabelOther);
    LabelServiceLabel->setWhatsThis(strStationLabelOther);
    LabelProgrType->setWhatsThis(strStationLabelOther);
    LabelServiceID->setWhatsThis(strStationLabelOther);
    LabelLanguage->setWhatsThis(strStationLabelOther);
    LabelCountryCode->setWhatsThis(strStationLabelOther);
    FrameAudioDataParams->setWhatsThis(strStationLabelOther);
    PushButtonService1->setWhatsThis(strServiceSel);
    PushButtonService2->setWhatsThis(strServiceSel);
    PushButtonService3->setWhatsThis(strServiceSel);
    PushButtonService4->setWhatsThis(strServiceSel);
    TextMiniService1->setWhatsThis(strServiceSel);
    TextMiniService2->setWhatsThis(strServiceSel);
    TextMiniService3->setWhatsThis(strServiceSel);
    TextMiniService4->setWhatsThis(strServiceSel);
#endif
}
