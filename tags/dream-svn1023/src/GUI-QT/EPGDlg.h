/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Julian Cable
 *
 * Description:
 *  ETSI DAB/DRM Electronic Programme Guide Viewer
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

#include "ui_EPGDlgbase.h"
#include "CWindow.h"
#include <../Parameter.h>
#include <QTimer>
#include <map>


/* Definitions ****************************************************************/
#define COL_NAME    1

/* Define the timer interval of updating */
#define GUI_TIMER_EPG_UPDATE        1000 /* ms (1 second) */

/* list view columns */
#define COL_START       0
#define COL_NAME        1
#define COL_GENRE       2
#define COL_DESCRIPTION 3
#define COL_DURATION    4


/* Classes ********************************************************************/

class EPG;
class QDomDocument;

class EPGDlg : public CWindow
{
    Q_OBJECT

public:
    EPGDlg(CSettings&, QWidget* parent = 0);
    virtual ~EPGDlg();
    void setServiceInformation(const CServiceInformation&, uint32_t);
    void setDecoder(EPG*);

protected:
    virtual void eventShow(QShowEvent*);
    virtual void eventHide(QHideEvent*);
    void setActive(QTreeWidgetItem*);
    bool isActive(QTreeWidgetItem*);

    QString getFileName(const QDate& date, uint32_t sid, bool bAdvanced);
    QString getFileName_etsi(const QDate& date, uint32_t sid, bool bAdvanced);
    QDomDocument* getFile (const QString&);
    QDomDocument* getFile (const QDate& date, uint32_t sid, bool bAdvanced);
    void select();

    Ui::CEPGDlgbase       *ui;
    EPG*                  pEpg;
    QTimer                Timer;
    QIcon                 greenCube;
    QTreeWidgetItem*      next;
    QDateTime             drmTime;

signals:
    void NowNext(QString);

private slots:
    void on_channel_activated(const QString&);
    void on_dateEdit_dateChanged(const QDate&);
    void on_DRMTimeChanged(QDateTime);
    void OnTimer();
};

#endif
