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

#include "MultimediaDlg.h"
#include "../DrmReceiver.h"
#include "../datadecoding/Journaline.h"
#include <Q3PopupMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QToolTip>
#include <QFileInfo>
#include <QTextStream>
#include <QFontDialog>
#include <QTextStream>
#include <QDateTime>
#include <QProcess>

/* Implementation *************************************************************/
MultimediaDlg::MultimediaDlg(CDRMReceiver& NDRMR,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f)
	: QDialog(parent, name, modal, f), Ui_MultimediaDlg(),
	Parameters(*NDRMR.GetParameters()), DataDecoder(*NDRMR.GetDataDecoder()),
	strCurrentSavePath("."), strDirMOTCache("MOTCache")
{

    setupUi(this);
    textBrowser->datadecoder = &DataDecoder;

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));
    textBrowser->setFont(fontTextBrowser);
    textBrowser->setDocument(&document);

	/* Set FhG IIS text */
	strFhGIISText =
		"<table><tr><td><img src=\":/icons/fhgiis.bmp\"></td>"
		"<td><font face=\"Courier\" size=\"-1\">Features NewsService "
		"Journaline(R) decoder technology by Fraunhofer IIS, Erlangen, "
		"Germany. For more information visit http://www.iis.fhg.de/dab"
		"</font></td></tr></table>";

	/* Set Journaline headline text */
	strJournalineHeadText =
		"<table><tr><td><img src=\":/icons/LogoJournaline.png\"></td>"
		"<td valign=\"middle\"><h2>NewsService Journaline" + QString(QChar(174)) /* (R) */ +
		"</h2></td></tr></table>";

	/* Inits for broadcast website application */
	strBWSHomePage = "";

	/* Set Menu ***************************************************************/
	/* File menu ------------------------------------------------------------ */
	pFileMenu = new Q3PopupMenu(this);
	Q_CHECK_PTR(pFileMenu);
	pFileMenu->insertItem(tr("C&lear all"), this, SLOT(OnClearAll()),
		Qt::CTRL+Qt::Key_X, 0);
	pFileMenu->insertSeparator();
	pFileMenu->insertItem(tr("&Save..."), this, SLOT(OnSave()), Qt::CTRL+Qt::Key_S, 1);
	pFileMenu->insertItem(tr("Save &all..."), this, SLOT(OnSaveAll()),
		Qt::CTRL+Qt::Key_A, 2);
	pFileMenu->insertSeparator();
	pFileMenu->insertItem(tr("&Close"), this, SLOT(close()), 0, 3);


	/* Settings menu  ------------------------------------------------------- */
	Q3PopupMenu* pSettingsMenu = new Q3PopupMenu(this);
	Q_CHECK_PTR(pSettingsMenu);
	pSettingsMenu->insertItem(tr("Set &Font..."), this, SLOT(OnSetFont()));


	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	Q_CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&File"), pFileMenu);
	pMenu->insertItem(tr("&Settings"), pSettingsMenu);

	/* Now tell the layout about the menu */
	vboxLayout->setMenuBar(pMenu);

	/* Update time for color LED */
	LEDStatus->SetUpdateTime(1000);

	/* Init slide-show (needed for setting up vectors and indices) */
	ClearAllSlideShow();

	/* Init container and GUI */
	InitApplication(DataDecoder.GetAppType());


	/* Connect controls */
	connect(ButtonStepBack, SIGNAL(clicked()), this, SLOT(OnButtonStepBack()));
	connect(ButtonStepForward, SIGNAL(clicked()), this, SLOT(OnButtonStepForw()));
	connect(ButtonJumpBegin, SIGNAL(clicked()), this, SLOT(OnButtonJumpBegin()));
	connect(ButtonJumpEnd, SIGNAL(clicked()), this, SLOT(OnButtonJumpEnd()));
	connect(textBrowser, SIGNAL(textChanged()), this, SLOT(OnTextChanged()));

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	Timer.stop();
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
	case CDataDecoder::AT_MOTSLISHOW:
		InitMOTSlideShow();
		break;

	case CDataDecoder::AT_JOURNALINE:
		InitJournaline();
		break;

	case CDataDecoder::AT_MOTBROADCASTWEBSITE:
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
//cerr << (textBrowser->text().toStdString()) << endl;
	if (textBrowser->text().compare(textBrowser->text().left(1), "<") != 0)
	{
		/* Save old ID */
		NewIDHistory.Add(iCurJourObjID);
		/* Set text to news ID text which was selected by the user */
		iCurJourObjID = textBrowser->text().toInt();
		SetJournalineText();

		/* Enable back button */
		ButtonStepBack->setEnabled(true);
	}
}

void MultimediaDlg::OnTimer()
{
	CMOTObject	NewObj;
	QPixmap		NewImage;
	int			iCurNumPict;
	bool	    bMainPage;
	bool	bShowInfo = true;

	Parameters.Lock();
	ETypeRxStatus status = Parameters.ReceiveStatus.MOT.GetStatus();
	Parameters.Unlock();
	switch(status)
	{
	case NOT_PRESENT:
		LEDStatus->Reset(); /* GREY */
		break;

	case CRC_ERROR:
		LEDStatus->SetLight(2); /* RED */
		break;

	case DATA_ERROR:
		LEDStatus->SetLight(1); /* YELLOW */
		break;

	case RX_OK:
		LEDStatus->SetLight(0); /* GREEN */
		break;
	}

	/* Check out which application is transmitted right now */

	CDataDecoder::EAppType eNewAppType = DataDecoder.GetAppType();

	if (eNewAppType != eAppType)
		InitApplication(eNewAppType);

	switch (eAppType)
	{
	case CDataDecoder::AT_MOTSLISHOW:
		/* Poll the data decoder module for new picture */
		if (DataDecoder.GetMOTObject(NewObj, eAppType) == true)
		{
			/* Store received picture */
			iCurNumPict = vecImageNames.size();
            CVector<_BYTE>& imagedata = NewObj.Body.vecData;

            /* Load picture in QT format */
			QPixmap pic;
            if (pic.loadFromData(&imagedata[0], imagedata.size()))
            {
                /* Set new picture in source factory */
                vecImages.push_back(QImage(pic));
                vecImageNames.push_back(NewObj.strName.c_str());
                QString ref = QString("slides://image%1").arg(iCurNumPict);
                document.addResource(QTextDocument::ImageResource, QUrl(ref), QVariant(vecImages[iCurNumPict]));
            }

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

	case CDataDecoder::AT_MOTBROADCASTWEBSITE:
		/* Poll the data decoder module for new object */
		if (DataDecoder.GetMOTObject(NewObj, eAppType) == true)
		{
			/* Store received MOT object on disk */
			const QString strNewObjName = NewObj.strName.c_str();
			const QString strFileName =
				QString(strDirMOTCache + "/" + strNewObjName);

			SaveMOTObject(NewObj.Body.vecData, strFileName);

			/* Check if DABMOT could not unzip */
			const bool bZipped =
				(strNewObjName.right(3) == ".gz");

			/* Add an html header to refresh the page every n seconds */
			if (bAddRefresh && (NewObj.strFormat == "html") && (bZipped == false))
				AddRefreshHeader(strFileName);

			/* Check if is the main page */
			bMainPage = false;

			if (strNewObjName.contains('/') == 0) /* if has a path is not the main page */
			{
				/* Get the current directory */
				CMOTDirectory MOTDir;

				if (DataDecoder.GetMOTDirectory(MOTDir, eAppType) == true)
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
							bMainPage = true;
					}
					else
					{
						/* we have the directory but there is nothing in the directory */
						bMainPage = (strNewObjName.left(9).upper() == "INDEX.HTM");
					}
				}
			}

			if (bMainPage == true)
			{
				/* The home page is available */
                strBWSHomePage = strNewObjName;
                InitBroadcastWebSite();
			}
		}
		break;

	case CDataDecoder::AT_JOURNALINE:
		SetJournalineText();
		break;

	default:
		bShowInfo = false;
		break;
	}

	/* Add the service description into the dialog caption */
	QString strTitle = tr("Multimedia");

	if (bShowInfo == true)
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
	setCaption(strTitle);
}

void MultimediaDlg::ExtractJournalineBody(const int iCurJourID,
										  const bool bHTMLExport,
										  QString& strTitle, QString& strItems)
{
	/* Get news from actual Journaline decoder */
	CNews News;
	DataDecoder.GetNews(iCurJourID, News);

	/* Decode UTF-8 coding for title */
	strTitle = QString().fromUtf8(News.sTitle.c_str());

	strItems = "";
	for (int i = 0; i < News.vecItem.Size(); i++)
	{
		QString strCurItem = QString().fromUtf8(News.vecItem[i].sText.c_str());

		/* Replace \n by html command <br> */
		strCurItem = strCurItem.replace(QRegExp("\n"), "<br>");

		switch(News.vecItem[i].iLink)
		{
        case JOURNALINE_IS_NO_LINK: /* Only text, no link */
			strItems += strCurItem + QString("<br>");
			break;
        case JOURNALINE_LINK_NOT_ACTIVE:
			/* Un-ordered list item without link */
			strItems += QString("<li>") + strCurItem + QString("</li>");
			break;
        default:
            QString strLinkStr = QString("%1").arg(News.vecItem[i].iLink);
            /* Un-ordered list item with link */
            strItems += QString("<li><a href=\"") + strLinkStr +
                QString("\">") + strCurItem + QString("</a></li>");
		}
	}
}

void MultimediaDlg::SetJournalineText()
{
	/* Get title and body with html links */
	QString strTitle("");
	QString strItems("");
	ExtractJournalineBody(iCurJourObjID, false, strTitle, strItems);

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

	/* Only update text browser if text has changed */
	if (textBrowser->text().compare(strAllText) != 0)
		textBrowser->setText(strAllText);

	/* Enable / disable "save" menu item if title is present or not */
	if (strTitle == "")
		pFileMenu->setItemEnabled(1, false);
	else
		pFileMenu->setItemEnabled(1, true);
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
	fontDefault = textBrowser->font();

	/* Retrieve the setting saved into the .ini file */
	string str = strDirMOTCache.latin1();
	str = Settings.Get("Multimedia Dialog", "MOTCache", str);
	strDirMOTCache = str.c_str();
	str = strCurrentSavePath.latin1();
	str = Settings.Get("Multimedia Dialog", "storagepath", str);
	SetCurrentSavePath(str.c_str());

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

	bAddRefresh = Settings.Get("Multimedia Dialog", "addrefresh", true);
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

	/* Store save path */
	string s = strCurrentSavePath.latin1();
	Settings.Put("Multimedia Dialog","storagepath", s);
	s = strDirMOTCache.latin1();
	Settings.Put("Multimedia Dialog", "MOTCache", s);

	/* Store current textBrowser font */
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
	case CDataDecoder::AT_MOTSLISHOW:
		iCurImagePos--;
		SetSlideShowPicture();
		break;

	case CDataDecoder::AT_JOURNALINE:
		/* Step one level back, get ID from history */
		iCurJourObjID = NewIDHistory.Back();

		/* If root ID is reached, disable back button */
		if (iCurJourObjID == 0)
			ButtonStepBack->setEnabled(false);

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
	case CDataDecoder::AT_MOTSLISHOW:
		iCurImagePos++;
		SetSlideShowPicture();
		break;

	case CDataDecoder::AT_MOTBROADCASTWEBSITE:
		/* Try to open browser */
		if (QProcess::execute(strDirMOTCache + "/" + strBWSHomePage))
		{
			QMessageBox::information(this, "Dream",
				tr("Failed to start the default browser.\n"
				"Open the home page:\n" + strDirMOTCache +
				"/" + strBWSHomePage + "\nmanually."), QMessageBox::Ok);
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
	iCurImagePos = vecImages.size()-1;
	SetSlideShowPicture();
}

void MultimediaDlg::SetSlideShowPicture()
{
    textBrowser->setText("<br>");
    QString ref = QString("slides://image%1").arg(iCurImagePos);
    textBrowser->setText("<center><img src=\""+ref+"\"></center>");

	/* Remove previous tool tip */
	QToolTip::remove(textBrowser);

	/* Add tool tip showing the name of the picture */
	const QString imagename = vecImageNames[iCurImagePos];
	if (imagename.length() != 0)
		QToolTip::add(textBrowser,imagename);

	UpdateAccButtonsSlideShow();
}

void MultimediaDlg::UpdateAccButtonsSlideShow()
{
	/* Set enable menu entry for saving a picture */
	if (iCurImagePos < 0)
	{
		pFileMenu->setItemEnabled(0, false);
		pFileMenu->setItemEnabled(1, false);
		pFileMenu->setItemEnabled(2, false);
	}
	else
	{
		pFileMenu->setItemEnabled(0, true);
		pFileMenu->setItemEnabled(1, true);
		pFileMenu->setItemEnabled(2, true);
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
		textBrowser->setText("<center><h2>" +
			tr("MOT Slideshow Viewer") + "</h2></center>");
	}
}

void MultimediaDlg::SetCurrentSavePath(const QString strFileName)
{
	strCurrentSavePath = QFileInfo(strFileName).dirPath();
	if (strCurrentSavePath.right(1) == QString("/"))
		strCurrentSavePath.truncate(strCurrentSavePath.length()-1);
}

void MultimediaDlg::OnSave()
{
	QString strFileName;
	QString strDefFileName;
	QString strExt;
	QString strFilter;
	int n;

	switch (eAppType)
	{
	case CDataDecoder::AT_MOTSLISHOW:

		n = vecImageNames[iCurImagePos].lastIndexOf('.');
		if(n>=0)
		{
            strExt = vecImageNames[iCurImagePos].mid(n+1);
			strFilter = "*." + strExt;
		}
		else
			strFilter = "*.*";

		/* Show "save file" dialog */
		/* Set file name */
		strDefFileName = vecImageNames[iCurImagePos];

		/* Use default file name if no file name was transmitted */
		if (strDefFileName.length() == 0)
			strDefFileName = "RecPic";

		if ((strDefFileName.contains(".") == 0) && (strExt.length() > 0))
				strDefFileName += "." + strExt;

		strFileName =
			QFileDialog::getSaveFileName(strCurrentSavePath + "/" + strDefFileName,
			strFilter, this);

		/* Check if user not hit the cancel button */
		if (!strFileName.isNull())
		{
			SetCurrentSavePath(strFileName);
			vecImages[iCurImagePos].save(strFileName);
		}
		break;

	case CDataDecoder::AT_JOURNALINE:
		{
#if 0
			/* Save to file current journaline page */
			QString strTitle("");
			QString strItems("");

			/* true = without html links */
			ExtractJournalineBody(iCurJourObjID, true, strTitle, strItems);

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

			strFileName = QFileDialog::getSaveFileName(strCurrentSavePath + "/" +
				strTitle + ".html", "*.html", this);

			if (!strFileName.isNull())
			{
				SetCurrentSavePath(strFileName);

				/* Save Journaline page as a text stream */
				QFile FileObj(strFileName);

				if (FileObj.open(QIODevice::WriteOnly))
				{
					QTextStream TextStream(&FileObj);
					TextStream << strJornalineText; /* Actual writing */
					FileObj.close();
				}
			}
#endif
		}
		break;

	default:
		break;
	}
}

void MultimediaDlg::OnSaveAll()
{
	/* Let the user choose a directory */
	QString strDirName = QFileDialog::getExistingDirectory(NULL, this);

	if (!strDirName.isNull())
	{
		/* add slashes if not present */
		if (strDirName.right(1) != "/")
			strDirName += "/";

		/* Loop over all pictures received yet */
		for (size_t j = 0; j < vecImages.size(); j++)
		{
			const QImage& o = vecImages[j];
			QString strFileName = vecImageNames[j];

			if (strFileName.length() == 0)
			{
				/* Construct file name from date and picture number (default) */
				strFileName = "Dream_" + QDate().currentDate().toString() +
					"_#" + QString().setNum(j);
			}

			/* Add directory and ending */
			strFileName = strDirName + strFileName;

			o.save(strFileName);
		}
	}
}

void MultimediaDlg::ClearAllSlideShow()
{
	/* Init vector which will store the received images with zero size */
	vecImages.clear();
	vecImageNames.clear();

	/* Init current image position */
	iCurImagePos = -1;

	/* Update GUI */
	UpdateAccButtonsSlideShow();

	/* Remove tool tips */
	QToolTip::remove(textBrowser);
}

void MultimediaDlg::InitNotSupported()
{
	/* Hide all controls, disable menu items */
	pFileMenu->setItemEnabled(0, false);
	pFileMenu->setItemEnabled(1, false);
	pFileMenu->setItemEnabled(2, false);
	ButtonStepForward->hide();
	ButtonJumpBegin->hide();
	ButtonJumpEnd->hide();
	LabelCurPicNum->hide();
	ButtonStepBack->hide();
	QToolTip::remove(textBrowser);

	/* Show that application is not supported */
	textBrowser->setText("<center><h2>" + tr("No data service or data service "
		"not supported.") + "</h2></center>");
}

void MultimediaDlg::InitBroadcastWebSite()
{
	/* Hide all controls, disable menu items */
	pFileMenu->setItemEnabled(0, false);
	pFileMenu->setItemEnabled(1, false);
	pFileMenu->setItemEnabled(2, false);
	ButtonStepForward->show();
	ButtonStepForward->setEnabled(false);
	ButtonJumpBegin->hide();
	ButtonJumpEnd->hide();
	LabelCurPicNum->hide();
	ButtonStepBack->hide();
	QToolTip::remove(textBrowser);

	if (strBWSHomePage != "")
	{
		/* This is the button for opening the browser */
		ButtonStepForward->setEnabled(true);

		/* Display text that index page was received an can be opened */
		textBrowser->setText("<center><h2>" + tr("MOT Broadcast Web Site")
			+ "</h2><br>"
			+ tr("The homepage is available.")
			+ "<br><br>" +
			tr("Press the button to open it in the default browser.")
			+ "</center>");
	}
	else
	{
		/* Show initial text */
		textBrowser->setText("<center><h2>" + tr("MOT Broadcast Web Site") +
			"</h2></center>");

		/* Create the cache directory if not exist */
		if (!QFileInfo(strDirMOTCache).exists())
			QDir().mkdir(strDirMOTCache);
	}
}

void MultimediaDlg::InitMOTSlideShow()
{
	/* Make all browse buttons visible */
	ButtonStepBack->show();
	ButtonStepForward->show();
	ButtonJumpBegin->show();
	ButtonJumpEnd->show();
	LabelCurPicNum->show();

	/* Set current image position to the last picture and display it (if at
	   least one picture is available) */
	iCurImagePos = vecImages.size()-1;
	if (iCurImagePos >= 0)
		SetSlideShowPicture();
	else
	{
		/* Remove tool tips */
		QToolTip::remove(textBrowser);
	}

	/* Update buttons and menu */
	UpdateAccButtonsSlideShow();
}

void MultimediaDlg::InitJournaline()
{
	/* Disable "clear all" menu item */
	pFileMenu->setItemEnabled(0, false);

	/* Disable "save" menu items */
	pFileMenu->setItemEnabled(1, false);
	pFileMenu->setItemEnabled(2, false);

	/* Only one back button is visible and enabled */
	ButtonStepForward->hide();
	ButtonJumpBegin->hide();
	ButtonJumpEnd->hide();
	LabelCurPicNum->hide();

	/* Show back button and disable it because we always start at the root
	   object */
	ButtonStepBack->show();
	ButtonStepBack->setEnabled(false);

	/* Init text browser window */
	iCurJourObjID = 0;
	SetJournalineText();

	/* Remove tool tips */
	QToolTip::remove(textBrowser);

	NewIDHistory.Reset();
}

void MultimediaDlg::CreateDirectories(const QString& filename)
{
/*
	This function is for creating a complete directory structure to a given
	file name. If there is a pathname like this:
	/html/files/images/myimage.gif
	this function create all the folders into MOTCache:
	/html
	/html/files
	/html/files/images
	QFileInfo only creates a file if the pathname is valid. If not all folders
	are created, QFileInfo will not save the file. There was no QT function
	or a hint the QT mailing list found in which does the job of this function.
*/
	int i = 0;

	while (i < filename.length())
	{
		bool bFound = false;

		while ((i < filename.length()) && (bFound == false))
		{
			if (filename[i] == '/')
				bFound = true;
			else
				i++;
		}

		if (bFound == true)
		{
			/* create directory */
			const QString sDirName = filename.left(i);

			if (!QFileInfo(sDirName).exists())
				QDir().mkdir(sDirName);
		}

		i++;
	}
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

void MultimediaDlg::SaveMOTObject(const CVector<_BYTE>& vecbRawData,
								  const QString& strFileName)
{
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

void MultimediaDlg::OnSetFont()
{
	bool bok;

	/* Open the font dialog */
	QFont newFont = QFontDialog::getFont(&bok, fontTextBrowser, this);

	if (bok == true)
	{
		/* Store the current text and then reset it */
		QString strOldText = textBrowser->text();
		textBrowser->setText("<br>");

		/* Set the new font */
		fontTextBrowser = newFont;

		textBrowser->setFont(fontTextBrowser);

		/* Restore the text for refresh it with the new font */
		textBrowser->setText(strOldText);
	}
}

