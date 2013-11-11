#ifndef DABAPPLICATIONS_H
#define DABAPPLICATIONS_H
#include "packetapplication.h"

/* Define for application types */
enum EDABAppType {
 AT_DREAM_EXPERIMENTAL = 1,
 AT_MOTSLIDESHOW = 2,
 AT_BROADCASTWEBSITE = 3,
 AT_TPEG = 4,
 AT_DGPS = 5,
 AT_TMC =	6,
 AT_EPG =	7,
 AT_JAVA =	8,
 AT_DMB  =	9,
 AT_IPDC =	0xa,
 AT_VOICE =	0xb,
 AT_MIDDLEWARE =	0xc,
 AT_JOURNALINE = 0x44A
};

class DABApplications
{
public:
    DABApplications();
    static PacketApplication* createDecoder(EDABAppType eAppType);
};

#endif // DABAPPLICATIONS_H
