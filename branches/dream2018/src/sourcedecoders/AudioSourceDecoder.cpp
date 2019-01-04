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
#ifdef _WIN32
# include <direct.h>
#else
# include <sys/stat.h>
# include <sys/types.h>
#endif


/* Implementation *************************************************************/

CAudioSourceDecoder::CAudioSourceDecoder()
    :	bWriteToFile(TRUE), TextMessage(FALSE),
        bUseReverbEffect(TRUE), codec(nullptr)
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
    _BOOLEAN bCurBlockOK;
    _BOOLEAN bGoodValues;

    //int iDecChannels;
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

    //cerr << "got one logical frame of length " << pvecInputData->Size() << " bits" << endl;

    /* Text Message ********************************************************** */
    /* Total frame size depends on whether text message is used or not */
    if (bTextMessageUsed == TRUE)
    {
        /* Decode last four bytes of input block for text message */
        for (int i = 0; i < SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
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
    if (eAudioCoding == CAudioParam::AC_AAC || eAudioCoding == CAudioParam::AC_OPUS)
    {
        /* AAC super-frame-header ------------------------------------------- */
        bGoodValues = TRUE;
        size_t iPrevBorder = 0;

        for (int i = 0; i < iNumBorders; i++)
        {
            /* Frame border in bytes (12 bits) */
            size_t iFrameBorder = (*pvecInputData).Separate(12);

            /* The length is difference between borders */
            if(iFrameBorder>=iPrevBorder)
            {
                int size = iFrameBorder - iPrevBorder;
                if (size < iNumHigherProtectedBytes)
                    size = iNumHigherProtectedBytes;
                else if (size > iMaxLenOneAudFrame)
                    size = iMaxLenOneAudFrame;
                audio_frame[i].resize(size);
            }
            else
                bGoodValues = FALSE;
            iPrevBorder = iFrameBorder;
        }

        /* Byte-alignment (4 bits) in case of odd number of borders */
        if (iNumBorders & 1)
            (*pvecInputData).Separate(4);

        /* Frame length of last frame */
        if (iNumBorders != iNumAudioFrames)
        {
            if(iAudioPayloadLen>=int(iPrevBorder))
            {
                int size = iAudioPayloadLen - iPrevBorder;
                if (size < iNumHigherProtectedBytes)
                    size = iNumHigherProtectedBytes;
                else if (size > iMaxLenOneAudFrame)
                    size = iMaxLenOneAudFrame;
                audio_frame[iNumBorders].resize(size);
            }
            else
                bGoodValues = FALSE;
        }

        /* Check if frame length entries represent possible values */
/*      // check not needed anymore, the size is checked above     //
        for (int i = 0; i < iNumAudioFrames; i++)
        {
            if(int(audio_frame[i].size()) > iMaxLenOneAudFrame)
            {
                bGoodValues = FALSE;
            }
        }
*/
        if (eAudioCoding == CAudioParam::AC_AAC) {
            if (bGoodValues == TRUE)
            {
                CAudioCodec::extractSamples(iNumAudioFrames, iNumHigherProtectedBytes, *pvecInputData, audio_frame, aac_crc_bits);
            }
        }
        if (eAudioCoding == CAudioParam::AC_OPUS) {
            if (bGoodValues == TRUE)
            {
                CAudioCodec::extractSamples(iNumAudioFrames, iNumHigherProtectedBytes, *pvecInputData, audio_frame, aac_crc_bits);
            }
        }

    }
    else if (eAudioCoding == CAudioParam::AC_xHE_AAC)
    {
        /*
         * The xHE-AAC audio super frame Header section has the following structure:
            • Frame border count
            • Bit reservoir level
            • Fixed header CRC
            The following definitions apply:
            4 bits. 4 bits. 8 bits
         * */
        int iFrameBorderCount = pvecInputData->Separate(4);
        int iBitReservoirLevel = pvecInputData->Separate(4);
        int iheaderCRC = pvecInputData->Separate(8);
        // TODO check CRC
        // get the directory
        // set the pointer to the end of the super-frame and then back b*16 bits
        vector<int> ivecborders(iFrameBorderCount);
        int directory_offset = iNumHigherProtectedBytes-iFrameBorderCount*16;
        cerr << "superframe has " << iNumHigherProtectedBytes << " bytes and " << iFrameBorderCount << " borders. Directory starts at " << directory_offset << endl;
        pvecInputData->Separate((directory_offset-2)*8);//__SIZEOF_BYTE);
        for(int i=iFrameBorderCount; i>=0; i--) {
            int iFrameBorderIndex = pvecInputData->Separate(12);
            int iFrameBorderCountRepeat =  pvecInputData->Separate(4);
            cerr << "border " << i << " of " << iFrameBorderCountRepeat << " starts at " << iFrameBorderCountRepeat << endl;
            ivecborders[i] = iFrameBorderIndex;
        }
        // now separate the frames using the borders
        // TODO
    }
    else if (eAudioCoding == CAudioParam::AC_CELP)
    {
    }
    else if (eAudioCoding == CAudioParam::AC_HVXC)
    {
    }


    /* Audio decoding ******************************************************** */
    /* Init output block size to zero, this variable is also used for
       determining the position for writing the output vector */
    iOutputBlockSize = 0;

    for (size_t j = 0; j < size_t(iNumAudioFrames); j++)
    {
        _BOOLEAN bCodecUpdated = FALSE;
        bool bCurBlockFaulty = false; // just for Opus or any other codec with FEC

        if (eAudioCoding == CAudioParam::AC_AAC)
        {
            if (bGoodValues == TRUE)
            {
                CAudioCodec::EDecError eDecError;
                string version = codec->DecGetVersion();
                if(version.find("Nero")==0) {
                    //cerr << "FAAD2" << endl;
                    /* The actual decoding */
                    int iDecChannels;
                    short *psDecOutSampleBuf = codec->Decode(audio_frame[j], aac_crc_bits[j], iDecChannels, eDecError);

                    /* Call decoder update */
                    if (!bCodecUpdated)
                    {
                        bCodecUpdated = TRUE;
                        Parameters.Lock();
                        int iCurSelAudioServ = Parameters.GetCurSelAudioService();
                        codec->DecUpdate(Parameters.Service[iCurSelAudioServ].AudioParam);
                        Parameters.Unlock();
                    }

                    bCurBlockOK = true;

                    if(psDecOutSampleBuf) // might be dummy decoder
                    {
                        /* Conversion from _SAMPLE vector to _REAL vector for
                           resampling. ATTENTION: We use a vector which was
                           allocated inside the decoder! */
                        if (iDecChannels == 1)
                        {
                            //cerr << "resample " << iLenDecOutPerChan << " mono samples" << endl;
                            /* Change type of data (short -> real) */
                            for (size_t i = 0; i < size_t(iLenDecOutPerChan); i++)
                                vecTempResBufInLeft[i] = psDecOutSampleBuf[i];

                            /* Resample data */
                            ResampleObjL.Resample(vecTempResBufInLeft,
                                                  vecTempResBufOutCurLeft);

                            //cerr << "copy " << iResOutBlockSize << " samples from left to right" << endl;
                            /* Mono (write the same audio material in both
                               channels) */
                            for (size_t i = 0; i < size_t(iResOutBlockSize); i++)
                            {
                                vecTempResBufOutCurRight[i] =
                                    vecTempResBufOutCurLeft[i];
                            }
                        }
                        else
                        {
                            /* Stereo */
                            //cerr << "resample " << iLenDecOutPerChan << " stereo samples" << endl;
                            for (size_t i = 0; i < size_t(iLenDecOutPerChan); i++)
                            {
                                vecTempResBufInLeft[i] = psDecOutSampleBuf[i * 2];
                                vecTempResBufInRight[i] =
                                    psDecOutSampleBuf[i * 2 + 1];
                            }

                            /* Resample data */
                            ResampleObjL.Resample(vecTempResBufInLeft,
                                                  vecTempResBufOutCurLeft);
                            ResampleObjR.Resample(vecTempResBufInRight,
                                                  vecTempResBufOutCurRight);
                            //cerr << "copied " << iResOutBlockSize << " samples" << endl;
                        }
                    }
                }
                else {
                    //cerr << "fdk" << endl;
                    /* The actual decoding */
                    int iDecChannels;
                    short *psDecOutSampleBuf = codec->Decode(audio_frame[j], aac_crc_bits[j], iDecChannels, eDecError);

                    bCurBlockOK = true;

                    if (iDecChannels == 1)
                    {
                        /* Mono */
                        //cerr << "mono " << iResOutBlockSize << endl;
                        for(int i = 0; i<iResOutBlockSize; i++) {
                            vecTempResBufOutCurLeft[i] = _REAL(psDecOutSampleBuf[i]) / 2.0;
                            vecTempResBufOutCurRight[i] = _REAL(psDecOutSampleBuf[i]) / 2.0;
                        }
                    }
                    else
                    {
                        /* Stereo docs claim non-interleaved but we are getting interleaved! */
                        //cerr << "stereo " << iResOutBlockSize << endl;
                        for(int i = 0; i<iResOutBlockSize; i++) {
                            vecTempResBufOutCurLeft[i] = _REAL(psDecOutSampleBuf[2*i]);
                            vecTempResBufOutCurRight[i] = _REAL(psDecOutSampleBuf[2*i+1]);
                        }
                    }
                }

                /* OPH: add frame status to vector for RSCI */
                Parameters.Lock();
                Parameters.vecbiAudioFrameStatus.Add(eDecError == CAudioCodec::DECODER_ERROR_OK ? 0 : 1);
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
        else if(eAudioCoding == CAudioParam::AC_OPUS)
        {
            if (bGoodValues == TRUE)
            {
                /* The actual decoding */
                int iDecChannels;
                CAudioCodec::EDecError eDecError;
                short *psDecOutSampleBuf = codec->Decode(audio_frame[j], aac_crc_bits[j], iDecChannels, eDecError);

                /* Call decoder update */
                if (!bCodecUpdated)
                {
                    bCodecUpdated = TRUE;
                    Parameters.Lock();
                    int iCurSelAudioServ = Parameters.GetCurSelAudioService();
                    codec->DecUpdate(Parameters.Service[iCurSelAudioServ].AudioParam);
                    Parameters.Unlock();
                }

                if (!(eDecError == CAudioCodec::DECODER_ERROR_CRC && bUseReverbEffect == FALSE) && eDecError != CAudioCodec::DECODER_ERROR_OK)
                {
                    //cerr << "AAC decode error" << endl;
                    bCurBlockOK = false;	/* Set error flag */
                }
                else
                {
                    bCurBlockOK = true;
                    /* Opus can have FEC embeded, thus the audio frame is always OK */
                    if (eDecError != CAudioCodec::DECODER_ERROR_OK)
                        bCurBlockFaulty = true;

                    if(psDecOutSampleBuf) // might be dummy decoder
                    {
                        /* Conversion from _SAMPLE vector to _REAL vector for
                           resampling. ATTENTION: We use a vector which was
                           allocated inside the decoder! */
                        if (iDecChannels == 1)
                        {
                            //cerr << "resample " << iLenDecOutPerChan << " mono samples" << endl;
                            /* Change type of data (short -> real) */
                            for (size_t i = 0; i < size_t(iLenDecOutPerChan); i++)
                                vecTempResBufInLeft[i] = psDecOutSampleBuf[i];

                            /* Resample data */
                            ResampleObjL.Resample(vecTempResBufInLeft,
                                                  vecTempResBufOutCurLeft);

                            cerr << "copy " << iResOutBlockSize << " samples from left to right" << endl;
                            /* Mono (write the same audio material in both
                               channels) */
                            for (size_t i = 0; i < size_t(iResOutBlockSize); i++)
                            {
                                vecTempResBufOutCurRight[i] =
                                    vecTempResBufOutCurLeft[i];
                            }
                        }
                        else
                        {
                            /* Stereo */
                            //cerr << "resample " << iLenDecOutPerChan << " stereo samples" << endl;
                            for (size_t i = 0; i < size_t(iLenDecOutPerChan); i++)
                            {
                                vecTempResBufInLeft[i] = psDecOutSampleBuf[i * 2];
                                vecTempResBufInRight[i] =
                                    psDecOutSampleBuf[i * 2 + 1];
                            }

                            /* Resample data */
                            ResampleObjL.Resample(vecTempResBufInLeft,
                                                  vecTempResBufOutCurLeft);
                            ResampleObjR.Resample(vecTempResBufInRight,
                                                  vecTempResBufOutCurRight);
                            //cerr << "copied " << iResOutBlockSize << " samples" << endl;
                        }
                    }
                }

                /* OPH: add frame status to vector for RSCI */
                Parameters.Lock();
                Parameters.vecbiAudioFrameStatus.Add(eDecError == CAudioCodec::DECODER_ERROR_OK ? 0 : 1);
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

        // This code is independent of particular audio source type and should work with all codecs

        /* Postprocessing of audio blocks, status informations -------------- */
		ETypeRxStatus status = DATA_ERROR;
        if (bCurBlockOK == FALSE)
        {
            if (bAudioWasOK == TRUE)
            {
                /* Post message to show that CRC was wrong (yellow light) */
				status = DATA_ERROR;

                /* Fade-out old block to avoid "clicks" in audio. We use linear
                   fading which gives a log-fading impression */
                for (int i = 0; i < iResOutBlockSize; i++)
                {
                    /* Linear attenuation with time of OLD buffer */
                    const _REAL rAtt = 1.0 - _REAL(i / iResOutBlockSize);

                    vecTempResBufOutOldLeft[i] *= rAtt;
                    vecTempResBufOutOldRight[i] *= rAtt;

                    if (bUseReverbEffect == TRUE)
                    {
                        /* Fade in input signal for reverberation to avoid
                           clicks */
                        const _REAL rAttRev = _REAL( i / iResOutBlockSize);

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
				status = CRC_ERROR;

                if (bUseReverbEffect == TRUE)
                {
                    /* Add Reverberation effect */
                    for (int i = 0; i < iResOutBlockSize; i++)
                    {
                        /* Mono reverberation signal */
                        vecTempResBufOutOldLeft[i] =
                            vecTempResBufOutOldRight[i] = AudioRev.
                                                          ProcessSample(0, 0);
                    }
                }
            }

            /* Write zeros in current output buffer */
            for (int i = 0; i < iResOutBlockSize; i++)
            {
                vecTempResBufOutCurLeft[i] = 0.0;
                vecTempResBufOutCurRight[i] = 0.0;
            }
        }
        else
        {
            /* Increment correctly decoded audio blocks counter */
			if (bCurBlockFaulty) {
				status = DATA_ERROR;
			}
			else {
                iNumCorDecAudio++;
				status = RX_OK;
			}

            if (bAudioWasOK == FALSE)
            {
                if (bUseReverbEffect == TRUE)
                {
                    /* Add "last" reverbration only to old block */
                    for (int i = 0; i < iResOutBlockSize; i++)
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
                for (int i = 0; i < iResOutBlockSize; i++)
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
        Parameters.Lock();
        Parameters.ReceiveStatus.SLAudio.SetStatus(status);
        Parameters.ReceiveStatus.LLAudio.SetStatus(status);
		Parameters.AudioComponentStatus[Parameters.GetCurSelAudioService()].SetStatus(status);
        Parameters.Unlock();

        /* Conversion from _REAL to _SAMPLE with special function */
        for (int i = 0; i < iResOutBlockSize; i++)
        {
            (*pvecOutputData)[iOutputBlockSize + i * 2] = Real2Sample(vecTempResBufOutOldLeft[i]);	/* Left channel */
            (*pvecOutputData)[iOutputBlockSize + i * 2 + 1] = Real2Sample(vecTempResBufOutOldRight[i]);	/* Right channel */
        }

        /* Add new block to output block size ("* 2" for stereo output block) */
        iOutputBlockSize += iResOutBlockSize * 2;

        /* Store current audio block */
        for (int i = 0; i < iResOutBlockSize; i++)
        {
            vecTempResBufOutOldLeft[i] = vecTempResBufOutCurLeft[i];
            vecTempResBufOutOldRight[i] = vecTempResBufOutCurRight[i];
        }
    }
}

void
CAudioSourceDecoder::InitInternal(CParameter & Parameters)
{
    /* Close previous decoder instance if any */
    CloseDecoder();

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
    int iAudioSampleRate;

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

        /* Get current selected audio param */
        CAudioParam& AudioParam(Parameters.Service[iCurSelServ].AudioParam);

        /* Get current audio coding */
        eAudioCoding = AudioParam.eAudioCoding;

        /* Current audio stream ID */
        iCurAudioStreamID = AudioParam.iStreamID;

        /* The requirement for this module is that the stream is used and the
           service is an audio service. Check it here */
        if ((Parameters.Service[iCurSelServ].eAudDataFlag != CService::SF_AUDIO) ||
                (iCurAudioStreamID == STREAM_ID_NOT_USED))
        {
            throw CInitErr(ET_ALL);
        }

        /* Init text message application ------------------------------------ */
        if (AudioParam.bTextflag)
        {
            bTextMessageUsed = TRUE;

            /* Get a pointer to the string */
            TextMessage.Init(&AudioParam.strTextMessage);

            /* Total frame size is input block size minus the bytes for the text
               message */
            iTotalFrameSize = iInputBlockSize -
                              SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR;

            /* Init vector for text message bytes */
            vecbiTextMessBuf.Init(SIZEOF__BYTE *
                                  NUM_BYTES_TEXT_MESS_IN_AUD_STR);
        }
        else {
            bTextMessageUsed = FALSE;

            /* All bytes are used for AAC data, no text message present */
            iTotalFrameSize = iInputBlockSize;
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

        int iNumHeaderBytes = 0;
        /* Length of higher protected part of audio stream */
        const int iLenAudHigh = Parameters.Stream[iCurAudioStreamID].iLenPartA;

        if (eAudioCoding == CAudioParam::AC_AAC)
        {
            /* Init for AAC decoding ---------------------------------------- */

            /* Set number of AAC frames in a AAC super-frame */
            switch (AudioParam.eAudioSamplRate)	/* Only 12 kHz and 24 kHz is allowed */
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
        }
        else if (eAudioCoding == CAudioParam::AC_xHE_AAC)
        {
            iNumHeaderBytes = 2;
            iNumAudioFrames = 0;
            iAudioPayloadLen = iTotalFrameSize / SIZEOF__BYTE - iNumHeaderBytes - iNumAudioFrames;
            iNumHigherProtectedBytes = iAudioPayloadLen;
            // all variable per superframe
        }
        else if (eAudioCoding == CAudioParam::AC_OPUS)
        {
            /* Init for OPUS decoding ---------------------------------------- */

            /* Number of audio frame */
            iNumAudioFrames = 20;

            /* Number of borders */
            iNumBorders = iNumAudioFrames;
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
        }
        else
        {
            /* Audio codec not supported */
            throw CInitErr(ET_AUDDECODER);
        }

        /* The maximum length for one audio frame is "iAudioPayloadLen". The
           regular size will be much shorter since all audio frames share
           the total size, but we do not know at this time how the data is
           split in the transmitter source coder */
        iMaxLenOneAudFrame = iAudioPayloadLen;

        /* Init decoder */
        codec->DecOpen(AudioParam, iAudioSampleRate, iLenDecOutPerChan);

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
        cerr << "output block size per channel " << iResOutBlockSize << " = samples " << iLenDecOutPerChan << " * " << Parameters.GetAudSampleRate() << " / " << iAudioSampleRate << endl;

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

