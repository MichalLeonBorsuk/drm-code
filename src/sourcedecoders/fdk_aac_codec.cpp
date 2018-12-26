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
    hDecoder(nullptr), hEncoder(nullptr),info(),decode_buf()
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

static void logConfig(const CStreamInfo& info) {
    switch (info.aot) {
    case AUDIO_OBJECT_TYPE::AOT_DRM_AAC:
        cerr << " AAC";
        break;
    case AUDIO_OBJECT_TYPE::AOT_DRM_SBR:
        cerr << " AAC+SBR";
        break;
    case AUDIO_OBJECT_TYPE::AOT_DRM_MPEG_PS:
        cerr << " AAC+SBR+PS";
        break;
    case AUDIO_OBJECT_TYPE::AOT_DRM_SURROUND:
        cerr << " AAC+Surround";
        break;
    case AUDIO_OBJECT_TYPE::AOT_DRM_USAC:
        cerr << " xHE-AAC";
        break;
    default:
        cerr << "unknown object type";
    }
    if(info.extAot == AUDIO_OBJECT_TYPE::AOT_SBR) {
        cerr << "+SBR";
    }
    if((info.flags & AC_PS_PRESENT) == AC_PS_PRESENT) {
        cerr << "+PS";
    }
    cerr << " AAC channels " << info.aacNumChannels
         << " AAC sample rate " << info.aacSampleRate
         << " channels " << info.numChannels
         << " channel config " << info.channelConfig
         << " sample rate " << info.sampleRate
         << " extended sample rate " << info.extSamplingRate
         << " samples per frame " << info.aacSamplesPerFrame
         << " decoded audio frame size " << info.frameSize
         << " flags " << hex << info.flags << dec;
    cerr << " channel 0 type " << int(info.pChannelType[0]) << " index " << int(info.pChannelIndices[0]);
    if(info.numChannels==2)
        cerr << " channel 1 type " << int(info.pChannelType[1]) << " index " << int(info.pChannelIndices[1]);
    cerr << endl;
}

bool
FdkAacCodec::DecOpen(CAudioParam& AudioParam, int& iAudioSampleRate, int& iLenDecOutPerChan)
{
    unsigned int Type9Size = 2;
    CVector<_BINARY> vecbiData;
    vecbiData.Init(16);
    vecbiData.ResetBitAccess();
    CSDCTransmit::DataEntityType9(vecbiData, AudioParam);
    vecbiData.ResetBitAccess();

    UCHAR t9b[2];
    t9b[0] = UCHAR(vecbiData.Separate(8));
    t9b[1] = UCHAR(vecbiData.Separate(8));

    UCHAR *t9 = &t9b[0];

    hDecoder = aacDecoder_Open (TRANSPORT_TYPE::TT_DRM, 3);

    if(hDecoder == nullptr)
        return false;

    //if(decode_buf != nullptr) {
        //delete [] decode_buf;
        //decode_buf = nullptr;
    //}

    cerr << "type9 " << hex << int(t9[1]) << " " << int(t9[0]) << dec << endl;
    AAC_DECODER_ERROR err = aacDecoder_ConfigRaw (hDecoder, &t9, &Type9Size);
    if(err == AAC_DEC_OK) {
        CStreamInfo *pinfo = aacDecoder_GetStreamInfo(hDecoder);
        if (pinfo==nullptr) {
            cerr << "DecOpen No stream info" << endl;
            iAudioSampleRate = 48000;
            iLenDecOutPerChan=1920;
            return true;// TODO
        }
        cerr << "DecOpen";
        logConfig(*pinfo);
        iAudioSampleRate = 48000;
        iLenDecOutPerChan=1920;
        return true;
    }
    iAudioSampleRate = 48000;
    iLenDecOutPerChan=1920;
    return true; // TODO
 }

_SAMPLE*
FdkAacCodec::Decode(vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, int& iChannels, CAudioCodec::EDecError& eDecError)
{
    /* Prepare data vector with CRC at the beginning (the definition with faad2 DRM interface) */


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


    eDecError = CAudioCodec::DECODER_ERROR_UNKNOWN;

    AAC_DECODER_ERROR err = aacDecoder_Fill(hDecoder, &pData, &bufferSize, &bytesValid);
    if(err != AAC_DEC_OK) {
        cerr << "fill failed " << int(err) << endl;
        return nullptr;
    }

    int output_size = 0;
    CStreamInfo *pinfo = aacDecoder_GetStreamInfo(hDecoder);
    if (pinfo==nullptr) {
        cerr << "No stream info" << endl;
        //return nullptr; this breaks everything!
    }
    else {
        info = *pinfo;

        logConfig(info);

        if(info.aacNumChannels > 0) {
            output_size = info.frameSize;
            iChannels = info.numChannels;
        }
    }
    if(err == AAC_DEC_OK) {
        //cerr << "aac decode after fill bufferSize " << bufferSize << ", bytesValid " << bytesValid << endl;
        if (bytesValid != 0) {
            cerr << "Unable to feed all " << bufferSize << " input bytes, bytes left " << bytesValid << endl;
            return nullptr; // wait for all frames of the superframe?
        }
        //if(decode_buf == nullptr) {
        //    decode_buf = new int16_t[output_size];
        //}
        if(sizeof (decode_buf) < sizeof(int16_t)*size_t(output_size)) {
            cerr << "can't fit output into decoder buffer" << endl;
        }
        memset(decode_buf, 0, sizeof(int16_t)*size_t(output_size));

        while(true) {
            err = aacDecoder_DecodeFrame(hDecoder, decode_buf, output_size, 0);
            if (err == AAC_DEC_NOT_ENOUGH_BITS)
                cerr << "not enough bits" << endl;
                break; // this is the good end as well as a possible error
            if (err != AAC_DEC_OK) {
                cerr << "Decode failed: " << err << endl;
                return nullptr;
            }
        }
        eDecError = CAudioCodec::DECODER_ERROR_OK;
        return decode_buf;
    }
    else {
        cerr << "Fill failed: " << err << endl;
        return nullptr;
    }
}

CAudioCodec::EDecError FdkAacCodec::FullyDecode(vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, vector<_SAMPLE>& left, vector<_SAMPLE>& right)
{
    return EDecError::DECODER_ERROR_UNKNOWN;
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

