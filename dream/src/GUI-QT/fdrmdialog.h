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

#ifndef __FDRMDIALOG_H
#define __FDRMDIALOG_H

#include <qlabel.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qstring.h>
#include <qmenubar.h>
#include <qlayout.h>
#include <qpalette.h>
#include <qcolordialog.h>
#include <qwt_thermo.h>
#include <QActionGroup>
#include <QSignalMapper>
#include <QDialog>
#include <QMenu>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include "ui_DRMMainWindow.h"
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

/* Classes ********************************************************************/
class BWSViewer;
class JLViewer;
class SlideShowViewer;
class CScheduler;

class FDRMDialog : public QMainWindow, public Ui_DRMMainWindow
{
    Q_OBJECT

public:
#ifdef HAVE_LIBHAMLIB
    FDRMDialog(CDRMReceiver&, CSettings&, CRig&, QWidget* parent = 0);
#else
    FDRMDialog(CDRMReceiver&, CSettings&, QWidget* parent = 0);
#endif
    void switchEvent();

    virtual ~FDRMDialog();

protected:
    CDRMReceiver&		DRMReceiver;
    CSettings&			Settings;
    QTimer				Timer;
    QTimer				TimerClose;
    vector<QLabel*>		serviceLabels;

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
    QButtonGroup*		pButtonGroup;
    QMenu*				pReceiverModeMenu;
    QMenu*				pSettingsMenu;
    QMenu*				pPlotStyleMenu;
    QSignalMapper*		plotStyleMapper;
    QActionGroup*		plotStyleGroup;
    QSystemTrayIcon*    pSysTray;
    QWidget*            pCurrentWindow;
    CFileMenu*			pFileMenu;
    CSoundCardSelMenu*	pSoundCardMenu;
    CAboutDlg		    AboutDlg;
    int			        iMultimediaServiceBit;
    int			        iLastMultimediaServiceSelected;
    QString             SysTrayTitle;
    QString             SysTrayMessage;
    QTimer				TimerSysTray;
    CScheduler* 	    pScheduler;
    QTimer*		        pScheduleTimer;
    CEventFilter        ef;

    void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
    virtual void	closeEvent(QCloseEvent* ce);
    virtual void	showEvent(QShowEvent* pEvent);
    virtual void	hideEvent(QHideEvent* pEvent);
    void		AddWhatsThisHelp();
    void		UpdateDRM_GUI();
    void		UpdateDisplay();
    void		ClearDisplay();

    void		SetDisplayColor(const QColor newColor);

    void		ChangeGUIModeToDRM();
    void		ChangeGUIModeToAM();
    void		ChangeGUIModeToFM();

    QString	GetCodecString(const CService&);
    QString	GetTypeString(const CService&);
    QString serviceSelector(CParameter&, int);
    void showTextMessage(const QString&);
    void showServiceInfo(const CService&);
    void startLogging();
    void stopLogging();
    void SysTrayCreate();
    void SysTrayStart();
    void SysTrayStop(const QString&);
    void SysTrayToolTip(const QString&, const QString&);
    void SetWindowGeometry();

public slots:
    void OnTimer();
    void OnScheduleTimer();
    void OnSysTrayTimer();
    void OnTimerClose();
    void OnSelectAudioService(int);
    void OnSelectDataService(int);
    void OnViewMultimediaDlg();
    void OnMenuSetDisplayColor();
    void OnNewAcquisition();
    void OnSwitchMode(int);
    void OnSwitchToFM();
    void OnSwitchToAM();
    void OnHelpAbout() {AboutDlg.show();}
    void OnSoundFileChanged(CDRMReceiver::ESFStatus) {ClearDisplay();};
    void OnWhatsThis();
    void OnSysTrayActivated(QSystemTrayIcon::ActivationReason);
signals:
    void plotStyleChanged(int);
};

#endif
