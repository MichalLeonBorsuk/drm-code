/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
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
#include "../util/Settings.h"
#include "../datadecoding/DataDecoder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QWebHistory>


//#define DEFAULT_DIRECTORY "MOT"
#define DEFAULT_DIRECTORY "data/mot"


BWSViewer::BWSViewer(CDRMReceiver& rec, CSettings& s, QWidget* parent, Qt::WFlags):
    QDialog(parent), Ui_BWSViewer(),
    receiver(rec), settings(s), decoder(NULL), initialised(false)
{
    /* Enable minimize and maximize box for QDialog */
	setWindowFlags(Qt::Window);

    setupUi(this);

    webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));

    connect(actionClear_All, SIGNAL(triggered()), SLOT(OnClearAll()));
    connect(actionSave, SIGNAL(triggered()), SLOT(OnSave()));
    connect(actionSave_All, SIGNAL(triggered()), SLOT(OnSaveAll()));
    connect(actionClose, SIGNAL(triggered()), SLOT(close()));
    connect(actionRestricted_Profile_Only, SIGNAL(triggered(bool)), SLOT(onSetProfile(bool)));

    /* Update time for color LED */
    LEDStatus->SetUpdateTime(1000);

    /* Connect controls */
    connect(ButtonStepBack, SIGNAL(clicked()), this, SLOT(OnBack()));
    connect(ButtonStepForward, SIGNAL(clicked()), this, SLOT(OnForward()));
    connect(ButtonHome, SIGNAL(clicked()), this, SLOT(OnHome()));
    connect(ButtonClearCache, SIGNAL(clicked()), this, SLOT(OnClearCache()));
    connect(webView, SIGNAL(linkClicked(const QUrl &)), this, SLOT(OnLinkClicked(const QUrl &)));
    connect(webView, SIGNAL(loadFinished(bool)), this, SLOT(OnLoadFinished(bool)));

    OnClearAll();

    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

    ButtonStepBack->setEnabled(false);
    ButtonStepForward->setEnabled(false);
    ButtonHome->setEnabled(false);
    ButtonClearCache->setEnabled(false);
}

BWSViewer::~BWSViewer()
{
}

void BWSViewer::UpdateButtons()
{
    ButtonStepBack->setEnabled(webView->history()->canGoBack());
    ButtonHome->setEnabled(initialised);
    ButtonStepForward->setEnabled(webView->history()->canGoForward());
}


void BWSViewer::OnTimer()
{
    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
    ETypeRxStatus status = Parameters.ReceiveStatus.MOT.GetStatus();
    /* Get current data service */
    const int iCurSelDataServ = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[iCurSelDataServ];
    Parameters.Unlock();

    if (decoder == NULL)
        decoder = receiver.GetDataDecoder();

    switch(status)
    {
    case NOT_PRESENT:
        LEDStatus->Reset();
        break;

    case CRC_ERROR:
        LEDStatus->SetLight(CMultColorLED::RL_RED);
        break;

    case DATA_ERROR:
        LEDStatus->SetLight(CMultColorLED::RL_YELLOW);
        break;

    case RX_OK:
        LEDStatus->SetLight(CMultColorLED::RL_GREEN);
        break;
    }

    if (Changed())
    {
        webView->reload();
//        UpdateButtons();
    }
}

void BWSViewer::OnSave()
{
}

void BWSViewer::OnSaveAll()
{
}

void BWSViewer::OnClearAll()
{
    webView->history()->clear();
    webView->setHtml("");
    webView->setToolTip("");

    actionClear_All->setEnabled(false);
    actionSave->setEnabled(false);
    actionSave_All->setEnabled(false);
    ButtonStepBack->setEnabled(false);
    ButtonStepForward->setEnabled(false);
    ButtonHome->setEnabled(false);
}

void BWSViewer::OnHome()
{
    if (!strCurrentSavePath.isEmpty() && !strHomeUrl.isEmpty())
    {
        QUrl url(strCurrentSavePath + "/" + strHomeUrl);
        webView->setUrl(url);
    }
    else
        webView->setHtml("");
}

void BWSViewer::OnBack()
{
    webView->history()->back();
//    UpdateButtons();
}

void BWSViewer::OnForward()
{
    webView->history()->forward();
//    UpdateButtons();
}

void BWSViewer::OnClearCache()
{
    if (strCurrentSavePath.size() >= 6) /* at least 6 digit directory */
        if (RemoveDir(strCurrentSavePath))
        {
            webView->history()->clear();
            webView->setHtml("");
            webView->setToolTip("");
            strHomeUrl = "";
            initialised = false;
            ButtonClearCache->setEnabled(false);
            UpdateButtons();
        }
}

void BWSViewer::OnLinkClicked(const QUrl & url)
{
    QString file(VerifyHtmlPath(url.toLocalFile()));
    if (file.endsWith(".stm")) // TODO hack
        file.append(".html");
    if (QFile(file).exists())
        webView->setUrl(QUrl(file));
}

void BWSViewer::OnLoadFinished(bool ok)
{
    UpdateButtons();
}

void BWSViewer::onSetProfile(bool isChecked)
{
	(void)isChecked;
}


void BWSViewer::showEvent(QShowEvent* e)
{
	EVENT_FILTER(e);
    /* Get window geometry data and apply it */
    CWinGeom g;
    settings.Get("BWS", g);
    const QRect WinGeom(g.iXPos, g.iYPos, g.iWSize, g.iHSize);

    if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
        setGeometry(WinGeom);

    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
    const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
    const uint32_t iAudioServiceID = Parameters.Service[iCurSelAudioServ].iServiceID;
    /* Get current data service */
    const int iCurSelDataServ = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[iCurSelDataServ];
    Parameters.Unlock();

    QString strTitle("MOT Broadcast Website");

    if (service.IsActive())
    {
        /* Do UTF-8 to QString (UNICODE) conversion with the label strings */
        QString strLabel = QString().fromUtf8(service.strLabel.c_str()).trimmed();


        /* Service ID (plot number in hexadecimal format) */
        QString strServiceID = "";

        /* show the ID only if differ from the audio service */
        if ((service.iServiceID != 0) && (service.iServiceID != iAudioServiceID))
        {
            if (strLabel != "")
                strLabel += " ";

            strServiceID = "- ID:" +
                           QString().setNum(long(service.iServiceID), 16).toUpper();
        }

        /* add the description on the title of the dialog */
        if (strLabel != "" || strServiceID != "")
            strTitle += " [" + strLabel + strServiceID + "]";
    }
    setWindowTitle(strTitle);

    /* Update window */
    OnTimer();

    /* Activate real-time timer when window is shown */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void BWSViewer::hideEvent(QHideEvent* e)
{
	EVENT_FILTER(e);
    /* Deactivate real-time timer so that it does not get new pictures */
    Timer.stop();

    /* Save window geometry data */
    QRect WinGeom = geometry();

    CWinGeom c;
    c.iXPos = WinGeom.x();
    c.iYPos = WinGeom.y();
    c.iHSize = WinGeom.height();
    c.iWSize = WinGeom.width();
    settings.Put("BWS", c);
}

void BWSViewer::SetupCurrentSavePath()
{
    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
    const int iCurSelDataServ = Parameters.GetCurSelDataService();
    const int iServiceID = Parameters.Service[iCurSelDataServ].iServiceID;
    Parameters.Unlock();

    QString strServiceID;
    strServiceID.setNum(iServiceID, 16).toUpper();
    strServiceID = strServiceID.rightJustified(6, '0');

    strCurrentSavePath = QString(settings.Get("BWS", "storagepath", string(DEFAULT_DIRECTORY)).c_str());
    settings.Put("BWS", "storagepath", string(strCurrentSavePath.latin1()));

    if (!strCurrentSavePath.isEmpty())
        strCurrentSavePath += "/";
    strCurrentSavePath += strServiceID;

    /* Create the cache directory if not exist */
    if (!QFileInfo(strCurrentSavePath).exists())
        QDir().mkdir(strCurrentSavePath);
}

bool BWSViewer::Changed()
{
    if (decoder == NULL)
        return false;

    CMOTObject obj;

    /* Poll the data decoder module for new object */
    if (decoder->GetMOTObject(obj, CDataDecoder::AT_BROADCASTWEBSITE) == TRUE)
    {
        QString strObjName = VerifyHtmlPath(obj.strName.c_str());
//printf("%s\n", obj.strMimeType.c_str());
//printf("file: %s\n", obj.strName.c_str());
		if (strObjName.endsWith(".stm")) // TODO hack
			strObjName.append(".html");

        /* Set strCurrentSavePath */
        SetupCurrentSavePath();

        /* Store received MOT object on disk */
        const QString strFileName = strCurrentSavePath + "/" + strObjName;
        SaveMOTObject(obj, strFileName);
#if 0
	    /* Store it in the internal map */
	    if (strObjName.endsWith(".html") /*|| strObjName.endsWith(".stm")*/)
        {
	        QString s;
	        for(int i=0; i<obj.Body.vecData.Size(); i++)
		        s += obj.Body.vecData[i];
	        s = s.replace(QRegExp("color=([0-9a-fA-F])"), "color=#\\1"); 
	
	        pages[strObjName] = s;
	    }
	    else
	    {
	        QByteArray ba(obj.Body.vecData.Size());
	        for(int i=0; i<obj.Body.vecData.Size(); i++)
		        ba[i] = obj.Body.vecData[i];
	        pages[strObjName] = ba;
	    }
#endif
        if (strObjName.contains('/') == 0) /* if has a path is not the main page */
        {
            /* Get the current directory */
            CMOTDirectory MOTDir;

            if (decoder->GetMOTDirectory(MOTDir, CDataDecoder::AT_BROADCASTWEBSITE) == TRUE)
            {
                /* Checks if the DirectoryIndex has values */
                if (MOTDir.DirectoryIndex.size() > 0)
                {
		            QString sNewHomeUrl;
                    if(MOTDir.DirectoryIndex.find(UNRESTRICTED_PC_PROFILE) != MOTDir.DirectoryIndex.end())
                        sNewHomeUrl =
                            MOTDir.DirectoryIndex[UNRESTRICTED_PC_PROFILE].c_str();
                    else if(MOTDir.DirectoryIndex.find(BASIC_PROFILE) != MOTDir.DirectoryIndex.end())
                        sNewHomeUrl =
                            MOTDir.DirectoryIndex[BASIC_PROFILE].c_str();
//printf("home: %s\n", sNewHomeUrl.toUtf8().data());
        		    if (sNewHomeUrl == "not_here.html") // this is a hack
            			sNewHomeUrl = "index.html";
        		    if (strHomeUrl.isEmpty() && !sNewHomeUrl.isEmpty())
        		    {
                        strHomeUrl = sNewHomeUrl;
        		    }
                }
            }
        }
        if (!initialised && strHomeUrl == strObjName)
        {
            initialised = true;
            OnHome();
        }
        ButtonClearCache->setEnabled(true);
    	return true;
    }
    return false;
}

void BWSViewer::SaveMOTObject(const CMOTObject& obj,
                                  const QString& strFileName)
{
    const CVector<_BYTE>& vecbRawData = obj.Body.vecData;

    /* First, create directory for storing the file (if not exists) */
    CreateDirectories(strFileName.latin1());

    /* Data size in bytes */
    const int iSize = vecbRawData.Size();

    /* Open file */
    FILE* pFiBody = fopen(strFileName.latin1(), "wb");

    if (pFiBody != NULL)
    {
        /* Write data byte-wise */
        for (int i = 0; i < iSize; i++)
        {
            const _BYTE b = vecbRawData[i];
            fwrite(&b, size_t(1), size_t(1), pFiBody);
        }

        /* Close the file afterwards */
        fclose(pFiBody);
    }
}

void BWSViewer::CreateDirectories(const QString& filename)
{
    int i = 0;

    while (i < filename.length())
    {
        _BOOLEAN bFound = FALSE;

        while ((i < filename.length()) && (bFound == FALSE))
        {
            if (filename[i] == '/')
                bFound = TRUE;
            else
                i++;
        }

        if (bFound == TRUE)
        {
            /* create directory */
            const QString sDirName = filename.left(i);

            if (!QFileInfo(sDirName).exists())
                QDir().mkdir(sDirName);
        }

        i++;
    }
}

bool BWSViewer::RemoveDir(const QString &dirName, int level)
{
    bool result = true;
    return result;
}

