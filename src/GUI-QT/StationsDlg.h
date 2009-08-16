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

#ifndef _STATIONS_DLG_H
#define _STATIONS_DLG_H

#include <QTimer>
#include <QNetworkAccessManager>
#include <QSortFilterProxyModel>
#include <qwt_thermo.h>
#include <qwt_counter.h>

#include "ScheduleModel.h"
#include "ui_StationsDlg.h"
#include "../util/Vector.h"
#include "../util/Settings.h"
#include "../ReceiverInterface.h"

/* Definitions ****************************************************************/

/* Define the timer interval of updating the UTC label */
#define GUI_TIMER		500 /* ms (0.5 second) */

/* Define the timer interval of updating s-meter */
#define GUI_TIMER_S_METER				300 /* ms (0.3 seconds) */

/* Define the timer interval of updating frequency */
#define GUI_TIMER_UPDATE_FREQUENCY		500 /* ms (0.5 seconds) */
#define GUI_TIMER_INTER_DIGIT			500 /* ms (0.5 seconds) */
#define GUI_TIME_TO_TUNE				2000 /* ms (2 seconds) */

/* File handle type */
#ifdef _WIN32
# define FILE_HANDLE					Qt::HANDLE
#else
# define FILE_HANDLE					int
#endif

/* Classes ********************************************************************/
class StationsDlg : public QDialog, public Ui_StationsDlg
{
	Q_OBJECT

public:

	StationsDlg(ReceiverInterface&, CSettings&, bool, QWidget* parent = 0,
		const char* name = 0, bool modal = false, Qt::WFlags f = 0);
	virtual ~StationsDlg();
	/* dummy assignment operator to help MSVC8 */
	StationsDlg& operator=(const StationsDlg&)
	{ throw "should not happen"; return *this;}

protected:

	void    LoadSettings(const CSettings&);
	void    SaveSettings(CSettings&);
	void    LoadSchedule(const string&);

	void    showEvent(QShowEvent* pEvent);
	void	hideEvent(QHideEvent* pEvent);
	void	AddWhatsThisHelp();
	void	EnableSMeter(const bool bStatus);

	ReceiverInterface&	Receiver;
	CSettings&		Settings;
	CTxSchedule	    Schedule;
	QTimer			TimerClock;
	QTimer			TimerSMeter;
	QTimer			TimerEditFrequency;
	QTimer			TimerMonitorFrequency;
	QTimer			TimerTuning;

	bool		    bTuningInProgress;
	bool			bReInitOnFrequencyChange;
	QString			ScheduleFile;

	QNetworkAccessManager* networkManager;
    QSortFilterProxyModel* proxyModel;

public slots:

	void OnFilterChanged(const QString&);
	void OnItemClicked(const QModelIndex&);
	void OnShowActive(int);
	void OnSelectPreview(int);

	void OnGetAnalogUpdate(bool);
	void OnGetDRMUpdate(bool);
	void OnUrlFinished(QNetworkReply*);

	void OnTimerClock();

	void OnTimerSMeter();

	void OnTimerEditFrequency();
	void OnTimerMonitorFrequency();
	void OnTimerTuning();
	void OnFreqCntNewValue(double dVal);
};

#endif
