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
#ifndef __SOUNDCARDMENU_H
#define __SOUNDCARDMENU_H

#include <QMenu>
#include <QActionGroup>
#include "../selectioninterface.h"
#include <vector>

class CSoundCardSelMenu : public QMenu
{
        Q_OBJECT

public:
        CSoundCardSelMenu(CSelectionInterface* pNSIn,
                CSelectionInterface* pNSOut, QWidget* parent = 0);

protected:
        CSelectionInterface*    pSoundInIF;
        CSelectionInterface*    pSoundOutIF;
	QMenu* Init(const QString& text, CSelectionInterface* intf);

public slots:
        void OnSoundInDevice(QAction*);
        void OnSoundOutDevice(QAction*);
};
#endif
