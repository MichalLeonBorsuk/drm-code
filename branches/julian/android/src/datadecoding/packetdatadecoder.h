#ifndef PACKETDATADECODER_H
#define PACKETDATADECODER_H

#include <../util/modul.h>
#include "packetapplication.h"

#define MAX_NUM_PACK_PER_STREAM					4

class CDataUnit
{
  public:
    CVector < _BINARY > vecbiData;
    _BOOLEAN bOK;
    _BOOLEAN bReady;

void Reset ()
{
    vecbiData.Init (0);
    bOK = FALSE;
    bReady = FALSE;
}
};

class PacketDataDecoder:public CReceiverModul < _BINARY, _BINARY >
{
public:
    PacketDataDecoder ();
    virtual ~PacketDataDecoder ();
    void setApplication(int, PacketApplication*);
    void setStreamID(int n) { iStreamID = n; }
    double bitRate(int);
    double proportionCorrect();

protected:
    void InitInternal (CParameter & Parameters);
    void ProcessDataInternal (CParameter & Parameters);

    int iContInd[MAX_NUM_PACK_PER_STREAM];
    CDataUnit DataUnit[MAX_NUM_PACK_PER_STREAM];
    PacketApplication* app[MAX_NUM_PACK_PER_STREAM];
    int iStreamID;
    bool DoNotProcessData;
    int iNumDataPackets;
    int iMaxPacketDataSize;
    int iTotalPacketSize;
    // to calculate bit rate;
    int iNumFrames;
    int iNumPackets[MAX_NUM_PACK_PER_STREAM];
    int iNumGoodPackets[MAX_NUM_PACK_PER_STREAM];
};
#endif // PACKETDATADECODER_H
