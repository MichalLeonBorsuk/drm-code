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
#include "FMMainWindow.h"
#include "ReceiverSettingsDlg.h"
#include "StationsDlg.h"
#include "DialogUtil.h"
#include <QMessageBox>

FMMainWindow::FMMainWindow(ReceiverInterface& r, CSettings& s, QWidget* p, Qt::WFlags f)
:QMainWindow(p, f), Ui_FMMainWindow(), Receiver(r), Settings(s)
{
    setupUi(this);

    /* Stations window */
    stationsDlg = new StationsDlg(Receiver, Settings, true, this, "", false, Qt::WindowMinMaxButtonsHint);

    /* receiver settings window */
    receiverSettingsDlg = new ReceiverSettingsDlg(Settings,
		       Receiver.GetParameters()->GPSData,
		       *Receiver.GetSoundInInterface(),
		       *Receiver.GetSoundOutInterface(),
		      this, Qt::Dialog);

    connect(actionStations, SIGNAL(triggered()), stationsDlg, SLOT(show()));
    connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

    connect(actionAM, SIGNAL(triggered()), this, SLOT(OnButtonAM()));
    connect(actionDRM, SIGNAL(triggered()), this, SLOT(OnButtonDRM()));
    connect(actionSettings, SIGNAL(triggered()), receiverSettingsDlg, SLOT(show()));

    /* Help Menu */
    CAboutDlg* pAboutDlg = new CAboutDlg(this);
    connect(actionWhatsThis, SIGNAL(triggered()), this, SLOT(OnHelpWhatsThis()));
    connect(actionAbout, SIGNAL(triggered()), pAboutDlg, SLOT(show()));

    connect(pushButtonStations, SIGNAL(clicked()), stationsDlg, SLOT(show()));
    connect(pushButtonDRM, SIGNAL(clicked()), this, SLOT(OnButtonDRM()));
    connect(pushButtonAM, SIGNAL(clicked()), this, SLOT(OnButtonAM()));
}

void
FMMainWindow::showEvent(QShowEvent* pEvent)
{
    CWinGeom s;
    Settings.Get("FMgGUI", s);
    const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
    if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
	    setGeometry(WinGeom);

    OnTimer();

    /* default close action is to exit */
    quitWanted = true;

    /* Activate real-time timer */
    Timer.start(GUI_CONTROL_UPDATE_TIME);

    if(Settings.Get("FMGUI", "Stationsvisible", false))
	stationsDlg->show();

    //UpdateControls();
}

void
FMMainWindow::hideEvent(QHideEvent* pEvent)
{
    /* stop real-time timers */
    Timer.stop();

    /* Close windows */
    Settings.Put("FMGUI", "Stationsvisible", stationsDlg->isVisible());
    stationsDlg->hide();

    /* Save window geometry data */
    CWinGeom s;
    QRect WinGeom = geometry();
    s.iXPos = WinGeom.x();
    s.iYPos = WinGeom.y();
    s.iHSize = WinGeom.height();
    s.iWSize = WinGeom.width();
    Settings.Put("FMGUI", s);
}

void
FMMainWindow::closeEvent(QCloseEvent* pEvent)
{
    if(quitWanted)
    {
	if(!Receiver.End())
	{
	    QMessageBox::critical(this, "Dream", tr("Exit"), tr("Termination of working thread failed"));
	}
	qApp->quit();
    }
    pEvent->accept();
}

void
FMMainWindow::OnTimer()
{
    CParameter& Parameter = *Receiver.GetAnalogParameters();
    Parameter.Lock();
    EModulationType eModulation = Parameter.eModulation;
    Parameter.Unlock();
    switch(eModulation)
    {
    case  WBFM:
	    // TODO
	break;
    case DRM: case AM: case  USB: case  LSB: case  CW: case  NBFM:
	quitWanted = false;
	close();
	break;
    case NONE:
	break;
    }
}

void
FMMainWindow::OnTune()
{
}

void
FMMainWindow::OnButtonAM()
{
    CParameter& Parameters = *Receiver.GetParameters();
    Parameters.Lock();
    Parameters.eModulation = AM;
    Parameters.RxEvent = ChannelReconfiguration;
    Parameters.Unlock();
}

void
FMMainWindow::OnButtonDRM()
{
    CParameter& Parameters = *Receiver.GetParameters();
    Parameters.Lock();
    Parameters.eModulation = DRM;
    Parameters.RxEvent = ChannelReconfiguration;
    Parameters.Unlock();
}

void
FMMainWindow::OnHelpWhatsThis()
{
}
