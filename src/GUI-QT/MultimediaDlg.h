/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#ifndef _MULTIMEDIADLG_H
#define _MULTIMEDIADLG_H

#include <QTextDocument>
#include <vector>
#include <map>
#include "ui_MultimediaDlg.h"
#include "../datadecoding/DABMOT.h"
#include "../datadecoding/DataDecoder.h"

/* Definitions ****************************************************************/
/* Maximum number of levels. A maximum of 20 hierarchy levels is set
   (including the Main Menu and the final Message Object) */
#define MAX_NUM_LEV_JOURNALINE			20

/* Classes ********************************************************************/
class CDRMReceiver;
class CParameter;
class CSettings;
class Q3PopupMenu;

class CNewIDHistory
{
public:
	CNewIDHistory() : veciNewsID(MAX_NUM_LEV_JOURNALINE), iNumHist(0) {}
	virtual ~CNewIDHistory() {}

	void Add(const int iNewID)
	{
		veciNewsID[iNumHist] = iNewID;
		iNumHist++;
	}

	int Back()
	{
		if (iNumHist > 0)
		{
			iNumHist--;
			return veciNewsID[iNumHist];
		}
		else
			return 0; /* Root ID */
	}

	void Reset() {iNumHist = 0;}

protected:
	std::vector<int> veciNewsID;
	int				iNumHist;
};

class MultimediaDlg : public QDialog, Ui_MultimediaDlg
{
	Q_OBJECT

public:
	MultimediaDlg(CDRMReceiver&, QWidget* parent = 0,
		const char* name = 0, bool modal = false, Qt::WFlags f = 0);
	virtual ~MultimediaDlg();
	/* dummy assignment operator to help MSVC8 */
	MultimediaDlg& operator=(const MultimediaDlg&)
	{ throw "should not happen"; return *this;}

	void LoadSettings(const CSettings&);
	void SaveSettings(CSettings&);

protected:

	CParameter&				Parameters;
	CDataDecoder&			DataDecoder;

	QTimer					Timer;
	QMenuBar*				pMenu;
	Q3PopupMenu*			pFileMenu;
	QTextDocument           document;
	vector<QImage>          vecImages;
	vector<QString>         vecImageNames;
	int						iCurImagePos;
	QString					strFhGIISText;
	QString					strJournalineHeadText;
	int						iCurJourObjID;
	EAppType	            eAppType;
	CNewIDHistory			NewIDHistory;
	QString					strCurrentSavePath;
	QString					strDirMOTCache;
	QString					strBWSHomePage;
	QFont					fontTextBrowser;
	QFont					fontDefault;
	bool				    bAddRefresh;
	int						iRefresh;

	virtual void			showEvent(QShowEvent* pEvent);
	virtual void			hideEvent(QHideEvent* pEvent);
	void SetSlideShowPicture();
	void SetJournalineText();
	void UpdateAccButtonsSlideShow();
	void SaveMOTObject(const CVector<_BYTE>& vecbRawData, const QString& strFileName);
	void CreateDirectories(const QString& filename);
	void ClearAllSlideShow();

	void InitApplication(EAppType eNewAppType);

	void InitNotSupported();
	void InitMOTSlideShow();
	void InitJournaline();
	void InitBroadcastWebSite();

	void ExtractJournalineBody(const int iCurJourID, const bool bHTMLExport,
		QString &strTitle, QString &strItems);

	void SetCurrentSavePath(const QString strFileName);
	void AddRefreshHeader(const QString& strFileName);

public slots:
	void OnTimer();
	void OnButtonStepBack();
	void OnButtonStepForw();
	void OnButtonJumpBegin();
	void OnButtonJumpEnd();
	void OnSave();
	void OnSaveAll();
	void OnClearAll() {ClearAllSlideShow();}
	void OnTextChanged();
	void OnSetFont();
};

#endif
