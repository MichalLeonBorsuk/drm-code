/******************************************************************************\
 *
 * Copyright (c) 2019
 *
 * Author(s):
 *  Julian Cable
 *
 * Description:
 *  FDK AAC codec class
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

static void EnqueueType9(const CAudioParam& p, CVector<_BINARY>& vecbiData)
{
    /* Audio coding */
    switch (p.eAudioCoding)
    {
    case CAudioParam::AC_AAC:
        vecbiData.Enqueue(p.eAudioCoding, 2);
        break;
    default:
        vecbiData.Enqueue(0, 2);
    }

    /* SBR flag */
    vecbiData.Enqueue(p.eSBRFlag, 1);

    /* Audio mode */
    vecbiData.Enqueue(p.eAudioMode, 2);

    // Audio sampling rate AS_9_6_KHZ, AS_12KHZ, AS_16KHZ, AS_19_2KHZ, AS_24KHZ, AS_32KHZ, AS_38_4KHZ, AS_48KHZ
    unsigned int iVal=0;
    switch (p.eAudioSamplRate)
    {
    case CAudioParam::AS_12KHZ:
        iVal = 1;
        break;
    case CAudioParam::AS_24KHZ:
        iVal = 3;
        break;
    default:
        ;
    }
    vecbiData.Enqueue(iVal, 3);

    /* Text flag */
    vecbiData.Enqueue(p.bTextflag?1:0, 1);

    /* Enhancement flag */
    vecbiData.Enqueue(p.bEnhanceFlag?1:0, 1);

    /* rfa 5 bit */
    vecbiData.Enqueue((uint32_t) 0, 5);

    /* rfa 1 bit */
    vecbiData.Enqueue((uint32_t) 0, 1);
}

static vector<uint8_t> getType9Bytes(const CAudioParam& p)
{
    CVector<_BINARY> bits;
    bits.Init(2*SIZEOF__BYTE);
    bits.ResetBitAccess();
    EnqueueType9(p, bits);
    bits.ResetBitAccess();
    vector<uint8_t> bytes;
    for(int i=0; i<bits.Size()/SIZEOF__BYTE; i++) {
        bytes.push_back(uint8_t(bits.Separate(SIZEOF__BYTE)));
    }
    return bytes;
}

FdkAacCodec::FdkAacCodec() :
    hDecoder(nullptr),decode_buf()
{
}


FdkAacCodec::~FdkAacCodec()
{
    DecClose();
}

static void aacinfo(LIB_INFO& inf) {
    LIB_INFO info[12];
    memset(info, 0, sizeof(info));
    aacDecoder_GetLibInfo(info);
    stringstream version;
    for(int i=0; info[i].module_id!=FDK_NONE && i<12; i++) {
        if(info[i].module_id == FDK_AACDEC) {
            inf = info[i];
        }
    }
}

string
FdkAacCodec::DecGetVersion()
{
    stringstream version;
    LIB_INFO info;
    aacinfo(info);
    version << info.title << " version " << info.versionStr << " " << info.build_date << endl;
    return version.str();
}

static void logAOT(const CStreamInfo& info) {
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
    default:
        cerr << "unknown object type";
    }
    if(info.extAot == AUDIO_OBJECT_TYPE::AOT_SBR) {
        cerr << "+SBR";
    }
}

static void logFlags(const CStreamInfo& info) {

    if((info.flags & AC_USAC) == AC_USAC) {
        cerr << "+USAC";
    }
    if((info.flags & AC_SBR_PRESENT) == AC_SBR_PRESENT) {
        cerr << "+SBR";
    }
    if((info.flags & AC_SBRCRC) == AC_SBRCRC) {
        cerr << "+SBR-CRC";
    }
    if((info.flags & AC_PS_PRESENT) == AC_PS_PRESENT) {
        cerr << "+PS";
    }
    if((info.flags & AC_MPS_PRESENT) == AC_MPS_PRESENT) {
        cerr << "+MPS";
    }
    if((info.flags & AC_INDEP) == AC_INDEP) {
        cerr << " (independent)";
    }
}

bool
FdkAacCodec::DecOpen(const CAudioParam& AudioParam, int& iAudioSampleRate)
{
    unsigned int type9Size;
    UCHAR *t9;
    hDecoder = aacDecoder_Open (TRANSPORT_TYPE::TT_DRM, 3);

    if(hDecoder == nullptr)
        return false;

    vector<uint8_t> type9 = getType9Bytes(AudioParam);
    type9Size = type9.size();
    t9 = &type9[0];

    //cerr << "type9 " << hex; for(size_t i=0; i<type9Size; i++) cerr << int(type9[i]) << " "; cerr << dec << endl;
    AAC_DECODER_ERROR err = aacDecoder_ConfigRaw (hDecoder, &t9, &type9Size);
    if(err == AAC_DEC_OK) {
        CStreamInfo *pinfo = aacDecoder_GetStreamInfo(hDecoder);
        if (pinfo==nullptr) {
            cerr << "DecOpen No stream info" << endl;
            iAudioSampleRate = 48000;
            return true;// TODO
        }
        cerr << "DecOpen";
        logAOT(*pinfo);
        logFlags(*pinfo);
        cerr << endl;
        iAudioSampleRate = pinfo->extSamplingRate;

        return true;
    }
    iAudioSampleRate = 48000;
    return true; // TODO
}

EDecError FdkAacCodec::Decode(const vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, CVector<_REAL>& left, CVector<_REAL>& right)
{
    vector<uint8_t> data;
    uint8_t* pData;
    UINT bufferSize;
    data.resize(audio_frame.size()+1);
    data[0] = aac_crc_bits;

    for (size_t i = 0; i < audio_frame.size(); i++)
        data[i + 1] = audio_frame[i];

    pData = &data[0];
    bufferSize = data.size();
    UINT bytesValid = bufferSize;

    AAC_DECODER_ERROR err = aacDecoder_Fill(hDecoder, &pData, &bufferSize, &bytesValid);
    if(err != AAC_DEC_OK) {
        cerr << "fill failed " << int(err) << endl;
        return DECODER_ERROR_UNKNOWN;
    }

    CStreamInfo *pinfo = aacDecoder_GetStreamInfo(hDecoder);
    if (pinfo==nullptr) {
        cerr << "No stream info" << endl;
    }

    if(pinfo->aacNumChannels == 0) {
        cerr << "zero output channels: " << err << endl;
        //return DECODER_ERROR_UNKNOWN;
    }
    else {
        //cerr << pinfo->aacNumChannels << " aac channels " << endl;
    }

    if(err != AAC_DEC_OK) {
        cerr << "Fill failed: " << err << endl;
        return DECODER_ERROR_UNKNOWN;
    }
    //cerr << "aac decode after fill bufferSize " << bufferSize << ", bytesValid " << bytesValid << endl;
    if (bytesValid != 0) {
        cerr << "Unable to feed all " << bufferSize << " input bytes, bytes left " << bytesValid << endl;
        return DECODER_ERROR_UNKNOWN;
    }

    if(pinfo->numChannels == 0) {
        cerr << "zero output channels: " << err << endl;
        //return DECODER_ERROR_UNKNOWN;
    }

    size_t output_size = pinfo->frameSize * pinfo->numChannels;
    if(sizeof (decode_buf) < sizeof(int16_t)*output_size) {
        cerr << "can't fit output into decoder buffer" << endl;
        return DECODER_ERROR_UNKNOWN;
    }

    memset(decode_buf, 0, sizeof(int16_t)*output_size);
    err = aacDecoder_DecodeFrame(hDecoder, decode_buf, output_size, 0);

    if(err == AAC_DEC_OK) {
        double d = 0.0;
        for(size_t i=0; i<output_size; i++) d += double(decode_buf[i]);
        //cerr << "energy in good frame " << (d/output_size) << endl;
    }
    else if(err == AAC_DEC_PARSE_ERROR) {
        cerr << "error parsing bitstream." << endl;
        return DECODER_ERROR_UNKNOWN;
    }
    else if(err == AAC_DEC_OUT_OF_MEMORY) {
        cerr << "Heap returned NULL pointer. Output buffer is invalid." << endl;
        return DECODER_ERROR_UNKNOWN;
    }
    else if(err == AAC_DEC_UNKNOWN) {
        cerr << "Error condition is of unknown reason, or from a another module. Output buffer is invalid." << endl;
        return DECODER_ERROR_UNKNOWN;
    }
    else {
        cerr << "other error " << hex << int(err) << dec << endl;
        return DECODER_ERROR_UNKNOWN;
    }

    left.Init(pinfo->frameSize, 0.0);
    right.Init(pinfo->frameSize, 0.0);

    if (pinfo->numChannels == 1)
    {
        /* Mono */
        // << "mono " << pinfo->frameSize << endl;
        for(int i = 0; i<pinfo->frameSize; i++) {
            left[int(i)] = _REAL(decode_buf[i]) / 2.0;
            right[int(i)] = _REAL(decode_buf[i]) / 2.0;
        }
    }
    else
    {
        /* Stereo docs claim non-interleaved but we are getting interleaved! */
        //cerr << "stereo " << iResOutBlockSize << endl;
        for(int i = 0; i<pinfo->frameSize; i++) {
            left[int(i)] = _REAL(decode_buf[2*i]);
            right[int(i)] = _REAL(decode_buf[2*i+1]);
        }
    }
    return DECODER_ERROR_OK;
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
