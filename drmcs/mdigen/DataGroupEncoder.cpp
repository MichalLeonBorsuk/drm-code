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

#include "DataGroupEncoder.h"
#include <iostream>
#include <Crc16.h>

void DataGroupEncoder::Configure(bool crc, bool seg, bool tid, const bytevector& ua)
{
    use_crc = crc;
    segment = seg;
    use_tid = tid;
    user_address = ua;
}

void DataGroupEncoder::Configure(bool crc, bool seg, bool tid)
{
    use_crc = crc;
    segment = seg;
    use_tid = tid;
    user_address.clear();
}

void DataGroupEncoder::putDataGroupSegment(bytevector& out, uint16_t transport_id,
        const bytevector& in, uint8_t type, uint8_t cont, uint16_t segment, bool last) const
{
    bytevector seg;
    putSegmentationHeader(seg, 0, in.size());
    seg.put(in);
    putDataGroup(out, transport_id, seg, type, cont, segment, last);
}

void DataGroupEncoder::putDataGroup(uint8_t type, bytevector& out, const bytevector& in, uint8_t cont)
{
    putDataGroup(out, 0, in, type, cont, 0, true);
}

void DataGroupEncoder::putDataGroup(bytevector& out, uint16_t transport_id, const bytevector& in,
                                    uint8_t type, uint8_t cont, uint16_t segment_num, bool last) const
{
    putDataGroupHeader(out, type, cont, 0x0F);
    if(use_tid || (user_address.size()>0) || segment)
        putSessionHeader(out, transport_id, segment_num, last);
    // data
    out.put(in);
    // crc
    if(use_crc) {
        CCrc16 crc;
        for(size_t i=0; i<out.data().size(); i++) crc(out.data()[i]);
        out.put(crc.result(), 16);
    }
}

void DataGroupEncoder::putDataGroupHeader(bytevector& out,
        uint8_t type, uint8_t cont, uint8_t rep, bool x, uint16_t ef) const
{
    // Data Group Header
    bool use_uaf = use_tid || user_address.size()>0;
    out.put(x?1:0, 1); // extension flag
    out.put(use_crc?1:0, 1); // crc flag
    out.put(segment?1:0, 1); // segment flag
    out.put(use_uaf?1:0, 1); // user access flag
    out.put(type, 4); // data group type
    out.put(cont, 4);
    out.put(rep, 4); // continuity indicator
    if(x)
        out.put(ef, 16);
}

void DataGroupEncoder::putSessionHeader(bytevector& out, uint16_t transport_id,
                                        uint16_t segment_num, bool last) const
{
    bool use_uaf = use_tid || user_address.size()>0;
    out.put(last?1:0, 1);
    out.put(segment_num, 15);
    if(use_uaf) {
        out.put(0, 3);	// rfa
        out.put(use_tid?1:0, 1);
        out.put((use_tid?2:0)+user_address.size(), 4);	//length
        if(use_tid)
            out.put(transport_id, 16);
        if(user_address.size()>0)
            out.put(user_address);
    }
}

void DataGroupEncoder::putSegmentationHeader(bytevector& out, uint16_t rep, size_t size) const
{
    //add a segment header
    out.put(rep, 3);
    out.put(size, 13);
}
