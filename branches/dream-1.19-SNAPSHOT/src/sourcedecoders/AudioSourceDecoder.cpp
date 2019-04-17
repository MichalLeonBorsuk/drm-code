/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Audio source decoder
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
#include "fdk_aac_codec.h"

CAudioSourceDecoder::CAudioSourceDecoder()
    :	bWriteToFile(FALSE), bUseReverbEffect(TRUE), AudioRev(),
	codec(new FdkAacCodec()),
	canDecodeAAC(TRUE),
        canDecodeCELP(FALSE), canDecodeHVXC(FALSE),
        pFile(NULL)
{
}

string
CAudioSourceDecoder::AACFileName(CParameter & Parameters)
{
    // Store AAC-data in file
    stringstream ss;
    ss << "test/aac_";

    if (Parameters.
            Service[Parameters.GetCurSelAudioService()].AudioParam.
            eAudioSamplRate == CAudioParam::AS_12KHZ)
    {
        ss << "12kHz_";
    }
    else
        ss << "24kHz_";

    switch (Parameters.
            Service[Parameters.GetCurSelAudioService()].
            AudioParam.eAudioMode)
    {
    case CAudioParam::AM_MONO:
        ss << "mono";
        break;

    case CAudioParam::AM_P_STEREO:
        ss << "pstereo";
        break;

    case CAudioParam::AM_STEREO:
        ss << "stereo";
        break;
    }

    if (Parameters.
            Service[Parameters.GetCurSelAudioService()].AudioParam.
            eSBRFlag == CAudioParam::SB_USED)
    {
        ss << "_sbr";
    }
    ss << ".dat";

    return ss.str();
}

string
CAudioSourceDecoder::CELPFileName(CParameter & Parameters)
{
    stringstream ss;
    ss << "test/celp_";
    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eAudioSamplRate == CAudioParam::AS_8_KHZ)
    {
        ss << "8kHz_" << 
            iTableCELP8kHzUEPParams
                 [Parameters.
                  Service[Parameters.GetCurSelAudioService()].
                  AudioParam.iCELPIndex][0];
    }
    else
    {
        ss << "16kHz_" <<
            iTableCELP16kHzUEPParams
                 [Parameters.
                  Service[Parameters.GetCurSelAudioService()].
                  AudioParam.iCELPIndex][0];
    }
    ss << "bps";

    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eSBRFlag == CAudioParam::SB_USED)
    {
        ss << "_sbr";
    }
    ss << ".dat";

    return ss.str();
}

string
CAudioSourceDecoder::HVXCFileName(CParameter & Parameters)
{
    stringstream ss;
    ss << "test/hvxc_";
    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eAudioSamplRate == CAudioParam::AS_8_KHZ)
    {
        ss << "8kHz";
    }
    else
    {
        ss << "unknown";
    }

    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eHVXCRate == CAudioParam::HR_2_KBIT)
    {
        ss << "_2kbps";
    }
    else if (Parameters.Service[Parameters.GetCurSelAudioService()].
             AudioParam.eHVXCRate == CAudioParam::HR_4_KBIT)
    {
        ss << "_4kbps";
    }
    else
    {
        ss << "_unknown";
    }

    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.bHVXCCRC)
    {
        ss << "_crc";
    }

    if (Parameters.Service[Parameters.GetCurSelAudioService()].
            AudioParam.eSBRFlag == CAudioParam::SB_USED)
    {
        ss << "_sbr";
    }
    ss << ".dat";

    return ss.str();
}

void
CAudioSourceDecoder::ProcessDataInternal(CParameter & Parameters)
{
    int i, j;
    _BOOLEAN bCurBlockOK;
    _BOOLEAN bGoodValues;

    bGoodValues = FALSE;

    Parameters.Lock();
    Parameters.vecbiAudioFrameStatus.Init(0);
    Parameters.vecbiAudioFrameStatus.ResetBitAccess();
    Parameters.Unlock();

    /* Check if something went wrong in the initialization routine */
    if (DoNotProcessData == TRUE)
    {
        return;
    }

    /* Text Message ********************************************************** */
    /* Total frame size depends on whether text message is used or not */
    if (bTextMessageUsed == TRUE)
    {
        /* Decode last for bytes of input block for text message */
        for (i = 0; i < SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
            vecbiTextMessBuf[i] = (*pvecInputData)[iTotalFrameSize + i];

        TextMessage.Decode(vecbiTextMessBuf);
    }

    /* Audio data header parsing ********************************************* */
    /* Check if audio shall not be decoded */
    if (DoNotProcessAudDecoder == TRUE)
    {
        return;
    }

    /* Reset bit extraction access */
    (*pvecInputData).ResetBitAccess();

    vector< vector<uint8_t> > audio_frame(iNumAudioFrames);
    vector<uint8_t> aac_crc_bits(iNumAudioFrames);

    /* Check which audio coding type is used */
    if (eAudioCoding == CAudioParam::AC_AAC)
    {
        /* AAC super-frame-header ------------------------------------------- */
        bGoodValues = TRUE;
        size_t iPrevBorder = 0;

        for (i = 0; i < iNumBorders; i++)
        {
            /* Frame border in bytes (12 bits) */
            size_t iFrameBorder = (*pvecInputData).Separate(12);

            /* The length is difference between borders */
            if(iFrameBorder>=iPrevBorder)
                audio_frame[i].resize(iFrameBorder - iPrevBorder);
            else
                bGoodValues = FALSE;
            iPrevBorder = iFrameBorder;
        }

        /* Byte-alignment (4 bits) in case of 10 audio frames */
        if (iNumBorders == 9)
            (*pvecInputData).Separate(4);

        /* Frame length of last frame */
        if(iAudioPayloadLen>=int(iPrevBorder))
            audio_frame[iNumBorders].resize(iAudioPayloadLen - iPrevBorder);
        else
            bGoodValues = FALSE;

        /* Check if frame length entries represent possible values */
        for (i = 0; i < iNumAudioFrames; i++)
        {
            if(int(audio_frame[i].size()) > iMaxLenOneAudFrame)
            {
                bGoodValues = FALSE;
            }
        }

        if (bGoodValues == TRUE)
        {
            /* Higher-protected part */
            for (i = 0; i < iNumAudioFrames; i++)
            {
                /* Extract higher protected part bytes (8 bits per byte) */
                for (j = 0; j < iNumHigherProtectedBytes; j++)
                    audio_frame[i][j] = _BINARY((*pvecInputData).Separate(8));

                /* Extract CRC bits (8 bits) */
                aac_crc_bits[i] = _BINARY((*pvecInputData).Separate(8));
            }

            /* Lower-protected part */
            for (i = 0; i < iNumAudioFrames; i++)
            {
                /* First calculate frame length, derived from higher protected
                   part frame length and total size */
                const int iNumLowerProtectedBytes =
                    audio_frame[i].size() - iNumHigherProtectedBytes;

                /* Extract lower protected part bytes (8 bits per byte) */
                for (j = 0; j < iNumLowerProtectedBytes; j++)
                {
                    audio_frame[i][iNumHigherProtectedBytes + j] =
                        _BINARY((*pvecInputData).Separate(8));
                }
            }
        }
    }
    else if (eAudioCoding == CAudioParam::AC_CELP)
    {
        /* celp_super_frame(celp_table_ind) --------------------------------- */
        /* Higher-protected part */
        for (i = 0; i < iNumAudioFrames; i++)
        {
            celp_frame[i].ResetBitAccess();

            /* Extract higher protected part bits */
            for (j = 0; j < iNumHigherProtectedBits; j++)
                celp_frame[i].Enqueue((*pvecInputData).Separate(1), 1);

            /* Extract CRC bits (8 bits) if used */
            if (bCELPCRC == TRUE)
                celp_crc_bits[i] = _BINARY((*pvecInputData).Separate(8));
        }

        /* Lower-protected part */
        for (i = 0; i < iNumAudioFrames; i++)
        {
            for (j = 0; j < iNumLowerProtectedBits; j++)
                celp_frame[i].Enqueue((*pvecInputData).Separate(1), 1);
        }
    }
    else if (eAudioCoding == CAudioParam::AC_HVXC)
    {
        for (i = 0; i < iNumAudioFrames; i++)
        {
            hvxc_frame[i].ResetBitAccess();

            for (j = 0; j < iNumHvxcBits; j++)
                hvxc_frame[i].Enqueue((*pvecInputData).Separate(1), 1);
        }
    }


    /* Audio decoding ******************************************************** */
    /* Init output block size to zero, this variable is also used for
       determining the position for writing the output vector */
    iOutputBlockSize = 0;

    for (j = 0; j < iNumAudioFrames; j++)
    {
        if (eAudioCoding == CAudioParam::AC_AAC)
        {
            if (bGoodValues == TRUE)
            {

                if (bWriteToFile && pFile!=NULL)
                {
                    int iNewFrL = audio_frame[j].size() + 1;
                    fwrite((void *) &iNewFrL, size_t(4), size_t(1), pFile);	// frame length
                    fwrite((void *) &audio_frame[i][0], size_t(1), size_t(iNewFrL), pFile);	// data
                    fflush(pFile);
                }

                /* Call decoder routine */
                EDecError eDecError = codec->Decode(audio_frame[i], aac_crc_bits[i], vecTempResBufOutCurLeft, vecTempResBufOutCurRight);

                /* OPH: add frame status to vector for RSCI */
                Parameters.Lock();
                Parameters.vecbiAudioFrameStatus.Add(eDecError == 0 ? 0 : 1);
                Parameters.Unlock();

            }
            else
            {
                /* DRM AAC header was wrong, set flag to "bad block" */
                bCurBlockOK = FALSE;
                /* OPH: update audio status vector for RSCI */
                Parameters.Lock();
                Parameters.vecbiAudioFrameStatus.Add(1);
                Parameters.Unlock();
            }
        }
        else
            bCurBlockOK = FALSE;

        // This code is independent of particular audio source type and should work fine with CELP and HVXC

        /* Postprocessing of audio blocks, status informations -------------- */
        if (bCurBlockOK == FALSE)
        {
            if (bAudioWasOK == TRUE)
            {
                /* Post message to show that CRC was wrong (yellow light) */
                Parameters.Lock();
                Parameters.ReceiveStatus.Audio.SetStatus(DATA_ERROR);
                Parameters.ReceiveStatus.LLAudio.SetStatus(DATA_ERROR);
                Parameters.Unlock();

                /* Fade-out old block to avoid "clicks" in audio. We use linear
                   fading which gives a log-fading impression */
                for (i = 0; i < iResOutBlockSize; i++)
                {
                    /* Linear attenuation with time of OLD buffer */
                    const _REAL rAtt =
                        (_REAL) 1.0 - (_REAL) i / iResOutBlockSize;

                    vecTempResBufOutOldLeft[i] *= rAtt;
                    vecTempResBufOutOldRight[i] *= rAtt;

                    if (bUseReverbEffect == TRUE)
                    {
                        /* Fade in input signal for reverberation to avoid
                           clicks */
                        const _REAL rAttRev = (_REAL) i / iResOutBlockSize;

                        /* Cross-fade reverberation effect */
                        const _REAL rRevSam = (1.0 - rAtt) * AudioRev.
                                              ProcessSample(vecTempResBufOutOldLeft[i] *
                                                            rAttRev,
                                                            vecTempResBufOutOldRight[i] *
                                                            rAttRev);

                        /* Mono reverbration signal */
                        vecTempResBufOutOldLeft[i] += rRevSam;
                        vecTempResBufOutOldRight[i] += rRevSam;
                    }
                }

                /* Set flag to show that audio block was bad */
                bAudioWasOK = FALSE;
            }
            else
            {
                Parameters.Lock();
                Parameters.ReceiveStatus.Audio.SetStatus(CRC_ERROR);
                Parameters.ReceiveStatus.LLAudio.SetStatus(CRC_ERROR);
                Parameters.Unlock();

                if (bUseReverbEffect == TRUE)
                {
                    /* Add Reverberation effect */
                    for (i = 0; i < iResOutBlockSize; i++)
                    {
                        /* Mono reverberation signal */
                        vecTempResBufOutOldLeft[i] =
                            vecTempResBufOutOldRight[i] = AudioRev.
                                                          ProcessSample(0, 0);
                    }
                }
            }

            /* Write zeros in current output buffer */
            for (i = 0; i < iResOutBlockSize; i++)
            {
                vecTempResBufOutCurLeft[i] = (_REAL) 0.0;
                vecTempResBufOutCurRight[i] = (_REAL) 0.0;
            }
        }
        else
        {
            /* Increment correctly decoded audio blocks counter */
            iNumCorDecAudio++;

            Parameters.Lock();
            Parameters.ReceiveStatus.Audio.SetStatus(RX_OK);
            Parameters.ReceiveStatus.LLAudio.SetStatus(RX_OK);
            Parameters.Unlock();

            if (bAudioWasOK == FALSE)
            {
                if (bUseReverbEffect == TRUE)
                {
                    /* Add "last" reverbration only to old block */
                    for (i = 0; i < iResOutBlockSize; i++)
                    {
                        /* Mono reverberation signal */
                        vecTempResBufOutOldLeft[i] =
                            vecTempResBufOutOldRight[i] = AudioRev.
                                                          ProcessSample(vecTempResBufOutOldLeft[i],
                                                                  vecTempResBufOutOldRight[i]);
                    }
                }

                /* Fade-in new block to avoid "clicks" in audio. We use linear
                   fading which gives a log-fading impression */
                for (i = 0; i < iResOutBlockSize; i++)
                {
                    /* Linear attenuation with time */
                    const _REAL rAtt = (_REAL) i / iResOutBlockSize;

                    vecTempResBufOutCurLeft[i] *= rAtt;
                    vecTempResBufOutCurRight[i] *= rAtt;

                    if (bUseReverbEffect == TRUE)
                    {
                        /* Cross-fade reverberation effect */
                        const _REAL rRevSam = (1.0 - rAtt) * AudioRev.
                                              ProcessSample(0, 0);

                        /* Mono reverberation signal */
                        vecTempResBufOutCurLeft[i] += rRevSam;
                        vecTempResBufOutCurRight[i] += rRevSam;
                    }
                }

                /* Reset flag */
                bAudioWasOK = TRUE;
            }
        }

        /* Conversion from _REAL to _SAMPLE with special function */
        for (i = 0; i < iResOutBlockSize; i++)
        {
            (*pvecOutputData)[iOutputBlockSize + i * 2] = Real2Sample(vecTempResBufOutOldLeft[i]);	/* Left channel */
            (*pvecOutputData)[iOutputBlockSize + i * 2 + 1] = Real2Sample(vecTempResBufOutOldRight[i]);	/* Right channel */
        }

        /* Add new block to output block size ("* 2" for stereo output block) */
        iOutputBlockSize += iResOutBlockSize * 2;

        /* Store current audio block */
        for (i = 0; i < iResOutBlockSize; i++)
        {
            vecTempResBufOutOldLeft[i] = vecTempResBufOutCurLeft[i];
            vecTempResBufOutOldRight[i] = vecTempResBufOutCurRight[i];
        }
    }
}

void
CAudioSourceDecoder::InitInternal(CParameter & Parameters)
{
    int iAudioSampleRate;

    /*
    	Since we use the exception mechanism in this init routine, the sequence of
    	the individual initializations is very important!
    	Requirement for text message is "stream is used" and "audio service".
    	Requirement for AAC decoding are the requirements above plus "audio coding
    	is AAC"
    */
    int iCurAudioStreamID;
    int iMaxLenResamplerOutput;
    int iCurSelServ;

    /* Init error flags and output block size parameter. The output block
       size is set in the processing routine. We must set it here in case
       of an error in the initialization, this part in the processing
       routine is not being called */
    DoNotProcessAudDecoder = FALSE;
    DoNotProcessData = FALSE;
    iOutputBlockSize = 0;

    /* Set audiodecoder to empty string - means "unknown" and "can't decode" to GUI */
    audiodecoder = "";

    try
    {

        Parameters.Lock();

        /* Init counter for correctly decoded audio blocks */
        iNumCorDecAudio = 0;

        /* Init "audio was ok" flag */
        bAudioWasOK = TRUE;

        /* Get number of total input bits for this module */
        iInputBlockSize = Parameters.iNumAudioDecoderBits;

        /* Get current selected audio service */
        iCurSelServ = Parameters.GetCurSelAudioService();

        /* Current audio stream ID */
        iCurAudioStreamID =
            Parameters.Service[iCurSelServ].AudioParam.iStreamID;

        /* The requirement for this module is that the stream is used and the
           service is an audio service. Check it here */
        if ((Parameters.Service[iCurSelServ].  eAudDataFlag != CService::SF_AUDIO) ||
                (iCurAudioStreamID == STREAM_ID_NOT_USED))
        {
            throw CInitErr(ET_ALL);
        }

	(void)codec->DecOpen(Parameters.Service[iCurSelServ].AudioParam, iAudioSampleRate);

        /* Init text message application ------------------------------------ */
        if (Parameters.Service[iCurSelServ].AudioParam.bTextflag)
        {
            bTextMessageUsed = TRUE;

            /* Get a pointer to the string */
            TextMessage.Init(&Parameters.Service[iCurSelServ].AudioParam.
                             strTextMessage);

            /* Total frame size is input block size minus the bytes for the text
               message */
            iTotalFrameSize = iInputBlockSize -
                              SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR;

            /* Init vector for text message bytes */
            vecbiTextMessBuf.Init(SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR); 
	}
	else {
            bTextMessageUsed = FALSE;

            /* All bytes are used for AAC data, no text message present */
            iTotalFrameSize = iInputBlockSize;
        }

        /* Get audio coding type */
        eAudioCoding =
            Parameters.Service[iCurSelServ].AudioParam.eAudioCoding;

        if (eAudioCoding == CAudioParam::AC_AAC)
        {
            audiodecoder = codec->DecGetVersion();

            /* Length of higher protected part of audio stream */
            const int iLenAudHigh = Parameters.Stream[iCurAudioStreamID].iLenPartA;

            int iNumHeaderBytes;

            /* Set number of AAC frames in a AAC super-frame */
            switch (Parameters.Service[iCurSelServ].AudioParam.eAudioSamplRate)	/* Only 12 kHz and 24 kHz is allowed */
            {
            case CAudioParam::AS_12KHZ:
                iNumAudioFrames = 5;
                iNumHeaderBytes = 6;
                break;

            case CAudioParam::AS_24KHZ:
                iNumAudioFrames = 10;
                iNumHeaderBytes = 14;
                break;

            default:
                /* Some error occurred, throw error */
                throw CInitErr(ET_AUDDECODER);
                break;
            }

            /* Number of borders */
            iNumBorders = iNumAudioFrames - 1;

            /* The audio_payload_length is derived from the length of the audio
               super frame (data_length_of_part_A + data_length_of_part_B)
               subtracting the audio super frame overhead (bytes used for the
               audio super frame header() and for the aac_crc_bits)
               (5.3.1.1, Table 5) */
            iAudioPayloadLen = iTotalFrameSize / SIZEOF__BYTE -
                               iNumHeaderBytes - iNumAudioFrames;

            /* Check iAudioPayloadLen value, only positive values make sense */
            if (iAudioPayloadLen < 0)
                throw CInitErr(ET_AUDDECODER);

            /* Calculate number of bytes for higher protected blocks */
            iNumHigherProtectedBytes = (iLenAudHigh - iNumHeaderBytes -
                                        iNumAudioFrames /* CRC bytes */ ) /
                                       iNumAudioFrames;

            if (iNumHigherProtectedBytes < 0)
                iNumHigherProtectedBytes = 0;

            /* The maximum length for one audio frame is "iAudioPayloadLen". The
               regular size will be much shorter since all audio frames share
               the total size, but we do not know at this time how the data is
               split in the transmitter source coder */
            iMaxLenOneAudFrame = iAudioPayloadLen;

            if(bWriteToFile)
            {
                string fn = AACFileName(Parameters);
                if(pFile)
                    fclose(pFile);
                pFile = fopen(fn.c_str(), "wb");
            }
        }
        else
        {
            /* Audio codec not supported */
            throw CInitErr(ET_AUDDECODER);
        }

        /* set string for GUI */
        Parameters.audiodecoder = audiodecoder;

        /* Set number of Audio frames for log file */
        Parameters.iNumAudioFrames = iNumAudioFrames;

        /* Since we do not correct for sample rate offsets here (yet), we do not
           have to consider larger buffers. An audio frame always corresponds
           to 400 ms */
        iMaxLenResamplerOutput = (int) ((_REAL) Parameters.GetAudSampleRate() *
                                        (_REAL) 0.4 /* 400ms */  *
                                        2 /* for stereo */ );

        iResOutBlockSize = (int) ((_REAL) iLenDecOutPerChan *
                                  Parameters.GetAudSampleRate() / iAudioSampleRate);

        /* Additional buffers needed for resampling since we need conversation
           between _REAL and _SAMPLE. We have to init the buffers with
           zeros since it can happen, that we have bad CRC right at the
           start of audio blocks */
        vecTempResBufInLeft.Init(iLenDecOutPerChan, (_REAL) 0.0);
        vecTempResBufInRight.Init(iLenDecOutPerChan, (_REAL) 0.0);
        vecTempResBufOutCurLeft.Init(iResOutBlockSize, (_REAL) 0.0);
        vecTempResBufOutCurRight.Init(iResOutBlockSize, (_REAL) 0.0);
        vecTempResBufOutOldLeft.Init(iResOutBlockSize, (_REAL) 0.0);
        vecTempResBufOutOldRight.Init(iResOutBlockSize, (_REAL) 0.0);

        /* Init resample objects */
        ResampleObjL.Init(iLenDecOutPerChan,
                          (_REAL) Parameters.GetAudSampleRate() / iAudioSampleRate);
        ResampleObjR.Init(iLenDecOutPerChan,
                          (_REAL) Parameters.GetAudSampleRate() / iAudioSampleRate);

        /* Clear reverberation object */
        AudioRev.Init(1.0 /* seconds delay */, Parameters.GetAudSampleRate());
        AudioRev.Clear();

        /* With this parameter we define the maximum lenght of the output
           buffer. The cyclic buffer is only needed if we do a sample rate
           correction due to a difference compared to the transmitter. But for
           now we do not correct and we could stay with a single buffer
           Maybe TODO: sample rate correction to avoid audio dropouts */
        iMaxOutputBlockSize = iMaxLenResamplerOutput;

        Parameters.Unlock();
    }

    catch(CInitErr CurErr)
    {
        Parameters.Unlock();

        switch (CurErr.eErrType)
        {
        case ET_ALL:
            /* An init error occurred, do not process data in this module */
            DoNotProcessData = TRUE;
            break;

        case ET_AUDDECODER:
            /* Audio part should not be decdoded, set flag */
            DoNotProcessAudDecoder = TRUE;
            break;

        default:
            DoNotProcessData = TRUE;
        }

        /* In all cases set output size to zero */
        iOutputBlockSize = 0;
    }
}

int
CAudioSourceDecoder::GetNumCorDecAudio()
{
    /* Return number of correctly decoded audio blocks. Reset counter afterwards */
    const int iRet = iNumCorDecAudio;
    iNumCorDecAudio = 0;
    return iRet;
}

CAudioSourceDecoder::~CAudioSourceDecoder()
{
    codec->DecClose();
    delete codec;
}
