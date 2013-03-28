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
#ifndef __StationsDlg_H
#define __StationsDlg_H

#include <QSignalMapper>
#include <QNetworkAccessManager>
#include <QTimer>
#include "ui_StationsDlgbase.h"

#include "DialogUtil.h"
#include "CWindow.h"
#include "../DrmReceiver.h"
#include "../util/Vector.h"


/* Definitions ****************************************************************/
/* Define the timer interval of updating the list view */
#define GUI_TIMER_LIST_VIEW_STAT		30000 /* ms (30 seconds) */

/* Define the timer interval of updating the UTC label */
#define GUI_TIMER_UTC_TIME_LABEL		1000 /* ms (1 second) */

/* Define the timer interval of updating s-meter */
#define GUI_TIMER_S_METER				300 /* ms (0.3 seconds) */


#define DRM_SCHEDULE_URL	"http://www.baseportal.com/cgi-bin/baseportal.pl?htx=/drmdx/scheduleini2"

/* File handle type */
#ifdef _WIN32
# define FILE_HANDLE					HANDLE
#else
# define FILE_HANDLE					int
#endif

/* Name for DRM and AM schedule file. If you change something here, make sure
   that you also change the strings and help texts!  */
#define DRMSCHEDULE_INI_FILE_NAME		"DRMSchedule.ini"
#define AMSCHEDULE_INI_FILE_NAME		"AMSchedule.ini"
#define AMSCHEDULE_CSV_FILE_NAME		"AMSchedule.csv"

/* Time definitions for preview */
#define NUM_SECONDS_PREV_5MIN			300
#define NUM_SECONDS_PREV_15MIN			900
#define NUM_SECONDS_PREV_30MIN			1800

#define NUM_SECONDS_SOON_INACTIVE		600

/* String definitions for schedule days */
#define FLAG_STR_IRREGULAR_TRANSM		"0000000"
#define CHR_ACTIVE_DAY_MARKER			'1'

#include <iostream>

namespace Station {
	enum EState {IS_ACTIVE, IS_INACTIVE, IS_PREVIEW, IS_SOON_INACTIVE};
};

/* Classes ********************************************************************/
class CaseInsensitiveTreeWidgetItem : public QTreeWidgetItem
{
public:
	CaseInsensitiveTreeWidgetItem(QTreeWidget* parent=0) : QTreeWidgetItem(parent)
	{
	}

	bool operator< ( const QTreeWidgetItem & rhs) const
	{
		// bug 29 - sort frequency and power numerically
		int col = treeWidget()->sortColumn();
		if (col == 0) // online/offline
			return data(0, Qt::UserRole).toInt() < rhs.data(0, Qt::UserRole).toInt();
		else if (col == 3) // integer frequency
			return text( col ).toInt() < rhs.text( col ).toInt();
		else if (col == 4) // real power
			return text( col ).toDouble() < rhs.text( col ).toDouble();
		else
			return text( col ).toLower() < rhs.text( col ).toLower();
	}
};

class CStationsItem
{
public:
	CStationsItem() : iStartHour(0), iStartMinute(0), iStopHour(0),
		iStopMinute(0), iFreq(0), strName(""),
		strTarget(""), strLanguage(""), strSite(""),
		strCountry(""), strDaysFlags(""), strDaysShow(""),
		rPower((_REAL) 0.0) { }

	Station::EState stateAt(const QDateTime&, int) const;
	_BOOLEAN activeAt(const QDateTime&) const;
	int StartTime() const {return iStartHour * 100 + iStartMinute;}
	int StopTime() const{return iStopHour * 100 + iStopMinute;}
	void SetStartTime(const int iStartTime)
	{
		/* Recover hours and minutes */
		iStartHour = iStartTime / 100;
		iStartMinute = iStartTime - iStartHour * 100;
	}
	void SetStopTime(const int iStopTime)
	{
		/* Recover hours and minutes */
		iStopHour = iStopTime / 100;
		iStopMinute = iStopTime - iStopHour * 100;
	}

	void SetDaysFlagString(const QString& strNewDaysFlags);

	int		iStartHour;
	int		iStartMinute;
	int		iStopHour;
	int		iStopMinute;
	int		iFreq;
	QString	strName;
	QString	strTarget;
	QString	strLanguage;
	QString	strSite;
	QString	strCountry;
	QString	strDaysFlags;
	QString	strDaysShow;
	_REAL	rPower;
};


class CDRMSchedule
{
public:
	CDRMSchedule();
	virtual ~CDRMSchedule() {}

	enum ESchedMode {SM_NONE, SM_DRM, SM_ANALOG};

	void ReadINIFile(FILE* pFile);
	void ReadCSVFile(FILE* pFile);
	ESchedMode GetSchedMode() {return eSchedMode;}
	void SetSchedMode(const ESchedMode);

	int GetStationNumber() {return StationsTable.size();}
	CStationsItem& GetItem(const int iPos) {return StationsTable[iPos];}
	Station::EState GetState(const int iPos);
	bool CheckFilter(const int iPos);

	void SetSecondsPreview(int iSec);
	int GetSecondsPreview();
	void UpdateStringListForFilter(const CStationsItem& StationsItem);
	void LoadSchedule();

	QStringList		ListTargets;
	QStringList		ListCountries;
	QStringList		ListLanguages;

	QString			countryFilterdrm, targetFilterdrm, languageFilterdrm;
	QString			countryFilteranalog, targetFilteranalog, languageFilteranalog;
	QString			schedFileName;
	QUrl			qurldrm, qurlanalog;

protected:
	void			SetAnalogUrl();

	vector<CStationsItem>	StationsTable;
	ESchedMode		eSchedMode;

	/* Minutes for stations preview in seconds if zero then only show active */
	int iSecondsPreviewdrm;
	int iSecondsPreviewanalog;
};

class StationsDlg : public CWindow, public Ui_StationsDlgbase
{
	Q_OBJECT

public:
#ifdef HAVE_LIBHAMLIB
	StationsDlg(CDRMReceiver&, CSettings&, CRig&, QMap<QWidget*,QString>&);
#else
	StationsDlg(CDRMReceiver&, CSettings&, QMap<QWidget*,QString>&);
#endif
	virtual ~StationsDlg();

protected:
	virtual void	eventClose(QCloseEvent* pEvent);
	virtual void	eventHide(QHideEvent* pEvent);
	virtual void	eventShow(QShowEvent* pEvent);
	void			LoadSettings();
	void			SaveSettings();
	void			CheckMode();
	void			LoadSchedule();
	void			LoadFilters();
	void			SetFrequencyFromGUI(int);
	void			SetStationsView();
	void			ClearStationsView();
	void			AddWhatsThisHelp();
	void			EnableSMeter();
	void			DisableSMeter();
	void			AddUpdateDateTime();
	_BOOLEAN		showAll();
	_BOOLEAN		GetSortAscending();
	void			SetSortAscending(_BOOLEAN b);
	void			ColumnParamFromStr(const QString& strColumnParam);
	void			ColumnParamToStr(QString& strColumnParam);
	int				currentSortColumn();
	_BOOLEAN		bCurrentSortAscendingdrm;
	_BOOLEAN		bCurrentSortAscendinganalog;
	int				iSortColumndrm;
	int				iSortColumnanalog;
	QString			strColumnParamdrm;
	QString			strColumnParamanalog;

	CDRMReceiver&	DRMReceiver;
#ifdef HAVE_LIBHAMLIB
	CRig&			Rig;
#endif
	CDRMSchedule	DRMSchedule;
	QIcon			greenCube;
	QIcon			redCube;
	QIcon			orangeCube;
	QIcon			pinkCube;
	QSignalMapper*	previewMapper;
	QActionGroup*	previewGroup;
	QSignalMapper*	showMapper;
	QActionGroup*	showGroup;
	QNetworkAccessManager *manager;
	QTimer			TimerList;
	QTimer			TimerUTCLabel;
	_BOOLEAN		bReInitOnFrequencyChange;

	QMutex			ListItemsMutex;

	QString			okMessage, badMessage;
	CDRMSchedule::ESchedMode eLastScheduleMode;

signals:
	void subscribeRig();
	void unsubscribeRig();
public slots:
	void OnSigStr(double);
	void OnTimerList();
	void OnTimerUTCLabel();
	void OnSMeterMenu(int iID);
	void OnSMeterMenu();
	void OnUrlFinished(QNetworkReply*);
	void OnShowStationsMenu(int iID);
	void OnShowPreviewMenu(int iID);
	void OnFreqCntNewValue(double dVal);
	void OnHeaderClicked(int c);
	void on_actionGetUpdate_triggered();
	void on_ListViewStations_itemSelectionChanged();
	void on_ComboBoxFilterTarget_activated(const QString&);
	void on_ComboBoxFilterCountry_activated(const QString&);
	void on_ComboBoxFilterLanguage_activated(const QString&);
};
#endif
