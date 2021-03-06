/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Volker Fischer
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
#include <QTimer>
#include "ui_StationsDlgbase.h"

#include "DialogUtil.h"
#include "CWindow.h"
#include "Schedule.h"
#include "../util-QT/scheduleloader.h"

/* Definitions ****************************************************************/

class RigDlg;

namespace Ui {
class StationsDlgbase;
}

class StationsDlg : public CWindow
{
    Q_OBJECT

public:
#ifdef HAVE_LIBHAMLIB
    StationsDlg(CSettings&, CRig&, QMap<QWidget*,QString>&);
#else
    StationsDlg(CSettings&, QMap<QWidget*,QString>&);
#endif
    virtual ~StationsDlg();
public slots:
    void SetFrequency(int);
    void OnSwitchMode(int);
    ERecMode mode() {
        return eRecMode;
    }

private:
#ifdef HAVE_LIBHAMLIB
    CRig&           Rig;
    RigDlg          *pRigDlg;
#endif
    Ui::StationsDlgbase* ui;
protected:
    struct Params {
        bool        bCurrentSortAscending;
        int         iSortColumn;
        QString     strColumnParam;
        QString     targetFilter;
        QString     languageFilter;
        QString     countryFilter;
        QString     url;
        QString     filename;
    };

    virtual void    eventClose(QCloseEvent* pEvent);
    virtual void    eventHide(QHideEvent* pEvent);
    virtual void    eventShow(QShowEvent* pEvent);
    void            LoadSettings();
    void            SaveSettings();
    void            LoadSchedule();
    void            LoadScheduleView();
    void            UpdateTransmissionStatus();
    void            AddWhatsThisHelp();
    void            EnableSMeter();
    void            DisableSMeter();
    bool            showAll();
    bool            GetSortAscending();
    void            SetSortAscending(bool);
    int             currentSortColumn();
    Params          params[RM_NONE+1];

    CSchedule       schedule;
    ScheduleLoader  scheduleLoader;
    QIcon           greenCube;
    QIcon           redCube;
    QIcon           orangeCube;
    QIcon           pinkCube;
    QSignalMapper*  previewMapper;
    QActionGroup*   previewGroup;
    QSignalMapper*  showMapper;
    QActionGroup*   showGroup;
    QTimer          Timer;

    QMutex          ListItemsMutex;

    QString         okMessage, badMessage;
    ERecMode        eRecMode;
    SMeter*         inputLevel;

signals:
    void subscribeRig();
    void unsubscribeRig();
    void frequencyChanged(int);

private slots:
    void setFine(bool on);
    void OnSigStr(double);
    void OnTimer();
    void OnUpdate();
    void OnFileReady();
    void OnShowStationsMenu(int iID);
    void OnShowPreviewMenu(int iID);
    void OnFreqCntNewValue(double dVal);
    void OnHeaderClicked(int c);
    void on_actionGetUpdate_triggered();
    void on_ListViewStations_itemSelectionChanged();
    void on_ComboBoxFilterTarget_activated(const QString&);
    void on_ComboBoxFilterCountry_activated(const QString&);
    void on_ComboBoxFilterLanguage_activated(const QString&);
    void on_actionEnable_S_Meter_triggered();
    void on_actionChooseRig_triggered();
};
#endif
