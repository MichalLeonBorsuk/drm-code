/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo
 *
 * Description:
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

#if !defined(DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_)
#define DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_

#include <QMenu>
#include <QWhatsThis>
#include <QCheckBox>

#include "ui_AboutDlg.h"
#include "../selectioninterface.h"
#include "../ReceiverInterface.h"

/* Definitions ****************************************************************/

/* Classes ********************************************************************/
/* DRM events --------------------------------------------------------------- */
class DRMEvent : public QEvent
{
public:
	DRMEvent(const int iNewMeTy, const int iNewSt) :
		QEvent(Type(QEvent::User + 11)), iMessType(iNewMeTy), iStatus(iNewSt) {}

	int iMessType;
	int iStatus;
};


/* About dialog ------------------------------------------------------------- */
class CAboutDlg : public QDialog, public Ui_AboutDlg
{
	Q_OBJECT

public:
	CAboutDlg(QWidget* parent = 0, const char* name = 0, bool modal = false,
		Qt::WFlags f = 0);
};


/* Help menu ---------------------------------------------------------------- */
class CDreamHelpMenu : public QMenu
{
	Q_OBJECT

public:
	CDreamHelpMenu(QWidget* parent = 0);

protected:
	CAboutDlg AboutDlg;

public slots:
	void OnHelpWhatsThis() {QWhatsThis::enterWhatsThisMode();}
	void OnHelpAbout() {AboutDlg.exec();}
};


/* Sound card selection menu ------------------------------------------------ */
class CSoundCardSelMenu : public QMenu
{
	Q_OBJECT

public:
	CSoundCardSelMenu(CSelectionInterface* pNS, QWidget* parent = 0);
    void showEvent(QShowEvent* pEvent);

protected:
	CSelectionInterface*	pSoundIF;
	vector<string>			vecNames;
	int						iNumDev;
	QActionGroup*			group;

public slots:
	void OnSoundDevice(QAction*);
};


/* GUI help functions ------------------------------------------------------- */
/* Converts from RGB to integer and back */
class CRGBConversion
{
public:
	static int RGB2int(const QColor newColor)
	{
		/* R, G and B are encoded as 8-bit numbers */
		int iReturn = newColor.red();
		iReturn <<= 8;
		iReturn |= newColor.green();
		iReturn <<= 8;
		iReturn |= newColor.blue();
		return iReturn;
	}

	static QColor int2RGB(const int iValue)
	{
		return QColor((iValue >> 16) & 255, (iValue >> 8) & 255, iValue & 255);
	}
};

void OnSaveAudio(QWidget*, QCheckBox*, ReceiverInterface&);

#endif // DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_
