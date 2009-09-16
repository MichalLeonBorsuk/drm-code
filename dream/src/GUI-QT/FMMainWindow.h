/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description: FM Main Window
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
#ifndef _FMMAINWINDOW_H
#define _FMMAINWINDOW_H

#include "ui_FMMainWindow.h"

class ReceiverInterface;
class CSettings;
class ReceiverSettingsDlg;

class FMMainWindow : public QMainWindow, public Ui_FMMainWindow
{
	Q_OBJECT

public:
	FMMainWindow(ReceiverInterface&, CSettings&, QWidget* parent = 0,
		Qt::WFlags f = 0);
	/* dummy assignment operator to help MSVC8 */
	FMMainWindow& operator=(const FMMainWindow&)
	{ throw "should not happen"; return *this;}

protected:
    ReceiverInterface&      Receiver;
    CSettings&              Settings;
    ReceiverSettingsDlg*    receiverSettingsDlg;
    QDialog*                stationsDlg;

    QTimer                  Timer;
    bool                    quitWanted;
    void                    showEvent(QShowEvent* pEvent);
    void                    hideEvent(QHideEvent* pEvent);
    void                    closeEvent(QCloseEvent* pEvent);

public slots:
    void OnTimer();
    void OnTune();
    void OnButtonAM();
    void OnButtonDRM();
    void OnHelpWhatsThis();
};

#endif
