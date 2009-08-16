/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	FAC
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

#include "FAC.h"
#include <iostream>


/* Implementation *************************************************************/
/******************************************************************************\
* CFACTransmit																   *
\******************************************************************************/
void CFACTransmit::FACParam(CVector<_BINARY>* pbiFACData, CParameter& Parameter)
{
	int			iCurShortID;

	/* Reset enqueue function */
	pbiFACData->ResetBitAccess();

	/* Put FAC parameters on stream */
	/* Channel parameters --------------------------------------------------- */
	/* Base/Enhancement flag, set it to base which is decodable by all DRM
	   receivers */
	pbiFACData->Enqueue(0 /* 0 */, 1);

	/* Identity */
	/* Manage index of FAC block in super-frame */
	switch (Parameter.FACParameters.iFrameId)
	{
	case 0:
		/* Assuming AFS is valid (AFS not used here), if AFS is not valid, the
		   parameter must be 3 (11) */
		pbiFACData->Enqueue(3 /* 11 */, 2);
		break;

	case 1:
		pbiFACData->Enqueue(1 /* 01 */, 2);
		break;

	case 2:
		pbiFACData->Enqueue(2 /* 10 */, 2);
		break;
	}

	/* Spectrum occupancy */
	switch (Parameter.Channel.eSpectrumOccupancy)
	{
	case SO_0:
		pbiFACData->Enqueue(0 /* 0000 */, 4);
		break;

	case SO_1:
		pbiFACData->Enqueue(1 /* 0001 */, 4);
		break;

	case SO_2:
		pbiFACData->Enqueue(2 /* 0010 */, 4);
		break;

	case SO_3:
		pbiFACData->Enqueue(3 /* 0011 */, 4);
		break;

	case SO_4:
		pbiFACData->Enqueue(4 /* 0100 */, 4);
		break;

	case SO_5:
		pbiFACData->Enqueue(5 /* 0101 */, 4);
		break;
	}

	/* Interleaver depth flag */
	switch (Parameter.Channel.eInterleaverDepth)
	{
	case SI_LONG:
		pbiFACData->Enqueue(0 /* 0 */, 1);
		break;

	case SI_SHORT:
		pbiFACData->Enqueue(1 /* 1 */, 1);
		break;
	}

	/* MSC mode */
	switch (Parameter.Channel.eMSCmode)
	{
	case CS_3_SM:
		pbiFACData->Enqueue(0 /* 00 */, 2);
		break;

	case CS_3_HMMIX:
		pbiFACData->Enqueue(1 /* 01 */, 2);
		break;

	case CS_3_HMSYM:
		pbiFACData->Enqueue(2 /* 10 */, 2);
		break;

	case CS_2_SM:
		pbiFACData->Enqueue(3 /* 11 */, 2);
		break;

	default:
		break;
	}

	/* SDC mode */
	switch (Parameter.Channel.eSDCmode)
	{
	case CS_2_SM:
		pbiFACData->Enqueue(0 /* 0 */, 1);
		break;

	case CS_1_SM:
		pbiFACData->Enqueue(1 /* 1 */, 1);
		break;

	default:
		break;
	}

	/* Number of services */
	/* Use Table */
	pbiFACData->Enqueue(iTableNumOfServices[Parameter.FACParameters.iNumAudioServices]
		[Parameter.FACParameters.iNumDataServices], 4);

	/* Reconfiguration index (not used) */
	pbiFACData->Enqueue((uint32_t) 0, 3);

	/* rfu */
	pbiFACData->Enqueue((uint32_t) 0, 2);

	/* Service parameters --------------------------------------------------- */
	/* Transmit service-information of service signalled in the FAC-repetition
	   array */
	iCurShortID = FACRepetition[FACRepetitionCounter];
	FACRepetitionCounter++;
	if (FACRepetitionCounter == FACNumRep)
		FACRepetitionCounter = 0;

	/* Service identifier */
	pbiFACData->Enqueue(Parameter.Service[iCurShortID].iServiceID, 24);

	/* Short ID */
	pbiFACData->Enqueue((uint32_t) iCurShortID, 2);

	/* CA indication */
	switch (Parameter.Service[iCurShortID].eCAIndication)
	{
	case CService::CA_NOT_USED:
		pbiFACData->Enqueue(0 /* 0 */, 1);
		break;

	case CService::CA_USED:
		pbiFACData->Enqueue(1 /* 1 */, 1);
		break;
	}

	/* Language */
	pbiFACData->Enqueue(
		(uint32_t) Parameter.Service[iCurShortID].iLanguage, 4);

	/* Audio/Data flag */
	switch (Parameter.Service[iCurShortID].eAudDataFlag)
	{
	case SF_AUDIO:
		pbiFACData->Enqueue(0 /* 0 */, 1);
		break;

	case SF_DATA:
		pbiFACData->Enqueue(1 /* 1 */, 1);
	}

	/* Service descriptor */
	pbiFACData->Enqueue(
		(uint32_t) Parameter.Service[iCurShortID].iServiceDescr, 5);

	/* Rfa */
	pbiFACData->Enqueue(uint32_t(0), 7);

	/* CRC ------------------------------------------------------------------ */
	/* Calculate the CRC and put at the end of the stream */
	CRCObject.Reset(8);

	pbiFACData->ResetBitAccess();

	for (size_t i = 0; i < NUM_FAC_BITS_PER_BLOCK / BITS_BINARY - 1; i++)
		CRCObject.AddByte((_BYTE) pbiFACData->Separate(BITS_BINARY));

	/* Now, pointer in "enqueue"-function is back at the same place,
	   add CRC */
	pbiFACData->Enqueue(CRCObject.GetCRC(), 8);
}

void CFACTransmit::Init(CParameter& Parameter)
{
	set<int>	actServ;

	/* Get active services */
	Parameter.GetActiveServices(actServ);
	const size_t iTotNumServices = actServ.size();

	/* Check how many audio and data services present */
	vector<int>	veciAudioServ;
	vector<int>	veciDataServ;
	size_t		iNumAudio = 0;
	size_t		iNumData = 0;

	for (set<int>::iterator i = actServ.begin(); i!=actServ.end(); i++)
	{
		if (Parameter.Service[*i].eAudDataFlag == SF_AUDIO)
		{
			veciAudioServ.push_back(*i);
			iNumAudio++;
		}
		else
		{
			veciDataServ.push_back(*i);
			iNumData++;
		}
	}

	/* Now check special cases which are defined in 6.3.6-------------------- */
	/* If we have only data or only audio services. When all services are of
	   the same type (e.g. all audio or all data) then the services shall be
	   signalled sequentially */
	if ((iNumAudio == iTotNumServices) || (iNumData == iTotNumServices))
	{
		/* Init repetition vector */
		FACNumRep = iTotNumServices;
		FACRepetition.resize(0);

		for (set<int>::iterator i = actServ.begin(); i!=actServ.end(); i++)
			FACRepetition.push_back(*i);
	}
	else
	{
		/* Special cases according to Table 60 (Service parameter repetition
		   patterns for mixtures of audio and data services) */
		if (iNumAudio == 1)
		{
			if (iNumData == 1)
			{
				/* Init repetion vector */
				FACNumRep = 5;
				FACRepetition.resize(FACNumRep);

				/* A1A1A1A1D1 */
				FACRepetition[0] = veciAudioServ[0];
				FACRepetition[1] = veciAudioServ[0];
				FACRepetition[2] = veciAudioServ[0];
				FACRepetition[3] = veciAudioServ[0];
				FACRepetition[4] = veciDataServ[0];
			}
			else if (iNumData == 2)
			{
				/* Init repetion vector */
				FACNumRep = 10;
				FACRepetition.resize(FACNumRep);

				/* A1A1A1A1D1A1A1A1A1D2 */
				FACRepetition[0] = veciAudioServ[0];
				FACRepetition[1] = veciAudioServ[0];
				FACRepetition[2] = veciAudioServ[0];
				FACRepetition[3] = veciAudioServ[0];
				FACRepetition[4] = veciDataServ[0];
				FACRepetition[5] = veciAudioServ[0];
				FACRepetition[6] = veciAudioServ[0];
				FACRepetition[7] = veciAudioServ[0];
				FACRepetition[8] = veciAudioServ[0];
				FACRepetition[9] = veciDataServ[1];
			}
			else /* iNumData == 3 */
			{
				/* Init repetion vector */
				FACNumRep = 15;
				FACRepetition.resize(FACNumRep);

				/* A1A1A1A1D1A1A1A1A1D2A1A1A1A1D3 */
				FACRepetition[0] = veciAudioServ[0];
				FACRepetition[1] = veciAudioServ[0];
				FACRepetition[2] = veciAudioServ[0];
				FACRepetition[3] = veciAudioServ[0];
				FACRepetition[4] = veciDataServ[0];
				FACRepetition[5] = veciAudioServ[0];
				FACRepetition[6] = veciAudioServ[0];
				FACRepetition[7] = veciAudioServ[0];
				FACRepetition[8] = veciAudioServ[0];
				FACRepetition[9] = veciDataServ[1];
				FACRepetition[10] = veciAudioServ[0];
				FACRepetition[11] = veciAudioServ[0];
				FACRepetition[12] = veciAudioServ[0];
				FACRepetition[13] = veciAudioServ[0];
				FACRepetition[14] = veciDataServ[2];
			}
		}
		else if (iNumAudio == 2)
		{
			if (iNumData == 1)
			{
				/* Init repetion vector */
				FACNumRep = 5;
				FACRepetition.resize(FACNumRep);

				/* A1A2A1A2D1 */
				FACRepetition[0] = veciAudioServ[0];
				FACRepetition[1] = veciAudioServ[1];
				FACRepetition[2] = veciAudioServ[0];
				FACRepetition[3] = veciAudioServ[1];
				FACRepetition[4] = veciDataServ[0];
			}
			else /* iNumData == 2 */
			{
				/* Init repetion vector */
				FACNumRep = 10;
				FACRepetition.resize(FACNumRep);

				/* A1A2A1A2D1A1A2A1A2D2 */
				FACRepetition[0] = veciAudioServ[0];
				FACRepetition[1] = veciAudioServ[1];
				FACRepetition[2] = veciAudioServ[0];
				FACRepetition[3] = veciAudioServ[1];
				FACRepetition[4] = veciDataServ[0];
				FACRepetition[5] = veciAudioServ[0];
				FACRepetition[6] = veciAudioServ[1];
				FACRepetition[7] = veciAudioServ[0];
				FACRepetition[8] = veciAudioServ[1];
				FACRepetition[9] = veciDataServ[1];
			}
		}
		else /* iNumAudio == 3 */
		{
			/* Init repetion vector */
			FACNumRep = 7;
			FACRepetition.resize(FACNumRep);

			/* A1A2A3A1A2A3D1 */
			FACRepetition[0] = veciAudioServ[0];
			FACRepetition[1] = veciAudioServ[1];
			FACRepetition[2] = veciAudioServ[2];
			FACRepetition[3] = veciAudioServ[0];
			FACRepetition[4] = veciAudioServ[1];
			FACRepetition[5] = veciAudioServ[2];
			FACRepetition[6] = veciDataServ[0];
		}
	}
}


/******************************************************************************\
* CFACReceive																   *
\******************************************************************************/
void CFACReceive::ChannelData(CVector<_BINARY>* pbiData,
    CCoreParameter& fc,
    CFACParameters& fp,
    bool bSetRobm)
{
		/* Base/Enhancement flag */
		fc.Channel.bEnhancementLayerInUse = pbiData->Separate(1);

        if(bSetRobm)
        {
            fc.Channel.eRobustness = (ERobMode)(pbiData->Separate(2));
        }
        else
        {
            /* Identity */
            switch (pbiData->Separate(2))
            {
            case 0: /* 00 */
                fp.iFrameId = 0;
                fp.bAFSindexValid = true;
                break;

            case 1: /* 01 */
                fp.iFrameId = 1;
                fp.bAFSindexValid = false;
                break;

            case 2: /* 10 */
                fp.iFrameId = 2;
                fp.bAFSindexValid = false;
                break;

            case 3: /* 11 */
                fp.iFrameId = 0;
                fp.bAFSindexValid = false;
                break;
            }
        }
		/* Spectrum occupancy */
		switch (pbiData->Separate(4))
		{
		case 0: /* 0000 */
			fc.Channel.eSpectrumOccupancy = SO_0;
			break;

		case 1: /* 0001 */
			fc.Channel.eSpectrumOccupancy = SO_1;
			break;

		case 2: /* 0010 */
			fc.Channel.eSpectrumOccupancy = SO_2;
			break;

		case 3: /* 0011 */
			fc.Channel.eSpectrumOccupancy = SO_3;
			break;

		case 4: /* 0100 */
			fc.Channel.eSpectrumOccupancy = SO_4;
			break;

		case 5: /* 0101 */
			fc.Channel.eSpectrumOccupancy = SO_5;
			break;
		}

		/* Interleaver depth flag */
		switch (pbiData->Separate(1))
		{
		case 0: /* 0 */
			fc.Channel.eInterleaverDepth = SI_LONG;
			break;

		case 1: /* 1 */
			fc.Channel.eInterleaverDepth = SI_SHORT;
			break;
		}

		/* MSC mode */
		switch (pbiData->Separate(2))
		{
		case 0: /* 00 */
			fc.Channel.eMSCmode = CS_3_SM;
			break;

		case 1: /* 01 */
			fc.Channel.eMSCmode = CS_3_HMMIX;
			break;

		case 2: /* 10 */
			fc.Channel.eMSCmode = CS_3_HMSYM;
			break;

		case 3: /* 11 */
			fc.Channel.eMSCmode = CS_2_SM;
			break;
		}

		/* SDC mode */
		switch (pbiData->Separate(1))
		{
		case 0: /* 0 */
			fc.Channel.eSDCmode = CS_2_SM;
			break;

		case 1: /* 1 */
			fc.Channel.eSDCmode = CS_1_SM;
			break;
		}

		/* Number of services */
		/* Search table for entry */
		int iNumServField = pbiData->Separate(4);
		for (int i = 0; i < 5; i++)
			for (int j = 0; j < 5; j++)
				if (iNumServField == iTableNumOfServices[i][j])
				{
				    fp.iNumAudioServices = i;
				    fp.iNumDataServices = j;
				}

		/* Reconfiguration index */
		fp.iReconfigurationIndex = pbiData->Separate(3);

		/* rfu */
		/* Do not use rfu */
		pbiData->Separate(2);
}

bool CFACReceive::FACParam(CVector<_BINARY>* pbiFACData,
							   CParameter& Parameter)
{
/*
	First get new data from incoming data stream, then check if the new
	parameter differs from the old data stored in the receiver. If yes, init
	the modules to the new parameter
*/
	/* CRC ------------------------------------------------------------------ */
	/* Check the CRC of this data block */
	CRCObject.Reset(8);

	pbiFACData->ResetBitAccess();

	for (size_t i = 0; i < NUM_FAC_BITS_PER_BLOCK / BITS_BINARY - 1; i++)
		CRCObject.AddByte((_BYTE) pbiFACData->Separate(BITS_BINARY));

	if (CRCObject.CheckCRC(pbiFACData->Separate(8)) == true)
	{
		/* CRC-check successful, extract data from FAC-stream */
		/* Reset separation function */
		pbiFACData->ResetBitAccess();

		/* Channel parameters ----------------------------------------------- */
		CCoreParameter param;

        ChannelData(pbiFACData, param, Parameter.FACParameters);

		Parameter.Lock();

        // No robustness mode in the FAC
        param.Channel.eRobustness = Parameter.NextConfig.Channel.eRobustness;

        if(param.Channel.eSpectrumOccupancy != Parameter.Channel.eSpectrumOccupancy)
        {
            Parameter.RxEvent = ChannelReconfiguration;
        }
        if(param.Channel.eSDCmode != Parameter.Channel.eSDCmode)
        {
            Parameter.RxEvent = ChannelReconfiguration;
        }
        if(param.Channel.eMSCmode != Parameter.Channel.eMSCmode)
        {
            Parameter.RxEvent = ChannelReconfiguration;
        }

        Parameter.NextConfig.Channel = param.Channel;

		/* Service parameters ----------------------------------------------- */
        uint32_t	iServiceID;
        int			iShortID;

		/* Service identifier */
		iServiceID = pbiFACData->Separate(24);

		/* Short ID (the short ID is the index of the service-array) */
		iShortID = pbiFACData->Separate(2);

		/* Set service identifier */
		Parameter.SetServiceID(iShortID, iServiceID);

		/* CA indication */
		switch (pbiFACData->Separate(1))
		{
		case 0: /* 0 */
			Parameter.Service[iShortID].eCAIndication = CService::CA_NOT_USED;
			break;

		case 1: /* 1 */
			Parameter.Service[iShortID].eCAIndication = CService::CA_USED;
			break;
		}

		/* Language */
		Parameter.Service[iShortID].iLanguage = pbiFACData->Separate(4);

		/* Audio/Data flag */
		switch (pbiFACData->Separate(1))
		{
		case 0: /* 0 */
			Parameter.Service[iShortID].eAudDataFlag = SF_AUDIO;
			break;

		case 1: /* 1 */
			Parameter.Service[iShortID].eAudDataFlag = SF_DATA;
			break;
		}

		/* Service descriptor */
		Parameter.Service[iShortID].iServiceDescr = pbiFACData->Separate(5);

		Parameter.Unlock();

		/* Rfa */
		/* Do not use Rfa */
		pbiFACData->Separate(7);

		/* CRC is ok, return true */
		return true;
	}
	else
	{
		/* Data is corrupted, do not use it. */
        /* Increase the frame-counter manually. If only FAC data was corrupted,
           the others can still decode if they have the right frame number. */
		Parameter.FACParameters.iFrameId++;
		if (Parameter.FACParameters.iFrameId == NUM_FRAMES_IN_SUPERFRAME)
			Parameter.FACParameters.iFrameId = 0;

		/* Return CRC failure as false */
		return false;
	}
}
