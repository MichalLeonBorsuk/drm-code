/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo, Julian Cable
 *
 * 6/8/2005 Andrea Russo
 *	- save Journaline pages as HTML
 * 6/16/2005 Andrea Russo
 *	- save path for storing pictures or Journaline pages
 * 11/17/2005 Andrea Russo
 * - BroadcastWebSite implementation
 * 11/29/2005 Andrea Russo
 * - set and save the TextBrowser font
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

#ifdef _WIN32
# include <windows.h>
#endif
#include "MultimediaDlg.h"
#include "../datadecoding/Journaline.h"
#include <qtextbrowser.h>
#include <qpopupmenu.h>
#include <qfiledialog.h>
#include <qstylesheet.h>
#include <qtextstream.h>

/* Implementation *************************************************************/
MultimediaDlg::MultimediaDlg(CDRMReceiver& DRMReceiver, CSettings& Settings,
                             QWidget* parent, const char* name, bool modal, Qt::WFlags f):
    MultimediaDlgBase(parent, name, modal, f), Settings(Settings),
    Parameters(*DRMReceiver.GetParameters()), DataDecoder(*DRMReceiver.GetDataDecoder()),
    JournalineDecoder(), bGetFromFile(false)
{
    /* Load settings */
    LoadSettings(Settings);

    QString fhgLogo, jlLogo;
    /* Add body's stylesheet */

    QStyleSheetItem* styleBody =
        new QStyleSheetItem(TextBrowser->styleSheet(), "stylebody");

    styleBody->setFontFamily(fontTextBrowser.family());
    styleBody->setFontSize(fontTextBrowser.pointSize());
    styleBody->setFontWeight(fontTextBrowser.weight());
    styleBody->setFontItalic(fontTextBrowser.italic());

    /* Picture controls should be invisable. These controls are only used for
       storing the resources */
    PixmapFhGIIS->hide();
    PixmapLogoJournaline->hide();

    /* Set pictures in source factory */
    QMimeSourceFactory::defaultFactory()->setImage("PixmapFhGIIS",
            PixmapFhGIIS->pixmap()->convertToImage());
    QMimeSourceFactory::defaultFactory()->setImage("PixmapLogoJournaline",
            PixmapLogoJournaline->pixmap()->convertToImage());

    fhgLogo = "PixmapFhGIIS";
    jlLogo = "PixmapLogoJournaline";

    /* Set FhG IIS text */
    strFhGIISText =
        "<table><tr><td><img source=\""+fhgLogo+"\"></td>"
        "<td><font face=\"" + QString(FONT_COURIER) + "\" size=\"-1\">Features NewsService "
        "Journaline(R) decoder technology by Fraunhofer IIS, Erlangen, "
        "Germany. For more information visit http://www.iis.fhg.de/dab"
        "</font></td></tr></table>";

    /* Set Journaline headline text */
    strJournalineHeadText =
        "<table><tr><td><img source=\""+jlLogo+"\"></td>"
        "<td valign=\"middle\"><h2>NewsService Journaline" + QString(QChar(174)) /* (R) */ +
        "</h2></td></tr></table>";

    /* Inits for broadcast website application */
    strBWSHomePage = "";

    /* Set Menu ***************************************************************/
    /* File menu ------------------------------------------------------------ */
    pFileMenu = new QPopupMenu(this);
    CHECK_PTR(pFileMenu);
    pFileMenu->insertItem(tr("C&lear all"), this, SLOT(OnClearAll()),
                          Qt::CTRL+Qt::Key_X, 0);
    pFileMenu->insertSeparator();
    pFileMenu->insertItem(tr("&Load..."), this, SLOT(OnLoad()), Qt::CTRL+Qt::Key_L, 1);
    pFileMenu->insertItem(tr("&Save..."), this, SLOT(OnSave()), Qt::CTRL+Qt::Key_S, 2);
    pFileMenu->insertItem(tr("Save &all..."), this, SLOT(OnSaveAll()), Qt::CTRL+Qt::Key_A, 3);
    pFileMenu->insertSeparator();
    pFileMenu->insertItem(tr("&Close"), this, SLOT(close()), 0, 4);


    /* Settings menu  ------------------------------------------------------- */
    QPopupMenu* pSettingsMenu = new QPopupMenu(this);
    CHECK_PTR(pSettingsMenu);
    pSettingsMenu->insertItem(tr("Set &Font..."), this, SLOT(OnSetFont()));


    /* Main menu bar -------------------------------------------------------- */
    pMenu = new QMenuBar(this);
    CHECK_PTR(pMenu);
    pMenu->insertItem(tr("&File"), pFileMenu);
    pMenu->insertItem(tr("&Settings"), pSettingsMenu);

    /* Now tell the layout about the menu */
    MultimediaDlgBaseLayout->setMenuBar(pMenu);

    /* Update time for color LED */
    LEDStatus->SetUpdateTime(1000);

    /* Init slide-show (needed for setting up vectors and indices) */
    ClearAllSlideShow();

    /* Init container and GUI */
    InitApplication(DataDecoder.GetAppType());


    /* Connect controls */
    connect(buttonOk, SIGNAL(clicked()),
            this, SLOT(close()));
    connect(PushButtonStepBack, SIGNAL(clicked()),
            this, SLOT(OnButtonStepBack()));
    connect(PushButtonStepForw, SIGNAL(clicked()),
            this, SLOT(OnButtonStepForw()));
    connect(PushButtonJumpBegin, SIGNAL(clicked()),
            this, SLOT(OnButtonJumpBegin()));
    connect(PushButtonJumpEnd, SIGNAL(clicked()),
            this, SLOT(OnButtonJumpEnd()));
    connect(TextBrowser, SIGNAL(textChanged()),
            this, SLOT(OnTextChanged()));

    connect(&Timer, SIGNAL(timeout()),
            this, SLOT(OnTimer()));
}

MultimediaDlg::~MultimediaDlg()
{
}

void MultimediaDlg::InitApplication(CDataDecoder::EAppType eNewAppType)
{
    /* Set internal parameter */
    eAppType = eNewAppType;

    /* Actual inits */
    switch (eAppType)
    {
    case CDataDecoder::AT_MOTSLIDESHOW:
        InitMOTSlideShow();
        break;

    case CDataDecoder::AT_JOURNALINE:
        InitJournaline();
        break;

    case CDataDecoder::AT_BROADCASTWEBSITE:
        InitBroadcastWebSite();
        break;

    default:
        InitNotSupported();
        break;
    }
}

void MultimediaDlg::OnTextChanged()
{
    /* Check, if the current text is a link ID or regular text */
    if (TextBrowser->text().compare(TextBrowser->text().left(1), "<") != 0)
    {
        /* Save old ID */
        NewIDHistory.Add(iCurJourObjID);

        /* Set text to news ID text which was selected by the user */
        iCurJourObjID = TextBrowser->text().toInt();
        SetJournalineText();

        /* Enable back button */
        PushButtonStepBack->setEnabled(TRUE);
    }
}

void MultimediaDlg::OnTimer()
{
    CMOTObject	NewObj;
    QPixmap		NewImage;
    int			iCurNumPict;
    _BOOLEAN	bMainPage;
    _BOOLEAN	bShowInfo = TRUE;
    CDataDecoder::EAppType eNewAppType;

    if(bGetFromFile)
    {
        eNewAppType = CDataDecoder::AT_JOURNALINE;
    }
    else
    {
        Parameters.Lock();
        ETypeRxStatus status = Parameters.ReceiveStatus.MOT.GetStatus();
        Parameters.Unlock();
        switch(status)
        {
        case NOT_PRESENT:
            LEDStatus->Reset(); /* GREY */
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

        /* Check out which application is transmitted right now */
        eNewAppType = DataDecoder.GetAppType();

    }

    if (eNewAppType != eAppType)
        InitApplication(eNewAppType);

    switch (eAppType)
    {
    case CDataDecoder::AT_MOTSLIDESHOW:
        /* Poll the data decoder module for new picture */
        if (DataDecoder.GetMOTObject(NewObj, eAppType) == TRUE)
        {
            /* Store received picture */
            iCurNumPict = vecRawImages.Size();
            vecRawImages.Add(NewObj);

            /* If the last received picture was selected, automatically show
               new picture */
            if (iCurImagePos == iCurNumPict - 1)
            {
                iCurImagePos = iCurNumPict;
                SetSlideShowPicture();
            }
            else
                UpdateAccButtonsSlideShow();
        }
        break;

    case CDataDecoder::AT_BROADCASTWEBSITE:
        /* Poll the data decoder module for new object */
        if (DataDecoder.GetMOTObject(NewObj, eAppType) == TRUE)
        {
            /* Store received MOT object on disk */
            const QString strNewObjName = VerifyFilename(NewObj.strName.c_str());
            const QString strFileName =
                QString(strCurrentSavePath + strNewObjName);

            SaveMOTObject(strFileName, NewObj);

            /* Check if DABMOT could not unzip */
            const _BOOLEAN bZipped =
                (strNewObjName.right(3) == ".gz");

            /* Add an html header to refresh the page every n seconds */
            if (bAddRefresh && (NewObj.strFormat == "html") && (bZipped == FALSE))
                AddRefreshHeader(strFileName);

            /* Check if is the main page */
            bMainPage = FALSE;

            if (strNewObjName.contains('/') == 0) /* if has a path is not the main page */
            {
                /* Get the current directory */
                CMOTDirectory MOTDir;

                if (DataDecoder.GetMOTDirectory(MOTDir, eAppType) == TRUE)
                {
                    /* Checks if the DirectoryIndex has values */
                    if (MOTDir.DirectoryIndex.size() > 0)
                    {
                        /* Check if is the main page */

                        QString strIndexPage("");

                        if(MOTDir.DirectoryIndex.find(UNRESTRICTED_PC_PROFILE) != MOTDir.DirectoryIndex.end())
                            strIndexPage =
                                MOTDir.DirectoryIndex[UNRESTRICTED_PC_PROFILE].c_str();
                        else if(MOTDir.DirectoryIndex.find(BASIC_PROFILE) != MOTDir.DirectoryIndex.end())
                            strIndexPage =
                                MOTDir.DirectoryIndex[BASIC_PROFILE].c_str();

                        if ((strIndexPage == strNewObjName) ||
                                (strIndexPage + ".gz" == strNewObjName))
                            bMainPage = TRUE;
                    }
                    else
                    {
                        /* we have the directory but there is nothing in the directory */
                        bMainPage = (strNewObjName.left(9).upper() == "INDEX.HTM");
                    }
                }
            }

            if (bMainPage == TRUE)
            {
                /* The home page is available */
                if (bZipped == FALSE)
                {
                    /* Set new homepage file name and init dialog */
                    strBWSHomePage = strNewObjName;
                    InitBroadcastWebSite();
                }
                else
                {
                    TextBrowser->setText("<center><h2>" +
                                         tr("MOT Broadcast Web Site")
                                         + "</h2><br>"
                                         + tr("The home page is available")
                                         + "<br><br>"
                                         + tr("Impossible to uncompress the home page.<br>"
                                              "For uncompress data compile Dream with zlib or Freeimage.<br>"
                                              "Compress files will be saved on disk here:<br>" +
                                              strCurrentSavePath) + "</center>");
                }
            }
        }
        break;

    case CDataDecoder::AT_JOURNALINE:
        SetJournalineText();
        break;

    default:
        bShowInfo = FALSE;
        break;
    }

    /* Add the service description into the dialog caption */
    QString strTitle = tr("Multimedia");

    if (bShowInfo == TRUE)
    {
        Parameters.Lock();
        const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
        const uint32_t iAudioServiceID = Parameters.Service[iCurSelAudioServ].iServiceID;

        /* Get current data service */
        const int iCurSelDataServ = Parameters.GetCurSelDataService();
        CService service = Parameters.Service[iCurSelDataServ];
        Parameters.Unlock();


        if (service.IsActive())
        {
            /* Do UTF-8 to QString (UNICODE) conversion with the label strings */
            QString strLabel = QString().fromUtf8(service.strLabel.c_str()).stripWhiteSpace();


            /* Service ID (plot number in hexadecimal format) */
            QString strServiceID = "";

            /* show the ID only if differ from the audio service */
            if ((service.iServiceID != 0) && (service.iServiceID != iAudioServiceID))
            {
                if (strLabel != "")
                    strLabel += " ";

                strServiceID = "- ID:" +
                               QString().setNum(long(service.iServiceID), 16).upper();
            }

            /* add the description on the title of the dialog */
            if (strLabel != "" || strServiceID != "")
                strTitle += " [" + strLabel + strServiceID + "]";
        }
    }
    SetDialogCaption(this, strTitle);
}

void MultimediaDlg::ExtractJournalineBody(const int iCurJourID,
        const _BOOLEAN bHTMLExport,
        QString& strTitle, QString& strItems)
{
    /* Get news from actual Journaline decoder */
    CNews News;
    if(bGetFromFile)
        JournalineDecoder.GetNews(iCurJourID, News);
    else
        DataDecoder.GetNews(iCurJourID, News);

    /* Decode UTF-8 coding for title */
    strTitle = QString().fromUtf8(QCString(News.sTitle.c_str()));

    strItems = "";
    for (int i = 0; i < News.vecItem.Size(); i++)
    {
        QString strCurItem;
        if (bHTMLExport == FALSE)
        {
            /* Decode UTF-8 coding of this item text */
            strCurItem = QString().fromUtf8(
                             QCString(News.vecItem[i].sText.c_str()));
        }
        else
        {
            /* In case of HTML export, do not decode UTF-8 coding */
            strCurItem = News.vecItem[i].sText.c_str();
        }

        /* Replace \n by html command <br> */
        strCurItem = strCurItem.replace(QRegExp("\n"), "<br>");

        if (News.vecItem[i].iLink == JOURNALINE_IS_NO_LINK)
        {
            /* Only text, no link */
            strItems += strCurItem + QString("<br>");
        }
        else if (News.vecItem[i].iLink == JOURNALINE_LINK_NOT_ACTIVE)
        {
            /* Un-ordered list item without link */
            strItems += QString("<li>") + strCurItem + QString("</li>");
        }
        else
        {
            if (bHTMLExport == FALSE)
            {
                QString strLinkStr = QString().setNum(News.vecItem[i].iLink);

                /* Un-ordered list item with link */
                strItems += QString("<li><a href=\"") + strLinkStr +
                            QString("\">") + strCurItem +
                            QString("</a></li>");

                /* Store link location in factory (stores ID) */
                QMimeSourceFactory::defaultFactory()->setText(strLinkStr, strLinkStr);
            }
            else
                strItems += QString("<li>") + strCurItem + QString("</li>");
        }
    }
}

void MultimediaDlg::SetJournalineText()
{
    /* Get title and body with html links */
    QString strTitle("");
    QString strItems("");
    ExtractJournalineBody(iCurJourObjID, FALSE, strTitle, strItems);

    /* Set html text. Standard design. The first character must be a "<". This
       is used to identify whether normal text is displayed or an ID was set */
    QString strAllText =
        strJournalineHeadText +
        "<table>"
        "<tr><td><hr></td></tr>" /* horizontial line */
        "<tr><td><stylebody><b><center>" + strTitle + "</center></b></stylebody></td></tr>"
        "<tr><td><stylebody><ul type=\"square\">" + strItems +
        "</ul></stylebody></td></tr>"
        "<tr><td><hr></td></tr>" /* horizontial line */
        "</table>"
        + strFhGIISText;
//QMessageBox::information( this, "Dream", strAllText );
    /* Only update text browser if text has changed */
    if (TextBrowser->text().compare(strAllText) != 0)
        TextBrowser->setText(strAllText);

    /* Enable / disable "save" menu item if title is present or not */
    if (strTitle == "")
        pFileMenu->setItemEnabled(2, FALSE);
    else
        pFileMenu->setItemEnabled(2, TRUE);
}

void MultimediaDlg::LoadSettings(const CSettings& Settings)
{
    /* Get window geometry data and apply it */
    CWinGeom s;
    Settings.Get("Multimedia Dialog", s);
    const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);

    if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
        setGeometry(WinGeom);

    /* Store the default font */
    fontDefault = TextBrowser->font();

    /* Retrieve the setting saved into the .ini file */
    strCurrentSavePath = QString::fromUtf8(Parameters.GetDataDirectory("MOT").c_str());

    /* Retrieve the font setting saved into the .ini file */
    string strFontFamily = Settings.Get("Multimedia Dialog", "fontfamily");
    if (strFontFamily != "")
    {
        fontTextBrowser =
            QFont(QString(strFontFamily.c_str()),
                  Settings.Get("Multimedia Dialog", "fontpointsize", 0),
                  Settings.Get("Multimedia Dialog", "fontweight", 0),
                  Settings.Get("Multimedia Dialog", "fontitalic", 0));
    }
    else
    {
        /* If not defined, retrieve the default font */
        fontTextBrowser = fontDefault;
    }

    bAddRefresh = Settings.Get("Multimedia Dialog", "addrefresh", TRUE);
    iRefresh = Settings.Get("Multimedia Dialog", "motbwsrefresh", 10);

}

void MultimediaDlg::SaveSettings(CSettings& Settings)
{
    /* Save window geometry data */
    QRect WinGeom = geometry();

    CWinGeom c;
    c.iXPos = WinGeom.x();
    c.iYPos = WinGeom.y();
    c.iHSize = WinGeom.height();
    c.iWSize = WinGeom.width();
    Settings.Put("Multimedia Dialog", c);

    /* Store current TextBrowser font */
    Settings.Put("Multimedia Dialog","fontfamily", fontTextBrowser.family().latin1());
    Settings.Put("Multimedia Dialog","fontpointsize", fontTextBrowser.pointSize());
    Settings.Put("Multimedia Dialog","fontweight", fontTextBrowser.weight());
    Settings.Put("Multimedia Dialog","fontitalic", fontTextBrowser.italic());
    Settings.Put("Multimedia Dialog", "addrefresh", bAddRefresh);
    Settings.Put("Multimedia Dialog", "motbwsrefresh", iRefresh);
}

void MultimediaDlg::showEvent(QShowEvent*)
{
    /* Update window */
    OnTimer();

    /* Activate real-time timer when window is shown */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void MultimediaDlg::hideEvent(QHideEvent*)
{
    /* Deactivate real-time timer so that it does not get new pictures */
    Timer.stop();

}

void MultimediaDlg::OnButtonStepBack()
{
    switch (eAppType)
    {
    case CDataDecoder::AT_MOTSLIDESHOW:
        iCurImagePos--;
        SetSlideShowPicture();
        break;

    case CDataDecoder::AT_JOURNALINE:
        /* Step one level back, get ID from history */
        iCurJourObjID = NewIDHistory.Back();

        /* If root ID is reached, disable back button */
        if (iCurJourObjID == 0)
            PushButtonStepBack->setEnabled(FALSE);

        SetJournalineText();
        break;

    default:
        break;
    }
}

void MultimediaDlg::OnButtonStepForw()
{
    /* Different behaviour for slideshow and broadcast web site */
    switch (eAppType)
    {
    case CDataDecoder::AT_MOTSLIDESHOW:
        iCurImagePos++;
        SetSlideShowPicture();
        break;

    case CDataDecoder::AT_BROADCASTWEBSITE:
        /* Try to open browser */
        if (!openBrowser(this, strCurrentSavePath + strBWSHomePage))
        {
            QMessageBox::information(this, "Dream",
                                     tr("Failed to start the default browser.\n"
                                        "Open the home page:\n" + strCurrentSavePath +
                                        strBWSHomePage + "\nmanually."), QMessageBox::Ok);
        }
        break;

    default:
        break;
    }
}

void MultimediaDlg::OnButtonJumpBegin()
{
    /* Reset current picture number to zero (begin) */
    iCurImagePos = 0;
    SetSlideShowPicture();
}

void MultimediaDlg::OnButtonJumpEnd()
{
    /* Go to last received picture */
    iCurImagePos = GetIDLastPicture();
    SetSlideShowPicture();
}

QImage* JL2Image(CVector<_BYTE>& imagedata)
{
    QImage* pNewImage = new QImage();
    if(pNewImage->loadFromData(&imagedata[0], imagedata.size()))
        return pNewImage;
    else
    {
        delete pNewImage;
        return NULL;
    }
}

void MultimediaDlg::SetSlideShowPicture()
{
    /* Copy current image from image storage vector */
    CMOTObject motObject(vecRawImages[iCurImagePos]);

    QString imagename(motObject.strName.c_str());
    QString imageformat(motObject.strFormat.c_str());

    /* The standard version of QT2 does not have jpeg support, if FreeImage
       library is installed, the following routine converts the picture to
       png which can be displayed */
    if (imageformat.compare("jpeg") == 0)
        JpgToPng(motObject);

    QImage *pImage = JL2Image(motObject.Body.vecData);

    if(pImage)
    {
        /* The slideshow pictures are not
           updated correctly without this line: */
        /* If the text is empty there is segmentation fault
        	 browsing the images */

        TextBrowser->setText("<br>");

        /* Set new picture in source factory and set it in text control */
        QMimeSourceFactory::defaultFactory()->setImage("MOTSlideShowimage", *pImage);
        TextBrowser->setText("<center><img source=\"MOTSlideShowimage\"></center>");
        delete pImage;
    }
    else
    {
        /* Show text that tells the user of load failure */
        TextBrowser->setText("<br><br><center><b>" + tr("Image could not be loaded, ") +
                             imageformat +
                             tr("-format not supported by this version of QT!") +
                             "</b><br><br><br>" + tr("If you want to view the image, "
                                     "save it to file and use an external viewer") + "</center>");
    }

    /* Remove previous tool tip */
    QToolTip::remove(TextBrowser);

    /* Add tool tip showing the name of the picture */
    if (imagename.length() != 0)
        QToolTip::add(TextBrowser, imagename);

    UpdateAccButtonsSlideShow();
}

void MultimediaDlg::UpdateAccButtonsSlideShow()
{
    /* Set enable menu entry for saving a picture */
    if (iCurImagePos < 0)
    {
        pFileMenu->setItemEnabled(0, FALSE);
        pFileMenu->setItemEnabled(2, FALSE);
        pFileMenu->setItemEnabled(3, FALSE);
    }
    else
    {
        pFileMenu->setItemEnabled(0, TRUE);
        pFileMenu->setItemEnabled(2, TRUE);
        pFileMenu->setItemEnabled(3, TRUE);
    }

    if (iCurImagePos <= 0)
    {
        /* We are already at the beginning */
        PushButtonStepBack->setEnabled(FALSE);
        PushButtonJumpBegin->setEnabled(FALSE);
    }
    else
    {
        PushButtonStepBack->setEnabled(TRUE);
        PushButtonJumpBegin->setEnabled(TRUE);
    }

    if (iCurImagePos == GetIDLastPicture())
    {
        /* We are already at the end */
        PushButtonStepForw->setEnabled(FALSE);
        PushButtonJumpEnd->setEnabled(FALSE);
    }
    else
    {
        PushButtonStepForw->setEnabled(TRUE);
        PushButtonJumpEnd->setEnabled(TRUE);
    }

    QString strTotImages = QString().setNum(GetIDLastPicture() + 1);
    QString strNumImage = QString().setNum(iCurImagePos + 1);

    QString strSep("");

    for (size_t i = 0; i < (strTotImages.length() - strNumImage.length()); i++)
        strSep += " ";

    LabelCurPicNum->setText(strSep + strNumImage + "/" + strTotImages);

    /* If no picture was received, show the following text */
    if (iCurImagePos < 0)
    {
        /* Init text browser window */
        TextBrowser->setText("<center><h2>" +
                             tr("MOT Slideshow Viewer") + "</h2></center>");
    }
}

void MultimediaDlg::OnLoad()
{
    QString strFileName;
    strFileName =
        QFileDialog::getOpenFileName("", "*.jml", this);
    if(strFileName != "")
    {
        bGetFromFile = true;
        JournalineDecoder.AddFile(strFileName.latin1());
        InitApplication(CDataDecoder::AT_JOURNALINE);
    }
}

void MultimediaDlg::OnSave()
{
    QString strFileName;
    QString strDefFileName;
    QString strExt;
    QString strFilter;

    switch (eAppType)
    {
    case CDataDecoder::AT_MOTSLIDESHOW:

        strExt = QString(vecRawImages[iCurImagePos].strFormat.c_str());

        if (strExt.length() == 0)
            strFilter = "*.*";
        else
            strFilter = "*." + strExt;

        /* Show "save file" dialog */
        /* Set file name */
        strDefFileName = VerifyFilename(vecRawImages[iCurImagePos].strName.c_str());

        /* Use default file name if no file name was transmitted */
        if (strDefFileName.length() == 0)
            strDefFileName = "RecPic";

        if ((strDefFileName.contains(".") == 0) && (strExt.length() > 0))
            strDefFileName += "." + strExt;

        strFileName =
            QFileDialog::getSaveFileName(strCurrentSavePath + strDefFileName,
                                         strFilter, this);

        /* Check if user not hit the cancel button */
        if (!strFileName.isEmpty())
            SaveMOTObject(strFileName, vecRawImages[iCurImagePos]);
        break;

    case CDataDecoder::AT_JOURNALINE:
    {
        /* Save to file current journaline page */
        QString strTitle("");
        QString strItems("");

        /* TRUE = without html links */
        ExtractJournalineBody(iCurJourObjID, TRUE, strTitle, strItems);

        /* Prepare HTML page for storing the content (header, body tags, etc) */
        QString strJornalineText = "<html>\n<head>\n"
                                   "<meta http-equiv=\"content-Type\" "
                                   "content=\"text/html; charset=utf-8\">\n<title>" + strTitle +
                                   "</title>\n</head>\n\n<body>\n<table>\n"
                                   "<tr><th>" + strTitle + "</th></tr>\n"
                                   "<tr><td><ul type=\"square\">" + strItems + "</ul></td></tr>\n"
                                   "</table>\n"
                                   /* Add current date and time */
                                   "<br><p align=right><font size=-2><i>" +
                                   QDateTime().currentDateTime().toString() + "</i></font></p>"
                                   "</body>\n</html>";

        strFileName = QFileDialog::getSaveFileName(strCurrentSavePath +
                      strTitle + ".html", "*.html", this);

        if (!strFileName.isEmpty())
        {
            /* Save Journaline page as a text stream */
            QFile FileObj(strFileName);

            if (FileObj.open(IO_WriteOnly))
            {
                QTextStream textStream(&FileObj);
                textStream << strJornalineText; /* Actual writing */
                FileObj.close();
            }
        }
    }
    break;

    default:
        break;
    }
}

void MultimediaDlg::OnSaveAll()
{
    /* Let the user choose a directory */
    QString strDirName =
        QFileDialog::getExistingDirectory(NULL, this);

    if (!strDirName.isEmpty())
    {
        /* add slashes if not present */
        if (strDirName.right(1) != "/")
            strDirName += "/";

        /* Loop over all pictures received yet */
        for (int j = 0; j < GetIDLastPicture() + 1; j++)
        {
            const CMOTObject& obj = vecRawImages[j];
            QString strFileName = VerifyFilename(obj.strName.c_str());
            QString strExt = QString(obj.strFormat.c_str());

            if (strFileName.length() == 0)
            {
                /* Construct file name from date and picture number (default) */
                strFileName = "Dream_" + QDate().currentDate().toString() +
                              "_#" + QString().setNum(j);
            }

            /* Add directory and ending */
            strFileName = strDirName + strFileName;

            if ((strFileName.contains(".") == 0) && (strExt.length() > 0))
                strFileName += "." + strExt;

            SaveMOTObject(strFileName, obj);
        }
    }
}

void MultimediaDlg::ClearAllSlideShow()
{
    /* Init vector which will store the received images with zero size */
    vecRawImages.Init(0);

    /* Init current image position */
    iCurImagePos = -1;

    /* Update GUI */
    UpdateAccButtonsSlideShow();

    /* Remove tool tips */
    QToolTip::remove(TextBrowser);
}

void MultimediaDlg::InitNotSupported()
{
    /* Hide all controls, disable menu items */
    pFileMenu->setItemEnabled(0, FALSE);
    pFileMenu->setItemEnabled(1, TRUE);
    pFileMenu->setItemEnabled(2, FALSE);
    pFileMenu->setItemEnabled(3, FALSE);
    PushButtonStepForw->hide();
    PushButtonJumpBegin->hide();
    PushButtonJumpEnd->hide();
    LabelCurPicNum->hide();
    PushButtonStepBack->hide();
    QToolTip::remove(TextBrowser);

    /* Show that application is not supported */
    TextBrowser->setText("<center><h2>" + tr("No data service or data service "
                         "not supported.") + "</h2></center>");
}

void MultimediaDlg::InitBroadcastWebSite()
{
    /* Hide all controls, disable menu items */
    pFileMenu->setItemEnabled(0, FALSE);
    pFileMenu->setItemEnabled(1, FALSE);
    pFileMenu->setItemEnabled(2, FALSE);
    pFileMenu->setItemEnabled(3, FALSE);
    PushButtonStepForw->show();
    PushButtonStepForw->setEnabled(FALSE);
    PushButtonJumpBegin->hide();
    PushButtonJumpEnd->hide();
    LabelCurPicNum->hide();
    PushButtonStepBack->hide();
    QToolTip::remove(TextBrowser);

    if (strBWSHomePage != "")
    {
        /* This is the button for opening the browser */
        PushButtonStepForw->setEnabled(TRUE);

        /* Display text that index page was received an can be opened */
        TextBrowser->setText("<center><h2>" + tr("MOT Broadcast Web Site")
                             + "</h2><br>"
                             + tr("The homepage is available.")
                             + "<br><br>" +
                             tr("Press the button to open it in the default browser.")
                             + "</center>");
    }
    else
    {
        /* Show initial text */
        TextBrowser->setText("<center><h2>" + tr("MOT Broadcast Web Site") +
                             "</h2></center>");

        /* Create the cache directory if not exist */
		CreateDirectories(strCurrentSavePath);
    }
}

void MultimediaDlg::InitMOTSlideShow()
{
    /* Make all browse buttons visible */
    PushButtonStepBack->show();
    PushButtonStepForw->show();
    PushButtonJumpBegin->show();
    PushButtonJumpEnd->show();
    LabelCurPicNum->show();

    /* Set current image position to the last picture and display it (if at
       least one picture is available) */
    iCurImagePos = GetIDLastPicture();
    if (iCurImagePos >= 0)
        SetSlideShowPicture();
    else
    {
        /* Remove tool tips */
        QToolTip::remove(TextBrowser);
    }

    /* Update buttons and menu */
    UpdateAccButtonsSlideShow();
}

void MultimediaDlg::InitJournaline()
{
    /* Disable "clear all" menu item */
    pFileMenu->setItemEnabled(0, FALSE);

    /* Enable "load" menu items */
    pFileMenu->setItemEnabled(1, TRUE);

    /* Disable "save" menu items */
    pFileMenu->setItemEnabled(2, FALSE);
    pFileMenu->setItemEnabled(3, FALSE);

    /* Only one back button is visible and enabled */
    PushButtonStepForw->hide();
    PushButtonJumpBegin->hide();
    PushButtonJumpEnd->hide();
    LabelCurPicNum->hide();

    /* Show back button and disable it because we always start at the root
       object */
    PushButtonStepBack->show();
    PushButtonStepBack->setEnabled(FALSE);

    /* Init text browser window */
    iCurJourObjID = 0;
    SetJournalineText();

    /* Remove tool tips */
    QToolTip::remove(TextBrowser);

    NewIDHistory.Reset();
}

void MultimediaDlg::AddRefreshHeader(const QString& strFileName)
{
    /*
    	Add a html header to refresh the page every n seconds.
    */
    /* Open file for append (text mode) */
    FILE* pFiBody = fopen(strFileName.latin1(), "at");

    if (pFiBody != NULL)
    {
        fputs("<META http-equiv=\"REFRESH\" content=\""
              + QString::number(iRefresh)
              + "\">", pFiBody);

        /* Close the file afterwards */
        fclose(pFiBody);
    }
}

void MultimediaDlg::SaveMOTObject(const QString& strFileName,
                                  const CMOTObject& obj)
{
    const CVector<_BYTE>& vecbRawData = obj.Body.vecData;

    /* First, create directory for storing the file (if not exists) */
    CreateDirectories(strFileName);

    /* Open file */
    QFile file(strFileName);
    if (file.open(IO_WriteOnly | IO_Truncate))
    {
        int i, written, size;
        size = vecbRawData.Size();

        /* Write data */
        for (i = 0, written = 0; size > 0 && written >= 0; i+=written, size-=written)
            written = file.writeBlock((const char*)&vecbRawData.at(i), size);

        /* Close the file afterwards */
        file.close();
    }
}

_BOOLEAN MultimediaDlg::openBrowser(QWidget *widget, const QString &filename)
{
    _BOOLEAN bResult = FALSE;

#ifdef _WIN32
    /* Running in an MS Windows environment */
    if (NULL != widget)
    {
        QString f = filename;

        bResult = (ShellExecute(NULL, "open",
                                f.replace(QRegExp("/"),"\\").latin1(), NULL, NULL, SW_SHOWNORMAL) > (HINSTANCE)32);
    }
#else
    Q_UNUSED(widget);

    /* try with KDE */
    string strStartBrowser = "kfmclient exec \"";
    strStartBrowser += filename.latin1();
    strStartBrowser += "\"";
    int retval = system(strStartBrowser.c_str());

    if (retval != -1)
    {
        if ((WEXITSTATUS(retval) != 1) && (retval != 0))
        {
            /* try with gnome */
            strStartBrowser = "gnome-open \"";
            strStartBrowser += filename.latin1();
            strStartBrowser += "\"";
            retval = system(strStartBrowser.c_str());
        }
    }

    if ((WEXITSTATUS(retval) == 1) || (retval == 0))
        bResult = TRUE;
#endif

    return bResult;
}

void MultimediaDlg::JpgToPng(CMOTObject&)
{
}

void MultimediaDlg::OnSetFont()
{
    _BOOLEAN bok;

    /* Open the font dialog */
    QFont newFont = QFontDialog::getFont(&bok, fontTextBrowser, this);

    if (bok == TRUE)
    {
        /* Store the current text and then reset it */
        QString strOldText = TextBrowser->text();
        TextBrowser->setText("<br>");

        /* Set the new font */
        fontTextBrowser = newFont;

        /* Change the body stylesheet */
        QStyleSheetItem* styleBody =
            TextBrowser->styleSheet()->item("stylebody");

        styleBody->setFontFamily(fontTextBrowser.family());
        styleBody->setFontSize(fontTextBrowser.pointSize());
        styleBody->setFontWeight(fontTextBrowser.weight());
        styleBody->setFontItalic(fontTextBrowser.italic());

        /* Restore the text for refresh it with the new font */
        TextBrowser->setText(strOldText);
    }
}
