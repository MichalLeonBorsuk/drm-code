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

#include "PftOut.h"
#include <crcbytevector.h>
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


PftOut::PftOut()
  :m_mtu(0), m_use_address(false),
  m_source_address(0), m_dest_address(0),
  m_sequence_counter(rand())
{
}

void PftOut::clearConfig()
{
    m_mtu=0;
    m_use_address=false;
    m_source_address = m_dest_address = 0;
    m_sequence_counter=rand();
}

PftOut::~PftOut()
{
}

void PftOut::ReConfigure(map<string,string>& config)
{
    clearConfig();
    for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++) {
        string param=i->first, value=i->second;
        if(param=="maxpaklen") {
            m_mtu = atoi(value.c_str());
        } else if(param=="saddr") {
            m_source_address = atoi(value.c_str());
            m_use_address=true;
        } else if(param=="daddr") {
            m_dest_address = atoi(value.c_str());
            m_use_address=true;
        }
    }
}

void PftOut::config(map<string,string>& config)
{
    if(m_mtu>0) {
        stringstream s;
        s << m_mtu;
        config["maxpaklen"]= s.str();
    }
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

void PftOut::makePFTheader(
    crcbytevector &out, size_t in_size,
    uint32_t Findex, uint32_t Fcount,
    bool fec, uint16_t rsK, uint16_t rsZ
)
{
    out.crc.reset();
    // write PFT Packet Header
    out.put("PF");
    out.put(m_sequence_counter, 16);
    out.put(Findex, 24);
    out.put(Fcount, 24);
    out.put(fec?1:0, 1);
    out.put(m_use_address, 1);
    out.put(in_size, 14);
    // optional FEC header field
    if (fec) {
        out.put(rsK, 8);
        out.put(rsZ, 8);
    }
    // optional address header field
    if (m_use_address) {
        out.put(m_source_address, 16);
        out.put(m_dest_address, 16);
    }
    // CRC
    out.put(out.crc.result(), 16);
}

int PftOut::headerLength(bool use_addr, bool use_fec)
{
    return 14+(use_addr?4:0)+(use_fec?2:0);
}

int PftOut::makePFT(
    const vector<uint8_t>& in, vector<uint8_t>& pfp,
    size_t header_bytesize, size_t payload_bytesize, uint16_t num_packets,
    bool fec, uint16_t rsK, uint16_t rsZ)
{
    crcbytevector out;
    uint16_t data_size = static_cast<uint16_t>(in.size());
    size_t space_needed = data_size+num_packets*header_bytesize;
    out.reserve(space_needed);
    out.clear();
    size_t bytes_remaining = data_size;
    const uint8_t *p = in.data();
    for (uint16_t n=0; n<num_packets; n++)
    {
        if(bytes_remaining < payload_bytesize)
            payload_bytesize = bytes_remaining; // last packet
        makePFTheader(out, payload_bytesize, n, num_packets, fec, rsK, rsZ);
        // write PFT Packet Payload
        out.putbytes((char*)p, payload_bytesize);
        p += payload_bytesize;
        bytes_remaining -= payload_bytesize;
    }
    pfp = out.data();
    // increment SEQ counter
    m_sequence_counter++;
    return ERR_NO_ERROR;
}

int SimplePftOut::makePFT(const vector<uint8_t>& in,
                          vector<uint8_t>& out, size_t &packet_bytesize)
{
    uint16_t num_packets, data_size=in.size();
    size_t header_bytesize, payload_bytesize;
    size_t space = size_t(m_mtu);
    header_bytesize = headerLength(m_use_address, false);
    if ((space>0) && (space<(data_size+header_bytesize))) {
        payload_bytesize = space - header_bytesize;
        num_packets = data_size / payload_bytesize;
        if(num_packets*payload_bytesize<data_size)
            num_packets++;
        payload_bytesize = data_size / num_packets;
        if(num_packets*payload_bytesize<data_size)
            payload_bytesize++;
    } else {
        num_packets = 1;
        payload_bytesize = data_size;
    }
    packet_bytesize = header_bytesize + payload_bytesize;
    return PftOut::makePFT(in, out, header_bytesize, payload_bytesize, num_packets, false, 0, 0);
}

void FecPftOut::ReConfigure(map<string,string>& config)
{
    PftOut::ReConfigure(config);
    m_expected_packet_losses = atoi(config["fec"].c_str());
}

void FecPftOut::config(map<string,string>& config)
{
    PftOut::config(config);
    config["fec"]=m_expected_packet_losses;
}

int FecPftOut::makePFT(const vector<uint8_t>& in,
                       vector<uint8_t>& out, size_t &packet_bytesize)
{
    uint16_t data_size=in.size();
    // c = ceil(l/kmax), l=AF packet len, kmax = 207
    uint16_t c = data_size/PFT_RS_N_MAX;
    if(c*PFT_RS_N_MAX<data_size) {
        c++;
    }
    // k = ceil(l/c)
    uint16_t k = data_size / c;
    if(c*k<data_size)
        k++;
    // z = c.k-l
    uint8_t z = c * k - data_size;
    // s_max = MIN( (c.p)/(m+1), MTU-h), for m>0
    // s_max = MTU-h, for m=0
    uint16_t s_max;
    size_t header_bytesize = headerLength(m_use_address, true);
    if(m_expected_packet_losses>0) {
        s_max = (c * PFT_RS_P)/(m_expected_packet_losses+1);
        if(s_max > (m_mtu - header_bytesize))
            s_max = m_mtu - header_bytesize;
    } else {
        s_max = m_mtu - header_bytesize;
    }

    uint16_t rs_block_size=(data_size+c*PFT_RS_P+z);

    // f = ceil( (l+c.p+z)/s_max) )
    uint8_t number_of_fragments = rs_block_size/s_max;
    if(number_of_fragments*s_max < rs_block_size)
        number_of_fragments++;
    // s = ceil( (l+c.p+z)/f) )
    uint8_t fragment_size = rs_block_size / number_of_fragments;
    if(number_of_fragments*fragment_size < rs_block_size)
        fragment_size++;

    size_t out_size = number_of_fragments*fragment_size;

    // ============ done calculating our parameters ======================

    uint8_t *data_to_send = new uint8_t[out_size];
    size_t inp=0, outp=0;
    for(uint16_t i=0; i<c-1; i++) {
        memcpy(&data_to_send[outp],  in.data()+inp, k);
        //memset(&data_to_send[outp], i+1, k);
        outp+=k;
        //memset(&data_to_send[outp], 255, PFT_RS_P);
        code.Encode(&data_to_send[outp-k], &data_to_send[outp]);
        outp+=PFT_RS_P;
        inp+=k;
    }
    memcpy(&data_to_send[outp], in.data()+inp, k-z);
    //memset(&data_to_send[outp], c, k);
    outp+=k-z;
    memset(&data_to_send[outp], 0x00, z);
    outp+=z;
    code.Encode(&data_to_send[outp-k], &data_to_send[outp]);
    //memset(&data_to_send[outp], 255, PFT_RS_P);

    vector<uint8_t> fec_bytes;
    fec_bytes.resize(out_size, 0xfe);
    for(int i=0; i<number_of_fragments; i++) {
        for(int j=0; j<fragment_size; j++) {
            size_t n = j*number_of_fragments+i;
            if(n<rs_block_size)
                fec_bytes[i*fragment_size+j] = data_to_send[n];
            else
                fec_bytes[i*fragment_size+j] = 0x00;
        }
    }
    delete[] data_to_send;
    packet_bytesize = header_bytesize + fragment_size;
    return PftOut::makePFT(fec_bytes, out, header_bytesize,
                           fragment_size, number_of_fragments, true, k, z);
}
