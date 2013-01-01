/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2005
 *
 * Author(s):
 *	Andrea Russo
 *
 * Description:
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

#ifndef __LiveScheduleDlg_H
#define LiveScheduleDlg_H

#include "../DrmReceiver.h"
#include "../util/Settings.h"
#include "DialogUtil.h"
#include <vector>
#include <QSignalMapper>
#include <QDialog>
#include <QTreeWidget>
#include "ui_LiveScheduleWindow.h"
#include <qpixmap.h>
#include <qradiobutton.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qmenubar.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qthread.h>


/* Definitions ****************************************************************/
/* Define the timer interval of updating the list view */
#define GUI_TIMER_LIST_VIEW_UPDATE		5000 /* ms (5 seconds) */

/* Define the timer interval of updating the UTC label */
#define GUI_TIMER_UTC_TIME_LABEL		1000 /* ms (1 second) */

#define COL_FREQ 0 /* frequency column */
#define COL_TARGET 4 /* target column */

/* Time definitions for preview */
#define NUM_SECONDS_PREV_5MIN			300
#define NUM_SECONDS_PREV_15MIN			900
#define NUM_SECONDS_PREV_30MIN			1800

#define NUM_SECONDS_SOON_INACTIVE		600

/* String definitions for schedule days */
#define CHR_ACTIVE_DAY_MARKER			'1'

/* Classes ********************************************************************/
class CLiveScheduleItem
{
public:
	CLiveScheduleItem() : strFreq(""), strTarget(""), iServiceID(SERV_ID_NOT_USED),
	strSystem(""), bInsideTargetArea(FALSE) {}

	_BOOLEAN IsActive(const time_t ltime);

	string		strFreq;
	string		strTarget;
	uint32_t	iServiceID;
	string		strSystem;
	_BOOLEAN	bInsideTargetArea;
	CAltFreqSched Schedule;
};

class CDRMLiveSchedule
{
public:
	CDRMLiveSchedule():StationsTable(),iSecondsPreview(0),
	dReceiverLatitude(0),dReceiverLongitude(0)
	{}

	virtual ~CDRMLiveSchedule() {}

	enum StationState {IS_ACTIVE, IS_INACTIVE, IS_PREVIEW, IS_SOON_INACTIVE};
	int GetStationNumber() {return StationsTable.size();}
	CLiveScheduleItem& GetItem(const int iPos) {return StationsTable[iPos];}
	StationState CheckState(const int iPos);

	void LoadAFSInformations(const CAltFreqSign& AltFreqSign);

	void LoadServiceDefinition(const CServiceDefinition& service,
			const CAltFreqSign& AltFreqSign, const uint32_t iServiceID=SERV_ID_NOT_USED);

	void DecodeTargets(const vector<CAltFreqRegion> vecAltFreqRegions,
		string& strRegions , _BOOLEAN& bIntoTargetArea);

	void SetSecondsPreview(int iSec) {iSecondsPreview = iSec;}
	int GetSecondsPreview() {return iSecondsPreview;}

	void SetReceiverCoordinates(double latitude, double longitude);

protected:
	_BOOLEAN IsActive(const int iPos, const time_t ltime);

	vector<CLiveScheduleItem>	StationsTable;

	/* Minutes for stations preview in seconds if zero then no active */
	int			iSecondsPreview;

	/* receiver coordinates */
	double		dReceiverLatitude;
	double		dReceiverLongitude;
};

class MyListLiveViewItem : public QTreeWidgetItem
{
public:
	MyListLiveViewItem(QTreeWidget* parent, QString s1, QString s2 = QString::null,
		QString s3 = QString::null, QString s4 = QString::null,
		QString s5 = QString::null, QString s6 = QString::null,
		QString s7 = QString::null, QString s8 = QString::null) :	
	QTreeWidgetItem(parent, QStringList() << s1 << s2 << s3 << s4 << s5 << s6 << s7 << s8)
	{}

	/* Custom "key()" function for correct sorting behaviour */
	virtual QString key(int column, bool ascending) const;
	void setPixmap(int col, QPixmap p) { setIcon(col, p); }
};

class CLiveScheduleDlgBase : public QDialog, public Ui_LiveScheduleWindow
{
public:
	CLiveScheduleDlgBase(QWidget* parent, const char*, bool, Qt::WFlags):
		QDialog(parent) {setWindowFlags(Qt::Window);}
	virtual ~CLiveScheduleDlgBase() {}
};

class LiveScheduleDlg : public CLiveScheduleDlgBase
{
	Q_OBJECT

public:
	LiveScheduleDlg(CDRMReceiver&, CSettings&,
		QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);
	virtual ~LiveScheduleDlg();

	void LoadSchedule();
	void SaveSettings(CSettings&);

protected:
	int				iCurrentSortColumn;
	_BOOLEAN		bCurrentSortAscending;
	void			LoadSettings(const CSettings&);
	void			SetStationsView();
	void			AddWhatsThisHelp();
	void			SetUTCTimeLabel();
	void			showEvent(QShowEvent* pEvent);
	void			hideEvent(QHideEvent* pEvent);
	QString			ExtractDaysFlagString(const int iDayCode);
	QString			ExtractTime(const CAltFreqSched& schedule);
	_BOOLEAN		showAll();
	int				currentSortColumn();

	CDRMReceiver&		DRMReceiver;
	CSettings&			Settings;
	CDRMLiveSchedule	DRMSchedule;
	QTimer			TimerList;
	QTimer			TimerUTCLabel;
	QIcon			smallGreenCube;
	QIcon			greenCube;
	QIcon			redCube;
	QIcon			orangeCube;
	QIcon			pinkCube;
	QSignalMapper*	previewMapper;
	QActionGroup*	previewGroup;
	QSignalMapper*	showMapper;
	QActionGroup*	showGroup;

	vector<MyListLiveViewItem*>	vecpListItems;
	QMutex			ListItemsMutex;
	QString			strCurrentSavePath;
	int				iColStationID;
	int				iWidthColStationID;
	CEventFilter	ef;

public slots:
	void OnTimerList();
	void OnTimerUTCLabel();
	void OnShowStationsMenu(int iID);
	void OnShowPreviewMenu(int iID);
	void OnHeaderClicked(int c);
	void OnSave();
	void OnCheckFreeze();
};

#endif
