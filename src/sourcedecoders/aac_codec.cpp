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

#include "aac_codec.h"
#if !defined(USE_FAAD2_LIBRARY) || !defined(USE_FAAC_LIBRARY)
# include "../util/LibraryLoader.h"
#endif


#ifndef USE_FAAD2_LIBRARY
static void* hFaadLib;
static NeAACDecOpen_t *NeAACDecOpen;
static NeAACDecInitDRM_t *NeAACDecInitDRM;
static NeAACDecClose_t *NeAACDecClose;
static NeAACDecDecode_t *NeAACDecDecode;
static const LIBFUNC FaadLibFuncs[] = {
	{ "NeAACDecOpen",    (void**)&NeAACDecOpen,    (void*)nullptr },
	{ "NeAACDecInitDRM", (void**)&NeAACDecInitDRM, (void*)nullptr },
	{ "NeAACDecClose",   (void**)&NeAACDecClose,   (void*)nullptr },
	{ "NeAACDecDecode",  (void**)&NeAACDecDecode,  (void*)nullptr },
	{ nullptr, nullptr, nullptr }
};
# if defined(_WIN32)
static const char* FaadLibNames[] = { "faad2_drm.dll", "libfaad2_drm.dll", "faad_drm.dll", "libfaad2.dll", nullptr };
# elif defined(__APPLE__)
static const char* FaadLibNames[] = { "libfaad_drm.dylib", nullptr };
# else
static const char* FaadLibNames[] = { "libfaad2_drm.so", "libfaad_drm.so", "libfaad.so.2", nullptr };
# endif
#endif


#ifndef USE_FAAC_LIBRARY
static void* hFaacLib;
static faacEncGetVersion_t* faacEncGetVersion;
static faacEncGetCurrentConfiguration_t* faacEncGetCurrentConfiguration;
static faacEncSetConfiguration_t* faacEncSetConfiguration;
static faacEncOpen_t* faacEncOpen;
static faacEncEncode_t* faacEncEncode;
static faacEncClose_t* faacEncClose;
static const LIBFUNC FaacLibFuncs[] = {
	{ "faacEncGetVersion",              (void**)&faacEncGetVersion,              (void*)nullptr },
	{ "faacEncGetCurrentConfiguration", (void**)&faacEncGetCurrentConfiguration, (void*)nullptr },
	{ "faacEncSetConfiguration",        (void**)&faacEncSetConfiguration,        (void*)nullptr },
	{ "faacEncOpen",                    (void**)&faacEncOpen,                    (void*)nullptr },
	{ "faacEncEncode",                  (void**)&faacEncEncode,                  (void*)nullptr },
	{ "faacEncClose",                   (void**)&faacEncClose,                   (void*)nullptr },
	{ nullptr, nullptr, nullptr }
};
# if defined(_WIN32)
static const char* FaacLibNames[] = { "faac_drm.dll", "libfaac_drm.dll", "libfaac.dll", "faac.dll", nullptr };
# elif defined(__APPLE__)
static const char* FaacLibNames[] = { "libfaac_drm.dylib", nullptr };
# else
static const char* FaacLibNames[] = { "libfaac_drm.so", "libfaac.so.0", nullptr };
# endif
static bool FaacCheckCallback()
{
    bool bLibOk = false;
    unsigned long lNumSampEncIn = 0;
    unsigned long lMaxBytesEncOut = 0;
    faacEncHandle hEncoder = faacEncOpen(24000, 1, &lNumSampEncIn, &lMaxBytesEncOut);
    if (hEncoder != nullptr)
    {
        /* lMaxBytesEncOut is odd when DRM is supported */
        bLibOk = lMaxBytesEncOut & 1;
        faacEncClose(hEncoder);
    }
    return bLibOk;
}
#endif


/******************************************************************************/
/* Implementation *************************************************************/

AacCodec::AacCodec() :
    hFaadDecoder(nullptr), hFaacEncoder(nullptr),pFile(nullptr)
{
#ifndef USE_FAAD2_LIBRARY
	if (hFaadLib == nullptr)
	{
		hFaadLib = CLibraryLoader::Load(FaadLibNames, FaadLibFuncs);
		if (!hFaadLib)
		    cerr << "No usable FAAD2 aac decoder library found" << endl;
		else
		    cerr << "Got FAAD2 library" << endl;
	}
#endif
#ifndef USE_FAAC_LIBRARY
    if (hFaacLib == nullptr)
    {
        hFaacLib = CLibraryLoader::Load(FaacLibNames, FaacLibFuncs, FaacCheckCallback);
        if (!hFaacLib)
            cerr << "No usable FAAC aac encoder library found" << endl;
        else
            cerr << "Got FAAC library" << endl;
    }
#endif
}
AacCodec::~AacCodec()
{
	DecClose();
	EncClose();
}

/******************************************************************************/
/* Decoder Implementation *****************************************************/

string
AacCodec::DecGetVersion()
{
	return string("Nero AAC version: ") + FAAD2_VERSION;
}

bool
AacCodec::CanDecode(CAudioParam::EAudCod eAudioCoding)
{
#ifndef USE_FAAD2_LIBRARY
	return hFaadLib && eAudioCoding == CAudioParam::AC_AAC;
#else
	return eAudioCoding == CAudioParam::AC_AAC;
#endif
}

bool
AacCodec::DecOpen(const CAudioParam& AudioParam, int& iAudioSampleRate, int& iLenDecOutPerChan)
{
	int iAACSampleRate = 12000;
	if (hFaadDecoder == nullptr)
		hFaadDecoder = NeAACDecOpen();
	if (hFaadDecoder != nullptr)
	{
		int iDRMchanMode = DRMCH_MONO;
		/* Only 12 kHz and 24 kHz is allowed */
		switch (AudioParam.eAudioSamplRate)
		{
		case CAudioParam::AS_12KHZ:
			iAACSampleRate = 12000;
			break;

		case CAudioParam::AS_24KHZ:
			iAACSampleRate = 24000;
			break;

		default:
			break;
		}
		/* Number of channels for AAC: Mono, PStereo, Stereo */
		switch (AudioParam.eAudioMode)
		{
		case CAudioParam::AM_MONO:
			if (AudioParam.eSBRFlag == CAudioParam::SB_USED)
				iDRMchanMode = DRMCH_SBR_MONO;
			else
				iDRMchanMode = DRMCH_MONO;
			break;

		case CAudioParam::AM_P_STEREO:
			/* Low-complexity only defined in SBR mode */
			iDRMchanMode = DRMCH_SBR_PS_STEREO;
			break;

		case CAudioParam::AM_STEREO:
			if (AudioParam.eSBRFlag == CAudioParam::SB_USED)
				iDRMchanMode = DRMCH_SBR_STEREO;
			else
				iDRMchanMode = DRMCH_STEREO;
			break;
		}
		NeAACDecInitDRM(&hFaadDecoder, iAACSampleRate,
			(unsigned char)iDRMchanMode);
	}
    /* In case of SBR, AAC sample rate is half the total sample rate.
       Length of output is doubled if SBR is used */
    if (AudioParam.eSBRFlag == CAudioParam::SB_USED)
    {
        iAudioSampleRate = iAACSampleRate * 2;
        iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH * 2;
    }
    else
    {
        iAudioSampleRate = iAACSampleRate;
        iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH;
    }
    return hFaadDecoder != nullptr;
}

_SAMPLE*
AacCodec::Decode(const vector<uint8_t>& audio_frame, uint8_t aac_crc_bits, int& iChannels, CAudioCodec::EDecError& eDecError)
{
    _SAMPLE* psDecOutSampleBuf = nullptr;
	NeAACDecFrameInfo DecFrameInfo;
	DecFrameInfo.channels = 1;
	DecFrameInfo.error = 1;

    /* Prepare data vector with CRC at the beginning (the definition with faad2 DRM interface) */
    CVector<uint8_t> vecbyPrepAudioFrame(int(audio_frame.size()+1));
    vecbyPrepAudioFrame[0] = aac_crc_bits;

    for (size_t i = 0; i < audio_frame.size(); i++)
        vecbyPrepAudioFrame[int(i + 1)] = audio_frame[i];

    if (pFile!=nullptr)
    {
        size_t iNewFrL = size_t(vecbyPrepAudioFrame.Size()) + 1;
        fwrite(&iNewFrL, size_t(4), size_t(1), pFile);	// frame length
        fwrite(&vecbyPrepAudioFrame[0], 1, iNewFrL, pFile);	// data
        fflush(pFile);
    }

    if (hFaadDecoder != nullptr)
	{
		psDecOutSampleBuf = (_SAMPLE*) NeAACDecDecode(hFaadDecoder,
			&DecFrameInfo, &vecbyPrepAudioFrame[0], vecbyPrepAudioFrame.size());
	}
    iChannels = DecFrameInfo.channels;
    eDecError = DecFrameInfo.error ? CAudioCodec::DECODER_ERROR_UNKNOWN : CAudioCodec::DECODER_ERROR_OK;
	return psDecOutSampleBuf;
}

void
AacCodec::DecClose()
{
    if(pFile != nullptr) {
        fclose(pFile);
        pFile = nullptr;
    }
	if (hFaadDecoder != nullptr)
	{
		NeAACDecClose(hFaadDecoder);
		hFaadDecoder = nullptr;
	}
}

void
AacCodec::DecUpdate(CAudioParam&)
{
}

/******************************************************************************/
/* Encoder Implementation *****************************************************/

string
AacCodec::EncGetVersion()
{
	char nul = 0;
	char *faac_id_string = &nul;
	char *faac_copyright_string = &nul;
	faacEncGetVersion(&faac_id_string, &faac_copyright_string);
	return string("FAAC version: ") + faac_id_string;
}

bool
AacCodec::CanEncode(CAudioParam::EAudCod eAudioCoding)
{
#ifndef USE_FAAC_LIBRARY
	return hFaacLib && eAudioCoding == CAudioParam::AC_AAC;
#else
	return eAudioCoding == CAudioParam::AC_AAC;
#endif
}

bool
AacCodec::EncOpen(const CAudioParam& AudioParam, unsigned long& lNumSampEncIn, unsigned long& lMaxBytesEncOut)
{
    unsigned long iSampleRate = 24000;
    if(AudioParam.eAudioSamplRate==CAudioParam::AS_12KHZ) iSampleRate = 12000;
    unsigned int iChannels=1;
    if(AudioParam.eAudioMode!=CAudioParam::AM_MONO) iChannels = 2;
    hFaacEncoder = faacEncOpen(iSampleRate, iChannels, &lNumSampEncIn, &lMaxBytesEncOut);
    //cerr << "lNumSampEncIn " << lNumSampEncIn << endl;
    //cerr << "lMaxBytesEncOut " << lMaxBytesEncOut << endl;
    return hFaacEncoder != nullptr;
}

int
AacCodec::Encode(const vector<_SAMPLE>& vecsEncInData, unsigned long lNumSampEncIn, CVector<uint8_t>& vecsEncOutData, unsigned long lMaxBytesEncOut)
{
	int bytesEncoded = 0;
	if (hFaacEncoder != nullptr)
	{
		bytesEncoded = faacEncEncode(hFaacEncoder,
			(int32_t *) &vecsEncInData[0],
            lNumSampEncIn, &vecsEncOutData[0],
			lMaxBytesEncOut);
    }
	return bytesEncoded;
}

void
AacCodec::EncClose()
{
	if (hFaacEncoder != nullptr)
	{
		faacEncClose(hFaacEncoder);
		hFaacEncoder = nullptr;
	}
}

void
AacCodec::EncSetBitrate(int iBitRate)
{
	if (hFaacEncoder != nullptr)
	{
		/* Set encoder configuration */
		faacEncConfigurationPtr CurEncFormat;
		CurEncFormat = faacEncGetCurrentConfiguration(hFaacEncoder);
		CurEncFormat->inputFormat = FAAC_INPUT_16BIT;
		CurEncFormat->useTns = 1;
		CurEncFormat->aacObjectType = LOW;
		CurEncFormat->mpegVersion = MPEG4;
		CurEncFormat->outputFormat = 0;	/* (0 = Raw; 1 = ADTS -> Raw) */
		CurEncFormat->bitRate = iBitRate;
		CurEncFormat->bandWidth = 0;	/* Let the encoder choose the bandwidth */
		faacEncSetConfiguration(hFaacEncoder, CurEncFormat);
	}
}

void
AacCodec::EncUpdate(CAudioParam&)
{
}

void
AacCodec::resetFile(string name)
{
    if(pFile != nullptr) {
        fclose(pFile);
        pFile = nullptr;
    }
    if(name != "") {
        pFile = fopen(name.c_str(), "wb");
    }
}


