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
 * container program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * container program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * container program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "BWSViewer.h"
#include "../DrmReceiver.h"
#include "../util-QT/Util.h"
#include "../datadecoding/DataDecoder.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QWebHistory>


#define CACHE_HOST          "127.0.0.1" /* Not an actual server, MUST be set to "127.0.0.1" */

#define ICON_REFRESH        ":/icons/Refresh.png"
#define ICON_STOP           ":/icons/Stop.png"

#undef ENABLE_HACK
#define ENABLE_HACK /* Do we really need these hack unless for vtc trial sample? */

#include "ui_bwsviewerwidget.h"

BWSViewer::BWSViewer(CMOTDABDec* dec, CSettings& Settings, QWidget* cont):
    container(cont),
    ui(new Ui::BWSViewerWidget),
    short_id(),stream_id(0),packet_id(0),serviceID(0),strServiceLabel(),
    settings(Settings),
    nam(container, cache, waitobjs, bAllowExternalContent, strCacheHost),
    decoder(dec), bHomeSet(false), bPageLoading(false),
    bSaveFileToDisk(false), bRestrictedProfile(false), bAllowExternalContent(true),
    bClearCacheOnNewService(true), bDirectoryIndexChanged(false),
    iLastAwaitingOjects(0), strCacheHost(CACHE_HOST)
{
    ui->setupUi(container);

    /* Setup ui->webView */
    ui->webView->page()->setNetworkAccessManager(&nam);
    ui->webView->pageAction(QWebPage::OpenLinkInNewWindow)->setVisible(false);
    ui->webView->pageAction(QWebPage::DownloadLinkToDisk)->setVisible(false);
    ui->webView->pageAction(QWebPage::OpenImageInNewWindow)->setVisible(false);
    ui->webView->pageAction(QWebPage::DownloadImageToDisk)->setVisible(false);

    /* Update time for color LED */
    ui->LEDStatus->SetUpdateTime(1000);

    /* Update various ui->buttons and ui->Labels */
    ui->buttonClearCache->setEnabled(false);
    //actionClear_Cache->setEnabled(false);
    ui->labelTitle->setText("");
    Update();

    /* container->connect controls */
    container->connect(ui->buttonStepBack, SIGNAL(clicked()), this, SLOT(OnBack()));
    container->connect(ui->buttonStepForward, SIGNAL(clicked()), this, SLOT(OnForward()));
    container->connect(ui->buttonHome, SIGNAL(clicked()), this, SLOT(OnHome()));
    container->connect(ui->buttonStopRefresh, SIGNAL(clicked()), this, SLOT(OnStopRefresh()));
    container->connect(ui->buttonClearCache, SIGNAL(clicked()), this, SLOT(OnClearCache()));
    container->connect(ui->webView, SIGNAL(loadStarted()), this, SLOT(OnWebViewLoadStarted()));
    container->connect(ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(OnWebViewLoadFinished(bool)));
    container->connect(ui->webView, SIGNAL(titleChanged(const QString &)), this, SLOT(OnWebViewTitleChanged(const QString &)));
    container->connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));
}

BWSViewer::~BWSViewer()
{
    delete ui;
}

QString BWSViewer::ObjectStr(unsigned int count)
{
    return QString(count > 1 ? tr("objects") : tr("object"));
}

void BWSViewer::Update()
{
    unsigned int count, size;
    cache.GetObjectCountAndSize(count, size);
    if (count == 0)
        ui->labelStatus->setText("");
    else
    {
        QString text(tr("%1 %2 cached, %3 kB"));
        text = text.arg(count).arg(ObjectStr(count)).arg((size+999) / 1000);
        iLastAwaitingOjects = waitobjs;
        if (iLastAwaitingOjects)
            text += QObject::tr("  |  %1 %2 pending").arg(iLastAwaitingOjects).arg(ObjectStr(iLastAwaitingOjects));
        if (bAllowExternalContent && ui->webView->url().isValid() && ui->webView->url().host() != strCacheHost)
            text += "  |  " + ui->webView->url().toString();
        ui->labelStatus->setText(text);
    }
    ui->buttonStepBack->setEnabled(ui->webView->history()->canGoBack());
    ui->buttonStepForward->setEnabled(ui->webView->history()->canGoForward());
    ui->buttonHome->setEnabled(bHomeSet);
    ui->buttonStopRefresh->setEnabled(bHomeSet);
    ui->buttonStopRefresh->setIcon(QIcon(bPageLoading ? ICON_STOP : ICON_REFRESH));
}

void BWSViewer::OnHome()
{
    ui->webView->load("http://" + strCacheHost);
}

void BWSViewer::OnStopRefresh()
{
    if (bPageLoading)
        ui->webView->stop();
    else
    {
        if (ui->webView->url().isEmpty())
            OnHome();
        else
            ui->webView->reload();
    }
}

void BWSViewer::OnBack()
{
    ui->webView->history()->back();
}

void BWSViewer::OnForward()
{
    ui->webView->history()->forward();
}

void BWSViewer::OnClearCache()
{
    ui->webView->setHtml("");
    ui->webView->history()->clear();
    cache.ClearAll();
    bHomeSet = false;
    bPageLoading = false;
    bDirectoryIndexChanged = false;
    ui->buttonClearCache->setEnabled(false);
    //actionClear_Cache->setEnabled(false);
    Update();
}

void BWSViewer::OnWebViewLoadStarted()
{
    bPageLoading = true;
    QTimer::singleShot(20, container, SLOT(Update()));
}

void BWSViewer::OnWebViewLoadFinished(bool ok)
{
    (void)ok;
    bPageLoading = false;
    QTimer::singleShot(20, container, SLOT(Update()));
}

void BWSViewer::OnWebViewTitleChanged(const QString& title)
{
    ui->labelTitle->setText("<b>" + title + "</b>");
}

void BWSViewer::OnAllowExternalContent(bool isChecked)
{
    bAllowExternalContent = isChecked;
}

void BWSViewer::OnSetProfile(bool isChecked)
{
    bRestrictedProfile = isChecked;
}

void BWSViewer::OnSaveFileToDisk(bool isChecked)
{
    bSaveFileToDisk = isChecked;
}

void BWSViewer::OnClearCacheOnNewService(bool isChecked)
{
    bClearCacheOnNewService = isChecked;
}

void BWSViewer::eventShow()
{
    bAllowExternalContent = settings.Get("BWS", "allowexternalcontent", bAllowExternalContent);
    //actionAllow_External_Content->setChecked(bAllowExternalContent);

    bSaveFileToDisk = settings.Get("BWS", "savefiletodisk", bSaveFileToDisk);
    //actionSave_File_to_Disk->setChecked(bSaveFileToDisk);

    bRestrictedProfile = settings.Get("BWS", "restrictedprofile", bRestrictedProfile);
    //actionRestricted_Profile_Only->setChecked(bRestrictedProfile);

    bClearCacheOnNewService = settings.Get("BWS", "clearcacheonnewservice", bClearCacheOnNewService);
    //actionClear_Cache_on_New_Service->setChecked(bClearCacheOnNewService);

    QString strLabel = strServiceLabel;

    if (strLabel != "")
        strLabel += " - ";

    /* Service ID (plot number in hexadecimal format) */
    QString strServiceID = QString("ID: %1").arg(serviceID, 0, 16).toUpper();

    /* Update window title */
    QString strTitle(tr("MOT Broadcast Website"));
    /* Add the description on the title of the dialog */
    if (strLabel != "" || strServiceID != "")
        strTitle += " [" + strLabel + strServiceID + "]";
    container->setWindowTitle(strTitle);

    /* Update window content */
    OnTimer();

    /* Activate real-time timer when window is shown */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void BWSViewer::eventHide()
{
    /* Deactivate real-time timer so that it does not get new pictures */
    Timer.stop();

    settings.Put("BWS", "savefiletodisk", bSaveFileToDisk);

    settings.Put("BWS", "restrictedprofile", bRestrictedProfile);

    settings.Put("BWS", "allowexternalcontent", bAllowExternalContent);

    settings.Put("BWS", "clearcacheonnewservice", bClearCacheOnNewService);
}

bool BWSViewer::Changed()
{
    bool bChanged = false;
    if (decoder != NULL)
    {
        CMOTObject obj;

        /* Poll the data decoder module for new object */
        while (decoder->NewObjectAvailable() == TRUE)
        {
            decoder->GetNextObject(obj);
            /* Get the current directory */
            CMOTDirectory MOTDir;
            decoder->GetDirectory(MOTDir);
            /* ETSI TS 101 498-1 Section 5.5.1 */

            /* Checks if the DirectoryIndex has values */
            if (MOTDir.DirectoryIndex.size() > 0)
            {
                QString strNewDirectoryIndex;
                /* TODO proper profile handling */
                if(MOTDir.DirectoryIndex.find(UNRESTRICTED_PC_PROFILE) != MOTDir.DirectoryIndex.end())
                    strNewDirectoryIndex =
                        MOTDir.DirectoryIndex[UNRESTRICTED_PC_PROFILE].c_str();
                else if(MOTDir.DirectoryIndex.find(BASIC_PROFILE) != MOTDir.DirectoryIndex.end())
                    strNewDirectoryIndex =
                        MOTDir.DirectoryIndex[BASIC_PROFILE].c_str();
#ifdef ENABLE_HACK
                if (strNewDirectoryIndex == "not_here.html") /* Hack needed for vtc trial sample */
                    strNewDirectoryIndex = "index.html";
#endif
                if (!strNewDirectoryIndex.isNull())
                    bDirectoryIndexChanged |= cache.SetDirectoryIndex(strNewDirectoryIndex);
            }

            /* Get object name */
            QString strObjName(obj.strName.c_str());

            /* Get ContentType */
            QString strContentType(obj.strMimeType.c_str());
#ifdef ENABLE_HACK
            /* Hack needed for vtc trial sample */
            if (strObjName.endsWith(".stm", Qt::CaseInsensitive) && !strContentType.compare("application/octet-stream", Qt::CaseInsensitive))
                strContentType = "text/html";
#endif
            /* Add received MOT object to ui->webView */
            cache.AddObject(strObjName, strContentType, obj.Body.vecData);

            /* Store received MOT object on disk */
            if (bSaveFileToDisk)
                SaveMOTObject(strObjName, obj);

            /* Set changed flag */
            bChanged = true;
        }
    }
    return bChanged;
}

void BWSViewer::SaveMOTObject(const QString& strObjName,
                              const CMOTObject& obj)
{
    const CVector<_BYTE>& vecbRawData = obj.Body.vecData;

    /* Generate safe filename */
    QString strFileName = strCurrentSavePath + VerifyHtmlPath(strObjName);

    /* First, create directory for storing the file (if not exists) */
    CreateDirectories(strFileName);

    /* Open file */
    QFile file(strFileName);
    if (file.open(QIODevice::WriteOnly))// | QIODevice::Truncate))
    {
        int i, written, size;
        size = vecbRawData.Size();

        /* Write data */
        for (i = 0, written = 0; size > 0 && written >= 0; i+=written, size-=written)
            written = file.write((const char*)&vecbRawData.at(i), size);

        /* Close the file afterwards */
        file.close();
    }
    else
    {
        QMessageBox::information(container, file.errorString(), strFileName);
    }
}

void BWSViewer::setSavePath(const QString& s)
{
    /* Append service ID to MOT save path */
    QString strSavePath;
    strSavePath.setNum(serviceID, 16).toUpper().rightJustified(8, '0');
    strCurrentSavePath = s + "/" + strSavePath + "/";
}

void BWSViewer::setServiceInfo(CService s)
{
    strServiceLabel = QString::fromUtf8(s.strLabel.c_str());
    serviceID = s.iServiceID;
    stream_id = s.DataParam.iStreamID;
    packet_id = s.DataParam.iPacketID;
}

void BWSViewer::setRxStatus(int streamID, int packetID, ETypeRxStatus s)
{
    if((streamID == stream_id) && (packetID == packet_id))
        SetStatus(ui->LEDStatus, s);
}

void BWSViewer::OnTimer()
{

}

//////////////////////////////////////////////////////////////////
// CWebsiteCache implementation

void CWebsiteCache::GetObjectCountAndSize(unsigned int& count, unsigned int& size)
{
    mutex.lock();
        count = objects.size();
        size = total_size;
    mutex.unlock();
}

void CWebsiteCache::ClearAll()
{
    mutex.lock();
        strDirectoryIndex = QString(); /* NULL string, not empty string! */
        objects.clear();
        total_size = 0;
    mutex.unlock();
}

void CWebsiteCache::AddObject(QString strObjName, QString strContentType, CVector<_BYTE>& vecbData)
{
    mutex.lock();
        /* increment id counter, 0 is reserved for error */
        id_counter++; if (!id_counter) id_counter++;

        /* Get the object name */
        strObjName = UrlEncodePath(strObjName);

        /* Erase previous object if any */
        map<QString,CWebsiteObject>::iterator it;
        it = objects.find(strObjName);
        if (it != objects.end())
        {
            total_size -= it->second.data.size();
            objects.erase(it);
        }

        /* Insert the new object */
        objects.insert(pair<QString,CWebsiteObject>(strObjName, CWebsiteObject(id_counter, strContentType, vecbData)));
        total_size += vecbData.Size();
    mutex.unlock();

    /* Signal that a new object is added */
    emit ObjectAdded(strObjName);
}

int CWebsiteCache::GetObjectContentType(const QString& strObjName, QString& strContentType)
{
    int id = 0;
    mutex.lock();
        CWebsiteObject* obj = FindObject(strObjName);
        if (obj)
        {
            id = obj->id;
            strContentType = obj->strContentType;
        }
    mutex.unlock();
    return id;
}

int CWebsiteCache::GetObjectSize(const QString& strObjName, const unsigned int id)
{
    int size = 0;
    mutex.lock();
        CWebsiteObject* obj = FindObject(strObjName);
        if (obj && obj->id == id)
        {
            size = obj->data.size();
        }
    mutex.unlock();
    return size;
}

int CWebsiteCache::CopyObjectData(const QString& strObjName, const unsigned int id, char *buffer, int maxsize, int offset)
{
    int size = -1;
    if (maxsize >= 0 && offset >= 0)
    {
        mutex.lock();
            CWebsiteObject* obj = FindObject(strObjName);
            if (obj && obj->id == id)
            {
                size = obj->data.size();
                if (offset < size)
                {
                    size -= offset;
                    if (size > maxsize)
                        size = maxsize;
                    memcpy(buffer, &obj->data.data()[offset], size);
                }
            }
        mutex.unlock();
    }
    return size;
}

CWebsiteObject* CWebsiteCache::FindObject(const QString& strObjName)
{
    map<QString,CWebsiteObject>::iterator it;
    it = objects.find(strObjName);
    return it != objects.end() ? &it->second : NULL;
}

bool CWebsiteCache::SetDirectoryIndex(const QString strNewDirectoryIndex)
{
    bool bChanged;
    mutex.lock();
        bChanged = strDirectoryIndex != strNewDirectoryIndex;
        if (bChanged)
            strDirectoryIndex = strNewDirectoryIndex;
    mutex.unlock();
    return bChanged;
}

QString CWebsiteCache::GetDirectoryIndex()
{
    mutex.lock();
        QString str = strDirectoryIndex;
    mutex.unlock();
    return str;
}


//////////////////////////////////////////////////////////////////
// CNetworkReplyCache implementation

CNetworkReplyCache::CNetworkReplyCache(QNetworkAccessManager::Operation op,
    const QNetworkRequest& req, CWebsiteCache& cache, CCounter& waitobjs)
    : cache(cache), waitobjs(waitobjs),
    readOffset(0), emitted(false), id(0)
{
    /* ETSI TS 101 498-1 Section 6.2.3 */
    QString strUrl(req.url().toString());
    QString strDirectoryIndex;
    if (IsUrlDirectory(strUrl))
        strDirectoryIndex = cache.GetDirectoryIndex();
    path = UrlEncodePath(strUrl + strDirectoryIndex);

    connect(&cache, SIGNAL(ObjectAdded(QString)), this, SLOT(CheckObject(QString)));
    setOperation(op);
    setRequest(req);
    setUrl(req.url());
    open(QIODevice::ReadOnly);
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    waitobjs++;
};

CNetworkReplyCache::~CNetworkReplyCache()
{
    waitobjs--;
};

void CNetworkReplyCache::customEvent(QEvent* event)
{
    if (event->type() == QEvent::User)
        CheckObject(path);
}

qint64 CNetworkReplyCache::bytesAvailable() const
{
    return cache.GetObjectSize(path, id);
}

qint64 CNetworkReplyCache::readData(char * data, qint64 maxSize)
{
    int len = cache.CopyObjectData(path, id, data, maxSize, readOffset);
    if (len > 0)
        readOffset += len;
    return len;
}

void CNetworkReplyCache::CheckObject(QString strObjName)
{
    if (!emitted && path == strObjName)
    {
        QString strContentType;
        unsigned int new_id = cache.GetObjectContentType(path, strContentType);
        if (new_id)
        {
            id = new_id;
            setRawHeader(QByteArray("Content-Type"), strContentType.toUtf8());
            emitted = true;
            emit readyRead(); /* needed for Qt 4.6 */
            emit finished();
        }
    }
}


//////////////////////////////////////////////////////////////////
// CNetworkAccessManager implementation

QNetworkReply* CNetworkAccessManager::createRequest(Operation op, const QNetworkRequest& req, QIODevice* outgoingData)
{
    if (!bAllowExternalContent || req.url().host() == strCacheHost)
        return new CNetworkReplyCache(op, req, cache, waitobjs);
    else
        return QNetworkAccessManager::createRequest(op, req, outgoingData);
}