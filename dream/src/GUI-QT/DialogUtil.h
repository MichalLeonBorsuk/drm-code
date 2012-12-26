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
#include "../DrmTransceiver.h"
#include "../sound/selectioninterface.h"

#include<map>

#include <qthread.h>
#include "ui_AboutDlgbase.h"
#include <QMenu>
#include <QDialog>
#include <QAction>
#include <QEvent>

#include <qwt_thermo.h> /* S-Meter */

class CRig;
typedef int rig_model_t;

/* Definitions ****************************************************************/

/* Definition for Courier font */
#ifdef _WIN32
# define FONT_COURIER    "Courier New"
#elif defined(__linux__)
# define FONT_COURIER    "Monospace"
#else
# define FONT_COURIER    "Courier"
#endif
/* Classes ********************************************************************/

/* About dialog ------------------------------------------------------------- */
class CAboutDlgBase : public QDialog, public Ui_CAboutDlgBase
{
public:
	CAboutDlgBase(QWidget* parent, const char*, bool, Qt::WFlags f):
		QDialog(parent,f){setupUi(this);}
	virtual ~CAboutDlgBase() {}
};

class CAboutDlg : public CAboutDlgBase
{
	Q_OBJECT

public:
	CAboutDlg(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
		Qt::WFlags f = 0);
};

/* Help Usage --------------------------------------------------------------- */
class CHelpUsage : public CAboutDlgBase
{
	Q_OBJECT

public:
	CHelpUsage(const char* usage, const char* argv0, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);
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

/* The purpose of this class is to prevent showEvent and
   hideEvent from spurious event like unmatched show/hide,
   which cause some problem for window save and restore.
   The class may be adapted for other type of filtering
   as well. The member isValid() return FALSE when the
   event must be ignored. */
class CEventFilter
{
public:
	CEventFilter() : eLastEventType(QEvent::Hide) {}
	~CEventFilter() {}
	bool isValid(const QEvent* event)
	{
		bool bValid = FALSE;
		QEvent::Type eEventType = event->type();
		switch (eEventType)
		{
		case QEvent::Hide:
			bValid = eLastEventType == QEvent::Show;
			eLastEventType = eEventType;
			break;
		case QEvent::Show:
			bValid = eLastEventType == QEvent::Hide;
			eLastEventType = eEventType;
			break;
		default:
			break;
		}
		return bValid;
	}
protected:
	QEvent::Type eLastEventType; 
};
#define EVENT_FILTER(e) do { if (!ef.isValid((QEvent*)e)) return; } while(0)


inline void SetDialogCaption(QDialog* pDlg, const QString sCap)
{
	pDlg->setWindowTitle(sCap);
}

class RemoteMenu : public QObject
{
	Q_OBJECT

public:
	RemoteMenu(QWidget*, CRig&);
	QMenu
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
		QMenu * pMenu;
	};
	std::map<int,Rigmenu> rigmenus;
	std::vector<rig_model_t> specials;
	CRig&	rig;
#endif
	QMenu* pRemoteMenu, *pRemoteMenuOther;
};

#define OTHER_MENU_ID (666)
#define SMETER_MENU_ID (667)

QString VerifyFilename(QString filename);

QString VerifyHtmlPath(QString path);

QString UrlEncodePath(QString url);
bool IsUrlDirectory(QString url);

/* s-meter thermo parameters */
#define S_METER_THERMO_MIN				((_REAL) -60.0) /* dB */
#define S_METER_THERMO_MAX				((_REAL) 60.0) /* dB */
#define S_METER_THERMO_ALARM			((_REAL) 0.0) /* dB */

void InitSMeter(QWidget* parent, QwtThermo* sMeter);

void Linkify(QString& text);

void CreateDirectories(const QString& strFilename);

void RestartTransceiver(CDRMTransceiver *DRMTransceiver);

#endif // DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_
