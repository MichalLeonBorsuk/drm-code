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

#include "PftIn.h"
#include <string.h>
#include <iostream>
#include <sstream>
using namespace std;

#define AF_CODED_PACKET_BUFFER_BYTESIZE 0x4000
#define PFT_CRC_GEN 0x11021
#define PFT_RS_N_MAX 207
#define PFT_RS_K 255U
#define PFT_RS_P (PFT_RS_K - PFT_RS_N_MAX)

#define ERR_NO_ERROR 0
#define ERR_NO_AFPF_PACKET_FOUND 16
#define ERR_PFT_MTU_TOO_SMALL 20
#define ERR_PFT_TOO_MANY_FRAGMENTS 21
#define ERR_PFT_PACKET_TOO_SHORT 22
#define ERR_PFT_INVALID_HEADER 23
#define ERR_PFT_PACKET_TOO_LONG 24
#define ERR_PFT_CRC_FAILURE 25
#define ERR_PFT_INVALID_FCNT 26
#define ERR_PFT_WRONG_PACKET_ORDER 27
#define ERR_PFT_FEC_DECODING_FAILURE 28
#define ERR_PFT_BUFFER_TOO_SMALL 29
#define ERR_AF_PACKET_TOO_SHORT 30
#define ERR_AF_INVALID_HEADER 31
#define ERR_AF_CRC_FAILURE 32


PftIn::PftIn()
{
    clearConfig();
}

void PftIn::clearConfig()
{
    m_use_address=false;
    m_source_address = m_dest_address = 0;
}

PftIn::~PftIn()
{
}

void PftIn::ReConfigure(map<string,string>& config)
{
    clearConfig();
    for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++) {
        string param=i->first, value=i->second;
        if(param=="saddr") {
            m_source_address = atoi(value.c_str());
            m_use_address=true;
        } else if(param=="daddr") {
            m_dest_address = atoi(value.c_str());
            m_use_address=true;
        }
    }
}

void PftIn::config(map<string,string>& config)
{
    if(m_use_address) {
        if(m_source_address!=0) {
            stringstream s;
            s << m_source_address;
            config["saddr"]= s.str();
        }
        if(m_dest_address!=0) {
            stringstream s;
            s << m_dest_address;
            config["daddr"]= s.str();
        }
    }
}

bool PftIn::decodePFT(bytev& out, const bytev& data)
{
    size_t p=0;
    char s0 = data[p++];
    char s1 = data[p++];
    if(s0!='P' || s1!='F')
        return false;
    uint16_t Pseq = (data[p]<<8)+data[p+1];
    p+=2;
    uint32_t Findex = ((data[p]<<8)+data[p+1])<<8+data[p+2];
    p+=3;
    uint32_t Fcount = ((data[p]<<8)+data[p+1])<<8+data[p+2];
    p+=3;
    uint16_t Plen = (data[p]<<8)+data[p+1];
    p+=2;
    bool fec = (Plen & 0x8000)!=0;
    bool addr = (Plen & 0x4000)!=0;
    Plen &= 0x3FFF;
    // optional FEC header field
    uint16_t rsK, rsZ;
    if (fec) {
        rsK = data[p++];
        rsZ = data[p++];
    }
    // optional address header field
    uint16_t source_address=0, dest_address=0;
    if (addr) {
        source_address = data[p]<<8+data[p+1];
        p+=2;
        dest_address = data[p]<<8+data[p+1];
        p+=2;
    }
    if(m_use_address && m_source_address != source_address)
        return false;
    if(m_use_address && m_dest_address != dest_address)
        return false;
    uint16_t crc = data[p]<<8+data[p+1];
    p+=2;
    // TODO check CRC
    if (fec)
        return decodePFTWithFEC(out, data, Pseq, Plen, Findex, Fcount, rsK, rsZ);
    else
        return decodeSimplePFT(out, data, Pseq, Plen, Findex, Fcount);
}

bool PftIn::decodeSimplePFT(bytev& out, const bytev& data, uint16_t Pseq, uint16_t Plen, uint32_t Findex, uint32_t Fcount)
{
    if(Fcount==1)
    {
        out = data;
        return true;
    }
    mapFragments[Pseq].AddSegment(data, Findex, Findex==(Fcount-1));

    if(mapFragments[Pseq].Ready())
    {
        out = mapFragments[Pseq].vecData;
        return true;
    }

    return false;
}

bool PftIn::decodePFTWithFEC(bytev& out, const bytev& data, uint16_t Pseq, uint16_t Plen, uint32_t Findex, uint32_t Fcount, uint16_t rsK, uint16_t rsZ)
{
    uint16_t decoded_size = Fcount*Plen;
    uint32_t c_max = Fcount*Plen/(rsK+PFT_RS_P);  /* rounded down */
    uint32_t rx_min = c_max*rsK/Plen;
    if(rx_min*Plen<c_max*rsK)
        rx_min++;
    mapFragments[Pseq].AddSegment(data, Findex, Fcount == (Findex+1));
    if(mapFragments[Pseq].segment_count()>=rx_min) {
        bytev deinterleaved;
        deinterleaved.resize(Fcount*Plen);
        deinterleave(mapFragments[Pseq].vecData, deinterleaved, Plen, Fcount);
        return rsCorrectData(deinterleaved, out, c_max, rsK, rsZ);
    }
    return false;
}

void PftIn::deinterleave(const bytev& input, bytev& output, uint16_t Plen, uint32_t Fcount)
{
    for(size_t fidx=0; fidx<Fcount; fidx++)
    {
        for (size_t r=0; r<Plen; r++)
        {
            output[fidx+r*Fcount] = input[fidx*Plen+r];
        }
    }
}

bool PftIn::rsCorrectData(const bytev& input, bytev& output, uint32_t c_max, uint16_t rsK, uint16_t rsZ)
{
    uint32_t index_coded = 0, index_out = 0;
    for (uint32_t i=0; i<c_max; i++)
    {
        memcpy(&output[index_out], &input[index_coded], rsK);
        index_coded += rsK;
        memcpy(&output[index_out+PFT_RS_N_MAX], &input[index_coded], PFT_RS_P);
        index_coded += PFT_RS_P;
        if(code.Decode(&output[index_out])<0)
            return false;
        index_out += rsK;
    }
    return true;
}
