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

#include "sdiout.h"
#include "SdcElement.h"
#include <iostream>
using namespace std;

SdiOut::SdiOut():tag_tx_order(),frame_count(rand()),reconfiguration_version(false)
{
    tag_tx_order.resize(7);
    tag_tx_order[0]="*ptr";
    tag_tx_order[1]="dlfc";
    tag_tx_order[2]="tist";
    tag_tx_order[3]="comp";
    tag_tx_order[4]="defn";
    tag_tx_order[5]="type";
    tag_tx_order[6]="_enc";
}

SdiOut::~SdiOut()
{
}

void SdiOut::ReConfigure()
{
    reconfiguration_version=!reconfiguration_version;
}

void SdiOut::buildFrame(ServiceComponentEncoder& sce, DrmTime& timestamp)
{
    bytevector dlfc,tist;
    frame.clear();
    frame["*ptr"] << "DSDI" << bitset<32>(0);
    // dlfc
    dlfc.put(frame_count, 32);

    // tist
    tist.put(timestamp.utco,14);
    tist.put(timestamp.tist_second(), 40);
    tist.put(timestamp.tist_ms(), 10);

    bytevector buf;
    SdcElement e;
    switch(sce.current.type)
    {
    case ServiceComponent::audio_sce:
        e.Type9(sce.current, 0, 0, false, false);
        frame["defn"]=e.out;
        sce.NextFrame(buf, sce.current.bytes_per_frame);
        frame["comp"]=buf;
        break;
    case ServiceComponent::text_sce: // no configurable parameters
        frame["comp"].put(sce.next_message());
        break;
    case ServiceComponent::data_stream_sce:
    case ServiceComponent::data_packet_mode_sce:
        e.Type5(sce.current, 0, 0, false);
        frame["defn"]=e.out;
        sce.NextFrame(buf, sce.current.bytes_per_frame);
        frame["comp"]=buf;
        break;
    default:
        throw string("bad component");
    }

    frame["type"].put(sce.current.typestring);
    frame["_enc"].put(sce.current.encoder_id);
    frame["dlfc"]=dlfc;
    frame["tist"]=tist;

    frame_count++;
}
