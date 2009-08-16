/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden, Julian Cable
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *	This module implements a buffer for decoded Digital Radio Mondiale (DRM)
 *  Multiplex Distribution Interface (MDI) packets at the receiver input.
 *
 *	see ETSI TS 102 820 and ETSI TS 102 821.
 *
 *
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

#include "MDIDecode.h"
#include "../SDC/SDCReceive.h"
#include <iostream>

void CDecodeRSIMDI::ProcessData(CParameter& Parameters,
				CVectorEx<_BINARY>* pvecInputData,
				CVectorEx<_BINARY>* pvecOutputData,
				CVectorEx<_BINARY>* pvecOutputData2,
				vector<CVectorEx<_BINARY>*>& vecpvecOutputData,
				int& iOutputBlockSize,
				int& iOutputBlockSize2,
				vector<int>& veciOutputBlockSize
				)
{
	// pass receiver parameter structure to all the decoders that need it
	TagPacketDecoder.SetParameterPtr(&Parameters);

	CTagPacketDecoder::Error err = TagPacketDecoder.DecodeAFPacket(*pvecInputData);

	Parameters.Lock();

	if(err == CTagPacketDecoder::E_OK)
	{
		Parameters.ReceiveStatus.Interface.SetStatus(RX_OK);
	}
	else
	{
		Parameters.ReceiveStatus.Interface.SetStatus(CRC_ERROR);
		Parameters.Unlock();
		return;
	}

	if (TagPacketDecoder.TagItemDecoderRobMod.IsReady())
		Parameters.Channel.eRobustness = TagPacketDecoder.TagItemDecoderRobMod.eRobMode;

	Parameters.Unlock();

	CVector<_BINARY>& vecbiFACData = TagPacketDecoder.TagItemDecoderFAC.vecbidata;
	CVector<_BINARY>& vecbiSDCData = TagPacketDecoder.TagItemDecoderSDC.vecbidata;
	pvecOutputData->Reset(0);
	if (TagPacketDecoder.TagItemDecoderFAC.IsReady() && vecbiFACData.Size() > 0)
	{
		/* Copy incoming MDI FAC data */
		pvecOutputData->ResetBitAccess();
		vecbiFACData.ResetBitAccess();

		if(pvecOutputData->Size() != 72)
		{
			cout << "FAC not initialised?" << endl;
/*
			return;
*/
		}

		/* FAC data is always 72 bits long which is 9 bytes, copy data
		   byte-wise */
		for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / BITS_BINARY; i++)
		{
			pvecOutputData->Enqueue(vecbiFACData.Separate(BITS_BINARY), BITS_BINARY);
		}
		iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
	}
	else
	{
		iOutputBlockSize = 0;
		Parameters.Lock();
		Parameters.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
		Parameters.Unlock();
	}

	if (TagPacketDecoder.TagItemDecoderSDCChanInf.IsReady())
	{
		CVector<_BINARY>& vecbisdciData = TagPacketDecoder.TagItemDecoderSDCChanInf.vecbidata;
        int iLengthOfBody = (vecbisdciData.Size()-8) / BITS_BINARY;
        CMSCParameters msc;
        vecbisdciData.ResetBitAccess();
        (void)vecbisdciData.Separate(4);

        CSDCReceive sdc;
        bool bError = sdc.DataEntityType0(
            &vecbisdciData, iLengthOfBody, Parameters, false);
		if(bError==false)
		{
			Parameters.Lock();
            Parameters.MSCParameters = Parameters.NextConfig.MSCParameters;
			Parameters.Unlock();
		}
	}

	pvecOutputData2->Reset(0);
	const int iLenBitsMDISDCdata = vecbiSDCData.Size();
	if (TagPacketDecoder.TagItemDecoderSDC.IsReady() && iLenBitsMDISDCdata > 0)
	{
		/* If receiver is correctly initialized, the input vector should be
		   large enough for the SDC data */
		const int iLenSDCDataBits = pvecOutputData2->Size();
		Parameters.iNumSDCBitsPerSuperFrame = iLenBitsMDISDCdata;

		if (iLenSDCDataBits >= iLenBitsMDISDCdata)
		{
			/* Copy incoming MDI SDC data */
			pvecOutputData2->ResetBitAccess();
            vecbiSDCData.ResetBitAccess();

			/* We have to copy bits instead of bytes since the length of SDC
			   data is usually not a multiple of 8 */
			for (int i = 0; i < iLenBitsMDISDCdata; i++)
				pvecOutputData2->Enqueue(vecbiSDCData.Separate(1), 1);

			iOutputBlockSize2 = iLenBitsMDISDCdata;
		}
		else
		{
			cout << "SDC not properly initialised ready for " << iLenSDCDataBits << " bits, got " << iLenBitsMDISDCdata << "bits" << endl;
		}
		iFramesSinceSDC = 0;
	}
	else
	{
		pvecOutputData2->Reset(0);
		iOutputBlockSize2 = 0;
		if(iFramesSinceSDC>2)
		{
			Parameters.Lock();
			Parameters.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
			Parameters.Unlock();
		}
		else
			iFramesSinceSDC++;
   }

	/* Get stream data from received RSCI / MDI packets */
	for(size_t i=0; i<vecpvecOutputData.size(); i++)
	{
		CVector<_BINARY>& vecbiStr = TagPacketDecoder.TagItemDecoderStr[i].vecbidata;
		CVector<_BINARY>* pvecOutputData = vecpvecOutputData[i];
		/* Now check length of data vector */
		const int iLen = pvecOutputData->Size();
		const int iStreamLen = vecbiStr.Size();
		if (iLen >= iStreamLen)
		{
			/* Copy data */
			vecbiStr.ResetBitAccess();
			pvecOutputData->ResetBitAccess();

			/* Data is always a multiple of 8 -> copy bytes */
			for (int j = 0; j < iStreamLen / BITS_BINARY; j++)
			{
				pvecOutputData->Enqueue(
					vecbiStr.Separate(BITS_BINARY), BITS_BINARY);
			}
			veciOutputBlockSize[i] = iStreamLen;
		}
		else
		{
			cerr << "MSC str" << i << " not properly initialised can accept " << iLen << " got " << iStreamLen << endl;
		}
    }

	if (TagPacketDecoder.TagItemDecoderRxDemodMode.IsReady()
		&& TagPacketDecoder.TagItemDecoderAMAudio.IsReady()
		&& TagPacketDecoder.TagItemDecoderRxDemodMode.eMode != DRM
		&& TagPacketDecoder.TagItemDecoderRxDemodMode.eMode != NONE)
	{
		CVector<_BINARY>& vecbiAMAudio = TagPacketDecoder.TagItemDecoderAMAudio.vecbidata;
		CVector<_BINARY>* pvecOutputData = vecpvecOutputData[0];
		// Now check length of data vector
		const int iLen = pvecOutputData->Size();
		const int iStreamLen = vecbiAMAudio.Size();
		if (iLen >= iStreamLen)
		{
			// Copy data
			vecbiAMAudio.ResetBitAccess();
			pvecOutputData->ResetBitAccess();
			// Data is always a multiple of 8 -> copy bytes
			for (int j = 0; j < iStreamLen / BITS_BINARY; j++)
			{
				pvecOutputData->Enqueue(
				vecbiAMAudio.Separate(BITS_BINARY), BITS_BINARY);
			}
			veciOutputBlockSize[0] = iStreamLen;
		}

		/* Get the audio parameters for decoding the coded AM */
		CAudioParam AudioParam = TagPacketDecoder.TagItemDecoderAMAudio.AudioParams;
		/* Write the audio settings into the parameter object
		 */
		Parameters.Lock();
		Parameters.AudioParam[0] = AudioParam;

		Parameters.MSCParameters.Stream[0].iLenPartA = 0;
		Parameters.MSCParameters.Stream[0].iLenPartB = iStreamLen/BITS_BINARY;
		Parameters.iNumDecodedBitsMSC = iStreamLen; // is this necessary?

		Parameters.FACParameters.iNumAudioServices = 1;
		Parameters.FACParameters.iNumDataServices = 0;
		Parameters.Service[0].iAudioStream = 0;
		Parameters.SetCurSelAudioService(0);

		Parameters.Service[0].strLabel = "";
		Parameters.Service[0].strCountryCode = "";
		Parameters.Service[0].iLanguage = 0;
		Parameters.Service[0].strLanguageCode = "";
		Parameters.Service[0].iServiceDescr = 0;
		Parameters.Service[0].iServiceID = 0;

		Parameters.Unlock();
	}
}

void CDecodeRSIMDI::Init(CParameter&)
{
	iFramesSinceSDC = 3;
}

void CDecodeRSI::InitInternal(CParameter& Parameters)
{
	Decoder.Init(Parameters);
	Parameters.Lock();

	/* set sensible values if Parameters not properly initialised */
	iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
	iOutputBlockSize2 = 0;
	iMaxOutputBlockSize2 = 1024;
	/*
	iOutputBlockSize2 = Parameters.iNumSDCBitsPerSFrame;
	if(iOutputBlockSize2 == 0)
		iMaxOutputBlockSize2 = 1024;
	*/
	veciOutputBlockSize.resize(MAX_NUM_STREAMS);
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
	{
		int streamlen = Parameters.GetStreamLen(i);
		if(streamlen == 0)
			streamlen = 2048;
		veciOutputBlockSize[i] = streamlen*BITS_BINARY;
	}

	Parameters.Unlock();
}

void CDecodeRSI::ProcessDataInternal(CParameter& Parameters)
{
	Decoder.ProcessData(Parameters, pvecInputData,
		pvecOutputData, pvecOutputData2, vecpvecOutputData,
		iOutputBlockSize, iOutputBlockSize2, veciOutputBlockSize
		);
}

void CDecodeMDI::InitInternal(CParameter& Parameters)
{
	size_t i;
	Decoder.Init(Parameters);
	iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
	iOutputBlockSize2 = 0;
	iMaxOutputBlockSize2 = 1500*BITS_BINARY;
	veciOutputBlockSize.resize(MAX_NUM_STREAMS);
	veciMaxOutputBlockSize.resize(MAX_NUM_STREAMS);
	vecpvecOutputData.resize(MAX_NUM_STREAMS);
	for(i=0; i<MAX_NUM_STREAMS; i++)
	{
		veciOutputBlockSize[i] = 0;
		veciMaxOutputBlockSize[i] = 1500*BITS_BINARY;
	}
	iInputBlockSize = 1; // something
}

void CDecodeMDI::Expect(int n)
{
	iInputBlockSize = n;
}

void CDecodeMDI::ProcessDataInternal(CParameter& Parameters)
{
	Decoder.ProcessData(Parameters, pvecInputData,
			pvecOutputData, pvecOutputData2, vecpvecOutputData,
		iOutputBlockSize, iOutputBlockSize2, veciOutputBlockSize);
}
