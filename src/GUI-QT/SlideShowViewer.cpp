/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description: SlideShow Viewer
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

#include "SlideShowViewer.h"
#include "../util/Settings.h"
#include "../datadecoding/DABMOT.h"
#include "../datadecoding/DataDecoder.h"
#include <QFileDialog>

SlideShowViewer::SlideShowViewer(ReceiverInterface& rec, CSettings& s,
        QWidget* parent,
		const char* name, Qt::WFlags f):
		QMainWindow(parent, f), Ui_SlideShowViewer(), Timer(), strCurrentSavePath("."),
		receiver(rec), settings(s),vecImages(),vecImageNames(),iCurImagePos(-1)
{
    setupUi(this);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));

	connect(actionClear_All, SIGNAL(triggered()), SLOT(OnClearAll()));
	connect(actionSave, SIGNAL(triggered()), SLOT(OnSave()));
	connect(actionSave_All, SIGNAL(triggered()), SLOT(OnSaveAll()));
	connect(actionClose, SIGNAL(triggered()), SLOT(close()));

	/* Update time for color LED */
	LEDStatus->SetUpdateTime(1000);

	/* Connect controls */
	connect(ButtonStepBack, SIGNAL(clicked()), this, SLOT(OnButtonStepBack()));
	connect(ButtonStepForward, SIGNAL(clicked()), this, SLOT(OnButtonStepForward()));
	connect(ButtonJumpBegin, SIGNAL(clicked()), this, SLOT(OnButtonJumpBegin()));
	connect(ButtonJumpEnd, SIGNAL(clicked()), this, SLOT(OnButtonJumpEnd()));

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

    OnClearAll();

	Timer.stop();
}

SlideShowViewer::~SlideShowViewer()
{
}

void SlideShowViewer::OnTimer()
{
    CParameter& Parameters = *receiver.GetParameters();
	Parameters.Lock();
	ETypeRxStatus status = Parameters.ReceiveStatus.MOT.GetStatus();
	int shortID = Parameters.GetCurSelDataService();
	int packetID = Parameters.Service[shortID].iPacketID;
	Parameters.Unlock();

	switch(status)
	{
	case NOT_PRESENT:
		LEDStatus->Reset(); /* GREY */
		break;

	case CRC_ERROR:
		LEDStatus->SetLight(CMultColorLED::RL_RED); /* RED */
		break;

	case DATA_ERROR:
		LEDStatus->SetLight(CMultColorLED::RL_YELLOW); /* YELLOW */
		break;

	case RX_OK:
		LEDStatus->SetLight(CMultColorLED::RL_GREEN); /* GREEN */
		break;
	}

	CDataDecoder& DataDecoder = *receiver.GetDataDecoder();
	CMOTDABDec *motdec = (CMOTDABDec*)DataDecoder.getApplication(packetID);

	if(motdec==NULL)
        return;

    /* Poll the data decoder module for new picture */
    TTransportID tid = motdec->GetNextTid();
    if (tid>=0)
    {
        CMOTObject	NewObj = motdec->GetObject(tid);
        /* Store received picture */
        int iCurNumPict = vecImageNames.size();
        CVector<_BYTE>& imagedata = NewObj.Body.vecData;

        /* Load picture in QT format */
        QPixmap pic;
        if (pic.loadFromData(&imagedata[0], imagedata.size()))
        {
            /* Set new picture in source factory */
            vecImages.push_back(pic);
            vecImageNames.push_back(NewObj.strName.c_str());
        }

        /* If the last received picture was selected, automatically show
           new picture */
        if (iCurImagePos == iCurNumPict - 1)
        {
            SetImage(iCurNumPict);
        }
    }
}

void SlideShowViewer::OnButtonStepBack()
{
    SetImage(iCurImagePos-1);
}

void SlideShowViewer::OnButtonStepForward()
{
    SetImage(iCurImagePos+1);
}

void SlideShowViewer::OnButtonJumpBegin()
{
    SetImage(0);
}

void SlideShowViewer::OnButtonJumpEnd()
{
    SetImage(vecImages.size()-1);
}

void SlideShowViewer::OnSave()
{
    QString fileName = QString(strCurrentSavePath.c_str()) + "/" + vecImageNames[iCurImagePos];
    fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                            fileName,
                            tr("Images (*.png *.jpg)"));
    vecImages[iCurImagePos].save(fileName);
    strCurrentSavePath = QDir(fileName).path().toUtf8().data();
}

void SlideShowViewer::OnSaveAll()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), strCurrentSavePath.c_str());
    for(size_t i=0; i<vecImages.size(); i++)
    {
        vecImages[i].save(dir+"/"+vecImageNames[i]);
    }
    strCurrentSavePath = dir.toUtf8().data();
}

void SlideShowViewer::OnClearAll()
{
    vecImages.clear();
    vecImageNames.clear();
    iCurImagePos = -1;

    actionClear_All->setEnabled(false);
    actionSave->setEnabled(false);
    actionSave_All->setEnabled(false);

    ButtonStepBack->setEnabled(false);
    ButtonStepForward->setEnabled(false);
    ButtonJumpBegin->setEnabled(false);
    ButtonJumpEnd->setEnabled(false);
}

void SlideShowViewer::showEvent(QShowEvent*)
{
	/* Get window geometry data and apply it */
	CWinGeom g;
	settings.Get("SlideShow", g);
	const QRect WinGeom(g.iXPos, g.iYPos, g.iWSize, g.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

	strCurrentSavePath = settings.Get("SlideShow", "storagepath", strCurrentSavePath);

    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
    const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
    const uint32_t iAudioServiceID = Parameters.Service[iCurSelAudioServ].iServiceID;

    /* Get current data service */
    const int iCurSelDataServ = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[iCurSelDataServ];
    Parameters.Unlock();

	/* Add the service description into the dialog caption */
	QString strTitle = tr("MOT Slide Show");

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

void SlideShowViewer::hideEvent(QHideEvent*)
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
	settings.Put("SlideShow", c);

	/* Store save path */
	settings.Put("SlideShow ","storagepath", strCurrentSavePath);
}

void SlideShowViewer::SetImage(int pos)
{
    if(vecImages.size()==0)
        return;
    if(pos<0)
        pos = 0;
    if(pos>int(vecImages.size()-1))
        pos = vecImages.size()-1;
    iCurImagePos = pos;
    image->setPixmap(vecImages[pos]);
	const QString& imagename = vecImageNames[pos];
	if (imagename.length() > 0)
		image->setToolTip(imagename);
	UpdateButtons();
}

void SlideShowViewer::UpdateButtons()
{
	/* Set enable menu entry for saving a picture */
	if (iCurImagePos < 0)
	{
		actionClear_All->setEnabled(false);
		actionSave->setEnabled(false);
		actionSave_All->setEnabled(false);
	}
	else
	{
		actionClear_All->setEnabled(true);
		actionSave->setEnabled(true);
		actionSave_All->setEnabled(true);
	}

	if (iCurImagePos <= 0)
	{
		/* We are already at the beginning */
		ButtonStepBack->setEnabled(false);
		ButtonJumpBegin->setEnabled(false);
	}
	else
	{
		ButtonStepBack->setEnabled(true);
		ButtonJumpBegin->setEnabled(true);
	}

	if (iCurImagePos == int(vecImages.size()-1))
	{
		/* We are already at the end */
		ButtonStepForward->setEnabled(false);
		ButtonJumpEnd->setEnabled(false);
	}
	else
	{
		ButtonStepForward->setEnabled(true);
		ButtonJumpEnd->setEnabled(true);
	}

	QString strTotImages = QString().setNum(vecImages.size());
	QString strNumImage = QString().setNum(iCurImagePos + 1);

	QString strSep("");

	for (int i = 0; i < (strTotImages.length() - strNumImage.length()); i++)
		strSep += " ";

	LabelCurPicNum->setText(strSep + strNumImage + "/" + strTotImages);

	/* If no picture was received, show the following text */
	if (iCurImagePos < 0)
	{
		/* Init text browser window */
		image->setText("<center>" + tr("MOT Slideshow Viewer") + "</center>");
	}
}

