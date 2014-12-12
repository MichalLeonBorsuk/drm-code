/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2001-2014, 2001-2014
 *
 * Author(s):
 *   Julian Cable, David Flamand (rewrite)
 *
 * Description: MOT Broadcast Website Viewer
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

#ifndef _BWSVIEWER_H
#define _BWSVIEWER_H

#include "ui_BWSViewer.h"
#include "DialogUtil.h"
#include "CWindow.h"
#include "bwsviewerwidget.h"
#include <QMutex>

class CMOTObject;
class CDRMReceiver;
class CDataDecoder;

class BWSViewer : public CWindow, public Ui_BWSViewer
{
    Q_OBJECT

public:
    BWSViewer(CDRMReceiver&, CSettings&, QWidget* parent = 0);
    virtual ~BWSViewer();

protected:
    CNetworkAccessManager nam;
    QTimer          Timer;
    CDRMReceiver&   receiver;
    CDataDecoder*   decoder;
    CWebsiteCache   cache;
    bool            bHomeSet;
    bool            bPageLoading;
    bool            bSaveFileToDisk;
    bool            bRestrictedProfile;
    bool            bAllowExternalContent;
    bool            bClearCacheOnNewService;
    bool            bDirectoryIndexChanged;
    unsigned int    iLastAwaitingOjects;
    CCounter        waitobjs;
    const QString   strCacheHost;
    uint32_t        iLastServiceID;
    uint32_t        iCurrentDataServiceID;
    bool            bLastServiceValid;
    uint32_t        iLastValidServiceID;
    QString         strLastLabel;
    QString         strDataDir;

    bool Changed();
    void SaveMOTObject(const QString& strObjName, const CMOTObject& obj);
    bool RemoveDir(const QString &dirName, int level = 0);
    QString savePath();
    void UpdateButtons();
    void UpdateStatus();
    void UpdateWindowTitle(const uint32_t iServiceID, const bool bServiceValid, QString strLabel);
    QString ObjectStr(unsigned int count);
    void GetServiceParams(uint32_t* iServiceID=NULL, bool* bServiceValid=NULL, QString* strLabel=NULL, ETypeRxStatus* eStatus=NULL);
    virtual void eventShow(QShowEvent*);
    virtual void eventHide(QHideEvent*);

public slots:
    void OnTimer();
    void OnClearCache();
    void OnHome();
    void OnStopRefresh();
    void OnBack();
    void OnForward();
    void OnSetProfile(bool);
    void OnSaveFileToDisk(bool);
    void OnAllowExternalContent(bool);
    void OnClearCacheOnNewService(bool);
    void OnWebViewLoadStarted();
    void OnWebViewLoadFinished(bool ok);
    void OnWebViewTitleChanged(const QString& title);
    void Update();
};

#endif

