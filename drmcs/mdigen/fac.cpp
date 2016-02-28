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

#include "fac.h"

static unsigned long CRC(const char *data, size_t bytesize,
                         unsigned short num_crc_bits, unsigned long crc_gen, bool invert)
{
    unsigned long crc_holder, ones, i, msb, databit;
    signed short j;

    ones = (1 << num_crc_bits) - 1;
    crc_holder = ones;
    for (i=0; i<bytesize; i++)
        for (j=7; j>=0; j--)
        {
            crc_holder <<= 1;
            msb = crc_holder >> num_crc_bits;
            databit = (data[i] >> j) & 1;
            if ((msb ^ databit) != 0)
                crc_holder = crc_holder ^ crc_gen;
            crc_holder = crc_holder & ones;
        }
    if (invert)
        crc_holder = crc_holder ^ ones; // invert checksum
    return crc_holder;
}

FAC::FAC():mux_plan(NULL),rep(0),short_id(0),service_pattern(0),service(),channel()
{
}

FAC::~FAC()
{
}
/*
     Number of audio services Number of data services Repetition pattern
0x4  1                        0
0x8  2                        0
0xc  3                        0
0x0  4                        0
0x1  0                        1
0x2  0                        2
0x3  0                        3
0xf  0                        4
0x5  1                        1                       A1A1A1A1D1
0x6  1                        2                       A1A1A1A1D1A1A1A1A1D2
0x7  1                        3                       A1A1A1A1D1A1A1A1A1D2A1A1A1A1D3
0x9  2                        1                       A1A2A1A2D1
0xa  2                        2                       A1A2A1A2D1A1A2A1A2D2
0xd  3                        1                       A1A2A3A1A2A3D1
=== SORTED
0x0  4                        0
0x1  0                        1
0x2  0                        2
0x3  0                        3
0x4  1                        0
0x5  1                        1                       A1A1A1A1D1
0x6  1                        2                       A1A1A1A1D1A1A1A1A1D2
0x7  1                        3                       A1A1A1A1D1A1A1A1A1D2A1A1A1A1D3
0x8  2                        0
0x9  2                        1                       A1A2A1A2D1
0xa  2                        2                       A1A2A1A2D1A1A2A1A2D2
0xc  3                        0
0xd  3                        1                       A1A2A3A1A2A3D1
0xf  0                        4
*/

const char* FAC::MUXPLAN[16] =
{   "A1A2A3A4", "D1", "D1D2", "D1D2D3",
    "A1", "A1A1A1A1D1", "A1A1A1A1D1A1A1A1A1D2", "A1A1A1A1D1A1A1A1A1D2A1A1A1A1D3",
    "A1A2", "A1A2A1A2D1", "A1A2A1A2D1A1A2A1A2D2", "",
    "A1A2A3", "A1A2A3A1A2A3D1", "", "D1D2D3D4"
};

void FAC::ReConfigure(const DrmMuxConfig& config, uint8_t new_service_pattern)
{
    service = config.service;
    channel = config.channel;
    service_pattern = new_service_pattern;
    mux_plan = MUXPLAN[service_pattern];
    audio.clear();
    data.clear();
    for(size_t i=0; i<config.service.size(); i++) {
        if(config.service[i].audio_ref.length()>0) {
            audio.push_back(static_cast<int>(i));
        }
        else if(config.service[i].data_ref.length()>0) {
            data.push_back(static_cast<int>(i));
        }
    }
    rep=0;
}

/*
The channel parameters are as follows:
 1 - Base/Enhancement flag 1 bit
 3 - Identity 2 bits
 7 - Spectrum occupancy 4 bits
 8 - Interleaver depth flag 1 bit
10 - MSC mode 2 bits
11 - SDC mode 1 bit
15 - Number of services 4 bits
18 - Reconfiguration index 3 bits
20 - Rfu 2 bits

The service parameters are as follows:
44 - Service identifier 24 bits
46 - Short identifier 2 bits
47 - Audio CA indication 1 bit
51 - Language 4 bits
52 - Audio/Data flag 1 bit
57 - Service descriptor 5 bits
58 - Data CA indication 1 bit
64 - Rfa 6 bits

crc is 8 bits
72
*/

void FAC::NextFrame(bytevector& out, uint8_t frame_number, bool afs_index_valid,
                    uint8_t reconfiguration_index)
{
    // FAC repetition
    switch(mux_plan[rep]) {
    case 'A':
        short_id = audio[mux_plan[rep+1]-'1']; // "A1" -> audio[0], "A2" -> audio[1], ...
        rep+=2;
        if(mux_plan[rep]==0)
            rep=0;
        break;
    case 'D':
        short_id = data[mux_plan[rep+1]-'1']; // "D1" -> data[0], "D2" -> data[1], ...
        rep+=2;
        if(mux_plan[rep]==0)
            rep=0;
        break;
    default:
        rep=0; // got to the end of the string or something went wrong
        break;
    }
    bool is_audio, ca_active;
    uint8_t identity;
    out.clear();
    size_t l;
    // Channel parameters
    out.put(0, 1); // base/enhancement flag
    /* Identity: this 2-bit field identifies the current frame and also validates the SDC AFS index (see clause 6.4) as follows:
       00: first FAC of the transmission super frame and AFS index is valid.
       01: second FAC of the transmission super frame.
       10: third FAC of the transmission super frame.
       11: first FAC of the transmission super frame and AFS index is invalid.
    */
    l=out.size();
    if(frame_number==0)
        if(afs_index_valid)
            identity = 0;
        else
            identity = 3;
    else
        identity = frame_number;
    out.put(identity, 2);
    out.put(channel.spectral_occupancy, 4);
    out.put(channel.interleaver_depth, 1);
    out.put(channel.msc_mode, 2);
    out.put(channel.sdc_mode, 1);
    out.put(service_pattern, 4);
    out.put(reconfiguration_index, 3);
    out.put(0, 2); // rfu
    is_audio = service[short_id].audio_ref.length()>0;
    ca_active = service[short_id].conditional_access;
    out.put(service[short_id].service_identifier);
    out.put(short_id, 2);
    out.put((is_audio && ca_active)?1:0, 1);
    out.put(service[short_id].language, 4);
    out.put(is_audio?0:1, 1);
    out.put(service[short_id].service_descriptor, 5);
    out.put((!is_audio && ca_active)?1:0, 1);
    out.put(0, 6); // rfa

    // 8-bit CRC
    size_t len = out.size();
    char crc_buf[8];
    for(size_t i=0; i<len; i++)
        crc_buf[i]=out[i];
    unsigned long c = CRC(crc_buf, len, 8, 0x11d, true);
    out.put(c, 8);
    //printf("fac frame=%u afs valid=%u, id=%u crc is %02lx\n", frame_number, afs_index_valid?1:0, identity, c);
    //fflush(stdout);
}
