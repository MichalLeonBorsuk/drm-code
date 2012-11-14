/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description: Journaline Viewer
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

#include "JLViewer.h"
#include "jlbrowser.h"
#include "../util/Settings.h"
#include <../datadecoding/DataDecoder.h>
#include <QFontDialog>

JLViewer::JLViewer(CDRMReceiver& rec, CSettings& s, QWidget* parent,
                   const char* name, Qt::WFlags f):
    QDialog(parent), Ui_JLViewer(), Timer(),
    receiver(rec), settings(s), decoderSet(false)
{
    (void)name; (void)f;
    /* Enable minimize and maximize box for QDialog */
	setWindowFlags(Qt::Window);

    setupUi(this);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));
    textBrowser->setDocument(&document);

    connect(actionClear_All, SIGNAL(triggered()), SLOT(OnClearAll()));
    connect(actionSave, SIGNAL(triggered()), SLOT(OnSave()));
    connect(actionSave_All, SIGNAL(triggered()), SLOT(OnSaveAll()));
    connect(actionClose, SIGNAL(triggered()), SLOT(close()));
    connect(actionSet_Font, SIGNAL(triggered()), SLOT(OnSetFont()));

    /* Update time for color LED */
    LEDStatus->SetUpdateTime(1000);

    /* Connect controls */
    connect(ButtonStepBack, SIGNAL(clicked()), this, SLOT(OnButtonStepBack()));
    connect(textBrowser, SIGNAL(backwardAvailable(bool)), ButtonStepBack, SLOT(setEnabled(bool)));
    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

    OnClearAll();

    Timer.stop();
}

JLViewer::~JLViewer()
{
}

void JLViewer::showEvent(QShowEvent*)
{
    /* Get window geometry data and apply it */
    CWinGeom g;
    settings.Get("Journaline", g);
    const QRect WinGeom(g.iXPos, g.iYPos, g.iWSize, g.iHSize);

    if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
        setGeometry(WinGeom);

    strCurrentSavePath = settings.Get("Journaline", "storagepath", strCurrentSavePath);

    /* Store the default font */
    QFont fontDefault = textBrowser->font();

    /* Retrieve the font setting saved into the .ini file */
    string strFontFamily = settings.Get("Journaline", "fontfamily");
    if (strFontFamily != "")
    {
        QFont fontTextBrowser = QFont(QString(strFontFamily.c_str()),
                                      settings.Get("Journaline", "fontpointsize", 0),
                                      settings.Get("Journaline", "fontweight", 0),
                                      settings.Get("Journaline", "fontitalic", 0));
        textBrowser->setFont(fontTextBrowser);
    }

    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
    const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
    const uint32_t iAudioServiceID = Parameters.Service[iCurSelAudioServ].iServiceID;

    /* Get current data service */
    int shortID = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[shortID];
    Parameters.Unlock();

    CDataDecoder* dec = receiver.GetDataDecoder();
    if(dec)
    {
        textBrowser->setDecoder(dec);
        decoderSet = true;
    }
    textBrowser->setSource(QUrl("0"));

    /* Add the service description into the dialog caption */
    QString strTitle = tr("Journaline");

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

void JLViewer::hideEvent(QHideEvent*)
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
    settings.Put("Journaline", c);

    /* Store save path */
    settings.Put("Journaline","storagepath", strCurrentSavePath);

    QFont fontTextBrowser = textBrowser->currentFont();
    /* Store current textBrowser font */
    settings.Put("Journaline","fontfamily", fontTextBrowser.family().toStdString());
    settings.Put("Journaline","fontpointsize", fontTextBrowser.pointSize());
    settings.Put("Journaline","fontweight", fontTextBrowser.weight());
    settings.Put("Journaline","fontitalic", fontTextBrowser.italic());
}

void JLViewer::OnTimer()
{
    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
    ETypeRxStatus status = Parameters.ReceiveStatus.MOT.GetStatus();

    /* Get current data service */
    int shortID = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[shortID];
    Parameters.Unlock();

    if(!decoderSet)
    {
        CDataDecoder* dec = receiver.GetDataDecoder();
        if(dec)
        {
            textBrowser->setDecoder(dec);
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
}

void JLViewer::OnButtonStepBack()
{
    textBrowser->backward();
}

void JLViewer::OnSave()
{
}

void JLViewer::OnSaveAll()
{
}

void JLViewer::OnClearAll()
{
    actionClear_All->setEnabled(false);
    actionSave->setEnabled(false);
    actionSave_All->setEnabled(false);
    ButtonStepBack->setEnabled(false);
    textBrowser->clear();
    textBrowser->clearHistory();
    // TODO - clear JL object cache ?
}

void JLViewer::OnSetFont()
{
    bool bok;

    /* Open the font dialog */
    QFont newFont = QFontDialog::getFont(&bok, textBrowser->currentFont(), this);

    if (bok == true)
    {
        /* Store the current text and then reset it */
        QString strOldText = textBrowser->toHtml();
        textBrowser->setText("<br>");

        textBrowser->setFont(newFont);

        /* Restore the text to refresh it with the new font */
        textBrowser->setText(strOldText);
    }
}
