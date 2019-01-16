/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 * Volker Fischer, Ollie Haffenden
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

#ifndef AUDIOSOURCEDECODER_H
#define AUDIOSOURCEDECODER_H

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../util/Modul.h"
#include "../util/CRC.h"
#include "../TextMessage.h"
#include "../resample/Resample.h"
#include "../datadecoding/DataDecoder.h"
#include "../util/Utilities.h"
#include "AudioCodec.h"
#include "../MSC/audiosuperframe.h"

#ifdef HAVE_SPEEX
# include "../resample/speexresampler.h"
#else
# include "../resample/caudioresample.h"
#endif

/* Classes ********************************************************************/

class CAudioSourceDecoder : public CReceiverModul<_BINARY, _SAMPLE>
{
public:
    CAudioSourceDecoder();

    virtual ~CAudioSourceDecoder();

    bool CanDecode(CAudioParam::EAudCod eAudCod) {
        switch (eAudCod)
        {
        case CAudioParam::AC_NONE: return true;
        case CAudioParam::AC_AAC:  return bCanDecodeAAC;
        case CAudioParam::AC_OPUS: return bCanDecodeOPUS;
        case CAudioParam::AC_xHE_AAC: return bCanDecodexHE_AAC;
        default: return false;
        }
        return false;
    }
    int GetNumCorDecAudio();
    void SetReverbEffect(const _BOOLEAN bNER) {
        bUseReverbEffect = bNER;
    }
    _BOOLEAN GetReverbEffect() {
        return bUseReverbEffect;
    }

    _BOOLEAN bWriteToFile;

protected:

    /* General */
    _BOOLEAN DoNotProcessData;
    _BOOLEAN DoNotProcessAudDecoder;
    int iNumCorDecAudio;

    /* Text message */
    bool bTextMessageUsed;
    CTextMessageDecoder TextMessage;
    CVector<_BINARY> vecbiTextMessBuf;

    /* Resampling */
    bool bResample;
    int iResOutBlockSize;

#ifdef HAVE_SPEEX
    SpeexResampler ResampleObjL;
    SpeexResampler ResampleObjR;
#else
    CAudioResample ResampleObjL;
    CAudioResample ResampleObjR;
#endif

    CVector<_REAL> vecTempResBufInLeft;
    CVector<_REAL> vecTempResBufInRight;
    CVector<_REAL> vecTempResBufOutCurLeft;
    CVector<_REAL> vecTempResBufOutCurRight;
    CVector<_REAL> vecTempResBufOutOldLeft;
    CVector<_REAL> vecTempResBufOutOldRight;

    /* Drop-out masking (reverberation) */
    _BOOLEAN bAudioWasOK;
    _BOOLEAN bUseReverbEffect;
    CAudioReverb AudioRev;

    int iLenDecOutPerChan;
    AudioSuperFrame* pAudioSuperFrame;

    CAudioParam::EAudCod eAudioCoding;
    CAudioCodec* codec;
    int iBadBlockCount;
    string audiodecoder;
    bool bCanDecodeAAC;
    bool bCanDecodeOPUS;
    bool bCanDecodexHE_AAC;

    virtual void InitInternal(CParameter& Parameters);
    virtual void ProcessDataInternal(CParameter& Parameters);
    void CloseDecoder();
};

#endif // AUDIOSOURCEDECODER_H
