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

#include "../util-QT/EPG.h"
#include <map>
#include <QDialog>
#include <QIcon>
#include <QTimer>

namespace Ui {
    class EPGDialog;
}
class QTreeWidgetItem;

/* Classes ********************************************************************/

class EPGDlg : public QDialog
{
    Q_OBJECT

public:
    explicit EPGDlg(string d, map<uint32_t,CServiceInformation>& si, QWidget* parent = 0);
    ~EPGDlg();

signals:
    void NowNext(QString);

public slots:
    void on_channel_activated(const QString&);
    void on_dateEdit_dateChanged(const QDate&);
    void OnTimer();

private:
    Ui::EPGDialog *ui;
    EPG epg;
    map<QString,uint32_t> sids;
    QIcon		greenCube;
    QTreeWidgetItem*	next;
    QTimer timer;
    void showEvent(QShowEvent*);
    void hideEvent(QHideEvent*);
    void setActive(QTreeWidgetItem*);
    bool isActive(QTreeWidgetItem*);
    void select();
    QString getFileName(const QDate& date, uint32_t sid, bool bAdvanced);
    QString getFileName_etsi(const QDate& date, uint32_t sid, bool bAdvanced);
    QDomDocument* getFile (const QString&);
    QDomDocument* getFile (const QDate& date, uint32_t sid, bool bAdvanced);
};

#endif
