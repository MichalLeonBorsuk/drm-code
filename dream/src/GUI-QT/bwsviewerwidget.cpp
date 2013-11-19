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

#include "bwsviewerwidget.h"
#include "ui_bwsviewerwidget.h"
#include <../datadecoding/DataDecoder.h>
#include <../util-QT/Util.h>
#include <QWebHistory>
#include <QMessageBox>

BWSViewerWidget::BWSViewerWidget(int s, QWidget* parent):
    QWidget(parent),
    ui(new Ui::BWSViewerWidget),
    short_id(s),decoder(NULL),service(),
    nam(this, cache, waitobjs, bAllowExternalContent, strCacheHost),
    bHomeSet(false), bPageLoading(false),
    bSaveFileToDisk(false), bRestrictedProfile(false), bAllowExternalContent(true),
    bClearCacheOnNewService(true), bDirectoryIndexChanged(false),
    iLastAwaitingOjects(0), strCacheHost("127.0.0.1"),
    strCurrentSavePath()
{
    ui->setupUi(this);
}

BWSViewerWidget::~BWSViewerWidget()
{
    delete ui;
}

void BWSViewerWidget::setDecoder(CDataDecoder* dec)
{
    if(dec)
        decoder = dec->getApplication(service.DataParam.iPacketID);
}

void BWSViewerWidget::setStatus(int s, ETypeRxStatus status)
{
    if(s==short_id)
    {
        SetStatus(ui->LEDStatus, status);
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
}

void BWSViewerWidget::setServiceInformation(CService s)
{
    service = s; // make a copy we can keep

    QString strLabel = QString().fromUtf8(service.strLabel.c_str()).trimmed();
    UpdateWindowTitle(service.iServiceID, true, strLabel);

    // assume it is a new service
    OnClearCache();
}

void BWSViewerWidget::setSavePath(const QString& s)
{
    /* Append service ID to MOT save path */
    strCurrentSavePath = s + QString().setNum(service.iServiceID, 16).toUpper().rightJustified(8, '0') + "/";
}

void BWSViewerWidget::UpdateButtons()
{
    ui->buttonStepBack->setEnabled(ui->webView->history()->canGoBack());
    ui->buttonStepForward->setEnabled(ui->webView->history()->canGoForward());
    ui->buttonHome->setEnabled(bHomeSet);
    ui->buttonStopRefresh->setEnabled(bHomeSet);
    ui->buttonStopRefresh->setIcon(QIcon(bPageLoading ? ":/icons/Stop.png" : ":/icons/Refresh.png"));
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
    Update();
}

void BWSViewerWidget::OnWebViewLoadStarted()
{
    bPageLoading = true;
    QTimer::singleShot(20, this, SLOT(Update()));
}

void BWSViewerWidget::OnWebViewLoadFinished(bool)
{
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

bool BWSViewerWidget::Changed()
{
    bool bChanged = false;
    if (decoder != NULL)
    {
        CMOTObject obj;

        /* Poll the data decoder module for new object */
        while(decoder->NewObjectAvailable())
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

void BWSViewerWidget::SaveMOTObject(const QString& strObjName,
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
        QMessageBox::information(this, file.errorString(), strFileName);
    }
}
