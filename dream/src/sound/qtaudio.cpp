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

#include "qtaudio.h"
#include <QThread>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>

/******************************/
/* QT Audio In Implementation */

CSoundInQT::CSoundInQT() :
    bDevChanged(false), iSampleRate(0), iBufferSize(0), bBlocking(FALSE),
    pAudioInput(NULL), pIODevice(NULL)
{
}

CSoundInQT::~CSoundInQT()
{
    Close();
}

_BOOLEAN CSoundInQT::Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking)
{
//    qDebug("CSoundInQT::Init() %i %i %i", iNewSampleRate, iNewBufferSize, bNewBlocking);
    _BOOLEAN bChanged = FALSE;
    iBufferSize = iNewBufferSize;
    bBlocking = bNewBlocking;

    if (iSampleRate != iNewSampleRate)
    {
        iSampleRate = iNewSampleRate;
        Close();
        bChanged = TRUE;
    }

    if (bDevChanged || !pAudioInput)
    {
        bool found=false;
        QAudioDeviceInfo di, diFirst;
        foreach(const QAudioDeviceInfo& tdi, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        {
            string name(tdi.deviceName().toLocal8Bit().constData());
//            qDebug("CSoundInQT::Init() name=%s", name.c_str());
            if (name == sDev)
            {
                di = tdi;
                found = true;
                break;
            }
        }
        if (!found)
            di = QAudioDeviceInfo::defaultInputDevice();

        bDevChanged = false;

        QAudioFormat format;
        format.setSampleRate(iSampleRate);
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::SignedInt);
        format.setChannelCount(2);
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setCodec("audio/pcm");
        QAudioFormat nearestFormat = di.nearestFormat(format);
        pAudioInput = new QAudioInput(di, nearestFormat);
        pAudioInput->setBufferSize(iSampleRate * 4 * 2); /* 2 sec. audio buffer */
        QIODevice* pIODevice = pAudioInput->start();
//        int n = pAudioInput->bufferSize();
        if (pAudioInput->error() == QAudio::NoError)
            this->pIODevice = pIODevice;
        else
            qDebug("CSoundInQT::Init() Can't open audio input");
    }
    return bChanged;
}

_BOOLEAN CSoundInQT::Read(CVector<short>& vecsSoundBuffer)
{
    _BOOLEAN bError = TRUE;
    if (bDevChanged)
    {
        Init(iSampleRate, iBufferSize, bBlocking);
    }
    if (pIODevice)
    {
        qint64 len = 2*vecsSoundBuffer.Size(); /* 2 = bytes per sample */
        for (int pos=0;;) {
            int ret = (int)pIODevice->read((char*)&vecsSoundBuffer[pos], len);
            if (ret < 0)
                break;
            pos += ret/2; /* 2 = bytes per sample */
            len -= ret;
            if (len <= 0)
                break;
            /* 1000000 = number of usec. in sec., 4 = bytes per sample * channels */
            qint64 timeToSleep = len * (1000000 / 4) / iSampleRate;
            if (timeToSleep)
                QThread::usleep((unsigned long)timeToSleep);
        }
        bError = len != 0;
    }
    return bError;
}

void CSoundInQT::Enumerate(vector<string>& names, vector<string>& descriptions)
{
    names.clear();
    descriptions.clear();
    names.push_back(""); /* Default device */
    foreach(const QAudioDeviceInfo& di, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        string name(di.deviceName().toLocal8Bit().constData());
//        qDebug("CSoundInQT::Enumerate() name=%s", name.c_str());
        names.push_back(name);
    }
}

string CSoundInQT::GetDev()
{
    return sDev;
}

void CSoundInQT::SetDev(string sNewDev)
{
//    qDebug("CSoundInQT::SetDev() name=%s", sNewDev.c_str());
    if (sDev != sNewDev)
    {
        sDev = sNewDev;
        bDevChanged = true;
    }
}

void CSoundInQT::Close()
{
//    qDebug("CSoundInQT::Close()");
    if (pAudioInput)
    {
        pAudioInput->stop();
        delete pAudioInput;
        pAudioInput = NULL;
        pIODevice = NULL;
    }

}


/*******************************/
/* QT Audio Out Implementation */

CSoundOutQT::CSoundOutQT() :
    bDevChanged(false), iSampleRate(0), iBufferSize(0), bBlocking(FALSE),
    pAudioOutput(NULL), pIODevice(NULL)
{
}

CSoundOutQT::~CSoundOutQT()
{
    Close();
}

_BOOLEAN CSoundOutQT::Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking)
{
//    qDebug("CSoundOutQT::Init() %i %i %i", iNewSampleRate, iNewBufferSize, bNewBlocking);
    _BOOLEAN bChanged = FALSE;
    iBufferSize = iNewBufferSize;
    bBlocking = bNewBlocking;

    if (iSampleRate != iNewSampleRate)
    {
        iSampleRate = iNewSampleRate;
        Close();
        bChanged = TRUE;
    }

    if (bDevChanged || !pAudioOutput)
    {
        bool found=false;
        QAudioDeviceInfo di, diFirst;
        foreach(const QAudioDeviceInfo& tdi, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        {
            string name(tdi.deviceName().toLocal8Bit().constData());
//            qDebug("CSoundOutQT::Init() name=%s", name.c_str());
            if (name == sDev)
            {
                di = tdi;
                found = true;
                break;
            }
        }
        if (!found)
            di = QAudioDeviceInfo::defaultOutputDevice();

        bDevChanged = false;

        QAudioFormat format;
        format.setSampleRate(iSampleRate);
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::SignedInt);
        format.setChannelCount(2);
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setCodec("audio/pcm");
        QAudioFormat nearestFormat = di.nearestFormat(format);
        pAudioOutput = new QAudioOutput(di, nearestFormat);
        pAudioOutput->setBufferSize(iSampleRate * 4 * 2); /* 2 sec. audio buffer */
        QIODevice* pIODevice = pAudioOutput->start();
//        int n = pAudioOutput->bufferSize();
        if (pAudioOutput->error() == QAudio::NoError)
            this->pIODevice = pIODevice;
        else
            qDebug("CSoundOutQT::Init() Can't open audio output");
    }
    return bChanged;
}

_BOOLEAN CSoundOutQT::Write(CVector<short>& vecsSoundBuffer)
{
    _BOOLEAN bError = TRUE;
    if (bDevChanged)
    {
        Init(iSampleRate, iBufferSize, bBlocking);
    }
    if (pIODevice)
    {
        qint64 len = 2*vecsSoundBuffer.Size(); /* 2 = bytes per sample */
        for (int pos=0;;) {
            int ret = (int)pIODevice->write((char*)&vecsSoundBuffer[pos], len);
            if (ret < 0)
                break;
            pos += ret/2; /* 2 = bytes per sample */
            len -= ret;
            if (len <= 0)
                break;
            /* 1000000 = number of usec. in sec., 4 = bytes per sample * channels */
            qint64 timeToSleep = len * (1000000 / 4) / iSampleRate;
            if (timeToSleep)
                QThread::usleep((unsigned long)timeToSleep);
        }
        bError = len != 0;
    }
    return bError;
}

void CSoundOutQT::Enumerate(vector<string>& names, vector<string>& descriptions)
{
    names.clear();
    descriptions.clear();
    names.push_back(""); /* Default device */
    foreach(const QAudioDeviceInfo& di, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    {
        string name(di.deviceName().toLocal8Bit().constData());
//        qDebug("CSoundOutQT::Enumerate() name=%s", name.c_str());
        names.push_back(name);
    }
}

string CSoundOutQT::GetDev()
{
    return sDev;
}

void CSoundOutQT::SetDev(string sNewDev)
{
//    qDebug("CSoundOutQT::SetDev() name=%s", sNewDev.c_str());
    if (sDev != sNewDev)
    {
        sDev = sNewDev;
        bDevChanged = true;
    }
}

void CSoundOutQT::Close()
{
//    qDebug("CSoundOutQT::Close()");
    if (pAudioOutput)
    {
        pAudioOutput->stop();
        delete pAudioOutput;
        pAudioOutput = NULL;
        pIODevice = NULL;
    }
}

