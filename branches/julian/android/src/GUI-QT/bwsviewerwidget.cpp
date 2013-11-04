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

#include "bwsviewerwidget.h"
#include "ui_bwsviewerwidget.h"

BWSViewerWidget::BWSViewerWidget(CDRMReceiver& rec, CSettings& Settings, int sid, QWidget* parent):
    QWidget(parent),
    ui(new Ui::BWSViewerWidget),
    short_id(sid),
    settings(Settings),
    nam(this, cache, waitobjs, bAllowExternalContent, strCacheHost),
    receiver(rec), decoder(NULL), bHomeSet(false), bPageLoading(false),
    bSaveFileToDisk(false), bRestrictedProfile(false), bAllowExternalContent(true),
    bClearCacheOnNewService(true), bDirectoryIndexChanged(false),
    iLastAwaitingOjects(0), strCacheHost(CACHE_HOST),
    iLastServiceID(0), iCurrentDataServiceID(0), bLastServiceValid(false), iLastValidServiceID(0)
{
    ui->setupUi(this);

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

    /* Connect controls */
    connect(ui->buttonDecode, SIGNAL(clicked()), this, SLOT(close()));
    //connect(actionClear_Cache, SIGNAL(triggered()), SLOT(OnClearCache()));
    //connect(actionClose, SIGNAL(triggered()), SLOT(close()));
    //connect(actionAllow_External_Content, SIGNAL(triggered(bool)), SLOT(OnAllowExternalContent(bool)));
    //connect(actionRestricted_Profile_Only, SIGNAL(triggered(bool)), SLOT(OnSetProfile(bool)));
    //connect(actionSave_File_to_Disk, SIGNAL(triggered(bool)), SLOT(OnSaveFileToDisk(bool)));
    //connect(actionClear_Cache_on_New_Service, SIGNAL(triggered(bool)), SLOT(OnClearCacheOnNewService(bool)));
    connect(ui->buttonStepBack, SIGNAL(clicked()), this, SLOT(OnBack()));
    connect(ui->buttonStepForward, SIGNAL(clicked()), this, SLOT(OnForward()));
    connect(ui->buttonHome, SIGNAL(clicked()), this, SLOT(OnHome()));
    connect(ui->buttonStopRefresh, SIGNAL(clicked()), this, SLOT(OnStopRefresh()));
    connect(ui->buttonClearCache, SIGNAL(clicked()), this, SLOT(OnClearCache()));
    connect(ui->webView, SIGNAL(loadStarted()), this, SLOT(Onui->webViewLoadStarted()));
    connect(ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(Onui->webViewLoadFinished(bool)));
    connect(ui->webView, SIGNAL(titleChanged(const QString &)), this, SLOT(Onui->webViewTitleChanged(const QString &)));
    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));
}

BWSViewerWidget::~BWSViewerWidget()
{
    delete ui;
}

void BWSViewerWidget::UpdateButtons()
{
    ui->buttonStepBack->setEnabled(ui->webView->history()->canGoBack());
    ui->buttonStepForward->setEnabled(ui->webView->history()->canGoForward());
    ui->buttonHome->setEnabled(bHomeSet);
    ui->buttonStopRefresh->setEnabled(bHomeSet);
    ui->buttonStopRefresh->setIcon(QIcon(bPageLoading ? ICON_STOP : ICON_REFRESH));
}

QString BWSViewerWidget::ObjectStr(unsigned int count)
{
    return QString(count > 1 ? tr("objects") : tr("object"));
}

void BWSViewerWidget::UpdateStatus()
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
            text += tr("  |  %1 %2 pending").arg(iLastAwaitingOjects).arg(ObjectStr(iLastAwaitingOjects));
        if (bAllowExternalContent && ui->webView->url().isValid() && ui->webView->url().host() != strCacheHost)
            text += "  |  " + ui->webView->url().toString();
        ui->labelStatus->setText(text);
    }
}

void BWSViewerWidget::UpdateWindowTitle(const uint32_t iServiceID, const bool bServiceValid, QString strLabel)
{
    QString strTitle("MOT Broadcast Website");
    iLastServiceID = iServiceID;
    bLastServiceValid = bServiceValid;
    strLastLabel = strLabel;
    if (bServiceValid)
    {
        if (strLabel != "")
            strLabel += " - ";

        /* Service ID (plot number in hexadecimal format) */
        QString strServiceID = "ID:" +
                       QString().setNum(iServiceID, 16).toUpper();

        /* Add the description on the title of the dialog */
        if (strLabel != "" || strServiceID != "")
            strTitle += " [" + strLabel + strServiceID + "]";
    }
    setWindowTitle(strTitle);
}

void BWSViewerWidget::Update()
{
    UpdateStatus();
    UpdateButtons();
}

void BWSViewerWidget::OnTimer()
{
    /* Get current service parameters */
    uint32_t iServiceID; bool bServiceValid; QString strLabel; ETypeRxStatus eStatus;
    GetServiceParams(&iServiceID, &bServiceValid, &strLabel, &eStatus);

    /* Set current data service ID */
    iCurrentDataServiceID = bServiceValid ? iServiceID : 0;

    /* Check for new valid data service */
    if (bServiceValid && iLastValidServiceID != iServiceID)
    {
        iLastValidServiceID = iServiceID;
        if (bClearCacheOnNewService)
            OnClearCache();
    }

    /* Update the window title if something have changed */
    if (iLastServiceID != iServiceID || bLastServiceValid != bServiceValid || strLastLabel != strLabel)
        UpdateWindowTitle(iServiceID, bServiceValid, strLabel);

    switch(eStatus)
    {
    case NOT_PRESENT:
        ui->LEDStatus->Reset();
        break;

    case CRC_ERROR:
        ui->LEDStatus->SetLight(CMultColorLED::RL_RED);
        break;

    case DATA_ERROR:
        ui->LEDStatus->SetLight(CMultColorLED::RL_YELLOW);
        break;

    case RX_OK:
        ui->LEDStatus->SetLight(CMultColorLED::RL_GREEN);
        break;
    }

    if (decoder == NULL)
    {
        decoder = receiver.GetDataDecoder();
        if (decoder == NULL)
            qDebug("can't get data decoder from receiver");
    }

    if (Changed())
    {
        if (bDirectoryIndexChanged)
        {
            bDirectoryIndexChanged = false;
            if (!bHomeSet)
            {
                bHomeSet = true;
                OnHome();
            }
        }
        Update();
        ui->buttonClearCache->setEnabled(true);
        //actionClear_Cache->setEnabled(true);
    }
    else
    {
        unsigned int iAwaitingOjects = waitobjs;
        if (iLastAwaitingOjects != iAwaitingOjects)
        {
            iLastAwaitingOjects = iAwaitingOjects;
            UpdateStatus();
        }
    }
}

void BWSViewerWidget::OnHome()
{
    ui->webView->load("http://" + strCacheHost);
}

void BWSViewerWidget::OnStopRefresh()
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

void BWSViewerWidget::OnBack()
{
    ui->webView->history()->back();
}

void BWSViewerWidget::OnForward()
{
    ui->webView->history()->forward();
}

void BWSViewerWidget::OnClearCache()
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

void BWSViewerWidget::OnWebViewLoadStarted()
{
    bPageLoading = true;
    QTimer::singleShot(20, this, SLOT(Update()));
}

void BWSViewerWidget::OnWebViewLoadFinished(bool ok)
{
    (void)ok;
    bPageLoading = false;
    QTimer::singleShot(20, this, SLOT(Update()));
}

void BWSViewerWidget::OnWebViewTitleChanged(const QString& title)
{
    ui->labelTitle->setText("<b>" + title + "</b>");
}

void BWSViewerWidget::OnAllowExternalContent(bool isChecked)
{
    bAllowExternalContent = isChecked;
}

void BWSViewerWidget::OnSetProfile(bool isChecked)
{
    bRestrictedProfile = isChecked;
}

void BWSViewerWidget::OnSaveFileToDisk(bool isChecked)
{
    bSaveFileToDisk = isChecked;
}

void BWSViewerWidget::OnClearCacheOnNewService(bool isChecked)
{
    bClearCacheOnNewService = isChecked;
}

void BWSViewerWidget::eventShow(QShowEvent*)
{
    bAllowExternalContent = settings.Get("BWS", "allowexternalcontent", bAllowExternalContent);
    //actionAllow_External_Content->setChecked(bAllowExternalContent);

    bSaveFileToDisk = settings.Get("BWS", "savefiletodisk", bSaveFileToDisk);
    //actionSave_File_to_Disk->setChecked(bSaveFileToDisk);

    bRestrictedProfile = settings.Get("BWS", "restrictedprofile", bRestrictedProfile);
    //actionRestricted_Profile_Only->setChecked(bRestrictedProfile);

    bClearCacheOnNewService = settings.Get("BWS", "clearcacheonnewservice", bClearCacheOnNewService);
    //actionClear_Cache_on_New_Service->setChecked(bClearCacheOnNewService);

    /* Update window title */
    uint32_t iServiceID; bool bServiceValid; QString strLabel;
    GetServiceParams(&iServiceID, &bServiceValid, &strLabel);
    UpdateWindowTitle(iServiceID, bServiceValid, strLabel);

    /* Update window content */
    OnTimer();

    /* Activate real-time timer when window is shown */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void BWSViewerWidget::eventHide(QHideEvent*)
{
    /* Deactivate real-time timer so that it does not get new pictures */
    Timer.stop();

    settings.Put("BWS", "savefiletodisk", bSaveFileToDisk);

    settings.Put("BWS", "restrictedprofile", bRestrictedProfile);

    settings.Put("BWS", "allowexternalcontent", bAllowExternalContent);

    settings.Put("BWS", "clearcacheonnewservice", bClearCacheOnNewService);
}

bool BWSViewerWidget::Changed()
{
    bool bChanged = false;
    if (decoder != NULL)
    {
        CMOTObject obj;

        /* Poll the data decoder module for new object */
        while (decoder->GetMOTObject(obj, CDataDecoder::AT_BROADCASTWEBSITE) == TRUE)
        {
            /* Get the current directory */
            CMOTDirectory MOTDir;
            if (decoder->GetMOTDirectory(MOTDir, CDataDecoder::AT_BROADCASTWEBSITE) == TRUE)
            {
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

void BWSViewerWidget::SaveMOTObject(const QString& strObjName,
                              const CMOTObject& obj)
{
    const CVector<_BYTE>& vecbRawData = obj.Body.vecData;

    QString strCurrentSavePath;

    /* Set up save path */
    SetupSavePath(strCurrentSavePath);

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
        QMessageBox::information(this, file.errorString(), strFileName);
    }
}

void BWSViewerWidget::SetupSavePath(QString& strSavePath)
{
    /* Append service ID to MOT save path */
    strSavePath = strSavePath.setNum(iCurrentDataServiceID, 16).toUpper().rightJustified(8, '0');
    strSavePath = QString::fromUtf8((*receiver.GetParameters()).GetDataDirectory("MOT").c_str()) + strSavePath + "/";
}

void BWSViewerWidget::GetServiceParams(uint32_t* iServiceID, bool* bServiceValid, QString* strLabel, ETypeRxStatus* eStatus)
{
    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
        const int iCurSelDataServ = Parameters.GetCurSelDataService();
        const CService service = Parameters.Service[iCurSelDataServ];
        if (eStatus)
            *eStatus = Parameters.DataComponentStatus[iCurSelDataServ].GetStatus();
    Parameters.Unlock();
    if (iServiceID)
        *iServiceID = service.iServiceID;
    if (bServiceValid)
        *bServiceValid = service.IsActive() && service.eAudDataFlag == CService::SF_DATA;
    /* Do UTF-8 to QString (UNICODE) conversion with the ui->Label strings */
    if (strLabel)
        *strLabel = QString().fromUtf8(service.strLabel.c_str()).trimmed();
}
