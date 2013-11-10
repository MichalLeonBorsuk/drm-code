#include "packetdatadecoder.h"
#include <../util/CRC.h>

PacketDataDecoder::PacketDataDecoder()
{
    for(int i=0; i<MAX_NUM_PACK_PER_STREAM; i++)
        app[i] = NULL;
}

PacketDataDecoder::~PacketDataDecoder()
{
}

void PacketDataDecoder::setApplication(int packetID, PacketApplication* a)
{
    app[packetID] = a;
}

void PacketDataDecoder::InitInternal (CParameter & Parameters)
{
    /* Init error flag */
    DoNotProcessData = false;

    /* Get number of total input bits (and bytes) for this module */
    int iTotalNumInputBytes = Parameters.GetStreamLen(iStreamID);

    int iPacketLen = Parameters.Stream[iStreamID].packet_length;
    /* Calculate total packet size. DRM standard: packet length: this
       field indicates the length in bytes of the data field of each
       packet specified as an unsigned binary number (the total packet
       length is three bytes longer as it includes the header and CRC
       fields) */
    iTotalPacketSize = iPacketLen + 3;

    /* Check total packet size, could be wrong due to wrong SDC */
    if ((iTotalPacketSize <= 0) ||
        (iTotalPacketSize > iTotalNumInputBytes))
    {
        /* Set error flag */
        DoNotProcessData = TRUE;
    }
    else
    {
        /* Maximum number of bits for the data part in a packet
           ("- 3" because two bits for CRC and one for the header) */
        iMaxPacketDataSize = iPacketLen * SIZEOF__BYTE;

        /* Number of data packets in one data block */
        iNumDataPackets = iTotalNumInputBytes / iTotalPacketSize;

        /* Init vector for storing the CRC results for each packet */
        vector<bool> veciCRCOk(iNumDataPackets);

        /* Reset data units for all possible data IDs */
        for (int i = 0; i < MAX_NUM_PACK_PER_STREAM; i++)
        {
            DataUnit[i].Reset();

            /* Reset continuity index (CI) */
            iContInd[i] = 0;
        }
    }

    /* Set input block size */
    iInputBlockSize = iTotalNumInputBytes * SIZEOF__BYTE;

    // initialise counters
    iNumFrames = 0;
    for (int i = 0; i < MAX_NUM_PACK_PER_STREAM; i++)
    {
        iNumPackets[i] = 0;
    }
}

void PacketDataDecoder::ProcessDataInternal (CParameter & Parameters)
{
    int iPacketID;
    int iNewContInd;
    int iNewPacketDataSize;
    int iOldPacketDataSize;
    int iNumSkipBytes;
    _BINARY biFirstFlag;
    _BINARY biLastFlag;
    _BINARY biPadPackInd;
    CCRC CRCObject;

    /* Check if something went wrong in the initialization routine */
    if (DoNotProcessData == TRUE)
        return;

    /* CRC check for all packets -------------------------------------------- */
    /* Reset bit extraction access */
    (*pvecInputData).ResetBitAccess();

    vector<bool> veciCRCOk(iNumDataPackets);
    for (int j = 0; j < iNumDataPackets; j++)
    {
        /* Check the CRC of this packet */
        CRCObject.Reset(16);

        /* "- 2": 16 bits for CRC at the end */
        for (int i = 0; i < iTotalPacketSize - 2; i++)
        {
            _BYTE b =pvecInputData->Separate(SIZEOF__BYTE);
            CRCObject.AddByte(b);
        }

        /* Store result in vector and show CRC in multimedia window */
        uint16_t crc = pvecInputData->Separate(16);
        ETypeRxStatus status = DATA_ERROR;
        if (CRCObject.CheckCRC(crc) == TRUE)
        {
            veciCRCOk[j] = true;	/* CRC ok */
            Parameters.ReceiveStatus.MSC.SetStatus(RX_OK);
        }
        else
        {
            veciCRCOk[j] = false;	/* CRC wrong */
            Parameters.ReceiveStatus.MSC.SetStatus(CRC_ERROR);
        }
    }

    /* Extract packet data -------------------------------------------------- */
    /* Reset bit extraction access */
    (*pvecInputData).ResetBitAccess();

    for (int j = 0; j < iNumDataPackets; j++)
    {
        /* Check if CRC was ok */
        if (veciCRCOk[j])
        {
            /* Read header data --------------------------------------------- */
            /* First flag */
            biFirstFlag = (_BINARY) (*pvecInputData).Separate(1);

            /* Last flag */
            biLastFlag = (_BINARY) (*pvecInputData).Separate(1);

            /* Packet ID */
            iPacketID = (int) (*pvecInputData).Separate(2);
            //cout << "new packet for stream " << iStreamID << " packet id " << iPacketID  << endl;
            Parameters.DataComponentStatus[iStreamID][iPacketID].SetStatus(veciCRCOk[j]?RX_OK:CRC_ERROR);

            /* Padded packet indicator (PPI) */
            biPadPackInd = (_BINARY) (*pvecInputData).Separate(1);

            /* Continuity index (CI) */
            iNewContInd = (int) (*pvecInputData).Separate(3);

            /* Act on parameters given in header */
            /* Continuity index: this 3-bit field shall increment by one
               modulo-8 for each packet with this packet Id */
            if ((iContInd[iPacketID] + 1) % 8 != iNewContInd)
                DataUnit[iPacketID].bOK = FALSE;

            /* Store continuity index */
            iContInd[iPacketID] = iNewContInd;

            /* Reset flag for data unit ok when receiving the first packet of
               a new data unit */
            if (biFirstFlag == 1)
            {
                DataUnit[iPacketID].Reset();
                DataUnit[iPacketID].bOK = TRUE;
            }

            /* If all packets are received correctely, data unit is ready */
            if (biLastFlag == 1)
            {
                if (DataUnit[iPacketID].bOK == TRUE)
                    DataUnit[iPacketID].bReady = TRUE;
            }

            /* Data field --------------------------------------------------- */
            /* Get size of new data block */
            if (biPadPackInd == 1)
            {
                /* Padding is present: the first byte gives the number of
                   useful data bytes in the data field. */
                iNewPacketDataSize =
                    (int) (*pvecInputData).Separate(SIZEOF__BYTE) *
                    SIZEOF__BYTE;

                if (iNewPacketDataSize > iMaxPacketDataSize)
                {
                    /* Error, reset flags */
                    DataUnit[iPacketID].bOK = FALSE;
                    DataUnit[iPacketID].bReady = FALSE;

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
                        iNewPacketDataSize / SIZEOF__BYTE;
                }

                /* Packets with no useful data are permitted if no packet
                   data is available to fill the logical frame. The PPI
                   shall be set to 1 and the first byte of the data field
                   shall be set to 0 to indicate no useful data. The first
                   and last flags shall be set to 1. The continuity index
                   shall be incremented for these empty packets */
                if ((biFirstFlag == 1) &&
                    (biLastFlag == 1) && (iNewPacketDataSize == 0))
                {
                    /* Packet with no useful data, reset flag */
                    DataUnit[iPacketID].bReady = FALSE;
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
                    (_BINARY) (*pvecInputData).Separate(1);

            /* Read bytes which are not used */
            for (int i = 0; i < iNumSkipBytes; i++)
                (*pvecInputData).Separate(SIZEOF__BYTE);

            /* Use data unit ------------------------------------------------ */
            if (DataUnit[iPacketID].bReady == TRUE)
            {
                cout << "new data unit for stream " << iStreamID << " packet id " << iPacketID  << endl;

                if(app[iPacketID])
                    app[iPacketID]->AddDataUnit(DataUnit[iPacketID].vecbiData);

                /* Packet was used, reset it now for new filling with new data
                   (this will also reset the flag
                   "DataUnit[iPacketID].bReady") */
                DataUnit[iPacketID].Reset();
            }
        }
        else
        {
            /* Skip incorrect packet */
            for (int i = 0; i < iTotalPacketSize; i++)
                (*pvecInputData).Separate(SIZEOF__BYTE);
        }
    }
}

double PacketDataDecoder::bitRate(int pid)
{
    return (iMaxPacketDataSize * iNumPackets[pid]) / (iNumFrames * 400 /* ms TODO Mode E */);
}
