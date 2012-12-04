/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2012
 *
 * Author(s):
 *      Julian Cable, David Flamand
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
#include "../DrmReceiver.h"
#include "../Parameter.h"
#include "../DRMSignalIO.h"
#include "../DataIO.h"
#include "../sound/selectioninterface.h"
#include <vector>

typedef struct CHANSEL {
    const char* Name;
    const int iChanSel;
} CHANSEL;

class CSoundCardSelMenu : public QMenu
{
    Q_OBJECT

public:
    CSoundCardSelMenu(CDRMReceiver* DRMReceiver,
        CSelectionInterface* pNSIn,
        CSelectionInterface* pNSOut,
        QWidget* parent = 0);

protected:
    CDRMReceiver* DRMReceiver;
    CSelectionInterface* pSoundInIF;
    CSelectionInterface* pSoundOutIF;
    QMenu* InitDevice(QMenu* parent, const QString& text, CSelectionInterface* intf);
    QMenu* InitChannel(QMenu* parent, const QString& text, const int iChanSel, const CHANSEL* ChanSel);
    QMenu* InitSampleRate(QMenu* parent, const QString& text, const int iCurrentSampleRate, const int* SampleRate);
    void RestartReceiver();

public slots:
    void OnSoundInChannel(QAction*);
    void OnSoundOutChannel(QAction*);
    void OnSoundInDevice(QAction*);
    void OnSoundOutDevice(QAction*);
    void OnSoundSampleRate(QAction*);
};

#endif
