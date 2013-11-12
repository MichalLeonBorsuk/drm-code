/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo, Julian Cable
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QString>
#include <QMenuBar>
#include <QLayout>
#include <QPalette>
#include <QColorDialog>
#include <QActionGroup>
#include <QSignalMapper>
#include <QDialog>
#include <QMenu>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>
#include <qwt_thermo.h>
#include "ui_DRMMainWindow.h"
#include "ui_MainWindow.h"

#include "CWindow.h"
#include "EvaluationDlg.h"
#include "SoundCardSelMenu.h"
#include "DialogUtil.h"
#include "StationsDlg.h"
#include "LiveScheduleDlg.h"
#include "EPGDlg.h"
#include "fmdialog.h"
#include "AnalogDemDlg.h"
#include "MultSettingsDlg.h"
#include "GeneralSettingsDlg.h"
#include "MultColorLED.h"
#include "Logging.h"
#include "../DrmReceiver.h"
#include "../util/Vector.h"
#include "../datadecoding/DataDecoder.h"
#include "audiodetailwidget.h"
#include "rfwidget.h"

/* Classes ********************************************************************/
class SlideShowViewer;
class CScheduler;

class MainWindow : public CWindow
{
    Q_OBJECT

public:
    explicit MainWindow(CDRMReceiver&, CSettings&,
                    #ifdef HAVE_LIBHAMLIB
                        CRig&,
                    #endif
                        QWidget* parent = 0);
    ~MainWindow();

private:

    struct ServiceWidget {
        ServiceWidget():service(),audio(NULL),data(NULL){}
        CService service;
        AudioDetailWidget* audio;
        QWidget* data;
    };

    Ui::MainWindow *ui;
    CDRMReceiver&		DRMReceiver;
    bool                bEngineering;
    QTimer				Timer;
    QTimer				TimerClose;

    CLogging*			pLogging;
    systemevalDlg*		pSysEvalDlg;
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
    CAboutDlg		    AboutDlg;
    int                 iFrequency;
    QString             SysTrayTitle;
    QString             SysTrayMessage;
    QTimer				TimerSysTray;
    CScheduler* 	    pScheduler;
    QTimer*		        pScheduleTimer;
    ServiceWidget       serviceWidgets[MAX_NUM_SERVICES];
    QWidget*            engineeringWidgets[4];
    EPGDlg*             pEpg;
    QWidget*            pTx;
    QWidget*            pAltFreq; // different from the engineering AFS

    virtual void        eventClose(QCloseEvent* ce);
    virtual void        eventHide(QHideEvent* pEvent);
    virtual void        eventShow(QShowEvent* pEvent);
    virtual void        eventUpdate();
    void		AddWhatsThisHelp();
    void		UpdateDisplay();

    void		ChangeGUIModeToDRM();
    void		ChangeGUIModeToAM();
    void		ChangeGUIModeToFM();

    QString audioServiceDescription(const CService&);
    QString dataServiceDescription(const CService&);
    void SysTrayCreate();
    void SysTrayStart();
    void SysTrayStop(const QString&);
    void SysTrayToolTip(const QString&, const QString&);
    void UpdateChannel(CParameter&);

public slots:
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
    void OnHelpAbout() {AboutDlg.show();}
    void OnSoundFileChanged(CDRMReceiver::ESFStatus);
    void OnWhatsThis();
    void OnSysTrayActivated(QSystemTrayIcon::ActivationReason);
    void on_actionEngineering_toggled(bool);
    void on_action_Programme_Guide_toggled(bool);
    void on_actionAlt_Frequencies_toggled(bool);
    void on_action_Transmissions_toggled(bool);
    void on_serviceTabs_currentChanged(int);
    void OnTuningRequest(int);

signals:
    void plotStyleChanged(int);
    void LEDFAC(ETypeRxStatus);
    void LEDSDC(ETypeRxStatus status);
    void LEDMSC(ETypeRxStatus status);
    void LEDFrameSync(ETypeRxStatus status);
    void LEDTimeSync(ETypeRxStatus status);
    void LEDIOInterface(ETypeRxStatus status);
    void LEDData(int, int, ETypeRxStatus);
    void LEDAudio(int, ETypeRxStatus);
    void SNR(double rSNR);
    void MER(double rMER, double rWMERMSC);
    void Delay_Doppler(double rSigmaEstimate, double rMinDelay);
    void SampleFrequencyOffset(double rCurSamROffs, double rSampleRate);
    void FrequencyOffset(double);
    void Channel(ERobMode, ESpecOcc, ESymIntMod, ECodScheme, ECodScheme);
    void Frequency(int);
    void Mode(int);
    void dataStatus(int, int, ETypeRxStatus);
};

#endif // MAINWINDOW_H

