/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2001-2014
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
#include <../datadecoding/DataDecoder.h>
#include <../util-QT/Util.h>
#include <QFontDialog>
#include "ThemeCustomizer.h"

JLViewer::JLViewer(CSettings& Settings, QWidget* parent):
    CWindow(parent, Settings, "Journaline")
{
    setupUi(this);

    string p = Settings.Get(
                "Receiver", "datafilesdirectory", string(DEFAULT_DATA_FILES_DIRECTORY));

    strCurrentSavePath = QString::fromUtf8((p+PATH_SEPARATOR+"Journaline").c_str());

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));
    connect(actionClose, SIGNAL(triggered()), SLOT(close()));
    connect(ButtonStepBack, SIGNAL(clicked()), textBrowser, SLOT(backward()));
    connect(textBrowser, SIGNAL(backwardAvailable(bool)), ButtonStepBack, SLOT(setEnabled(bool)));

    textBrowser->setDocument(&document);

    /* Update time for color LED */
    LEDStatus->SetUpdateTime(1000);

    on_actionClear_All_triggered();

    APPLY_CUSTOM_THEME();
}

JLViewer::~JLViewer()
{
}

void JLViewer::setDecoder(CDataDecoder* dec)
{
    textBrowser->setDecoder(dec);
}

void JLViewer::setServiceInformation(const CService& service, uint32_t iAudioServiceID)
{
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
}

void JLViewer::setStatus(int, ETypeRxStatus status)
{
    SetStatus(LEDStatus, status);

    if(textBrowser->changed())
    {
        textBrowser->reload();
    }
}

void JLViewer::eventShow(QShowEvent*)
{
    /* Store the default font */
    QFont fontDefault = textBrowser->font();

    /* Retrieve the font setting saved into the .ini file */
    const QString strFontFamily = getSetting("fontfamily", QString());
    if (strFontFamily != "")
    {
        QFont fontTextBrowser = QFont(strFontFamily,
                                      getSetting("fontpointsize", 0),
                                      getSetting("fontweight", 0),
                                      getSetting("fontitalic", false));
        textBrowser->setFont(fontTextBrowser);
    }

    textBrowser->setSource(QUrl("0"));
}

void JLViewer::eventHide(QHideEvent*)
{
    /* Store current textBrowser font */
    QFont fontTextBrowser = textBrowser->currentFont();
    putSetting("fontfamily", fontTextBrowser.family());
    putSetting("fontpointsize", fontTextBrowser.pointSize());
    putSetting("fontweight", fontTextBrowser.weight());
    putSetting("fontitalic", fontTextBrowser.italic());
}

void JLViewer::on_actionSave_triggered()
{
}

void JLViewer::on_actionSave_All_triggered()
{
}

void JLViewer::on_actionClear_All_triggered()
{
    actionClear_All->setEnabled(false);
    actionSave->setEnabled(false);
    actionSave_All->setEnabled(false);
    ButtonStepBack->setEnabled(false);
    textBrowser->clear();
    textBrowser->clearHistory();
    // TODO - clear JL object cache ?
}

void JLViewer::on_actionSet_Font_triggered()
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
