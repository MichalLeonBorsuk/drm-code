/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable, Volker Fischer, Andrea Russo
 *
 * Description:
 *  QT4 Model for managing schedule data
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

#ifndef _SCHEDULE_MODEL_H
#define _SCHEDULE_MODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QPixmap>
#include "../Parameter.h"


extern const char* DRM_SCHEDULE_UPDATE_URL;
extern const char* AM_SCHEDULE_UPDATE_URL;

/* Name for DRM and AM schedule file. If you change something here, make sure
   that you also change the strings and help texts!  */
extern const char* DRMSCHEDULE_INI_FILE_NAME;
extern const char* AMSCHEDULE_INI_FILE_NAME;

/* Time definitions for preview */
#define NUM_SECONDS_PREV_5MIN			300
#define NUM_SECONDS_PREV_15MIN			900
#define NUM_SECONDS_PREV_30MIN			1800

#define NUM_SECONDS_SOON_INACTIVE		600

/* String definitions for schedule days */
#define FLAG_STR_IRREGULAR_TRANSM		"0000000"
#define CHR_ACTIVE_DAY_MARKER			'1'

/* Classes ********************************************************************/
class CScheduleItem
{
public:

	enum State {IS_ACTIVE, IS_INACTIVE, IS_PREVIEW, IS_SOON_INACTIVE};

	CScheduleItem() : iStartHour(0), iStartMinute(0), iStopHour(0),
		iStopMinute(0), iFreq(0), strName(""),
		strTarget(""), strLanguage(""), strSite(""),
		strCountry(""), strDaysFlags(""), strDaysShow(""),
		rPower((_REAL) 0.0) { }

	int GetStartTimeNum() const {return iStartHour * 100 + iStartMinute;}
	int GetStopTimeNum() const {return iStopHour * 100 + iStopMinute;}
	void SetStartTimeNum(const int iStartTime)
	{
		/* Recover hours and minutes */
		iStartHour = iStartTime / 100;
		iStartMinute = iStartTime - iStartHour * 100;
	}
	void SetStopTimeNum(const int iStopTime)
	{
		/* Recover hours and minutes */
		iStopHour = iStopTime / 100;
		iStopMinute = iStopTime - iStopHour * 100;
	}

	void SetDaysFlagString(const string strNewDaysFlags);
	bool IsActive(time_t ltime, int iSecondsPreview) const;

	int		iStartHour;
	int		iStartMinute;
	int		iStopHour;
	int		iStopMinute;
	int		iFreq;
	string	strName;
	string	strTarget;
	string	strLanguage;
	string	strSite;
	string	strCountry;
	string	strDaysFlags;
	QString	strDaysShow;
	_REAL	rPower;
	State	state;
};


class CTxSchedule : public QAbstractTableModel
{
public:

	CTxSchedule();
	virtual ~CTxSchedule() {}

	int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
	int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

	void ReadIniFile(FILE* pFile);
	void ReadCSVFile(FILE* pFile);
	void clear() { ScheduleTable.clear(); }
	void load(const string&);
	void update();

	void SetSecondsPreview(int iSec) {iSecondsPreview = iSec;}
	int GetSecondsPreview() const {return iSecondsPreview;}

	QStringList			ListTargets;
	QStringList			ListCountries;
	QStringList			ListLanguages;

protected:

	vector<CScheduleItem>	ScheduleTable;

	/* Minutes for stations preview in seconds if zero then no active */
	int				iSecondsPreview;
	QPixmap			BitmCubeGreen;
	QPixmap			BitmCubeYellow;
	QPixmap			BitmCubeRed;
	QPixmap			BitmCubeOrange;
	QPixmap			BitmCubePink;
};

#endif
