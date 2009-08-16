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

#include "DialogUtil.h"
#include "../util/Settings.h"
#include "../Parameter.h"
#include <../ReceiverInterface.h>

#include "ui_LiveScheduleDlg.h"
#include <QTimer>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

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
	strSystem("") {}
	virtual ~CLiveScheduleItem() {}

	enum State {IS_ACTIVE, IS_INACTIVE, IS_PREVIEW, IS_SOON_INACTIVE};

	void			updateState(const time_t ltime, int iPreview);
	QString			ExtractDaysFlagString() const;
	QString			ExtractTime() const;

	string			strFreq;
	string			strTarget;
	string			strStation;
	uint32_t		iServiceID;
	string			strSystem;
	CAltFreqSched 	schedule;

	State			state;
	bool			InsideTargetArea;
};

class CDRMLiveSchedule: public QAbstractTableModel
{
public:
	CDRMLiveSchedule();

	virtual ~CDRMLiveSchedule() {}


	int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
	int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

	void LoadAFSInformation(
		const map <uint32_t,CServiceInformation>& ServiceInformation);

	void LoadServiceDefinition(const CServiceDefinition& service,
			const CServiceInformation& ServiceInformation);

	void update();

	void DecodeTargets(const vector<CAltFreqRegion> vecAltFreqRegions,
		string& strRegions , bool& bIntoTargetArea);

	void SetSecondsPreview(int iSec) {iSecondsPreview = iSec;}
	int GetSecondsPreview() {return iSecondsPreview;}

	void SetReceiverCoordinates(double latitude, double longitude);

	QString toHTML(const QString& StationName);

protected:

	vector<CLiveScheduleItem>	ScheduleTable;

	/* Minutes for stations preview in seconds if zero then no active */
	int			iSecondsPreview;

	/* receiver coordinates */
	double		dReceiverLatitude;
	double		dReceiverLongitude;

	QPixmap		BitmCubeGreen;
	QPixmap		BitmCubeGreenLittle;
	QPixmap		BitmCubeYellow;
	QPixmap		BitmCubeRed;
	QPixmap		BitmCubeOrange;
	QPixmap		BitmCubePink;
};

class LiveScheduleDlg : public QDialog, public Ui_LiveScheduleDlg
{
	Q_OBJECT

public:

	LiveScheduleDlg(ReceiverInterface&, CSettings& s,
		QWidget* parent = 0,
		const char* name = 0, bool modal = false, Qt::WFlags f = 0);
	virtual ~LiveScheduleDlg();
	/* dummy assignment operator to help MSVC8 */
	LiveScheduleDlg& operator=(const LiveScheduleDlg&)
	{ throw "should not happen"; return *this;}

	void					LoadSchedule();

	int						iCurrentSortColumn;
	bool				    bCurrentSortAscending;

protected:
	void					SetStationsView();
	void					AddWhatsThisHelp();
	void					showEvent(QShowEvent* pEvent);
	void					hideEvent(QHideEvent* pEvent);

	ReceiverInterface&		DRMReceiver;
	CSettings&				Settings;
	CDRMLiveSchedule		Schedule;
	QTimer					TimerList;
	QTimer					TimerUTCLabel;
	bool					bShowAll;

	QMutex					ListItemsMutex;
	QString					strCurrentSavePath;
    QSortFilterProxyModel* 	proxyModel;

public slots:
	void OnTimerList();
	void OnTimerUTCLabel();
	void OnSave();
	void OnCheckFreeze();
	void OnFilterChanged(const QString&);
	void OnItemClicked(const QModelIndex&);
	void OnShowActive(int);
	void OnSelectPreview(int);
};
