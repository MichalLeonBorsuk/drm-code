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
class QAudioDeviceInfo;

/* Classes ********************************************************************/

class CSoundCommonQT : public CSelectionInterface
{
public:
    CSoundCommonQT(bool bInput);
    virtual ~CSoundCommonQT();
    virtual void        Enumerate(vector<string>&, vector<string>&);
    virtual string      GetDev();
    virtual void        SetDev(string);
protected:
    bool                isDeviceGood(const QAudioDeviceInfo &di) const;
    bool                bInput;
    string              sDev;
    bool                bDevChanged;
    int                 iSampleRate;
    int                 iBufferSize;
    _BOOLEAN            bBlocking;
    QIODevice*          pIODevice;
};

class CSoundInQT : public CSoundInInterface, CSoundCommonQT
{
public:
    CSoundInQT();
    virtual ~CSoundInQT();
    virtual void        Enumerate(vector<string>&n, vector<string>&d) { CSoundCommonQT::Enumerate(n, d); }
    virtual string      GetDev() { return CSoundCommonQT::GetDev(); }
    virtual void        SetDev(string s) { CSoundCommonQT::SetDev(s); }
    virtual _BOOLEAN    Init(int, int, _BOOLEAN);
    virtual _BOOLEAN    Read(CVector<short>&);
    virtual void        Close();
private:
    QAudioInput*        pAudioInput;
};

class CSoundOutQT : public CSoundOutInterface, CSoundCommonQT
{
public:
    CSoundOutQT();
    virtual ~CSoundOutQT();
    virtual void        Enumerate(vector<string>&n, vector<string>&d) { CSoundCommonQT::Enumerate(n, d); }
    virtual string      GetDev() { return CSoundCommonQT::GetDev(); }
    virtual void        SetDev(string s) { CSoundCommonQT::SetDev(s); }
    virtual _BOOLEAN    Init(int, int, _BOOLEAN);
    virtual _BOOLEAN    Write(CVector<short>&);
    virtual void        Close();
private:
    QAudioOutput*       pAudioOutput;
};

#endif // _QTAUDIO_H
