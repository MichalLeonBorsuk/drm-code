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
#ifndef __TransmDlg_H
#define __TransmDlg_H
// (DF) TODO: to be enabled and removed in a future release
//#define ENABLE_TRANSM_CODECPARAMS

#include <qpushbutton.h>
#include <qstring.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qtabwidget.h>
#include <qcombobox.h>
#include <qstring.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qmenubar.h>
#include <qlayout.h>
#include <qthread.h>
#include <qtimer.h>
#include <qwt_thermo.h>
#include <QMainWindow>
#include <QMenu>
#include "ui_TransmDlgbase.h"

#ifdef _WIN32
# include "windows.h"
#endif
#include "DialogUtil.h"
#include "../DrmTransmitter.h"
#include "../Parameter.h"
#include "../util/Settings.h"
#ifdef ENABLE_TRANSM_CODECPARAMS
# include "CodecParams.h"
#endif


/* Classes ********************************************************************/
/* Thread class for the transmitter */
class CTransmitterThread : public QThread 
{
public:
	CTransmitterThread(CSettings& Settings) : DRMTransmitter(&Settings) {}

	void Stop()
	{
		/* Stop working thread */
		DRMTransmitter.Stop();
	}

	virtual void run()
	{
		/* Set thread priority (The working thread should have a higher priority
		   than the GUI) */
#ifdef _WIN32
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
#endif

		try
		{
			/* Call receiver main routine */
			DRMTransmitter.Start();
		}

		catch (CGenErr GenErr)
		{
			ErrorMessage(GenErr.strError);
		}
	}

	CDRMTransmitter	DRMTransmitter;
};

class TransmDlgBase : public QMainWindow, public Ui_TransmDlgBase
{
public:
	TransmDlgBase(QWidget* parent = 0, const char* name = 0,
		bool modal = FALSE, Qt::WFlags f = 0):
		QMainWindow(parent){(void)name;(void)modal;(void)f;setupUi(this);}
	virtual ~TransmDlgBase() {}
};

class TransmDialog : public TransmDlgBase
{
	Q_OBJECT

public:
	TransmDialog(CSettings&,
		QWidget* parent=0, const char* name=0, bool modal=FALSE, Qt::WFlags f=0);
	virtual ~TransmDialog();

protected:
	void closeEvent(QCloseEvent*);
	void DisableAllControlsForSet();
	void EnableAllControlsForSet();
	void TabWidgetEnableTabs(QTabWidget* tabWidget, bool enable);

	CTransmitterThread	TransThread; /* Working thread object */
	CDRMTransmitter&	DRMTransmitter;
	CSettings&			Settings;
	QMenu*				pSettingsMenu;
	CAboutDlg			AboutDlg;
	QTimer				Timer;
	QTimer				TimerStop;
#ifdef ENABLE_TRANSM_CODECPARAMS
	CodecParams*		CodecDlg;
#endif

	_BOOLEAN			bIsStarted;
	CVector<string>		vecstrTextMessage;
	int					iIDCurrentText;
	int					iServiceDescr;
	_BOOLEAN			bCloseRequested;
#ifdef ENABLE_TRANSM_CODECPARAMS
	int					iButtonCodecState;
	void				ShowButtonCodec(_BOOLEAN bShow, int iKey);
#endif
	_BOOLEAN			GetMessageText(const int iID);
	void				UpdateMSCProtLevCombo();
	void				EnableTextMessage(const _BOOLEAN bFlag);
	void				EnableAudio(const _BOOLEAN bFlag);
	void				EnableData(const _BOOLEAN bFlag);
	void				AddWhatsThisHelp();


public slots:
	void OnButtonStartStop();
	void OnPushButtonAddText();
	void OnButtonClearAllText();
	void OnPushButtonAddFileName();
	void OnButtonClearAllFileNames();
#if defined(ENABLE_TRANSM_CODECPARAMS)
	void OnButtonCodec();
#endif
	void OnToggleCheckBoxHighQualityIQ(bool bState);
	void OnToggleCheckBoxAmplifiedOutput(bool bState);
	void OnToggleCheckBoxEnableData(bool bState);
	void OnToggleCheckBoxEnableAudio(bool bState);
	void OnToggleCheckBoxEnableTextMessage(bool bState);
	void OnToggleCheckBoxRemovePath(bool bState);
	void OnComboBoxMSCInterleaverActivated(int iID);
	void OnComboBoxMSCConstellationActivated(int iID);
	void OnComboBoxSDCConstellationActivated(int iID);
	void OnComboBoxLanguageActivated(int iID);
	void OnComboBoxProgramTypeActivated(int iID);
	void OnComboBoxTextMessageActivated(int iID);
	void OnComboBoxMSCProtLevActivated(int iID);
	void OnRadioRobustnessMode(int iID);
	void OnRadioBandwidth(int iID);
	void OnRadioOutput(int iID);
#if defined(ENABLE_TRANSM_CODECPARAMS)
	void OnRadioCodec(int iID);
#endif
	void OnRadioCurrentTime(int iID);
	void OnTextChangedServiceLabel(const QString& strLabel);
	void OnTextChangedServiceID(const QString& strID);
	void OnTextChangedSndCrdIF(const QString& strIF);
	void OnTimer();
	void OnTimerStop();
	void on_actionWhats_This();
};
#endif
