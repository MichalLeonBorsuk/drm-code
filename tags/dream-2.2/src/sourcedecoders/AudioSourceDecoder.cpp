/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Audio source encoder/decoder
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

#include "AudioSourceDecoder.h"
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>
#ifdef _WIN32
# include <direct.h>
#else
# include <sys/stat.h>
# include <sys/types.h>
#endif

#include "../MSC/aacsuperframe.h"
#include "../MSC/xheaacsuperframe.h"

/* Implementation *************************************************************/

CAudioSourceDecoder::CAudioSourceDecoder()
    :	bWriteToFile(false), TextMessage(false),
      bUseReverbEffect(true), codec(nullptr)
{
    /* Initialize Audio Codec List */
    CAudioCodec::InitCodecList();

    /* Needed by fdrmdialog.cpp to report missing codec */
    bCanDecodeAAC  = CAudioCodec::GetDecoder(CAudioParam::AC_AAC,  true) != nullptr;
    bCanDecodexHE_AAC = CAudioCodec::GetDecoder(CAudioParam::AC_xHE_AAC, true) != nullptr;
    bCanDecodeOPUS = CAudioCodec::GetDecoder(CAudioParam::AC_OPUS, true) != nullptr;
}

CAudioSourceDecoder::~CAudioSourceDecoder()
{
    /* Unreference Audio Codec List */
    CAudioCodec::UnrefCodecList();
}



void
CAudioSourceDecoder::ProcessDataInternal(CParameter & Parameters)
{
    bool bCurBlockOK;
    bool bGoodValues = false;


    Parameters.Lock();
    Parameters.vecbiAudioFrameStatus.Init(0);
    Parameters.vecbiAudioFrameStatus.ResetBitAccess();
    Parameters.Unlock();

    /* Check if something went wrong in the initialization routine */
    if (DoNotProcessData)
    {
        return;
    }

    //cerr << "got one logical frame of length " << pvecInputData->Size() << " bits" << endl;

    /* Text Message ********************************************************** */
    if (bTextMessageUsed)
    {
        /* Decode last four bytes of input block for text message */
        for (int i = 0; i < SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
            vecbiTextMessBuf[i] = (*pvecInputData)[iInputBlockSize - SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR + i];

        TextMessage.Decode(vecbiTextMessBuf);
    }

    /* Audio data header parsing ********************************************* */
    /* Check if audio shall not be decoded */
    if (DoNotProcessAudDecoder)
    {
        return;
    }

    /* Reset bit extraction access */
    (*pvecInputData).ResetBitAccess();

    bGoodValues = pAudioSuperFrame->parse(*pvecInputData);

    /* Audio decoding ******************************************************** */
    /* Init output block size to zero, this variable is also used for
       determining the position for writing the output vector */
    iOutputBlockSize = 0;
    int iResOutBlockSize = 0;
    //cerr << "audio superframe with " << pAudioSuperFrame->getNumFrames() << " frames" << endl;
    for (unsigned j = 0; j < pAudioSuperFrame->getNumFrames(); j++)
    {
        bool bCodecUpdated = false;
        bool bCurBlockFaulty = false; // just for Opus or any other codec with FEC

        if (bGoodValues)
        {
            CAudioCodec::EDecError eDecError;
            vector<uint8_t> audio_frame;
            uint8_t aac_crc_bits;
            pAudioSuperFrame->getFrame(audio_frame, aac_crc_bits, j);
            if(inputSampleRate != outputSampleRate) {
                eDecError = codec->Decode(audio_frame, aac_crc_bits, vecTempResBufInLeft, vecTempResBufInRight);
                if(eDecError==CAudioCodec::DECODER_ERROR_OK) {
                    /* Resample data */
                    iResOutBlockSize = outputSampleRate * vecTempResBufInLeft.Size() / inputSampleRate;
                    if(iResOutBlockSize != vecTempResBufOutCurLeft.Size()) { // NOOP for AAC, needed for xHE-AAC
                        vecTempResBufOutCurLeft.Init(iResOutBlockSize, 0.0);
                        vecTempResBufOutCurRight.Init(iResOutBlockSize, 0.0);
                        _REAL rRatio = _REAL(outputSampleRate) / _REAL(inputSampleRate);
                        ResampleObjL.Init(vecTempResBufInLeft.Size(), rRatio);
                        ResampleObjR.Init(vecTempResBufInLeft.Size(), rRatio);
                    }

                    ResampleObjL.Resample(vecTempResBufInLeft, vecTempResBufOutCurLeft);
                    ResampleObjR.Resample(vecTempResBufInRight, vecTempResBufOutCurRight);
                }
            }
            else {
                eDecError = codec->Decode(audio_frame, aac_crc_bits, vecTempResBufOutCurLeft, vecTempResBufOutCurRight);
            }

            iResOutBlockSize = vecTempResBufOutCurLeft.Size();

            bCurBlockOK = (eDecError == CAudioCodec::DECODER_ERROR_OK);

            /* Call decoder update */
            if (!bCodecUpdated)
            {
                bCodecUpdated = true;
                Parameters.Lock();
                int iCurSelAudioServ = Parameters.GetCurSelAudioService();
                codec->DecUpdate(Parameters.Service[unsigned(iCurSelAudioServ)].AudioParam);
                Parameters.Unlock();
            }
            /* OPH: add frame status to vector for RSCI */
            Parameters.Lock();
            Parameters.vecbiAudioFrameStatus.Add(eDecError == CAudioCodec::DECODER_ERROR_OK ? 0 : 1);
            Parameters.Unlock();
        }
        else
        {
            /* DRM super-frame header was wrong, set flag to "bad block" */
            bCurBlockOK = false;
            /* OPH: update audio status vector for RSCI */
            Parameters.Lock();
            Parameters.vecbiAudioFrameStatus.Add(1);
            Parameters.Unlock();
        }

        // This code is independent of particular audio source type and should work with all codecs

        /* Postprocessing of audio blocks, status informations -------------- */
        ETypeRxStatus status = reverb.apply(bCurBlockOK, bCurBlockFaulty, vecTempResBufOutCurLeft, vecTempResBufOutCurRight);

        if (bCurBlockOK && !bCurBlockFaulty)
        {
            /* Increment correctly decoded audio blocks counter */
            iNumCorDecAudio++;
        }

        {
            double l = 0.0, r = 0.0;
            for(int i=0; i<vecTempResBufOutCurLeft.Size(); i++) {
                l += vecTempResBufOutCurLeft[i];
                r += vecTempResBufOutCurRight[i];
            }
            //cerr << "energy after resampling and reverb left " << (l/vecTempResBufOutCurLeft.Size()) << " right " << (l/vecTempResBufOutCurRight.Size()) << endl;
        }

        Parameters.Lock();
        Parameters.ReceiveStatus.SLAudio.SetStatus(status);
        Parameters.ReceiveStatus.LLAudio.SetStatus(status);
        Parameters.AudioComponentStatus[unsigned(Parameters.GetCurSelAudioService())].SetStatus(status);
        Parameters.Unlock();

        /* Conversion from _REAL to _SAMPLE with special function */
        for (int i = 0; i < iResOutBlockSize; i++)
        {
            (*pvecOutputData)[iOutputBlockSize + i * 2] = Real2Sample(vecTempResBufOutCurLeft[i]);	/* Left channel */
            (*pvecOutputData)[iOutputBlockSize + i * 2 + 1] = Real2Sample(vecTempResBufOutCurRight[i]);	/* Right channel */
        }

        /* Add new block to output block size ("* 2" for stereo output block) */
        iOutputBlockSize += iResOutBlockSize * 2;

        if(iOutputBlockSize==0) {
            //cerr << "iOutputBlockSize is zero" << endl;
        }
        else {
            double d=0.0;
            for (int i = 0; i < iOutputBlockSize; i++)
            {
                double n = (*pvecOutputData)[i];
                d += n*n;
            }
            //cerr << "energy after converting " << iOutputBlockSize << " samples back to int " << sqrt(d/iOutputBlockSize) << endl;
        }

    }
}

void
CAudioSourceDecoder::InitInternal(CParameter & Parameters)
{
    /* Close previous decoder instance if any */
    CloseDecoder();

    /* Init error flags and output block size parameter. The output block
       size is set in the processing routine. We must set it here in case
       of an error in the initialization, this part in the processing
       routine is not being called */
    DoNotProcessAudDecoder = false;
    DoNotProcessData = false;
    iOutputBlockSize = 0;

    /* Set audiodecoder to empty string - means "unknown" and "can't decode" to GUI */
    audiodecoder = "";

    try
    {
        Parameters.Lock();

        /* Init counter for correctly decoded audio blocks */
        iNumCorDecAudio = 0;

        /* Init "audio was ok" flag */
        bAudioWasOK = true;

         /* Get number of total input bits for this module */
         iInputBlockSize = Parameters.iNumAudioDecoderBits;

        /* Get current selected audio service */
        CService& service = Parameters.Service[unsigned(Parameters.GetCurSelAudioService())];

        /* Get current audio coding */
        eAudioCoding = service.AudioParam.eAudioCoding;

        /* The requirement for this module is that the stream is used and the service is an audio service. Check it here */
        if ((service.eAudDataFlag != CService::SF_AUDIO) || (service.AudioParam.iStreamID == STREAM_ID_NOT_USED))
        {
            throw CInitErr(ET_ALL);
        }

        /* Current audio stream ID */
        unsigned curAudioStreamID =  unsigned(service.AudioParam.iStreamID);

        int iTotalFrameSize = Parameters.Stream[curAudioStreamID].iLenPartA+Parameters.Stream[curAudioStreamID].iLenPartB;

        /* Init text message application ------------------------------------ */
        if (service.AudioParam.bTextflag)
        {
            bTextMessageUsed = true;

            /* Get a pointer to the string */
            TextMessage.Init(&service.AudioParam.strTextMessage);

            /* Init vector for text message bytes */
            vecbiTextMessBuf.Init(SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR);

            iTotalFrameSize -= NUM_BYTES_TEXT_MESS_IN_AUD_STR;
        }
        else {
            bTextMessageUsed = false;
        }
        if(eAudioCoding==CAudioParam::AC_xHE_AAC) {
            XHEAACSuperFrame* p = new XHEAACSuperFrame();
            // part B should be enough as xHE-AAC MUST be EEP but its easier to just add them
            p->init(service.AudioParam, unsigned(iTotalFrameSize));
            pAudioSuperFrame = p;
        }
        else {
            AACSuperFrame *p = new AACSuperFrame();
            p->init(service.AudioParam,  Parameters.GetWaveMode(), unsigned(Parameters.Stream[curAudioStreamID].iLenPartA), unsigned(Parameters.Stream[curAudioStreamID].iLenPartB));
            pAudioSuperFrame = p;
        }
        /* Get decoder instance */
        codec = CAudioCodec::GetDecoder(eAudioCoding);

        if (codec->CanDecode(eAudioCoding))
            audiodecoder = codec->DecGetVersion();

        if(bWriteToFile)
        {
            codec->openFile(Parameters);
        }
        else {
            codec->closeFile();
        }

        codec->Init(service.AudioParam, iInputBlockSize);

        /* Init decoder */
        codec->DecOpen(service.AudioParam, inputSampleRate);
        if (inputSampleRate == 0) {
            cerr << "DecOpen sample rate was zero - this should not happen" << endl;
            throw CInitErr(ET_AUDDECODER);
            //inputSampleRate = 12000; //mjf - 05May19 - stops this from dying in Mode C TODO
        }

        int iLenDecOutPerChan = 0; // no need to use the one from the codec
        unsigned numFrames = unsigned(pAudioSuperFrame->getNumFrames());
        if(numFrames==0) {
            // xHE-AAC - can't tell yet!
        }
        else {
            int samplesPerChannelPerSuperFrame = int(pAudioSuperFrame->getSuperFrameDurationMilliseconds()) * inputSampleRate / 1000;
            iLenDecOutPerChan = samplesPerChannelPerSuperFrame / int(numFrames);
        }

        /* set string for GUI */
        Parameters.audiodecoder = audiodecoder;

        /* Set number of Audio frames for log file */
        // TODO Parameters.iNumAudioFrames = iNumAudioFrames;

        outputSampleRate = Parameters.GetAudSampleRate();
        Parameters.Unlock();

        /* Since we do not do Mode E or correct for sample rate offsets here (yet), we do not
           have to consider larger buffers. An audio frame always corresponds to 400 ms */
        int iMaxLenResamplerOutput = int(_REAL(outputSampleRate) * 0.4 /* 400ms */  * 2 /* for stereo */ );

        if(inputSampleRate != outputSampleRate) {
            _REAL rRatio = _REAL(outputSampleRate) / _REAL(inputSampleRate);
            /* Init resample objects */
            ResampleObjL.Init(iLenDecOutPerChan, rRatio);
            ResampleObjR.Init(iLenDecOutPerChan, rRatio);
        }

        int iResOutBlockSize = outputSampleRate * iLenDecOutPerChan / inputSampleRate;

        //cerr << "output block size per channel " << iResOutBlockSize << " = samples " << iLenDecOutPerChan << " * " << Parameters.GetAudSampleRate() << " / " << iAudioSampleRate << endl;

        /* Additional buffers needed for resampling since we need conversation
           between _REAL and _SAMPLE. We have to init the buffers with
           zeros since it can happen, that we have bad CRC right at the
           start of audio blocks */
        vecTempResBufInLeft.Init(iLenDecOutPerChan, 0.0);
        vecTempResBufInRight.Init(iLenDecOutPerChan, 0.0);
        vecTempResBufOutCurLeft.Init(iResOutBlockSize, 0.0);
        vecTempResBufOutCurRight.Init(iResOutBlockSize, 0.0);

        reverb.Init(outputSampleRate, bUseReverbEffect);

        /* With this parameter we define the maximum length of the output
           buffer. The cyclic buffer is only needed if we do a sample rate
           correction due to a difference compared to the transmitter. But for
           now we do not correct and we could stay with a single buffer
           Maybe TODO: sample rate correction to avoid audio dropouts */
        iMaxOutputBlockSize = iMaxLenResamplerOutput;
    }

    catch(CInitErr CurErr)
    {
        Parameters.Unlock();

        switch (CurErr.eErrType)
        {
        case ET_ALL:
            /* An init error occurred, do not process data in this module */
            DoNotProcessData = true;
            break;

        case ET_AUDDECODER:
            /* Audio part should not be decoded, set flag */
            DoNotProcessAudDecoder = true;
            break;
        }

        /* In all cases set output size to zero */
        iOutputBlockSize = 0;
    }
}

int
CAudioSourceDecoder::GetNumCorDecAudio()
{
    /* Return number of correctly decoded audio blocks. Reset counter
       afterwards */
    const int iRet = iNumCorDecAudio;

    iNumCorDecAudio = 0;

    return iRet;
}

void
CAudioSourceDecoder::CloseDecoder()
{
    if (codec != nullptr)
        codec->DecClose();
}

