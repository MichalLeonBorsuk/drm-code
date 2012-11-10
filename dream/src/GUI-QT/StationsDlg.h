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

#include <qpixmap.h>
#include <qradiobutton.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qdir.h>
#include <qmenubar.h>
#include <qlayout.h>
#include <qthread.h>
#include <qaction.h>
#include <qlabel.h>
#include <qfileinfo.h>
#include <qdatetime.h>
#include <qwt_thermo.h>
#include <qwt_counter.h>
#if QT_VERSION < 0x040000
# include <qpopupmenu.h>
# include <qurloperator.h>
# include <qlistview.h>
# include "StationsDlgbase.h"
typedef QNetworkOperation QNetworkReply; // needed to keep moc happy
#else
# include <QActionGroup>
# include <QSignalMapper>
# include <QNetworkAccessManager>
# include <QButtonGroup>
# include <QDialog>
# include "ui_StationsDlgbase.h"
#endif

#include "DialogUtil.h"
#include "../DrmReceiver.h"
#include "../util/Vector.h"
#include "../util/Settings.h"


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

/* Classes ********************************************************************/
#if QT_VERSION < 0x040000
class MyListViewItem : public QListViewItem
{
public:
	/* If you want to add another columns, change also MAX_COLUMN_NUMBER in
	   Settings.h! */
	MyListViewItem(QListView* parent, QString s1, QString s2 = QString::null,
		QString s3 = QString::null, QString s4 = QString::null,
		QString s5 = QString::null, QString s6 = QString::null,
		QString s7 = QString::null, QString s8 = QString::null) :
		QListViewItem(parent, s1, s2, s3, s4, s5, s6, s7, s8) {}

	/* Custom "key()" function for correct sorting behaviour */
	virtual QString key(int column, bool ascending) const;
};
#else
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
		if (col == 2) // integer frequency
			return text( col ).toInt() < rhs.text( col ).toInt();
		else if (col == 4) // real power
			return text( col ).toDouble() < rhs.text( col ).toDouble();
		else
			return text( col ).toLower() < rhs.text( col ).toLower();
	}
};
#endif
class CStationsItem
{
public:
	CStationsItem() : iStartHour(0), iStartMinute(0), iStopHour(0),
		iStopMinute(0), iFreq(0), strName(""),
		strTarget(""), strLanguage(""), strSite(""),
		strCountry(""), strDaysFlags(""), strDaysShow(""),
		rPower((_REAL) 0.0) { }

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
#if QT_VERSION < 0x040000
	QListViewItem* item;
#endif
};


class CDRMSchedule
{
public:
	CDRMSchedule();
	virtual ~CDRMSchedule() {}

	enum ESchedMode {SM_DRM, SM_ANALOG};
	enum StationState {IS_ACTIVE, IS_INACTIVE, IS_PREVIEW, IS_SOON_INACTIVE};

	void ReadINIFile(FILE* pFile);
	void ReadCSVFile(FILE* pFile);
	ESchedMode GetSchedMode() {return eSchedMode;}
	void SetSchedMode(const ESchedMode);

	int GetStationNumber() {return StationsTable.size();}
	CStationsItem& GetItem(const int iPos) {return StationsTable[iPos];}
	StationState CheckState(const int iPos);
	bool CheckFilter(const int iPos);

	void SetSecondsPreview(int iSec);
	int GetSecondsPreview();
	void UpdateStringListForFilter(const CStationsItem& StationsItem);
	void LoadSchedule();

	QStringList			ListTargets;
	QStringList			ListCountries;
	QStringList			ListLanguages;

	QString countryFilterdrm, targetFilterdrm, languageFilterdrm;
	QString countryFilteranalog, targetFilteranalog, languageFilteranalog;
	QString schedFileName;
	QUrl *qurldrm, *qurlanalog;

protected:
	_BOOLEAN IsActive(const int iPos, const time_t ltime);
	void			SetAnalogUrl();

	vector<CStationsItem>	StationsTable;
	ESchedMode		eSchedMode;

	/* Minutes for stations preview in seconds if zero then only show active */
	int iSecondsPreviewdrm;
	int iSecondsPreviewanalog;
};



class RemoteMenu;
class QSocket;

#if QT_VERSION >= 0x040000
class CStationsDlgBase : public QDialog, public Ui_StationsDlgbase
{
public:
	CStationsDlgBase(QWidget* parent = 0, const char* name = 0,
		bool modal = FALSE, Qt::WFlags f = 0):
		QDialog(parent) {(void)name;(void)modal;(void)f; setWindowFlags(Qt::Window);}
	virtual ~CStationsDlgBase() {}
};
#endif
class StationsDlg : public CStationsDlgBase
{
	Q_OBJECT

public:

	StationsDlg(CDRMReceiver&, CRig&, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);
	virtual ~StationsDlg();

	void LoadSettings(const CSettings&);
	void SaveSettings(CSettings&);

protected:
	void			LoadSchedule();
	void			LoadFilters();
	void			SetFrequencyFromGUI(int);
	void			SetStationsView();
	void			ClearStationsView();
	void			showEvent(QShowEvent* pEvent);
	void			hideEvent(QHideEvent* pEvent);
	void			AddWhatsThisHelp();
	void			EnableSMeter();
	void			DisableSMeter();
	void			AddUpdateDateTime();
	_BOOLEAN		showAll();
	_BOOLEAN		GetSortAscending();
	void			SetSortAscending(_BOOLEAN b);
	int			currentSortColumn();
	_BOOLEAN		bCurrentSortAscendingdrm;
	_BOOLEAN		bCurrentSortAscendinganalog;
	int			iSortColumndrm;
	int			iSortColumnanalog;

	CDRMReceiver&		DRMReceiver;
	CDRMSchedule		DRMSchedule;
#if QT_VERSION < 0x040000
	void			setupUi(QObject*);
	QPixmap			BitmCubeGreen;
	QPixmap			BitmCubeYellow;
	QPixmap			BitmCubeRed;
	QPixmap			BitmCubeOrange;
	QPixmap			BitmCubePink;
	QPopupMenu*		pViewMenu;
	QPopupMenu*		pPreviewMenu;
	QPopupMenu*		pUpdateMenu;
	vector<MyListViewItem*>	vecpListItems;
	QUrlOperator	UrlUpdateSchedule;
	QFile *schedFile;
	QSocket *httpSocket;
	bool httpHeader;
#else
	QIcon			greenCube;
	QIcon			redCube;
	QIcon			orangeCube;
	QIcon			pinkCube;
	QSignalMapper* previewMapper;
	QActionGroup* previewGroup;
	QSignalMapper* showMapper;
	QActionGroup* showGroup;
	QNetworkAccessManager *manager;
#endif
	QTimer			TimerList;
	QTimer			TimerUTCLabel;
	_BOOLEAN		bReInitOnFrequencyChange;

	QMutex			ListItemsMutex;

	RemoteMenu*		pRemoteMenu;
	QString		okMessage, badMessage;
	CEventFilter	ef;

signals:
	void subscribeRig();
	void unsubscribeRig();
public slots:
	void OnSigStr(double);
	void OnTimerList();
	void OnTimerUTCLabel();
	void OnSMeterMenu(int iID);
	void OnSMeterMenu();
#if QT_VERSION < 0x040000
	void httpConnected();
	void httpDisconnected();
	void httpRead();
	void httpError(int);
	void OnListItemClicked(QListViewItem* item);
	void OnUrlFinished(QNetworkOperation* pNetwOp);
#else
	void OnUrlFinished(QNetworkReply*);
#endif
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
