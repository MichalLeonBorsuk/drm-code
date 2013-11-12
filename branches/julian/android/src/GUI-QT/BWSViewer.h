/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009, 2012
 *
 * Author(s):
 *	 Julian Cable, David Flamand (rewrite)
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

#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <string>
#include <../util/Vector.h>
#include <../util/Utilities.h>
#include <../Parameter.h>

namespace Ui {
class BWSViewerWidget;
}

class CMOTObject;
class CDRMReceiver;
class CMOTDABDec;

class CWebsiteObject
{
public:
    CWebsiteObject(unsigned int id, const QString& strContentType, CVector<_BYTE>& vecbData)
        : id(id), strContentType(strContentType),
        data(QByteArray((const char*)&vecbData[0], vecbData.Size())) {}
    virtual ~CWebsiteObject() {}
    unsigned int    id;
    QString         strContentType;
    QByteArray      data;
};


class CWebsiteCache : public QObject
{
    Q_OBJECT

public:
    CWebsiteCache() : id_counter(0), total_size(0) {}
    virtual ~CWebsiteCache() {}
    void GetObjectCountAndSize(unsigned int& count, unsigned int& size);
    void ClearAll();
    void AddObject(QString strObjName, QString strContentType, CVector<_BYTE>& vecbData);
    int GetObjectContentType(const QString& strObjName, QString& strContentType);
    int GetObjectSize(const QString& strObjName, const unsigned int id);
    int CopyObjectData(const QString& strObjName, const unsigned int id, char *buffer, int maxsize, int offset);
    bool SetDirectoryIndex(const QString strNewDirectoryIndex);
    QString GetDirectoryIndex();

protected:
    CWebsiteObject* FindObject(const QString& strObjName);
    std::map<QString,CWebsiteObject> objects;
    QMutex          mutex;
    QString         strDirectoryIndex;
    unsigned int    id_counter;
    unsigned int    total_size;

signals:
	void ObjectAdded(QString strObjName);
};


class CNetworkReplyCache : public QNetworkReply
{
    Q_OBJECT

public:
    CNetworkReplyCache(QNetworkAccessManager::Operation op,
        const QNetworkRequest& req, CWebsiteCache& cache, CCounter& waitobjs);
    virtual ~CNetworkReplyCache();
    void abort() { id = 0; }
    qint64 bytesAvailable() const;
    bool isSequential() const { return true; }

protected:
    qint64 readData(char * data, qint64 maxSize);
    void customEvent(QEvent* event);
    CWebsiteCache&      cache;
    CCounter&           waitobjs;
    QString             path;
    int                 readOffset;
    bool                emitted;
    unsigned int        id;

public slots:
    void CheckObject(QString strObjName);
};


class CNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    CNetworkAccessManager(QObject* parent, CWebsiteCache& cache,
        CCounter& waitobjs, const bool& bAllowExternalContent, const QString& strCacheHost)
        : QNetworkAccessManager(parent), cache(cache), waitobjs(waitobjs),
        bAllowExternalContent(bAllowExternalContent), strCacheHost(strCacheHost) {};
    virtual ~CNetworkAccessManager() {};

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest & req, QIODevice *);
    CWebsiteCache&  cache;
    CCounter&       waitobjs;
    const bool&     bAllowExternalContent;
    const QString&  strCacheHost;
};


class BWSViewer: public QObject
{
    Q_OBJECT

public:
    explicit BWSViewer(CMOTDABDec*, CSettings&, QWidget*);
    ~BWSViewer();
    void eventShow();
    void eventHide();
public slots:
    void setServiceInfo(CService);
    void setRxStatus(int, int, ETypeRxStatus);

private:
    QWidget*        container;
    Ui::BWSViewerWidget *ui;
    int short_id, stream_id, packet_id;
    uint32_t        serviceID;
    QString         strServiceLabel;
    CSettings&      settings;
    CNetworkAccessManager nam;
    QTimer          Timer;
    CMOTDABDec*     decoder;
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
    QString         strCurrentSavePath;

    bool Changed();
    void SaveMOTObject(const QString& strObjName, const CMOTObject& obj);
    bool RemoveDir(const QString &dirName, int level = 0);
    QString ObjectStr(unsigned int count);

public slots:
    void setSavePath(const QString& strSavePath);
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
