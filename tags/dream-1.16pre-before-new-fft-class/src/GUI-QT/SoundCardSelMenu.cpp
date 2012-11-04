/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2012
 *
 * Author(s):
 *      Julian Cable
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
#include "SoundCardSelMenu.h"

CSoundCardSelMenu::CSoundCardSelMenu(
    CSelectionInterface* pNSIn, CSelectionInterface* pNSOut, QWidget* parent) :
    QMenu(parent), pSoundInIF(pNSIn), pSoundOutIF(pNSOut)
{
    setTitle("Sound Card Selection");
    connect(Init(tr("Sound In"), pSoundInIF), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundInDevice(QAction*)));
    connect(Init(tr("Sound Out"), pSoundOutIF), SIGNAL(triggered(QAction*)), this, SLOT(OnSoundOutDevice(QAction*)));
}

void CSoundCardSelMenu::OnSoundInDevice(QAction* action)
{
    pSoundInIF->SetDev(action->data().toInt());
}

void CSoundCardSelMenu::OnSoundOutDevice(QAction* action)
{
    pSoundOutIF->SetDev(action->data().toInt());
}

QMenu* CSoundCardSelMenu::Init(const QString& text, CSelectionInterface* intf)
{
    QMenu* menu = addMenu(text);
    QActionGroup* group = new QActionGroup(this);
    vector<string> names;
    intf->Enumerate(names);
    int iNumSoundDev = names.size();
    int iDefaultDev = intf->GetDev();
    if ((iDefaultDev > iNumSoundDev) || (iDefaultDev < 0))
        iDefaultDev = iNumSoundDev;

    for (int i = 0; i < iNumSoundDev; i++)
    {
        QString name(names[i].c_str());
        QAction* m = menu->addAction(name);
        m->setCheckable(true);
        group->addAction(m);
        m->setData(i);
        if(i==iDefaultDev)
	{
	    menu->setDefaultAction(m);
	    m->setChecked(true);
	}
    }
    return menu;
}

