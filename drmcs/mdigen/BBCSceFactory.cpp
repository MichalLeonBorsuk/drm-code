/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):  Julian Cable, Ollie Haffenden, Andrew Murphy
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

#include "BBCSceFactory.h"
#include "FileSCE.h"
#include "DUFileSCE.h"
#include "PacketFileSCE.h"
#include "CTAudioMemSCE.h"
#include "TranslatingTextSCE.h"
#include "TextFileSCE.h"
#ifdef NOT_JUST_A_TEST_GENERATOR
# include "CTAudioFileSCE.h"
# include "TimedTextSCE.h"
# include "TickerTextSCE.h"
//# include "webTextSCE.h"
# include "udpTextSCE.h"
# include "MOTSCE.h"
# include "JMLSCE.h"
#endif

/* TODO (jfbc#2#): find a way for these to be less hard coded - ability to
                   register new encoder types - DLLs ? */
/* TODO (jfbc#1#): Add support for external SCEs using DCP bidirectional */
/* TODO (jfbc#1#): Test at least one service component which is a pre-
                   coded file - should work for text, audio and data but
                   completely UNTESTED */
/* TODO (jfbc#1#): Add support for W3C Timed Text text SCE */
/* DONE (jfbc#1#): Add support for BBC Ticker TextSCE - could be stand
                   alone external SCE */
/* DONE (jfbc#1#): Add support for live audio encoding */

ServiceComponentEncoder* BBCSceFactory::create(const ServiceComponent& config)
{
    if(config.implementor == "File") {
        return new FileSCE();
    }
    if(config.implementor == "DUFile") {
        return new DUFileSCE();
    }
    if(config.implementor == "PacketFile") {
        return new PacketFileSCE();
    } else if(config.implementor == "CTAudioMem") {
        return new CCTAudioMemSCE();
    } else if(config.implementor == "FixedText") {
        return new CTranslatingTextSCE();
    } else if(config.implementor == "TextFile") {
        return new TextFileSCE();
#ifdef NOT_JUST_A_TEST_GENERATOR
    } else if(config.implementor == "CTAudioFile") {
        return new CCTAudioFileSCE();
    } else if(config.implementor == "udpText") {
        return new udpTextSCE();
    } else if(config.implementor == "JML") {
        return new JMLSCE();
    } else if(config.implementor == "TimedText") {
        return new CTimedTextSCE();
    } else if(config.implementor == "MOT") {
        return new MOTSCE();
    } else if(config.implementor == "TickerText") {
        return new CTickerTextSCE();
#endif
    } else
        throw string("unknown implementor tag ")+config.implementor;
}
