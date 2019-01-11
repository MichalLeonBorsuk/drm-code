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

    eAudioCoding = AudioParam.eAudioCoding; // save this so we can call the most appropriate partition routines, etc.

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

bool
CAudioCodec::Partition(CVectorEx<_BINARY>& vecInputData, vector< vector<uint8_t> >& audio_frame, vector<uint8_t>& aac_crc_bits)
{
    if(eAudioCoding == CAudioParam::AC_AAC) {
        return PartitionAAC(vecInputData, audio_frame, aac_crc_bits);
    }
    else if(eAudioCoding == CAudioParam::AC_xHE_AAC) {
        return PartitionUSAC(vecInputData, audio_frame, aac_crc_bits);
    }
    else if(eAudioCoding == CAudioParam::AC_OPUS) {
        return PartitionAAC(vecInputData, audio_frame, aac_crc_bits);
    }
    else {
        return false;
    }
}

bool
CAudioCodec::PartitionAAC(CVectorEx<_BINARY>& vecInputData, vector< vector<uint8_t> >& audio_frame, vector<uint8_t>& aac_crc_bits)
{
    /* AAC super-frame-header ------------------------------------------- */
    bool bGoodValues = true;
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
            bGoodValues = false;
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
            bGoodValues = false;
    }

    /* Check if frame length entries represent possible values */

    if (bGoodValues)
    {
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
    return bGoodValues;
}

bool
CAudioCodec::PartitionUSAC(CVectorEx<_BINARY>& vecInputData, vector<vector<uint8_t> >& audio_frame, vector<uint8_t>& aac_crc_bits)
{
    /*
 * 5.3.1.3 Transport of xHE-AAC audio frames within the payload section
The USAC access unit encoder generates a continuous sequence of audio frames at a constant bit rate over the long term.
The individual length of each audio frame in the continuous sequence is variable but constrained by the bit
reservoir mechanism in the audio encoder to allow for improved audio quality. The encoder's bit reservoir buffer level is
signalled to the decoder, to reduce required input buffer size and the extra tune-in delay to a minimum.

Audio frame transport

The xHE-AAC audio encoder generates a sequence of audio super frames (occupying one DRM logical frame for robustness modes A, B, C and D,
or two DRM logical frames for robustness mode E). The audio frames as generated by the USAC access unit encoder are inserted into
the Payload section of the audio super frame as a continuous byte sequence without any padding bytes in-between.
Should padding be required to achieve the overall fixed bit rate and byte-alignment, it is inserted by the USAC access unit encoder
into the audio frames themselves.

The frame borders of audio frames do not need to be and typically will not be aligned with the audio super frame boundaries.
Instead, audio frames are not synchronized to audio super frames; they continue from the current audio super frame into the Payload
section of subsequent audio super frame(s). The frame borders within the Payload section of an audio super frame can be derived from
the Header and Directory section of the audio super frame (there may be none!).

An audio super frame shall not contain a Frame border description element without at least one byte of the corresponding audio frame data.
If the available space in an audio super frame is not sufficient to hold at least 1 byte of the next audio frame in the Payload section
plus the 2 bytes of the related extra Frame border description element in the Directory section, then the remaining space in the Payload
section shall be filled with audio frame content, while the related Frame border description element is carried as the first Frame border
description element in the Directory section of the following audio super frame (i.e. located at the end of the Directory section).
The Frame border index value of such a delayed Frame border description element shall carry the special value 0xFFE or 0xFFF; with 0xFFF
indicating the start of the audio frame at the last byte of the Payload section of the previous audio super frame. A decoder therefore
always needs to buffer the last 2 bytes within the Payload section for a possible later processing along with the next audio super frame.
 */

    int bGoodValues = TRUE;
    /*
 * The xHE-AAC audio super frame Header section has the following structure:
    • Frame border count
    • Bit reservoir level
    • Fixed header CRC
    The following definitions apply:
    4 bits. 4 bits. 8 bits
 * */
    int iFrameBorderCount = vecInputData.Separate(4);
    int iBitReservoirLevel = vecInputData.Separate(4);
    int iheaderCRC = vecInputData.Separate(8);
    // TODO check CRC
    // TODO handle frames split across audio superframes
    // TODO handle reservoir - should it only be passed to the first frame in a superframe?
    // get the directory
    int directory_offset = iTotalFrameSize - 16 * iFrameBorderCount;
    //cerr << "directory offset " << directory_offset << " bits " << (directory_offset/SIZEOF__BYTE) << " bytes" << endl;
    CVector<_BINARY> vecbiDirectory(16 * iFrameBorderCount);
    for (int i = 0; i < 16 * iFrameBorderCount; i++) {
        vecbiDirectory[i] = vecInputData[directory_offset + i];
    }
    vecbiDirectory.ResetBitAccess();
    vector<size_t> ivecborders;
    ivecborders.resize(unsigned(iFrameBorderCount));
    for(int i=iFrameBorderCount-1; i>=0; i--) {
        size_t iFrameBorderIndex = vecbiDirectory.Separate(12);
        int iFrameBorderCountRepeat =  vecbiDirectory.Separate(4);
        //cerr << "border " << i << " of " << iFrameBorderCountRepeat << " starts at " << hex << iFrameBorderIndex << dec << endl;
        ivecborders[i] = iFrameBorderIndex;
    }
    ivecborders.push_back(directory_offset/SIZEOF__BYTE);  // last frame ends at start of directory
    // now separate the frames using the borders
    iNumAudioFrames = iFrameBorderCount + 1;
    size_t start = 2; // offset at the beginning
    audio_frame.resize(iNumAudioFrames);
    for (size_t i = 0; i < audio_frame.size(); i++)
    {
        //cerr << hex << "extracting frame " << i << " from offset " << start << " to offset " << ivecborders[i] << dec << endl;
        audio_frame[i].resize(ivecborders[i]-start);
        for (size_t j = 0; j < audio_frame[i].size(); j++) {
            audio_frame[i][j] = _BINARY(vecInputData.Separate(8));
        }
        start = ivecborders[i];
    }
    return bGoodValues;
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
