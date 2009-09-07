/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	ETSI DAB/DRM Electronic Programme Guide Viewer
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

#ifndef _EPGDLG_H
#define _EPGDLG_H

#include "../datadecoding/epg/EPG.h"
#include "../util/Settings.h"
#include "../ReceiverInterface.h"

#include "ui_EPGDlg.h"
#include <QTimer>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

/* Definitions ****************************************************************/
#define COL_NAME	1

/* Define the timer interval of updating */
#define GUI_TIMER_EPG_UPDATE		1000 /* ms (1 second) */

/* list view columns */
#define COL_START		0
#define COL_NAME		1
#define	COL_GENRE		2
#define	COL_DESCRIPTION	3
#define COL_DURATION	4

class EPGModel : public EPG, public QAbstractTableModel
{
public:
    EPGModel(CParameter& p);
    virtual ~EPGModel() {}
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role) const;

    void select (const uint32_t, const QDate&);
    bool IsActive(const QString& start, const QString& duration, const tm& now);

    QPixmap BitmCubeGreen;
};

/* Classes ********************************************************************/
class EPGDlg : public QDialog, public Ui_EPGDlg
{
	Q_OBJECT

public:

	EPGDlg(ReceiverInterface&, CSettings&, QWidget* parent = 0, Qt::WFlags f = 0);
	virtual ~EPGDlg();
	/* dummy assignment operator to help MSVC8 */
	EPGDlg& operator=(const EPGDlg&)
	{ throw "should not happen"; return *this;}

    void select();

protected:

    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent* pEvent);
    void updateXML(const QDate& date, uint32_t sid, bool advanced);

    QDate           date;
    EPGModel        epg;
	ReceiverInterface&	DRMReceiver;
	CSettings&		Settings;
	QTimer			Timer;
	uint32_t        currentSID;
    QSortFilterProxyModel* 	proxyModel;

public slots:
    void setDate(const QDate&);
    void selectChannel(const QString &);
    void OnPrev();
    void OnNext();
    void OnTimer();
    void OnClearCache();
    void OnSave();
};

#endif
