#ifndef DABAPPLICATIONS_H
#define DABAPPLICATIONS_H
#include "packetapplication.h"

/* Define application types ETSI TS 101 756 v1.5.1 */
enum EAppType {
 AT_RESERVED = 0,
 AT_NOT_SUP = 1,
 AT_MOTSLIDESHOW = 2,        // TS 101 499
 AT_BROADCASTWEBSITE = 3,    // TS 101 498
 AT_TPEG = 4,
 AT_DGPS = 5,
 AT_TMC =	6,               // TS 102 368
 AT_EPG =	7,               // TS 102 818
 AT_JAVA =	8,               // TS 101 993
 AT_DMB  =	9,               // TS 102 428
 AT_IPDC =	0xa,             // TS 102 978
 AT_VOICE =	0xb,             // TS 102 632
 AT_MIDDLEWARE =	0xc,     // TS 102 635
 AT_FILECASTING = 0xd,       // TS 103 177
 AT_JOURNALINE = 0x44A,      // TS 102 979
 AT_EXPERIMENTAL
};

class DABApplications
{
public:
    DABApplications();
    // use a reference to a pointer to avoid casting
    static PacketApplication* createDecoder(EAppType eAppType);
};

#endif // DABAPPLICATIONS_H
