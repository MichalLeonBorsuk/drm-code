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
class FMDialog;
class systemevalDlg;
class GeneralSettingsDlg;
class CSysTray;
class CFileMenu;
class BWSViewer;
class JLViewer;
class SlideShowViewer;
class CScheduler;
class DreamTabWidget;

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
    QTimer				Timer;
    QTimer				TimerClose;

    CLogging*			pLogging;
    systemevalDlg*		pSysEvalDlg;
    BWSViewer*			pBWSDlg;
    JLViewer*			pJLDlg;
    SlideShowViewer*	pSlideShowDlg;
    MultSettingsDlg*	pMultSettingsDlg;
    StationsDlg*		pStationsDlg;
    LiveScheduleDlg*	pLiveScheduleDlg;
    EPGDlg*				pEPGDlg;
    AnalogDemDlg*		pAnalogDemDlg;
    FMDialog*			pFMDlg;
    GeneralSettingsDlg* pGeneralSettingsDlg;
    QMenuBar*			pMenu;
    QMenu*				pReceiverModeMenu;
    QMenu*				pSettingsMenu;
    QMenu*				pPlotStyleMenu;
    CSysTray*           pSysTray;
    CWindow*            pCurrentWindow;
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
    QWidget*            pMultimediaWindow;

    virtual void        eventClose(QCloseEvent* ce);
    virtual void        eventHide(QHideEvent* pEvent);
    virtual void        eventShow(QShowEvent* pEvent);
    virtual void        eventUpdate();
    void		AddWhatsThisHelp();
    void		UpdateDRM_GUI();
    void		UpdateDisplay();
    void		ClearDisplay();
    void		UpdateWindowTitle();

    void		SetDisplayColor(const QColor newColor);

    void		ChangeGUIModeToDRM();
    void		ChangeGUIModeToAM();
    void		ChangeGUIModeToFM();

    QString formatTextMessage(const QString&) const;
    void showServiceInfo(const CService&);
    void startLogging();
    void stopLogging();
    void SysTrayCreate();
    void SysTrayStart();
    void SysTrayStop(const QString&);
    void SysTrayToolTip(const QString&, const QString&);
	void setBars(int);

private slots:
    void OnTimer();
    void OnScheduleTimer();
    void OnSysTrayTimer();
    void OnTimerClose();
    void OnSelectAudioService(int);
    void OnSelectDataService(int);
    void OnMenuSetDisplayColor();
    void OnNewAcquisition();
    void OnSwitchMode(int);
    void OnSwitchToFM();
    void OnSwitchToAM();
    void OnWhatsThis();
    void OnHelpAbout();
    void OnSoundFileChanged(CDRMReceiver::ESFStatus);
    void OnSysTrayActivated(QSystemTrayIcon::ActivationReason);
    void OnGUISetFrequency(int);
    void initialiseSchedule();
    void on_actionGeneralSettings_triggered();
    void onUserEnteredPosition(double, double);
    void onUseGPSd(const QString&);
    void onSaveAudio(const string&);
    void onMuteAudio(bool);
    void onSetTimeInt(CChannelEstimation::ETypeIntTime);
    void onSetFreqInt(CChannelEstimation::ETypeIntFreq);
    void onSetTiSyncTracType(CTimeSyncTrack::ETypeTiSyncTrac);
    void onSetNumMSCMLCIterations(int);
    void onSetFlippedSpectrum(bool);
    void onSetReverbEffect(bool);
    void onSetRecFilter(bool);
    void onSetIntCons(bool);
    void on_action_Multimedia_Dialog_triggered();
    void on_actionSingle_Window_Mode_triggered(bool checked);
    void onTextMessageChanged(int, QString);

signals:
    void serviceChanged(int, const CService&);
    void dataStatusChanged(int, ETypeRxStatus);
    void plotStyleChanged(int);
    void frequencyChanged(int);
    void SwitchMode(int);
    void position(double,double);
    void AFS(const CAltFreqSign&);
    void setAFS(bool);
    void serviceInformation(const map <uint32_t,CServiceInformation>);
    void textMessageChanged(int, const QString&);
};

#endif // _FDRMDIALOG_H_
