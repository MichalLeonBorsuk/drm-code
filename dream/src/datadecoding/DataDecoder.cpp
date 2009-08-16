/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	Data module (using multimedia information carried in DRM stream)
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

#include "DataDecoder.h"
#include <iostream>

CDataDecoder::CDataDecoder ():factory()
{
    for(size_t i=0; i<MAX_NUM_PACK_PER_STREAM; i++)
    {
        app[i] = NULL;
    }
}

CDataDecoder::~CDataDecoder ()
{
    for(map<int, map<int, DataApplicationFactory*> >::iterator i=factory.begin();
        i!=factory.end(); i++)
    {
        for(map<int, DataApplicationFactory*>::iterator j = i->second.begin();
            j!=i->second.end(); j++)
        {
            delete j->second;
        }
    }
    for(size_t i=0; i<MAX_NUM_PACK_PER_STREAM; i++)
    {
        if(app[i])
            delete app[i];
    }
}

void
CDataDecoder::ProcessDataInternal(CParameter & ReceiverParam)
{
    if(ReceiverParam.MSCParameters.Stream[
        ReceiverParam.GetCurSelDataService()
        ].ePacketModInd == PM_PACKET_MODE)
    {
        CCRC CRCObject;

        /* CRC check for all packets -------------------------------------------- */
        /* Reset bit extraction access */
        (*pvecInputData).ResetBitAccess();

        for (int i = 0; i < iNumDataPackets; i++)
        {
            /* Check the CRC of this packet */
            CRCObject.Reset(16);

            /* "- 2": 16 bits for CRC at the end */
            for (int j = 0; j < iTotalPacketSize - 2; j++)
            {
                _BYTE b =pvecInputData->Separate(BITS_BINARY);
                CRCObject.AddByte(b);
            }

            /* Store result in vector and show CRC in multimedia window */
            uint16_t crc = pvecInputData->Separate(16);
            if (CRCObject.CheckCRC(crc) == true)
            {
                veciCRCOk[i] = 1;	/* CRC ok */
                ReceiverParam.ReceiveStatus.MOT.SetStatus(RX_OK);
            }
            else
            {
                veciCRCOk[i] = 0;	/* CRC wrong */
                ReceiverParam.ReceiveStatus.MOT.SetStatus(CRC_ERROR);
            }
        }

        /* Extract packet data -------------------------------------------------- */
        /* Reset bit extraction access */
        (*pvecInputData).ResetBitAccess();
        /* TODO measure the bit rate of individual packet streams */
        for (int i = 0; i < iNumDataPackets; i++)
        {
            /* Check if CRC was ok */
            if (veciCRCOk[i] == 1)
            {
                decodePacket(*pvecInputData);
            }
            else
            {
                /* Skip incorrect packet */
                for (int j = 0; j < iTotalPacketSize; j++)
                    (*pvecInputData).Separate(BITS_BINARY);
            }
        }
    }
    else // stream based processing - pass the whole MSC stream segment
    {
        if(app[0])
            app[0]->AddDataUnit(*pvecInputData);
    }
}

void
CDataDecoder::InitInternal(CParameter & ReceiverParam)
{
	/* Get number of total input bits (and bytes) for this module */
	int iTotalNumInputBits = 0;
	if(iStreamID != STREAM_ID_NOT_USED)
	{
        iTotalNumInputBits = ReceiverParam. GetStreamLen(iStreamID) * BITS_BINARY;
	}

	int iTotalNumInputBytes = iTotalNumInputBits / BITS_BINARY;

	/* Set input block size */
	iInputBlockSize = iTotalNumInputBits;

    if(ReceiverParam.MSCParameters.Stream[iStreamID].ePacketModInd == PM_PACKET_MODE)
    {
        /* Calculate total packet size. DRM standard: packet length: this
           field indicates the length in bytes of the data field of each
           packet specified as an unsigned binary number (the total packet
           length is three bytes longer as it includes the header and CRC
           fields) */
        iTotalPacketSize = ReceiverParam.MSCParameters.Stream[iStreamID].iPacketLen + 3;

        /* Check total packet size, could be wrong due to wrong SDC */
        if ((iTotalPacketSize <= 0) ||
            (iTotalPacketSize > iTotalNumInputBytes))
        {
            return;
        }

        /* Maximum number of bits for the data part in a packet
           ("- 3" because two bits for CRC and one for the header) */
        iMaxPacketDataSize = (iTotalPacketSize - 3) * BITS_BINARY;

        /* Number of data packets in one data block */
        iNumDataPackets = iTotalNumInputBytes / iTotalPacketSize;

        /* Init vector for storing the CRC results for each packet */
        veciCRCOk.Init(iNumDataPackets);

        /* Reset data units for all possible data IDs */
        for (int i = 0; i < MAX_NUM_PACK_PER_STREAM; i++)
        {
            DataUnit[i].Reset();

            /* Reset continuity index (CI) */
            iContInd[i] = 0;

            // TODO - keep the app if its the same app and the same serviceid
            if(app[i])
                delete app[i];
            app[i] = createApp(ReceiverParam.DataParam[iStreamID][i], ReceiverParam);
            // TODO save the service ID
            //app[i]->serviceid = iNewServID;
        }
	}
	else // init for stream based processing
	{
	    if(app[0])
            delete app[0];
        app[0] = createApp(ReceiverParam.DataParam[iStreamID][0], ReceiverParam);
	}
}

DataApplication *CDataDecoder::getApplication(int packetId)
{
    // stream based apps always use packet id 0 which is the default arg
    if((packetId>=0) && (packetId<MAX_NUM_PACK_PER_STREAM))
        return app[packetId];
    return NULL;
}

DataApplication *CDataDecoder::createApp(const CDataParam& dp, CParameter& p)
{
    if((dp.eAppDomain == CDataParam::AD_DAB_SPEC_APP) && dp.eUserAppIdent == AT_NOT_SUP)
    {
        return NULL;
    }
    // match domain
    map<int, map<int, DataApplicationFactory*> >::const_iterator
        f = factory.find(dp.eAppDomain);
    if(f==factory.end())
    {
        return NULL;
    }

    // match application
    map<int, DataApplicationFactory*>::const_iterator
        ff = f->second.find(dp.eUserAppIdent);
    if(ff==f->second.end())
    {
        return NULL;
    }

    // we have an app for this application type
    return ff->second->create(p);
}

void CDataDecoder::decodePacket(CVector<_BINARY>& data)
{
	int iPacketID;
	int iNewContInd;
	int iNewPacketDataSize;
	int iOldPacketDataSize;
	int iNumSkipBytes;
	bool bFirstFlag;
	bool bLastFlag;
	bool bPadPackInd;

    /* Read header data --------------------------------------------- */
    /* First flag */
	bFirstFlag = (data.Separate(1)==1)?true:false;

    /* Last flag */
    bLastFlag = (data.Separate(1))?true:false;

    /* Packet ID */
    iPacketID = (int) (*pvecInputData).Separate(2);

    /* Padded packet indicator (PPI) */
    bPadPackInd = (data.Separate(1))?true:false;

    /* Continuity index (CI) */
    iNewContInd = (int) data.Separate(3);

    /* Act on parameters given in header */
    /* Continuity index: this 3-bit field shall increment by one
       modulo-8 for each packet with this packet Id */
    if ((iContInd[iPacketID] + 1) % 8 != iNewContInd)
        DataUnit[iPacketID].bOK = false;

    /* Store continuity index */
    iContInd[iPacketID] = iNewContInd;

    /* Reset flag for data unit ok when receiving the first packet of
       a new data unit */
    if (bFirstFlag == true)
    {
        DataUnit[iPacketID].Reset();
        DataUnit[iPacketID].bOK = true;
    }

    /* If all packets are received correctely, data unit is ready */
    if (bLastFlag == true)
    {
        if (DataUnit[iPacketID].bOK == true)
            DataUnit[iPacketID].bReady = true;
    }

    /* Data field --------------------------------------------------- */
    /* Get size of new data block */
    if (bPadPackInd)
    {
        /* Padding is present: the first byte gives the number of
           useful data bytes in the data field. */
        iNewPacketDataSize =
            (int) data.Separate(BITS_BINARY) *
            BITS_BINARY;

        if (iNewPacketDataSize > iMaxPacketDataSize)
        {
            /* Error, reset flags */
            DataUnit[iPacketID].bOK = false;
            DataUnit[iPacketID].bReady = false;

            /* Set values to read complete packet size */
            iNewPacketDataSize = iNewPacketDataSize;
            iNumSkipBytes = 2;	/* Only CRC has to be skipped */
        }
        else
        {
            /* Number of unused bytes ("- 2" because we also have the
               one byte which stored the size, the other byte is the
               header) */
            iNumSkipBytes = iTotalPacketSize - 2 -
                iNewPacketDataSize / BITS_BINARY;
        }

        /* Packets with no useful data are permitted if no packet
           data is available to fill the logical frame. The PPI
           shall be set to 1 and the first byte of the data field
           shall be set to 0 to indicate no useful data. The first
           and last flags shall be set to 1. The continuity index
           shall be incremented for these empty packets */
        if (bFirstFlag && bLastFlag && (iNewPacketDataSize == 0))
        {
            /* Packet with no useful data, reset flag */
            DataUnit[iPacketID].bReady = false;
        }
    }
    else
    {
        iNewPacketDataSize = iMaxPacketDataSize;

        /* All bytes are useful bytes, only CRC has to be skipped */
        iNumSkipBytes = 2;
    }

    /* Add new data to data unit vector (bit-wise copying) */
    iOldPacketDataSize = DataUnit[iPacketID].vecbiData.Size();

    DataUnit[iPacketID].vecbiData.Enlarge(iNewPacketDataSize);

    /* Read useful bits */
    for (int i = 0; i < iNewPacketDataSize; i++)
        DataUnit[iPacketID].vecbiData[iOldPacketDataSize + i] =
            (_BINARY) data.Separate(1);

    /* Read bytes which are not used */
    for (int i = 0; i < iNumSkipBytes; i++)
        data.Separate(BITS_BINARY);

    /* Use data unit ------------------------------------------------ */
    if (DataUnit[iPacketID].bReady == true)
    {
        /* Decode all IDs regardless whether activated or not */
        if(app[iPacketID])
            app[iPacketID]->AddDataUnit(DataUnit[iPacketID].vecbiData);

        /* Packet was used, reset it now for new filling with new data
           (this will also reset the flag
           "DataUnit[iPacketID].bReady") */
        DataUnit[iPacketID].Reset();
    }
}
