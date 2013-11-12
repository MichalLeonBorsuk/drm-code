#include "dabapplications.h"
#include "Journaline.h"
#include "DABMOT.h"

DABApplications::DABApplications()
{
}

PacketApplication* DABApplications::createDecoder(EAppType eAppType)
{
    switch (eAppType)
    {
    case AT_MOTSLIDESHOW:	/* MOTSlideshow */
        return new CMOTDABDec();
        break;

    case AT_EPG:	/* EPG */
        return new CMOTDABDec();
        break;

    case AT_BROADCASTWEBSITE:	/* MOT Broadcast Web Site */
        return new CMOTDABDec();
        break;

    case AT_JOURNALINE:
        return new CJournaline();
        break;

    default:
        return NULL;
        ;
    }
}
