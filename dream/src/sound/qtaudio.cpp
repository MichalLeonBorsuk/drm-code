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


#define BUFFER_LEN_IN_SEC 2 /* 2 sec. audio buffer */

// TODO do the conversion if the device support mono only

// On linux with pulseaudio, sample rate can range from 1Hz to 192kHz,
//  but supportedSampleRates() return only standard sample rate (11025 22050 44100 48000).
// TODO isSampleRateSupported() hack is very ugly


/**********************************/
/* QT Audio Common Implementation */

CSoundCommonQT::CSoundCommonQT(bool bInput) :
    bInput(bInput), bDevChanged(false),
    iSampleRate(0), iBufferSize(0), bBlocking(FALSE),
    pIODevice(NULL)
{
}

CSoundCommonQT::~CSoundCommonQT()
{
}

void CSoundCommonQT::sleep(int len) const
{
    /* 1000000 = number of usec. in sec., 2 = channels */
    qint64 timeToSleep = (qint64)len * (1000000 / sizeof(short) / 2) / iSampleRate;
    /* timeToSleep is the worst case time, zero is the best case time,
       so let divide this time by 2 to have a mean between worst and best case time,
       pros: latency and jitter on input is improved, and output will have less chance to underflow
       cons: function call increased, increased overhead */
    timeToSleep /= 2;
    if (timeToSleep)
        QThread::usleep((unsigned long)timeToSleep);
}

bool CSoundCommonQT::isSampleRateSupported(const QAudioDeviceInfo &di, int samplerate) const
{
    samplerate = abs(samplerate);
#if defined(__linux__) && !defined(__ANDROID__)
    (void)di;
    return samplerate >= 1 && samplerate <= 192000;
#else
    return di.supportedSampleRates().contains(samplerate);
#endif
}

bool CSoundCommonQT::isDeviceGood(const QAudioDeviceInfo &di, const int *desiredsamplerate) const
{
    bool bSampleRateOk = false;
    for (const int* dsr=desiredsamplerate; *dsr; dsr++)
    {
        int samplerate = abs(*dsr);
        if (isSampleRateSupported(di, samplerate))
        {
            bSampleRateOk = true;
            break;
        }
    }
    return
        bSampleRateOk &&
        di.supportedChannelCounts().contains(2) && // TODO
        di.supportedSampleSizes().contains(16) &&
        di.supportedSampleTypes().contains(QAudioFormat::SignedInt) &&
        di.supportedByteOrders().contains(QAudioFormat::LittleEndian) &&
        di.supportedCodecs().contains("audio/pcm");
}

void CSoundCommonQT::setSamplerate(deviceprop& dp, const QAudioDeviceInfo &di, const int *desiredsamplerate) const
{
    dp.samplerates.clear();
    for (const int* dsr=desiredsamplerate; *dsr; dsr++)
    {
        int samplerate = abs(*dsr);
        dp.samplerates[samplerate] = isSampleRateSupported(di, samplerate);
    }
}

QAudioDeviceInfo CSoundCommonQT::getDevice()
{
    QAudioDeviceInfo di;
    if (sDev == DEFAULT_DEVICE_NAME)
        di = bInput ? QAudioDeviceInfo::defaultInputDevice() : QAudioDeviceInfo::defaultOutputDevice();
    else
    {
        QAudio::Mode mode = bInput ? QAudio::AudioInput : QAudio::AudioOutput;
        foreach(const QAudioDeviceInfo& tdi, QAudioDeviceInfo::availableDevices(mode))
        {
            string name(tdi.deviceName().toLocal8Bit().constData());
//            qDebug("CSoundCommonQT::getDevice() %i name=%s", bInput, name.c_str());
            if (name == sDev)
            {
                di = tdi;
                break;
            }
        }
    }
    return di;
}

QAudioFormat CSoundCommonQT::getFormat()
{
    QAudioFormat format;
    format.setSampleRate(iSampleRate);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setChannelCount(2);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");
    return format;
}

void CSoundCommonQT::Enumerate(vector<deviceprop>& devs, const int *desiredsamplerate)
{
    devs.clear();
    deviceprop dp;

    /* Default device */
    const QAudioDeviceInfo& di(bInput ? QAudioDeviceInfo::defaultInputDevice() : QAudioDeviceInfo::defaultOutputDevice());
    if (isDeviceGood(di, desiredsamplerate))
    {
        dp.name = DEFAULT_DEVICE_NAME;
        setSamplerate(dp, di, desiredsamplerate);
        devs.push_back(dp);
    }

    foreach(const QAudioDeviceInfo& di, QAudioDeviceInfo::availableDevices(bInput ? QAudio::AudioInput : QAudio::AudioOutput))
    {
        if (isDeviceGood(di, desiredsamplerate))
        {
            string name(di.deviceName().toLocal8Bit().constData());
//            qDebug("CSoundCommonQT::Enumerate() %i name=%s", bInput, name.c_str());
            dp.name = name;
            setSamplerate(dp, di, desiredsamplerate);
            devs.push_back(dp);
        }
    }
}

string CSoundCommonQT::GetDev()
{
    return sDev;
}

void CSoundCommonQT::SetDev(string sNewDev)
{
//    qDebug("CSoundCommonQT::SetDev() name=%s", sNewDev.c_str());
    if (sDev != sNewDev)
    {
        sDev = sNewDev;
        bDevChanged = true;
    }
}


/******************************/
/* QT Audio In Implementation */

CSoundInQT::CSoundInQT() :
    CSoundCommonQT(true), pAudioInput(NULL)
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
        bDevChanged = false;
        QAudioDeviceInfo di = getDevice();
        QAudioFormat format = getFormat();
        pAudioInput = new QAudioInput(di, format);
        if (pAudioInput->format() == format)
        {
            int size = iSampleRate * (int)sizeof(short) * pAudioInput->format().channelCount() * BUFFER_LEN_IN_SEC;
            pAudioInput->setBufferSize(size);
            QIODevice* pIODevice = pAudioInput->start();
    //        int n = pAudioInput->bufferSize();
            if (pAudioInput->error() == QAudio::NoError)
                this->pIODevice = pIODevice;
            else
                qDebug("CSoundInQT::Init() Can't open audio input (error = %i)", (int)pAudioInput->error());
        }
        else
            qDebug("CSoundInQT::Init() Can't open audio input (bad format)");
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
        int len = (int)(sizeof(short) * vecsSoundBuffer.Size());
        for (int pos=0;;) {
            int ret = (int)pIODevice->read((char*)&vecsSoundBuffer[pos], len);
            if (ret < 0)
                break;
            pos += ret/sizeof(short);
            len -= ret;
            if (len <= 0)
                break;
            sleep(len);
        }
        bError = len != 0;
    }
    return bError;
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
    CSoundCommonQT(false), pAudioOutput(NULL)
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
        bDevChanged = false;
        QAudioDeviceInfo di = getDevice();
        QAudioFormat format = getFormat();
        pAudioOutput = new QAudioOutput(di, format);
        if (pAudioOutput->format() == format)
        {
            int size = iSampleRate * (int)sizeof(short) * pAudioOutput->format().channelCount() * BUFFER_LEN_IN_SEC;
            pAudioOutput->setBufferSize(size);
            QIODevice* pIODevice = pAudioOutput->start();
    //        int n = pAudioOutput->bufferSize();
            if (pAudioOutput->error() == QAudio::NoError)
                this->pIODevice = pIODevice;
            else
                qDebug("CSoundInQT::Init() Can't open audio output (error = %i)", (int)pAudioOutput->error());
        }
        else
            qDebug("CSoundInQT::Init() Can't open audio output (bad format)");
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
        int len = (int)(sizeof(short) * vecsSoundBuffer.Size());
        for (int pos=0;;) {
            int ret = (int)pIODevice->write((char*)&vecsSoundBuffer[pos], len);
            if (ret < 0)
                break;
            pos += ret/sizeof(short);
            len -= ret;
            if (len <= 0)
                break;
            sleep(len);
        }
        bError = len != 0;
    }
    return bError;
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

