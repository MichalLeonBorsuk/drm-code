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

#ifndef _FDRMDIALOG_H_
#define _FDRMDIALOG_H_


#include "CWindow.h"
#include <QTimer>
#include <QSystemTrayIcon>
#include <../DrmReceiver.h>
#include "receivercontroller.h"

/* Classes ********************************************************************/
class CDRMReceiver;
class CLogging;
class EPGDlg;
class MultSettingsDlg;
class CMultColorLED;
class CSoundCardSelMenu;
class CAboutDlg;
class ServiceSelector;
class StationsDlg;
class LiveScheduleDlg;
class AnalogDemDlg;
class systemevalDlg;
class GeneralSettingsDlg;
class CSysTray;
class CFileMenu;
class BWSViewer;
class JLViewer;
class SlideShowViewer;
class CScheduler;
class DreamTabWidget;
class EngineeringTabWidget;

namespace Ui {
class DRMMainWindow;
}

class FDRMDialog : public CWindow
{
    Q_OBJECT

public:
#ifdef HAVE_LIBHAMLIB
    explicit FDRMDialog(CDRMReceiver&, CSettings&, CRig&, QWidget* parent = 0);
#else
    explicit FDRMDialog(CDRMReceiver&, CSettings&, QWidget* parent = 0);
#endif
    ~FDRMDialog();

private:
    Ui::DRMMainWindow*  ui;
    CDRMReceiver&		DRMReceiver;
    ReceiverController* controller;
    QTimer				TimerClose;

    CLogging*			pLogging;
    systemevalDlg*		pSysEvalDlg;
    BWSViewer*			pBWSDlg;
    JLViewer*			pJLDlg;
    SlideShowViewer*	pSlideShowDlg;
    StationsDlg*		pStationsDlg;
    LiveScheduleDlg*	pLiveScheduleDlg;
    EPGDlg*				pEPGDlg;
    AnalogDemDlg*		pAnalogDemDlg;
    GeneralSettingsDlg* pGeneralSettingsDlg;
    MultSettingsDlg*	pMultSettingsDlg;
    CSysTray*           pSysTray;
    CFileMenu*			pFileMenu;
    CSoundCardSelMenu*	pSoundCardMenu;
    CAboutDlg*		    pAboutDlg;
    QString             SysTrayTitle;
    QString             SysTrayMessage;
    QTimer				TimerSysTray;
    CScheduler* 	    pScheduler;
    QTimer*		        pScheduleTimer;
    int                 iCurrentFrequency;
    ServiceSelector*    pServiceSelector;
    DreamTabWidget*     pServiceTabs;
    EngineeringTabWidget* pEngineeringTabs;
    QWidget*            pMultimediaWindow;
    QString             baseWindowTitle;

    virtual void        eventClose(QCloseEvent* ce);
    virtual void        eventHide(QHideEvent* pEvent);
    virtual void        eventShow(QShowEvent* pEvent);
    virtual void        eventUpdate();
    void		AddWhatsThisHelp();
    void		UpdateWindowTitle();

    void		SetDisplayColor(const QColor newColor);

    QString formatTextMessage(const QString&) const;
    void startLogging();
    void stopLogging();
    void SysTrayCreate();
    void SysTrayStart();
    void SysTrayStop(const QString&);
    void SysTrayToolTip(const QString&, const QString&);
    void setBars(int);
    void connectController();
    void setupWindowMode();
    void changeRecMode(int, bool);

private slots:
    void OnScheduleTimer();
    void OnSysTrayTimer();
    void OnTimerClose();
    void OnSelectDataService(int);
    void OnMenuSetDisplayColor();
    void OnMenuMessageStyle(int);
    void OnMenuPlotStyle(int);
    void OnSwitchToAM();
    void OnSwitchToDRM();
    void OnSwitchToFM();
    void OnWhatsThis();
    void OnHelpAbout();
    void tune();
    void OnSoundFileChanged(CDRMReceiver::ESFStatus);
    void OnSysTrayActivated(QSystemTrayIcon::ActivationReason);
    void initialiseSchedule();
    void on_actionGeneralSettings_triggered();
    void on_actionMultimediaSettings_triggered();
    void onUserEnteredPosition(double, double);
    void onUseGPSd(const QString&);
    void on_action_Multimedia_Dialog_triggered();
    void on_actionSingle_Window_Mode_triggered(bool);
    void on_actionEngineering_Mode_triggered(bool);
    void on_textMessageChanged(int, QString);
    void on_MSCChanged(ETypeRxStatus);
    void on_SDCChanged(ETypeRxStatus);
    void on_FACChanged(ETypeRxStatus);
    void on_InputSignalLevelChanged(double);
    void on_modeChanged(int);
    void on_serviceChanged(int, const CService&);
    void on_signalLost();
    void on_frequencyChanged(int);
    void on_channelReceptionChanged(Reception);

signals:
    void plotStyleChanged(int);
    void frequencyChanged(int);
};

#endif // _FDRMDIALOG_H_
