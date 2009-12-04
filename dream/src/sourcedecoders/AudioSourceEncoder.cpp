/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Audio source encoder
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
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 1111
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "AudioSourceEncoder.h"

int FAACAPI dummyfaacEncGetVersion(char **, char **) { return 0; }
faacEncConfigurationPtr FAACAPI dummyfaacEncGetCurrentConfiguration(faacEncHandle) { return NULL; }

int FAACAPI dummyfaacEncSetConfiguration(faacEncHandle, faacEncConfigurationPtr) { return 0; }

faacEncHandle FAACAPI dummyfaacEncOpen(unsigned long,
				  unsigned int,
				  unsigned long *,
				  unsigned long *) { return NULL; }

int FAACAPI dummyfaacEncGetDecoderSpecificInfo(faacEncHandle, unsigned char **,
					  unsigned long *) { return 0; }

int FAACAPI dummyfaacEncEncode(faacEncHandle, int32_t *, unsigned int,
			 unsigned char *,
			 unsigned int) { return 0; }

int FAACAPI dummyfaacEncClose(faacEncHandle) { return 0; }

CAudioSourceEncoderImplementation::CAudioSourceEncoderImplementation()
	 : bUsingTextMessage(false), hEncoder(NULL)
{
    faacEncGetVersion = dummyfaacEncGetVersion;
    faacEncGetCurrentConfiguration = dummyfaacEncGetCurrentConfiguration;
    faacEncSetConfiguration = dummyfaacEncSetConfiguration;
    faacEncOpen = faacEncOpen;
    faacEncGetDecoderSpecificInfo = dummyfaacEncGetDecoderSpecificInfo;
    faacEncEncode = dummyfaacEncEncode;
    faacEncClose = dummyfaacEncClose;
#ifdef _WIN32
    hlib = LoadLibrary(TEXT("libfaac_drm.dll"));
    if(hlib)
    {
	faacEncGetVersion = (faacEncGetVersion_t*)GetProcAddress(hlib, TEXT("faacEncGetVersion"));
	faacEncGetCurrentConfiguration = (faacEncGetCurrentConfiguration_t*)GetProcAddress(hlib, TEXT("faacEncGetCurrentConfiguration"));
	faacEncSetConfiguration = (faacEncSetConfiguration_t*)GetProcAddress(hlib, TEXT("faacEncSetConfiguration"));
	faacEncOpen = (faacEncOpen_t*)GetProcAddress(hlib, TEXT("faacEncOpen"));
	faacEncGetDecoderSpecificInfo = (faacEncGetDecoderSpecificInfo_t*)GetProcAddress(hlib, TEXT("faacEncGetDecoderSpecificInfo"));
	faacEncEncode = (faacEncEncode_t*)GetProcAddress(hlib, TEXT("faacEncEncode"));
	faacEncClose = (faacEncClose_t*)GetProcAddress(hlib, TEXT("faacEncClose"));
    }
#else
# if defined(__APPLE__)
    hlib = dlopen("libfaac_drm.dylib", RTLD_LOCAL | RTLD_NOW);
# else
    hlib = dlopen("libfaac_drm.so", RTLD_LOCAL | RTLD_NOW);
# endif
    if(hlib)
    {
	faacEncGetVersion = (faacEncGetVersion_t*)dlsym(hlib, "faacEncGetVersion");
	faacEncGetCurrentConfiguration = (faacEncGetCurrentConfiguration_t*)dlsym(hlib, "faacEncGetCurrentConfiguration");
	faacEncSetConfiguration = (faacEncSetConfiguration_t*)dlsym(hlib, "faacEncSetConfiguration");
	faacEncOpen = (faacEncOpen_t*)dlsym(hlib, "faacEncOpen");
	faacEncGetDecoderSpecificInfo = (faacEncGetDecoderSpecificInfo_t*)dlsym(hlib, "faacEncGetDecoderSpecificInfo");
	faacEncEncode = (faacEncEncode_t*)dlsym(hlib, "faacEncEncode");
	faacEncClose = (faacEncClose_t*)dlsym(hlib, "faacEncClose");
    }
#endif
}

void
CAudioSourceEncoderImplementation::ProcessDataInternal
(
    CVectorEx <_SAMPLE>* pvecInputData,
    CVectorEx < _BINARY >* pvecOutputData,
    int& iInputBlockSize, int& iOutputBlockSize
)
{
	/* Reset data to zero. This is important since usually not all data is used
	   and this data has to be set to zero as defined in the DRM standard */
	for (int i = 0; i < iOutputBlockSize; i++)
		(*pvecOutputData)[i] = 0;

	/* AAC encoder ------------------------------------------------------ */
	/* Resample data to encoder bit-rate */
	/* Change type of data (short -> real), take left channel! */
	for (int i = 0; i < iInputBlockSize / 2; i++)
		vecTempResBufIn[i] = (*pvecInputData)[i * 2];

	/* Resample data */
	ResampleObj.Resample(vecTempResBufIn, vecTempResBufOut);

	/* Split data in individual audio blocks */
	for (int j = 0; j < iNumAACFrames; j++)
	{
		/* Convert _REAL type to _SAMPLE type, copy in smaller buffer */
		for (unsigned long k = 0; k < lNumSampEncIn; k++)
		{
			vecsEncInData[k] = _SAMPLE(vecTempResBufOut[j * lNumSampEncIn + k]);
		}

		/* Actual AAC encoding */
		CVector<unsigned char>vecsTmpData(lMaxBytesEncOut);
		int bytesEncoded = faacEncEncode(hEncoder,
										 (int32_t *) & vecsEncInData[0],
										 lNumSampEncIn, &vecsTmpData[0],
										 lMaxBytesEncOut);

		if (bytesEncoded > 0)
		{
			/* Extract CRC */
			aac_crc_bits[j] = vecsTmpData[0];

			/* Extract actual data */
			for (int i = 0; i < bytesEncoded - 1 /* "-1" for CRC */ ; i++)
				audio_frame[j][i] = vecsTmpData[i + 1];

			/* Store block lengths for boarders in AAC super-frame-header */
			veciFrameLength[j] = bytesEncoded - 1;
		}
		else
		{
			/* Encoder is in initialization phase, reset CRC and length */
			aac_crc_bits[j] = 0;
			veciFrameLength[j] = 0;
		}
	}

	/* Write data to output vector */
	/* First init buffer with zeros */
	for (int i = 0; i < iOutputBlockSize; i++)
		(*pvecOutputData)[i] = 0;

	/* Reset bit extraction access */
	(*pvecOutputData).ResetBitAccess();

	/* AAC super-frame-header */
	int iAccFrameLength = 0;
	for (int j = 0; j < iNumAACFrames - 1; j++)
	{
		iAccFrameLength += veciFrameLength[j];

	/* Frame border in bytes (12 bits) */
		(*pvecOutputData).Enqueue(iAccFrameLength, 12);
	}

		/* Byte-alignment (4 bits) in case of 10 audio frames */
	if (iNumAACFrames == 10)
		(*pvecOutputData).Enqueue(0, 4);

	/* Higher protected part */
		int iCurNumBytes = 0;
	for (int j = 0; j < iNumAACFrames; j++)
	{
		/* Data */
		for (int i = 0; i < iNumHigherProtectedBytes; i++)
		{
			/* Check if enough data is available, set data to 0 if not */
			if (i < veciFrameLength[j])
				(*pvecOutputData).Enqueue(audio_frame[j][i], 8);
			else
				(*pvecOutputData).Enqueue(0, 8);

			iCurNumBytes++;
		}

		/* CRCs */
		(*pvecOutputData).Enqueue(aac_crc_bits[j], 8);
	}

	/* Lower protected part */
	for (int j = 0; j < iNumAACFrames; j++)
	{
		for (int i = iNumHigherProtectedBytes; i < veciFrameLength[j]; i++)
		{
			/* If encoder produced too many bits, we have to drop them */
			if (iCurNumBytes < iAudioPayloadLen)
				(*pvecOutputData).Enqueue(audio_frame[j][i], 8);

			iCurNumBytes++;
		}
	}

#ifdef _DEBUG_
	/* Save number of bits actually used by audio encoder */
	static FILE *pFile = fopen("test/audbits.dat", "w");
	fprintf(pFile, "%d %d\n", iAudioPayloadLen, iCurNumBytes);
	fflush(pFile);
#endif

	/* text message application ---------------------------- */
	/* Last four bytes in stream are written */
	if (bUsingTextMessage == true)
	{
		/* Always four bytes for text message "piece" */
		CVector < _BINARY >
			vecbiTextMessBuf(BITS_BINARY * NUM_BYTES_TEXT_MESS_IN_AUD_STR);

		/* Get a "piece" */
		TextMessage.Encode(vecbiTextMessBuf);

		/* Calculate start point for text message */
		const int iByteStartTextMess =
			iTotNumBitsForUsage -
			BITS_BINARY * NUM_BYTES_TEXT_MESS_IN_AUD_STR;

		/* Add text message bytes to output stream */
		for (int i = iByteStartTextMess; i < iTotNumBitsForUsage; i++)
			(*pvecOutputData)[i] =
				vecbiTextMessBuf[i - iByteStartTextMess];
	}
}

void
CAudioSourceEncoderImplementation::InitInternalTx
(
    CParameter & TransmParam, int &iInputBlockSize, int &iOutputBlockSize
)
{


	TransmParam.Lock();

	int iCurStreamID = -1;

	// find the first audio service and use its stream
	for(size_t i=0; i<TransmParam.Service.size(); i++)
	{
	    if(TransmParam.Service[i].eAudDataFlag==SF_AUDIO)
	    iCurStreamID = TransmParam.Service[i].iAudioStream;
	}

	/* Set the total available number of bits, byte aligned */
	iTotNumBitsForUsage = TransmParam.GetStreamLen(iCurStreamID) * BITS_BINARY;

	CAudioParam::EAudSamRat sampleRate = TransmParam.AudioParam[iCurStreamID].eAudioSamplRate;

	if(TransmParam.AudioParam[iCurStreamID].eAudioMode != CAudioParam::AM_MONO)
	throw CGenErr("Audio - can only do mono, but stereo selected");

    int iLenPartA = TransmParam.MSCParameters.Stream[iCurStreamID].iLenPartA;
    int iLenPartB = TransmParam.MSCParameters.Stream[iCurStreamID].iLenPartB;

    if(iLenPartB == 0)
	throw CGenErr("Audio - requested block length is zero");

    bUsingTextMessage = TransmParam.AudioParam[iCurStreamID].bTextflag;

	TransmParam.Unlock();

	/* Calculate number of input samples in mono. Audio block are always
	   400 ms long */
	const int iNumInSamplesMono = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
										 (_REAL) 0.4 /* 400 ms */ );

	/* Total number of bytes which can be used for text and audio */
	//const int iTotNumBytesForUsage = iTotNumBitsForUsage / BITS_BINARY;

	/* Total frame size is input block size minus the bytes for the text
	   message (if text message is used) */
	int iTotAudFraSizeBits = iTotNumBitsForUsage;
	if (bUsingTextMessage == true)
	{
		iTotAudFraSizeBits -= BITS_BINARY * NUM_BYTES_TEXT_MESS_IN_AUD_STR;
	}

	/* Set encoder sample rate. This parameter decides other parameters */

	int iTimeEachAudBloMS = 40;
	int iNumHeaderBytes = 14;

	switch(sampleRate)
	{
	case CAudioParam::AS_12KHZ:
		lEncSamprate = 12000;
		iTimeEachAudBloMS = 80;	/* ms */
		iNumAACFrames = 5;
		iNumHeaderBytes = 6;
		break;

	case CAudioParam::AS_24KHZ:
		lEncSamprate = 24000;
		iTimeEachAudBloMS = 40;	/* ms */
		iNumAACFrames = 10;
		iNumHeaderBytes = 14;
		break;
	}

	/* The audio_payload_length is derived from the length of the audio
	   super frame (data_length_of_part_A + data_length_of_part_B)
	   subtracting the audio super frame overhead (bytes used for the audio
	   super frame header() and for the aac_crc_bits) (5.3.1.1, Table 5) */
	iAudioPayloadLen = iTotAudFraSizeBits / BITS_BINARY - iNumHeaderBytes - iNumAACFrames /* for CRCs */ ;

	const int iActEncOutBytes = (int) (iAudioPayloadLen / iNumAACFrames);

	// TEST needed since 960 transform length is not yet implemented in faac!
	int iBitRate;
	if (lNumSampEncIn == 1024)
	{
		iBitRate = (int) (((_REAL) iActEncOutBytes * BITS_BINARY * 960.0 / 1024.0) / iTimeEachAudBloMS * 1000);
	}
	else
	{
		iBitRate = (int) (((_REAL) iActEncOutBytes * BITS_BINARY) / iTimeEachAudBloMS * 1000);
	}

	/* Open encoder instance */
	if (hEncoder != NULL)
		faacEncClose(hEncoder);

	hEncoder = faacEncOpen(lEncSamprate, 1 /* mono */ , &lNumSampEncIn, &lMaxBytesEncOut);
	/* Set encoder configuration */
	CurEncFormat = faacEncGetCurrentConfiguration(hEncoder);
	CurEncFormat->inputFormat = FAAC_INPUT_16BIT;
	CurEncFormat->useTns = 1;
	CurEncFormat->aacObjectType = LOW;
	CurEncFormat->mpegVersion = MPEG4;
	CurEncFormat->outputFormat = 0;	/* (0 = Raw; 1 = ADTS -> Raw) */
	CurEncFormat->bitRate = iBitRate;
	CurEncFormat->bandWidth = 0;	/* Let the encoder choose the bandwidth */
	faacEncSetConfiguration(hEncoder, CurEncFormat);

	/* Init storage for actual data, CRCs and frame lengths */
	audio_frame.Init(iNumAACFrames, lMaxBytesEncOut);
	vecsEncInData.Init(lNumSampEncIn);
	aac_crc_bits.Init(iNumAACFrames);
	veciFrameLength.Init(iNumAACFrames);

	/* Additional buffers needed for resampling since we need conversation
		   between _SAMPLE and _REAL */
	vecTempResBufIn.Init(iNumInSamplesMono);
	vecTempResBufOut.Init(lNumSampEncIn * iNumAACFrames, (_REAL) 0.0);

	/* Init resample objects */
	// TEST needed since 960 transform length is not yet implemented in faac!
	if (lNumSampEncIn == 1024)
	{
		ResampleObj.Init(iNumInSamplesMono,
						 (_REAL) lEncSamprate / SOUNDCRD_SAMPLE_RATE *
						 1024.0 / 960.0);
	}
	else
	{
		ResampleObj.Init(iNumInSamplesMono,
							 (_REAL) lEncSamprate / SOUNDCRD_SAMPLE_RATE);
	}

	/* Calculate number of bytes for higher protected blocks */

	iNumHigherProtectedBytes = (iLenPartA - iNumHeaderBytes - iNumAACFrames /* CRC bytes */ )
				/ iNumAACFrames;

	if (iNumHigherProtectedBytes < 0)
		iNumHigherProtectedBytes = 0;

	/* Define input and output block size */
	iOutputBlockSize = iTotNumBitsForUsage;
	iInputBlockSize = iNumInSamplesMono * 2 /* stereo */ ;
}

void
CAudioSourceEncoderImplementation::InitInternalRx(CParameter & Param,
												  int &iInputBlockSize,
												  int &iOutputBlockSize)
{
	Param.Lock();

	/* Calculate number of input samples in mono. Audio block are always 400 ms long */
	const int iNumInSamplesMono = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
										 (_REAL) 0.4 /* 400 ms */ );

	/* Set the total available number of bits, byte aligned */
	iTotNumBitsForUsage =
		(Param.MSCParameters.Stream[0].iLenPartA + Param.MSCParameters.Stream[0].iLenPartB) * BITS_BINARY;

	/* Total number of bytes which can be used for data and audio */
	//const int iTotNumBytesForUsage = iTotNumBitsForUsage / BITS_BINARY;

	/* Audio service ---------------------------------------------------- */

	/* Total frame size is input block size minus the bytes for the text
	   message (if text message is used) */
	int iTotAudFraSizeBits = iTotNumBitsForUsage;
	if (bUsingTextMessage == true)
		iTotAudFraSizeBits -= BITS_BINARY * NUM_BYTES_TEXT_MESS_IN_AUD_STR;

	/* Set encoder sample rate. This parameter decides other parameters */
	// TEST make threshold decision TODO: improvement
	if (iTotAudFraSizeBits > 7000)	/* in bits! */
		lEncSamprate = 24000;
	else
		lEncSamprate = 12000;

	int iTimeEachAudBloMS = 40;
	int iNumHeaderBytes = 14;

	switch (lEncSamprate)
	{
	case 12000:
		iTimeEachAudBloMS = 80;	/* ms */
		iNumAACFrames = 5;
		iNumHeaderBytes = 6;
		Param.AudioParam[0].eAudioSamplRate = CAudioParam::AS_12KHZ;	/* Set parameter in global struct */
		break;

	case 24000:
		iTimeEachAudBloMS = 40;	/* ms */
		iNumAACFrames = 10;
		iNumHeaderBytes = 14;
		Param.AudioParam[0].eAudioSamplRate = CAudioParam::AS_24KHZ;	/* Set parameter in global struct */
		break;
	}

	/* The audio_payload_length is derived from the length of the audio
	   super frame (data_length_of_part_A + data_length_of_part_B)
	   subtracting the audio super frame overhead (bytes used for the audio
	   super frame header() and for the aac_crc_bits) (5.3.1.1, Table 5) */
	iAudioPayloadLen = iTotAudFraSizeBits / BITS_BINARY -
		iNumHeaderBytes - iNumAACFrames /* for CRCs */ ;

	const int iActEncOutBytes = (int) (iAudioPayloadLen / iNumAACFrames);

	/* Set to mono */
	Param.AudioParam[0].eAudioMode = CAudioParam::AM_MONO;

	/* Open encoder instance */
	if (hEncoder != NULL)
		faacEncClose(hEncoder);

	hEncoder = faacEncOpen(lEncSamprate, 1 /* mono */ ,
						   &lNumSampEncIn, &lMaxBytesEncOut);

// TEST needed since 960 transform length is not yet implemented in faac!
	int iBitRate;
	if (lNumSampEncIn == 1024)
	{
		iBitRate = (int) (((_REAL) iActEncOutBytes * BITS_BINARY * 960.0 /
						   1024.0) / iTimeEachAudBloMS * 1000);
	}
	else
	{
		iBitRate = (int) (((_REAL) iActEncOutBytes * BITS_BINARY) /
						  iTimeEachAudBloMS * 1000);
	}

	/* Set encoder configuration */
	CurEncFormat = faacEncGetCurrentConfiguration(hEncoder);
	CurEncFormat->inputFormat = FAAC_INPUT_16BIT;
	CurEncFormat->useTns = 1;
	CurEncFormat->aacObjectType = LOW;
	CurEncFormat->mpegVersion = MPEG4;
	CurEncFormat->outputFormat = 0;	/* (0 = Raw; 1 = ADTS -> Raw) */
	CurEncFormat->bitRate = iBitRate;
	CurEncFormat->bandWidth = 0;	/* Let the encoder choose the bandwidth */
	faacEncSetConfiguration(hEncoder, CurEncFormat);

	/* Init storage for actual data, CRCs and frame lengths */
	audio_frame.Init(iNumAACFrames, lMaxBytesEncOut);
	vecsEncInData.Init(lNumSampEncIn);
	aac_crc_bits.Init(iNumAACFrames);
	veciFrameLength.Init(iNumAACFrames);

	/* Additional buffers needed for resampling since we need conversation
	   between _SAMPLE and _REAL */
	vecTempResBufIn.Init(iNumInSamplesMono);
	vecTempResBufOut.Init(lNumSampEncIn * iNumAACFrames, (_REAL) 0.0);

	/* Init resample objects */
// TEST needed since 960 transform length is not yet implemented in faac!
	if (lNumSampEncIn == 1024)
	{
		ResampleObj.Init(iNumInSamplesMono,
						 (_REAL) lEncSamprate / SOUNDCRD_SAMPLE_RATE *
						 1024.0 / 960.0);
	}
	else
	{
		ResampleObj.Init(iNumInSamplesMono,
						 (_REAL) lEncSamprate / SOUNDCRD_SAMPLE_RATE);
	}

	/* Calculate number of bytes for higher protected blocks */
	iNumHigherProtectedBytes = 0;

	/* Define input and output block size */
	iOutputBlockSize = iTotNumBitsForUsage;
	iInputBlockSize = iNumInSamplesMono * 2 /* stereo */ ;

	Param.Unlock();
}

void
CAudioSourceEncoderImplementation::SetTextMessage(const string & strText)
{
	/* Set text message in text message object */
	TextMessage.SetMessage(strText);
}

void
CAudioSourceEncoderImplementation::ClearTextMessage()
{
	/* Clear all text segments */
	TextMessage.ClearAllText();
}

CAudioSourceEncoderImplementation::~CAudioSourceEncoderImplementation()
{
	/* Close encoder instance afterwards */
	if (hEncoder != NULL)
		faacEncClose(hEncoder);
}
