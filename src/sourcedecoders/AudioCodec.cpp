/******************************************************************************\
 *
 * Copyright (c) 2013
 *
 * Author(s):
 *  David Flamand
 *
 * Description:
 *  Audio codec base class
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

#include "AudioCodec.h"
#include "null_codec.h"
#include "aac_codec.h"
#include "opus_codec.h"
#ifdef HAVE_LIBFDK_AAC
# include "fdk_aac_codec.h"
#endif

CAudioCodec::CAudioCodec():pFile(nullptr)
{

}

CAudioCodec::~CAudioCodec() {

}

vector<CAudioCodec*>
CAudioCodec::CodecList;

int
CAudioCodec::RefCount = 0;

void
CAudioCodec::InitCodecList()
{
	if (CodecList.size() == 0)
	{
		/* Null codec, MUST be the first */
		CodecList.push_back(new NullCodec);

		/* AAC */
#ifdef HAVE_LIBFDK_AAC
        CodecList.push_back(new FdkAacCodec);
#endif
        CodecList.push_back(new AacCodec);

		/* Opus */
		CodecList.push_back(new OpusCodec);
	}
	RefCount ++;
}

void
CAudioCodec::UnrefCodecList()
{
	RefCount --;
	if (!RefCount)
	{
		while (CodecList.size() != 0)
		{
			delete CodecList.back();
			CodecList.pop_back();
		}
	}
}

CAudioCodec*
CAudioCodec::GetDecoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr)
{
    const int size = int(CodecList.size());
	for (int i = 1; i < size; i++)
        if (CodecList[unsigned(i)]->CanDecode(eAudioCoding))
            return CodecList[unsigned(i)];
	/* Fallback to null codec */
    return bCanReturnNullPtr ? nullptr : CodecList[0]; // ie the null codec
}

CAudioCodec*
CAudioCodec::GetEncoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr)
{
	const int size = CodecList.size();
	for (int i = 1; i < size; i++)
		if (CodecList[i]->CanEncode(eAudioCoding))
			return CodecList[i];
	/* Fallback to null codec */
    return bCanReturnNullPtr ? nullptr : CodecList[0]; // ie the null codec
}

void
CAudioCodec::Init(const CAudioParam& AudioParam, int iInputBlockSize, int iLenAudHigh)
{
    int iNumHeaderBytes = 0;

    if (AudioParam.bTextflag)
    {
        /* Total frame size is input block size minus the bytes for the text message */
        iTotalFrameSize = iInputBlockSize - SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR;
    }
    else {
        /* All bytes are used for audio data, no text message present */
        iTotalFrameSize = iInputBlockSize;
    }
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

    iAudioPayloadLen = iTotalFrameSize / SIZEOF__BYTE - iNumHeaderBytes - iNumAudioFrames;

    /* Check iAudioPayloadLen value, only positive values make sense */
    if (iAudioPayloadLen < 0)
        throw CInitErr(ET_AUDDECODER);

    /* Calculate number of bytes for higher protected blocks */
    iNumHigherProtectedBytes = (iLenAudHigh - iNumHeaderBytes - iNumAudioFrames /* CRC bytes */ ) / iNumAudioFrames;

    if (iNumHigherProtectedBytes < 0)
        iNumHigherProtectedBytes = 0;
    /* The maximum length for one audio frame is "iAudioPayloadLen". The
       regular size will be much shorter since all audio frames share
       the total size, but we do not know at this time how the data is
       split in the transmitter source coder */
    iMaxLenOneAudFrame = iAudioPayloadLen;
}

void CAudioCodec::Partition(CVectorEx<_BINARY>& vecInputData, vector< vector<uint8_t> >& audio_frame, vector<uint8_t>& aac_crc_bits)
{
    /* AAC super-frame-header ------------------------------------------- */
    int bGoodValues = TRUE;
    size_t iPrevBorder = 0;

    audio_frame.resize(iNumBorders+1);
    aac_crc_bits.resize(audio_frame.size());

    for (int i = 0; i < iNumBorders; i++)
    {
        /* Frame border in bytes (12 bits) */
        size_t iFrameBorder = vecInputData.Separate(12);

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
        vecInputData.Separate(4);

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

    if (bGoodValues == TRUE)
    {
        /* Higher-protected part */
        for (size_t i = 0; i < iNumAudioFrames; i++)
        {
            /* Extract higher protected part bytes (8 bits per byte) */
            for (size_t j = 0; j < iNumHigherProtectedBytes; j++)
                audio_frame[i][j] = _BINARY(vecInputData.Separate(8));

            /* Extract CRC bits (8 bits) */
            aac_crc_bits[i] = _BINARY(vecInputData.Separate(8));
        }

        /* Lower-protected part */
        for (size_t i = 0; i < iNumAudioFrames; i++)
        {
            /* First calculate frame length, derived from higher protected
               part frame length and total size */
            const size_t iNumLowerProtectedBytes =
                audio_frame[i].size() - iNumHigherProtectedBytes;

            /* Extract lower protected part bytes (8 bits per byte) */
            for (size_t j = 0; j < iNumLowerProtectedBytes; j++)
            {
                audio_frame[i][iNumHigherProtectedBytes + j] =
                    _BINARY(vecInputData.Separate(8));
            }
        }
    }
}

void
CAudioCodec::openFile(const CParameter& Parameters)
{
    if(pFile != nullptr) {
        fclose(pFile);
        pFile = nullptr;
    }
    pFile = fopen(fileName(Parameters).c_str(), "wb");
}

void
CAudioCodec::writeFile(const vector<uint8_t>& audio_frame)
{
    if (pFile!=nullptr)
    {
        size_t iNewFrL = size_t(audio_frame.size()) + 1;
        fwrite(&iNewFrL, size_t(4), size_t(1), pFile);	// frame length
        fwrite(&audio_frame[0], 1, iNewFrL, pFile);	// data
        fflush(pFile);
    }
}

void
CAudioCodec::closeFile() {
    if(pFile != nullptr) {
        fclose(pFile);
        pFile = nullptr;
    }
}
