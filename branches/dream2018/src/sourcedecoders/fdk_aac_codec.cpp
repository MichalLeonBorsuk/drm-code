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
#include <fdk-aac/aacenc_lib.h>
#include <fdk-aac/FDK_audio.h>
#include "src/SDC/SDC.h"
#include <cstring>

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
    if(eAudioCoding == CAudioParam::AC_AAC) return true;
#ifdef AC_USAC
    if(eAudioCoding == CAudioParam::AC_xHE_AAC) return true;
#endif
    return false;
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
#ifdef AC_USAC
    case AUDIO_OBJECT_TYPE::AOT_DRM_SURROUND:
        cerr << " AAC+Surround";
        break;
    case AUDIO_OBJECT_TYPE::AOT_DRM_USAC:
        cerr << " xHE-AAC";
        break;
#endif
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
FdkAacCodec::DecOpen(const CAudioParam& AudioParam, int& iAudioSampleRate, int& iLenDecOutPerChan)
{

    unsigned int Type9Size;
    UCHAR *t9;
    if(AudioParam.eAudioCoding==CAudioParam::AC_xHE_AAC) {
        Type9Size = AudioParam.t9Bytes.size();
        t9 = const_cast<UCHAR*>(&AudioParam.t9Bytes[0]);
    }
    else {
        Type9Size = 2;
        CVector<_BINARY> vecbiData;
        vecbiData.Init(16);
        vecbiData.ResetBitAccess();
        CSDCTransmit::DataEntityType9(vecbiData, AudioParam);
        vecbiData.ResetBitAccess();

        UCHAR t9b[2];
        t9b[0] = UCHAR(vecbiData.Separate(8));
        t9b[1] = UCHAR(vecbiData.Separate(8));

        UCHAR *t9 = &t9b[0];

    }

    hDecoder = aacDecoder_Open (TRANSPORT_TYPE::TT_DRM, 3);

    if(hDecoder == nullptr)
        return false;

    //if(decode_buf != nullptr) {
        //delete [] decode_buf;
        //decode_buf = nullptr;
    //}

    //cerr << "type9 " << hex << int(t9[1]) << " " << int(t9[0]) << dec << endl;
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
FdkAacCodec::Decode(const vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, int& iChannels, CAudioCodec::EDecError& eDecError)
{
    /* Prepare data vector with CRC at the beginning (the definition with faad2 DRM interface) */


    CVector<uint8_t> vecbyPrepAudioFrame(int(audio_frame.size()+1));
    vecbyPrepAudioFrame[0] = aac_crc_bits;

    for (size_t i = 0; i < audio_frame.size(); i++)
        vecbyPrepAudioFrame[int(i + 1)] = audio_frame[i];

    writeFile(audio_frame);

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

        //logConfig(info);

        if(info.aacNumChannels > 0) {
            output_size = info.frameSize * info.numChannels;
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

        err = aacDecoder_DecodeFrame(hDecoder, decode_buf, output_size, 0);
        eDecError = CAudioCodec::DECODER_ERROR_OK;
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
    if(eAudioCoding != CAudioParam::AC_AAC)
        return false;
    if(hEncoder == nullptr) {
        AACENC_ERROR r = aacEncOpen(&hEncoder, 0x01|0x02|0x04|0x08, 0); // allocate all modules except the metadata module, let library allocate channels in case we get to use MPS
        if(r!=AACENC_OK) {
            hEncoder = nullptr;
            return false;
        }
        UINT aot = aacEncoder_GetParam(hEncoder, AACENC_AOT);
        if(aot==AOT_DRM_AAC)
                return true;
        if(aot==AOT_DRM_SBR)
                return true;
        if(aot==AOT_DRM_MPEG_PS)
                return true;
        //if(aot==AOT_DRM_SURROUND)
        //        return true;
        r = aacEncoder_SetParam(hEncoder, AACENC_AOT, AOT_DRM_SBR);
        aacEncoder_SetParam(hEncoder, AACENC_AOT, aot);
        aacEncClose(&hEncoder);
        hEncoder = nullptr;
        return r==AACENC_OK;
    }
    else {
        UINT aot = aacEncoder_GetParam(hEncoder, AACENC_AOT);
        if(aot==AOT_DRM_AAC)
                return true;
        if(aot==AOT_DRM_SBR)
                return true;
        if(aot==AOT_DRM_MPEG_PS)
                return true;
        //if(aot==AOT_DRM_SURROUND)
        //        return true;
    }
    return false;
}

bool
FdkAacCodec::EncOpen(const CAudioParam& AudioParam, unsigned long& lNumSampEncIn, unsigned long& lMaxBytesEncOut)
{
    AACENC_ERROR r = aacEncOpen(&hEncoder, 0x01|0x02|0x04|0x08, 0); // allocate all modules except the metadata module, let library allocate channels in case we get to use MPS
    if(r!=AACENC_OK) {
        cerr << "error opening encoder " << r << endl;
        return false;
    }
    /*
     *  AOT_DRM_AAC      = 143,  Virtual AOT for DRM (ER-AAC-SCAL without SBR)
     *  AOT_DRM_SBR      = 144,  Virtual AOT for DRM (ER-AAC-SCAL with SBR)
     *  AOT_DRM_MPEG_PS  = 145,  Virtual AOT for DRM (ER-AAC-SCAL with SBR and MPEG-PS)
     *  AOT_DRM_SURROUND = 146,  Virtual AOT for DRM Surround (ER-AAC-SCAL (+SBR) +MPS)
     *  AOT_DRM_USAC     = 147   Virtual AOT for DRM with USAC
     * NB decoder uses only AOT_DRM_AAC and puts SBR, PS in sub fields - what should we do with the encoder?
     */
    switch (AudioParam.eAudioMode) {
    case CAudioParam::AM_MONO:
        if(AudioParam.eSBRFlag) {
            r = aacEncoder_SetParam(hEncoder, AACENC_AOT, AOT_DRM_SBR);
        }
        else {
            r = aacEncoder_SetParam(hEncoder, AACENC_AOT, AOT_DRM_AAC); // OpenDigitalRadio version doesn't claim to support AOT_DRM_AAC
        }
        break;
    case CAudioParam::AM_P_STEREO:
        r = aacEncoder_SetParam(hEncoder, AACENC_AOT, AOT_DRM_MPEG_PS);
        break;
    case CAudioParam::AM_STEREO:
        r = aacEncoder_SetParam(hEncoder, AACENC_AOT, AOT_DRM_SBR);
        break;
    //case CAudioParam::AM_SURROUND:
        //r = aacEncoder_SetParam(hEncoder, AACENC_AOT, AOT_DRM_SURROUND);
    }
    if(r!=AACENC_OK) {
        if(r==AACENC_INVALID_CONFIG) {
            cerr << "invalid config setting DRM mode" << endl;
        }
        else {
            cerr << "error setting DRM mode" << hex << r << dec << endl;
        }
        return false;
    }
    switch (AudioParam.eAudioSamplRate) {
    case CAudioParam::AS_12KHZ:
        r = aacEncoder_SetParam(hEncoder, AACENC_SAMPLERATE, 12000);
        break;
    case CAudioParam::AS_24KHZ:
        r = aacEncoder_SetParam(hEncoder, AACENC_SAMPLERATE, 24000);
    }
    if(r!=AACENC_OK) {
        cerr << "error setting sample rate " << hex << r << dec << endl;
        return false;
    }
    switch (AudioParam.eAudioMode) {
    case CAudioParam::AM_MONO:
        r = aacEncoder_SetParam(hEncoder, AACENC_CHANNELMODE, MODE_1);
        break;
    case CAudioParam::AM_P_STEREO:
        r = aacEncoder_SetParam(hEncoder, AACENC_CHANNELMODE, MODE_2);
        break;
    case CAudioParam::AM_STEREO:
        r = aacEncoder_SetParam(hEncoder, AACENC_CHANNELMODE, MODE_2);
       break;
    //case CAudioParam::AM_SURROUND:
        //r = aacEncoder_SetParam(hEncoder, AACENC_CHANNELMODE, MODE_6_1); // TODO provide more options ES 201 980 6.4.3.10
    }
    if(r!=AACENC_OK) {
        cerr << "error setting channel mode " << hex << r << dec << endl;
        return false;
    }
    r = aacEncoder_SetParam(hEncoder, AACENC_TRANSMUX, TT_DRM); // OpenDigitalRadio doesn't have TT_DRM in it
    if(r!=AACENC_OK) {
        cerr << "error setting transport type " << hex << r << dec << endl;
        return false;
    }
    r = aacEncoder_SetParam(hEncoder, AACENC_BITRATE,  1000*960*8/400);
    if(r!=AACENC_OK) {
        cerr << "error setting bitrate " << hex << r << dec << endl;
        return false;
    }
    r = aacEncoder_SetParam(hEncoder, AACENC_GRANULE_LENGTH, 960); // might let us set frameLength
    if(r!=AACENC_OK) {
        cerr << "error setting frame length " << hex << r << dec << endl;
        return false;
    }
    lNumSampEncIn=960;
    lMaxBytesEncOut=769; // TODO
    r = aacEncEncode(hEncoder, nullptr, nullptr, nullptr, nullptr);
    if(r!=AACENC_OK) {
        cerr << "error initialising encoder " << hex << r << dec << endl;
        return false;
    }
    AACENC_InfoStruct info;
    r = aacEncInfo(hEncoder, &info);
    if(r!=AACENC_OK) {
        cerr << "error getting encoder info " << hex << r << dec << endl;
        return false;
    }
    cerr << "maxOutBufBytes " << info.maxOutBufBytes << endl;
    cerr << "maxAncBytes " << info.maxAncBytes << endl;
    cerr << "inputChannels " << info.inputChannels << endl;
    cerr << "frameLength " << info.frameLength << endl;
    cerr << "maxAncBytes " << info.maxAncBytes << endl;
    return true;
}

int
FdkAacCodec::Encode(CVector<_SAMPLE>& vecsEncInData, unsigned long lNumSampEncIn, CVector<uint8_t>& vecsEncOutData, unsigned long lMaxBytesEncOut)
{
    int bytesEncoded = 0;
    if (hEncoder != nullptr) {
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
        aacEncoder_SetParam(hEncoder, AACENC_BITRATE, iBitRate);
    }
}

void
FdkAacCodec::EncUpdate(CAudioParam&)
{
}

string
FdkAacCodec::fileName(const CParameter& Parameters) const
{
    // Store AAC-data in file
    stringstream ss;
    ss << "test/aac_";

//    Parameters.Lock(); // TODO CAudioSourceDecoder::InitInternal() already have the lock
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
