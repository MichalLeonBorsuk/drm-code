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

#ifndef __EVALUATIONDLG_H
#define __EVALUATIONDLG_H

#include <QMainWindow>
#include "ui_systemevalDlgbase.h"
#include "DRMPlot.h"

#include "DialogUtil.h"
#include "MultColorLED.h"
#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../DrmReceiver.h"
#include "../util/Settings.h"

/* Definitions ****************************************************************/
/* Define this macro if you prefer the QT-type of displaying date and time */
#define GUI_QT_DATE_TIME_TYPE


/* Classes ********************************************************************/

class systemevalDlgBase : public QDialog, public Ui::SystemEvaluationWindow
{
public:
	systemevalDlgBase(QWidget* parent = 0, const char* name = 0,
		bool modal = FALSE, Qt::WFlags f = 0):
		QDialog(parent) {(void)name;(void)modal;(void)f; setWindowFlags(Qt::Window); setupUi(this);}
	virtual ~systemevalDlgBase() {}
};

class systemevalDlg : public systemevalDlgBase
{
	Q_OBJECT

public:
	systemevalDlg(CDRMReceiver&, CSettings&, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);

	virtual ~systemevalDlg();

	void SetStatus(CMultColorLED*, ETypeRxStatus);
	void StopLogTimers();
	void StartLogging(bool bStart);
	void SetFrequency(int iFrequency);

protected:
	CDRMReceiver&	DRMReceiver;
	CSettings&		Settings;

	QTimer			Timer;
	QTimer			TimerInterDigit;

	CDRMPlot*		MainPlot;

	virtual void	showEvent(QShowEvent* pEvent);
	virtual void	hideEvent(QHideEvent* pEvent);
#ifdef _WIN32
	virtual bool	winEvent(MSG *message, long *result);
#endif
	void			UpdateGPS(CParameter&);
	void			UpdateControls();
	void			AddWhatsThisHelp();
	CDRMPlot*		OpenChartWin(CDRMPlot::ECharType eNewType);
	QTreeWidgetItem* FindItemByECharType(CDRMPlot::ECharType eCharType);
	string			ECharTypeToPlotName(CDRMPlot::ECharType eCharType);
	CDRMPlot::ECharType PlotNameToECharType(const string& PlotName);

	QString			GetRobModeStr();
	QString			GetSpecOccStr();

	QMenu*			pTreeWidgetContextMenu;
	CDRMPlot::ECharType eCurCharType, eNewCharType;
	int				iPlotStyle;
	vector<CDRMPlot*>	vecpDRMPlots;
	CEventFilter	ef;

	_BOOLEAN		bEdtFrequencyMutex;

public slots:
	void OnTimer();
	void OnTimerInterDigit();
	void OnRadioTimeLinear();
	void OnRadioTimeWiener();
	void OnRadioFrequencyLinear();
	void OnRadioFrequencyDft();
	void OnRadioFrequencyWiener();
	void OnRadioTiSyncFirstPeak();
	void OnRadioTiSyncEnergy();
	void OnSliderIterChange(int value);
	void OnCheckFlipSpectrum();
	void OnCheckBoxMuteAudio();
	void OnCheckBoxReverb();
	void OnCheckWriteLog();
	void OnCheckSaveAudioWAV();
	void OnCheckRecFilter();
	void OnCheckModiMetric();
	void OnFrequencyEdited (const QString&);
	void OnListSelChanged(QTreeWidgetItem*, QTreeWidgetItem*);
	void OnTreeWidgetContMenu(bool);
	void OnCustomContextMenuRequested(const QPoint&);
	void UpdatePlotStyle(int);
signals:
	void startLogging();
	void stopLogging();
};

#endif
