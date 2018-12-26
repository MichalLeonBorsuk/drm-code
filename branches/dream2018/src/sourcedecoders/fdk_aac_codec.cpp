/******************************************************************************\
 *
 * Copyright (c) 2013
 *
 * Author(s):
 *  David Flamand
 *
 * Description:
 *  AAC codec class
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

#include "fdk_aac_codec.h"
#include "src/SDC/SDC.h"

FdkAacCodec::FdkAacCodec() :
    hDecoder(nullptr), hEncoder(nullptr),info()
{
}


FdkAacCodec::~FdkAacCodec()
{
	DecClose();
	EncClose();
}

string
FdkAacCodec::DecGetVersion()
{
    /*
    LIB_INFO info;
    aacDecoder_GetLibInfo(&info);
    stringstream version;
    version << int((info.version >> 24) & 0xff)
            << '.'
            << int((info.version >> 16) & 0xff)
            << '.'
            << int((info.version >> 8) & 0xff);
    return version.str();
    */
    return "1.0";
}

bool
FdkAacCodec::CanDecode(CAudioParam::EAudCod eAudioCoding)
{
	return eAudioCoding == CAudioParam::AC_AAC;
}

static int n=0;

bool
FdkAacCodec::DecOpen(CAudioParam& AudioParam, int *iAudioSampleRate, int *iLenDecOutPerChan)
{
    unsigned int Type9Size = 2;
    CVector<_BINARY> vecbiData;
    vecbiData.Init(16);
    vecbiData.ResetBitAccess();
    CSDCTransmit::DataEntityType9(vecbiData, AudioParam);
    vecbiData.ResetBitAccess();

    //uint32_t t9[1];
    //t9[0] = vecbiData.Separate(16);
    //UCHAR *t9b = reinterpret_cast<UCHAR*>(t9);
    //UCHAR c = t9b[0]; t9b[0] = t9b[1]; t9b[1] = c;

    UCHAR t9b[2];
    t9b[0] = UCHAR(vecbiData.Separate(8));
    t9b[1] = UCHAR(vecbiData.Separate(8));

    cerr << "type9 " << hex << int(t9b[0]) << int(t9b[1]) << dec << endl;

    UCHAR *t9 = &t9b[0];

    hDecoder = aacDecoder_Open (TRANSPORT_TYPE::TT_DRM, 2);

    if(hDecoder == nullptr)
        return false;
    AAC_DECODER_ERROR err = aacDecoder_ConfigRaw (hDecoder, &t9, &Type9Size);
    if(err == AAC_DEC_OK) {
        CStreamInfo *pinfo = aacDecoder_GetStreamInfo(hDecoder);
        if (pinfo==nullptr || pinfo->sampleRate <= 0) {
            cerr << "No stream info" << endl;
            *iAudioSampleRate = 24000;
            *iLenDecOutPerChan=1920;
            return true;// TODO
        }
        info = *pinfo;
        cerr << "channels " << info.aacNumChannels
             << " sample rate " << info.aacSampleRate
             << " samples per frame " << info.aacSamplesPerFrame
             << endl;
        *iAudioSampleRate = 24000;
        *iLenDecOutPerChan=1920;
        return true;
    }
    *iAudioSampleRate = 24000;
    *iLenDecOutPerChan=1920;
    return true; // TODO
 }

_SAMPLE*
FdkAacCodec::Decode(vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, int *iChannels, CAudioCodec::EDecError *eDecError)
{
    /* Prepare data vector with CRC at the beginning (the definition with faad2 DRM interface) */

    //if(n++>2000) { exit(0);}

    CVector<uint8_t> vecbyPrepAudioFrame(int(audio_frame.size()+1));
    vecbyPrepAudioFrame[0] = aac_crc_bits;

    for (size_t i = 0; i < audio_frame.size(); i++)
        vecbyPrepAudioFrame[int(i + 1)] = audio_frame[i];

    uint8_t* pData = vecbyPrepAudioFrame.data();
    UINT bufferSize = unsigned(vecbyPrepAudioFrame.Size());
    UINT bytesValid = unsigned(vecbyPrepAudioFrame.Size());


    //UINT bufferSize = audio_frame.size();
    //uint8_t data[bufferSize];
    ///memccpy(&data[0], &audio_frame[0], bufferSize, 1);
    //UINT bytesValid = bufferSize;

    int output_size = 1920; // TODO
    int16_t *decode_buf = new int16_t[output_size];

    memset(decode_buf, 0, output_size);

    *eDecError = CAudioCodec::DECODER_ERROR_UNKNOWN;

    //uint8_t* pData = data;
    //cerr << "pData " << static_cast<const void*>(pData) << " data " << static_cast<const void*>(&data[0]) << " bufferSize " << bufferSize << ", bytesValid " << bytesValid << endl;
    AAC_DECODER_ERROR err = aacDecoder_Fill(hDecoder, &pData, &bufferSize, &bytesValid);
    //cerr << "pData " << static_cast<const void*>(pData) << " data " << static_cast<const void*>(&data[0]) << " bufferSize " << bufferSize << ", bytesValid " << bytesValid << endl;
    CStreamInfo *pinfo = aacDecoder_GetStreamInfo(hDecoder);
    if (pinfo==nullptr || pinfo->sampleRate <= 0) {
        cerr << "No stream info" << endl;
    }
    else {
        info = *pinfo;
        cerr << "channels " << info.aacNumChannels
             << " sample rate " << info.aacSampleRate
             << " AAC samples per frame " << info.aacSamplesPerFrame
             << " frame size " << info.frameSize
             << endl;
    }
    if(err == AAC_DEC_OK) {
        //cerr << "aac decode after fill bufferSize " << bufferSize << ", bytesValid " << bytesValid << endl;
        if (bytesValid != 0) {
            cerr << "Unable to feed all " << bufferSize << " input bytes, bytes left " << bytesValid << endl;
            return nullptr; // wait for all frames of the superframe?
        }
        while(true) {
            err = aacDecoder_DecodeFrame(hDecoder, decode_buf, output_size / sizeof(INT_PCM), 0);
            if (err == AAC_DEC_NOT_ENOUGH_BITS)
                fprintf(stderr, "not enough bits\n");
                break; // this is the good end as well as a possible error
            if (err != AAC_DEC_OK) {
                cerr << "Decode failed: " << err << endl;
                return nullptr;
            }
        }
        *eDecError = CAudioCodec::DECODER_ERROR_OK;
        //for(int i=1919; i<1922; i++) cerr << decode_buf[unsigned(i)] << " "; cerr << endl;
        return decode_buf;
    }
    else {
        cerr << "Fill failed: " << err << endl;
        return nullptr;
    }
}


void
FdkAacCodec::DecClose()
{
    if (hDecoder != nullptr)
	{
        aacDecoder_Close(hDecoder);
        hDecoder = nullptr;
	}
}

void
FdkAacCodec::DecUpdate(CAudioParam&)
{
}


string
FdkAacCodec::EncGetVersion()
{
    /*
    LIB_INFO info;
    aacEncGetLibInfo(&info);
    stringstream version;
    version << int((info.version >> 24) & 0xff)
            << '.'
            << int((info.version >> 16) & 0xff)
            << '.'
            << int((info.version >> 8) & 0xff);
    return version.str();
    */
    return "1.0";
}

bool
FdkAacCodec::CanEncode(CAudioParam::EAudCod eAudioCoding)
{
	return eAudioCoding == CAudioParam::AC_AAC;
}

bool
FdkAacCodec::EncOpen(int iSampleRate, int iChannels, unsigned long *lNumSampEncIn, unsigned long *lMaxBytesEncOut)
{
    unsigned int bits_per_frame = static_cast<unsigned int>(*lMaxBytesEncOut * SIZEOF__BYTE);
    unsigned int bitrate = 1000*bits_per_frame/400;
    aacEncOpen(&hEncoder, 0, 2);
    aacEncoder_SetParam(hEncoder, AACENC_AOT, AUDIO_OBJECT_TYPE::AOT_DRM_AAC);
    aacEncoder_SetParam(hEncoder, AACENC_BITRATE, bitrate);
    aacEncoder_SetParam(hEncoder, AACENC_SAMPLERATE, unsigned(iSampleRate));
    return hEncoder != nullptr;
}

int
FdkAacCodec::Encode(CVector<_SAMPLE>& vecsEncInData, unsigned long lNumSampEncIn, CVector<uint8_t>& vecsEncOutData, unsigned long lMaxBytesEncOut)
{
	int bytesEncoded = 0;
    if (hEncoder != nullptr)
	{
	}
	return bytesEncoded;
}

void
FdkAacCodec::EncClose()
{
    if (hEncoder != nullptr)
	{
        aacEncClose(&hEncoder);
        hEncoder = nullptr;
	}
}

void
FdkAacCodec::EncSetBitrate(int iBitRate)
{
    if (hEncoder != nullptr)
	{
	}
}

void
FdkAacCodec::EncUpdate(CAudioParam&)
{
}

