/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2006
 *
 * Author(s):
 *	Andrea Russo
 *
 * Description:
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

#include "MultSettingsDlg.h"
#if QT_VERSION < 0x040000
# include <qfiledialog.h>
#else
# include <QFileDialog>
# include <QShowEvent>
# include <QHideEvent>
#endif
#include <qlabel.h>
#include <qtooltip.h>

/* Implementation *************************************************************/

MultSettingsDlg::MultSettingsDlg(CParameter& NP, CSettings& NSettings, QWidget* parent,
	const char* name, bool modal, Qt::WFlags f) :
	CMultSettingsDlgBase(parent, name, modal, f),
	Parameters(NP), Settings(NSettings)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

#if QT_VERSION >= 0x040000
	// TODO
	CheckBoxAddRefresh->hide();
	EdtSecRefresh->hide();
#endif

	/* Connect buttons */

	connect(PushButtonChooseDir, SIGNAL(clicked()),
		this, SLOT(OnbuttonChooseDir()));

	connect(buttonClearCacheMOT, SIGNAL(clicked()),
		this, SLOT(OnbuttonClearCacheMOT()));

	connect(buttonClearCacheEPG, SIGNAL(clicked()),
		this, SLOT(OnbuttonClearCacheEPG()));

	EdtSecRefresh->setValidator(new QIntValidator(MIN_MOT_BWS_REFRESH_TIME, MAX_MOT_BWS_REFRESH_TIME, EdtSecRefresh));
}

MultSettingsDlg::~MultSettingsDlg()
{
}

void MultSettingsDlg::hideEvent(QHideEvent*)
{
#if QT_VERSION < 0x040000
	/* save current settings */
	Settings.Put("Multimedia Dialog", "addrefresh", CheckBoxAddRefresh->isChecked());

	QString strRefresh = EdtSecRefresh->text();
	int iMOTRefresh = strRefresh.toUInt();

	if (iMOTRefresh < MIN_MOT_BWS_REFRESH_TIME)
		iMOTRefresh = MIN_MOT_BWS_REFRESH_TIME;

	if (iMOTRefresh > MAX_MOT_BWS_REFRESH_TIME)
		iMOTRefresh = MAX_MOT_BWS_REFRESH_TIME;

	Settings.Put("Multimedia Dialog", "motbwsrefresh", iMOTRefresh);
#else
	// TODO
#endif
}

void MultSettingsDlg::showEvent(QShowEvent*)
{
#if QT_VERSION < 0x040000
	if (Settings.Get("Multimedia Dialog", "addrefresh", TRUE))
		CheckBoxAddRefresh->setChecked(TRUE);

	EdtSecRefresh->setText(QString().setNum(Settings.Get("Multimedia Dialog", "motbwsrefresh", 10)));
#else
	// TODO
#endif

    SetDataDirectoryControls();
}

void MultSettingsDlg::ClearCache(QString sPath, QString sFilter = "", _BOOLEAN bDeleteDirs)
{
	/* Delete files into sPath directory with scan recursive */

	QDir dir(sPath);

	/* Check if the directory exists */
	if (dir.exists())
	{
		dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoSymLinks);

#if QT_VERSION < 0x040000
		/* Eventually apply the filter */
		if (sFilter != "")
			dir.setNameFilter(sFilter);

		dir.setSorting( QDir::DirsFirst );

		const QFileInfoList *list = dir.entryInfoList();
		QFileInfoListIterator it( *list ); /* create list iterator */
		for(QFileInfo *fi; (fi=it.current()); ++it )
		{
#else
		/* Eventually apply the filter */
		if (sFilter != "")
			dir.setNameFilters(QStringList(sFilter));

		dir.setSorting( QDir::DirsFirst );
		const QList<QFileInfo> list = dir.entryInfoList();
		for(QList<QFileInfo>::const_iterator fi = list.begin(); fi!=list.end(); fi++)
		{
#endif

			/* for each file/dir */
			/* if directory...=> scan recursive */
			if (fi->isDir())
			{
				if(fi->fileName()!="." && fi->fileName()!="..")
				{
					ClearCache(fi->filePath(), sFilter, bDeleteDirs);

					/* Eventually delete the directory */
					if (bDeleteDirs == TRUE)
						dir.rmdir(fi->fileName());
				}
			}
			else
			{
				/* Is a file so remove it */
				dir.remove(fi->fileName());
			}
		}
	}
}

void MultSettingsDlg::OnbuttonChooseDir()
{
#if QT_VERSION < 0x040000
	QString strFilename = QFileDialog::getExistingDirectory(TextLabelDir->text(), this);
#else
	QString strFilename = QFileDialog::getExistingDirectory(this, TextLabelDir->text());
#endif
	/* Check if user not hit the cancel button */
	if (!strFilename.isEmpty())
	{
#if QT_VERSION < 0x040000
		Parameters.SetDataDirectory(string(strFilename.utf8().data()));
#else
		Parameters.SetDataDirectory(string(strFilename.toUtf8().data()));
#endif
		SetDataDirectoryControls();
	}
}

void MultSettingsDlg::OnbuttonClearCacheMOT()
{
	/* Delete all files and directories in the MOT directory */
	ClearCache(QString::fromUtf8(Parameters.GetDataDirectory("MOT").c_str()), "", TRUE);
}

void MultSettingsDlg::OnbuttonClearCacheEPG()
{
	/* Delete all EPG files */
	ClearCache(QString::fromUtf8(Parameters.GetDataDirectory("EPG").c_str()), "*.EHA;*.EHB");
}

void MultSettingsDlg::SetDataDirectoryControls()
{
	QString strFilename(QString::fromUtf8(Parameters.GetDataDirectory().c_str()));
#undef PATH_SEP
#ifdef _WIN32
	strFilename.replace(QRegExp("/"), "\\");
# define PATH_SEP '\\'
#else
# define PATH_SEP '/'
#endif
	if (!strFilename.isEmpty() && strFilename.at(strFilename.length()-1) == QChar(PATH_SEP))
		strFilename.remove(strFilename.length()-1, 1);
#if QT_VERSION < 0x040000
	QToolTip::add(TextLabelDir, strFilename);
#else
	TextLabelDir->setToolTip(strFilename);
#endif
	TextLabelDir->setText(strFilename);
}

void MultSettingsDlg::AddWhatsThisHelp()
{
	//TODO
}
