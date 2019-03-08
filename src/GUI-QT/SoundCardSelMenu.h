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
#ifndef SOUNDCARDMENU_H
#define SOUNDCARDMENU_H

#include <QMenu>
#include <QMenuBar>
#include <QActionGroup>
#include <QMainWindow>
#include "../DrmReceiver.h"
#include "../DrmTransceiver.h"
#include "../sound/selectioninterface.h"
#include "ctrx.h"

typedef struct CHANSEL {
    const char* Name;
    const int iChanSel;
} CHANSEL;

class CFileMenu;
class CSoundCardSelMenu : public QMenu
{
    Q_OBJECT

public:
    CSoundCardSelMenu(
        CTRx& ntrx,
        CFileMenu* pFileMenu,
        QWidget* parent = 0);

protected:
    CTRx&               trx;
    CParameter&			Parameters;
    QMenu*				menuSigInput;
    QMenu*				menuSigDevice;
    QMenu*				menuSigSampleRate;
    const bool			bReceiver;

    QMenu* InitDevice(QMenu* self, QMenu* parent, const QString& text, bool bInput);
    QMenu* InitChannel(QMenu* parent, const QString& text, const int iChanSel, const CHANSEL* ChanSel);
    QMenu* InitSampleRate(QMenu* parent, const QString& text, const int iCurrentSampleRate, const int* SampleRate);

public slots:
    void OnSoundInChannel(QAction*);
    void OnSoundOutChannel(QAction*);
    void OnSoundInDevice(QAction*);
    void OnSoundOutDevice(QAction*);
    void OnSoundSampleRate(QAction*);
    void OnSoundSignalUpscale(bool);
    void OnSoundFileChanged(QString);

signals:
    void sampleRateChanged();
    void soundInDeviceChanged(QString);
    void soundOutDeviceChanged(QString);
};

class CFileMenu : public QMenu
{
    Q_OBJECT

public:
    CFileMenu(CTRx& ntrx, QMainWindow* parent, QMenu* menuInsertBefore);

protected:
    CTRx&               trx;
    QAction*			actionOpenFile;
    QAction*			actionCloseFile;

public slots:
    void OnOpenFile();
    void OnCloseFile();

signals:
    void soundFileChanged(QString);
};

#endif
