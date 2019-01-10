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
    hDecoder(nullptr), hEncoder(nullptr),bUsac(false),decode_buf()
{
}


FdkAacCodec::~FdkAacCodec()
{
	DecClose();
	EncClose();
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

bool
FdkAacCodec::CanDecode(CAudioParam::EAudCod eAudioCoding)
{
    LIB_INFO linfo;
    aacinfo(linfo);
    if(eAudioCoding == CAudioParam::AC_AAC) {
        if((linfo.flags & CAPF_AAC_DRM_BSFORMAT) == 0)
            return false;
        if((linfo.flags & CAPF_SBR_DRM_BS) ==0 )
            return false;
        if((linfo.flags & CAPF_SBR_PS_DRM) == 0)
            return false;
        return true;
    }
    if(eAudioCoding == CAudioParam::AC_xHE_AAC) {
        if((linfo.flags & CAPF_AAC_USAC) != 0)
            return true;
    }
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
    case AUDIO_OBJECT_TYPE::AOT_DRM_SURROUND:
        cerr << " AAC+Surround";
        break;
    case AUDIO_OBJECT_TYPE::AOT_USAC:
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
    cerr << " channels coded " << info.aacNumChannels
         << " coded sample rate " << info.aacSampleRate
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

void
FdkAacCodec::Init(const CAudioParam& AudioParam, int iInputBlockSize, int iLenAudHigh)
{
    CAudioCodec::Init(AudioParam, iInputBlockSize, iLenAudHigh);
   if (AudioParam.eAudioCoding == CAudioParam::AC_xHE_AAC)
    {
        int iNumHeaderBytes = 2;
        iNumAudioFrames = 0;
        iAudioPayloadLen = iTotalFrameSize / SIZEOF__BYTE - iNumHeaderBytes - iNumAudioFrames;
        iNumHigherProtectedBytes = iAudioPayloadLen;
        // all variable per superframe

    }
}

void
FdkAacCodec::Partition(CVectorEx<_BINARY>& vecInputData, vector<vector<uint8_t> >& audio_frame, vector<uint8_t>& aac_crc_bits)
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
}

bool
FdkAacCodec::DecOpen(const CAudioParam& AudioParam, int& iAudioSampleRate, int& iLenDecOutPerChan)
{
    unsigned int type9Size;
    UCHAR *t9;
    hDecoder = aacDecoder_Open (TRANSPORT_TYPE::TT_DRM, 3);

    if(hDecoder == nullptr)
        return false;

    vector<uint8_t> type9 = AudioParam.getType9Bytes();
    type9Size = type9.size();
    t9 = &type9[0];

    AAC_DECODER_ERROR err = aacDecoder_ConfigRaw (hDecoder, &t9, &type9Size);
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
        bUsac = false;
        if(pinfo->aot == AUDIO_OBJECT_TYPE::AOT_USAC) bUsac = true;
        if(pinfo->aot == AUDIO_OBJECT_TYPE::AOT_DRM_USAC) bUsac = true;
        return true;
    }
    iAudioSampleRate = 48000;
    iLenDecOutPerChan=1920;
    return true; // TODO
 }

_SAMPLE*
FdkAacCodec::Decode(const vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, int& iChannels, CAudioCodec::EDecError& eDecError)
{
    if(bUsac) {
        return DecodeUSAC(audio_frame, aac_crc_bits, iChannels, eDecError);
    }
    else {
        return DecodeAAC(audio_frame, aac_crc_bits, iChannels, eDecError);
    }
}

_SAMPLE*
FdkAacCodec::DecodeUSAC(const vector<uint8_t>& audio_frame, uint8_t reservoir, int& iChannels, CAudioCodec::EDecError& eDecError)
{
    writeFile(audio_frame);

    uint8_t* pData = const_cast<uint8_t*>(&audio_frame[0]);
    UINT bufferSize = audio_frame.size();
    UINT bytesValid = audio_frame.size();

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
    }
    else {

        cerr << "Decode";
        logConfig(*pinfo);

        if(pinfo->aacNumChannels > 0) {
            output_size = pinfo->frameSize * pinfo->numChannels;
            iChannels = pinfo->numChannels;
        }
    }
    if(err == AAC_DEC_OK) {
        cerr << "aac decode after fill bufferSize " << bufferSize << ", bytesValid " << bytesValid << endl;
        if (bytesValid != 0) {
            cerr << "Unable to feed all " << bufferSize << " input bytes, bytes left " << bytesValid << endl;
            return nullptr; // wait for all frames of the superframe?
        }
        if(sizeof (decode_buf) < sizeof(int16_t)*size_t(output_size)) {
            cerr << "can't fit output into decoder buffer" << endl;
        }
        memset(decode_buf, 0, sizeof(int16_t)*size_t(output_size));

        err = aacDecoder_DecodeFrame(hDecoder, decode_buf, output_size, 0);
        if(err == AAC_DEC_OK) {
            eDecError = CAudioCodec::DECODER_ERROR_OK;
            cerr << "xHE-AAC frame decoded OK" << endl;
            return decode_buf;
        }
        eDecError = CAudioCodec::DECODER_ERROR_UNKNOWN;
        if(err == AAC_DEC_OUT_OF_MEMORY) {
            cerr << "Heap returned NULL pointer. Output buffer is invalid." << endl;
        }
        if(err == AAC_DEC_UNKNOWN) {
            cerr << "Error condition is of unknown reason, or from a another module. Output buffer is invalid." << endl;
        }
        cerr << "err " << err << endl;
        return nullptr;
    }
    else {
        cerr << "Fill failed: " << err << endl;
        return nullptr;
    }
}

_SAMPLE*
FdkAacCodec::DecodeAAC(const vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, int& iChannels, CAudioCodec::EDecError& eDecError)
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

        //cerr << "Decode";
        //logConfig(info);

        if(pinfo->aacNumChannels > 0) {
            output_size = pinfo->frameSize * pinfo->numChannels;
            iChannels = pinfo->numChannels;
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
        if(err == AAC_DEC_OK) {
            eDecError = CAudioCodec::DECODER_ERROR_OK;
            return decode_buf;
        }
        eDecError = CAudioCodec::DECODER_ERROR_UNKNOWN;
        if(err == AAC_DEC_OUT_OF_MEMORY) {
            cerr << "Heap returned NULL pointer. Output buffer is invalid." << endl;
        }
        if(err == AAC_DEC_UNKNOWN) {
            cerr << "Error condition is of unknown reason, or from a another module. Output buffer is invalid." << endl;
        }
        cerr << "err " << err << endl;
        return nullptr;
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
