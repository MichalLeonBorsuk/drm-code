#ifndef PACKETAPPLICATION_H
#define PACKETAPPLICATION_H

#include <../util/Vector.h>

class PacketApplication
{
public:
    PacketApplication();
    virtual void AddDataUnit (CVector <_BINARY>&);
};

#endif // PACKETAPPLICATION_H
