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
#include "CTAudioMemSCE.h"
#include "TranslatingTextSCE.h"
#include "TextFileSCE.h"
#include "PacketSCE.h"

    /* TODO (jfbc#2#): find a way for these to be less hard coded - ability to 
                       register new encoder types - DLLs ? */
    /* TODO (jfbc#1#): Add support for external SCEs using DCP bidirectional */
    /* TODO (jfbc#1#): Test at least one service component which is a pre-
                       coded file - should work for text, audio and data but 
                       completely UNTESTED */
    /* TODO (jfbc#1#): Add support for W3C Timed Text text SCE */
    /* TODO (jfbc#1#): Add support for BBC Ticker TextSCE - could be stand 
                       alone external SCE */
    /* DONE (jfbc#1#): Add support for live audio encoding */

CDrmServiceSCE* BBCSceFactory::newComponent(xmlNodePtr n)
{
   xmlChar *mode;
   CDrmServiceSCE *enc = NULL;
   mode = xmlGetProp(n, BAD_CAST "type");
   if(mode) {
       if(xmlStrEqual(mode, BAD_CAST "audio")){
         enc = new CCTAudioMemSCE;
         enc->m_type=CDrmServiceSCE::audio_sce;
       } else if(xmlStrEqual(mode, BAD_CAST "text")){
         enc = new CTextFileSCE;
         enc->m_type=CDrmServiceSCE::text_sce;
       } else if(xmlStrEqual(mode, BAD_CAST "data_stream")){
         enc = new CDataSCE;
         enc->m_type=CDrmServiceSCE::data_stream_sce;
       } else if(xmlStrEqual(mode, BAD_CAST "data_packet")){
         enc = new CPacketSCE;
         enc->m_type=CDrmServiceSCE::data_packet_mode_sce;
       }
   }
   if(enc==NULL) {
        enc = SceFactory::newComponent(n);
   }
   return enc;
}
