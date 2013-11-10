#ifndef FECPACKETDATADECODER_H
#define FECPACKETDATADECODER_H

#include "packetdatadecoder.h"

class FECPacketDataDecoder:public PacketDataDecoder
{
public:
    FECPacketDataDecoder ();
    virtual ~FECPacketDataDecoder ();
    void setFEC(int r, int c) { R=r; C=c;}

protected:
    void InitInternal (CParameter & Parameters);
    void ProcessDataInternal (CParameter & Parameters);

private:
    int R, C;
};

#endif // FECPACKETDATADECODER_H
