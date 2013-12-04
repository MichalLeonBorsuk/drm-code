/******************************************************************************\
 * Copyright (c) 2013
 *
 * Author(s):
 *  Julian Cable, David Flamand
 *
 * Description:
 *  QT Sound Interface
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

#ifndef _QTAUDIO_H
#define _QTAUDIO_H

#include "soundinterface.h"

class QIODevice;
class QAudioInput;
class QAudioOutput;

/* Classes ********************************************************************/

class CSoundInQT : public CSoundInInterface
{
public:
    CSoundInQT();
    virtual ~CSoundInQT();
    virtual _BOOLEAN    Init(int, int, _BOOLEAN);
    virtual _BOOLEAN    Read(CVector<short>&);
    virtual void        Enumerate(vector<string>&choices, vector<string>&);
    virtual string      GetDev();
    virtual void        SetDev(string);
    virtual void        Close();
private:
    string              sDev;
    bool                bDevChanged;
    int                 iSampleRate;
    int                 iBufferSize;
    _BOOLEAN            bBlocking;
    QAudioInput*        pAudioInput;
    QIODevice*          pIODevice;
};

class CSoundOutQT : public CSoundOutInterface
{
public:
    CSoundOutQT();
    virtual ~CSoundOutQT();
    virtual _BOOLEAN    Init(int, int, _BOOLEAN);
    virtual _BOOLEAN    Write(CVector<short>&);
    virtual void        Enumerate(vector<string>& choices, vector<string>&);
    virtual string      GetDev();
    virtual void        SetDev(string);
    virtual void        Close();
private:
    string              sDev;
    bool                bDevChanged;
    int                 iSampleRate;
    int                 iBufferSize;
    _BOOLEAN            bBlocking;
    QAudioOutput*       pAudioOutput;
    QIODevice*          pIODevice;
};

#endif // _QTAUDIO_H
