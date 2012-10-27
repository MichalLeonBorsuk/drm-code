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

#include "../Parameter.h"
#include "../selectioninterface.h"

#include<map>

#include <qthread.h>
#if QT_VERSION < 0x040000
# include <qaction.h>
# include <qpopupmenu.h>
# include "AboutDlgbase.h"
#else
# include "ui_AboutDlgbase.h"
# include <QMenu>
# include <QDialog>
# include <QAction>
#endif

class CRig;
typedef int rig_model_t;

/* Definitions ****************************************************************/

/* Definition for Courier font */
#ifdef _WIN32
	#define FONT_COURIER    "Courier New"
#else
	#define FONT_COURIER    "Courier"
#endif
/* Classes ********************************************************************/

/* About dialog ------------------------------------------------------------- */
#if QT_VERSION >= 0x040000
class CAboutDlgBase : public QDialog, public Ui_CAboutDlgBase
{
public:
	CAboutDlgBase(QWidget* parent, const char*, bool, Qt::WFlags f):
		QDialog(parent,f){setupUi(this);}
	virtual ~CAboutDlgBase() {}
};
#endif

class CAboutDlg : public CAboutDlgBase
{
	Q_OBJECT

public:
	CAboutDlg(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
		Qt::WFlags f = 0);
};

#if QT_VERSION < 0x040000
/* Help menu ---------------------------------------------------------------- */
class CDreamHelpMenu : public QPopupMenu
{
	Q_OBJECT

public:
	CDreamHelpMenu(QWidget* parent = 0);

protected:
	CAboutDlg AboutDlg;

public slots:
	void OnHelpWhatsThis();
	void OnHelpAbout() {AboutDlg.exec();}
};

/* Sound card selection menu ------------------------------------------------ */
class CSoundCardSelMenu : public QPopupMenu
{
	Q_OBJECT

public:
	CSoundCardSelMenu(CSelectionInterface* pNSIn,
		CSelectionInterface* pNSOut, QWidget* parent = 0);

protected:
	CSelectionInterface*	pSoundInIF;
	CSelectionInterface*	pSoundOutIF;

        vector<string>          vecSoundInNames;
        vector<string>          vecSoundOutNames;
        int                     iNumSoundInDev;
        int                     iNumSoundOutDev;
        QPopupMenu*             pSoundInMenu;
        QPopupMenu*             pSoundOutMenu;

public slots:
	void OnSoundInDevice(int id);
	void OnSoundOutDevice(int id);
};
#endif

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


inline void SetDialogCaption(QDialog* pDlg, const QString sCap)
{
#if QT_VERSION < 0x030000
	/* Under Windows QT only sets the caption if a "Qt" is
	   present in the name. Make a little "trick" to display our desired
	   name without seeing the "Qt" (by Andrea Russo) */
	QString sTitle = "";
#ifdef _MSC_VER
	sTitle.fill(' ', 10000);
	sTitle += "Qt";
#endif
	pDlg->setCaption(sCap + sTitle);
#else
# if QT_VERSION < 0x040000
	pDlg->setCaption(sCap);
# else
	pDlg->setWindowTitle(sCap);
# endif
#endif
}

class RemoteMenu : public QObject
{
	Q_OBJECT

public:
	RemoteMenu(QWidget*, CRig&);
# if QT_VERSION < 0x040000
	QPopupMenu
#else
	QMenu
#endif
	* menu(){ return pRemoteMenu; }

public slots:
	void OnModRigMenu(int iID);
	void OnRemoteMenu(int iID);
	void OnComPortMenu(QAction* action);

signals:
	void SMeterAvailable();

protected:
#ifdef HAVE_LIBHAMLIB
	struct Rigmenu {
		std::string mfr;
# if QT_VERSION < 0x040000
	QPopupMenu
#else
	QMenu
#endif
		* pMenu;
	};
	std::map<int,Rigmenu> rigmenus;
	std::vector<rig_model_t> specials;
	CRig&	rig;
#endif
# if QT_VERSION < 0x040000
	QPopupMenu
#else
	QMenu
#endif
	* pRemoteMenu, *pRemoteMenuOther;
};

#define OTHER_MENU_ID (666)
#define SMETER_MENU_ID (667)

#endif // DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_
