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

FMMainWindow::FMMainWindow(ReceiverInterface& r, CSettings& s, QWidget* p, Qt::WFlags f)
:QMainWindow(p, f), Ui_FMMainWindow(), Receiver(r), Settings(s)
{
    setupUi(this);
}

void
FMMainWindow::showEvent(QShowEvent* pEvent)
{
}

void
FMMainWindow::hideEvent(QHideEvent* pEvent)
{
}

void
FMMainWindow::closeEvent(QCloseEvent* pEvent)
{
}

void
FMMainWindow::OnTimer()
{
}

void
FMMainWindow::OnShowStations()
{
}

void
FMMainWindow::OnTune()
{
}

void
FMMainWindow::OnButtonAM()
{
}

void
FMMainWindow::OnButtonDRM()
{
}

void
FMMainWindow::OnHelpWhatsThis()
{
}

