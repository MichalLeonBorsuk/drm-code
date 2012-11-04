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

BWSViewer::BWSViewer(CDRMReceiver& rec, CSettings& s,QWidget* parent, Qt::WFlags f):
		QMainWindow(parent, f), Ui_BWSViewer(), Timer(),
		receiver(rec), settings(s), homeUrl(""), decoderSet(false)
{
    setupUi(this);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));

	connect(actionClear_All, SIGNAL(triggered()), SLOT(OnClearAll()));
	connect(actionSave, SIGNAL(triggered()), SLOT(OnSave()));
	connect(actionSave_All, SIGNAL(triggered()), SLOT(OnSaveAll()));
	connect(actionClose, SIGNAL(triggered()), SLOT(close()));
	connect(actionRestricted_Profile_Only, SIGNAL(triggered(bool)), SLOT(onSetProfile(bool)));

	/* Update time for color LED */
	LEDStatus->SetUpdateTime(1000);

	/* Connect controls */
	connect(ButtonStepBack, SIGNAL(clicked()), this, SLOT(OnButtonStepBack()));
	connect(ButtonStepForward, SIGNAL(clicked()), this, SLOT(OnButtonStepForward()));
	connect(ButtonHome, SIGNAL(clicked()), this, SLOT(OnButtonHome()));

    OnClearAll();

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	Timer.stop();
}

BWSViewer::~BWSViewer()
{
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

    if(!decoderSet)
    {
        CDataDecoder* dec = receiver.GetDataDecoder();
        CMOTDABDec *decoder = (CMOTDABDec*)dec->getApplication(service.DataParam.iPacketID);

        if(decoder)
        {
            textBrowser->setDecoder(decoder);
            decoderSet = true;
        }
    }

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

    if(textBrowser->changed())
    {
        textBrowser->reload();
    }

    if(homeUrl=="")
    {
        homeUrl = textBrowser->homeUrl();
        if(homeUrl!="")
            textBrowser->setSource(QUrl(homeUrl));
    }
}

void BWSViewer::OnButtonStepBack()
{
    textBrowser->backward();
}

void BWSViewer::OnButtonStepForward()
{
    textBrowser->forward();
}

void BWSViewer::OnButtonHome()
{
    textBrowser->home();
}

void BWSViewer::OnSave()
{
}

void BWSViewer::OnSaveAll()
{
}

void BWSViewer::OnClearAll()
{
    textBrowser->clear();
    textBrowser->clearHistory();
	textBrowser->setToolTip("");

    actionClear_All->setEnabled(false);
    actionSave->setEnabled(false);
    actionSave_All->setEnabled(false);
    ButtonStepBack->setEnabled(false);
    ButtonStepForward->setEnabled(false);
    ButtonHome->setEnabled(false);
}

void BWSViewer::onSetProfile(bool isChecked)
{
    textBrowser->setRestrictedProfile(isChecked);
}

void BWSViewer::showEvent(QShowEvent*)
{
	/* Get window geometry data and apply it */
	CWinGeom g;
	settings.Get("BWS", g);
	const QRect WinGeom(g.iXPos, g.iYPos, g.iWSize, g.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

	strCurrentSavePath = settings.Get("BWS", "storagepath", strCurrentSavePath);

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

void BWSViewer::hideEvent(QHideEvent*)
{
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

	/* Store save path */
	settings.Put("BWS ","storagepath", strCurrentSavePath);
}


