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

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "timestamp.h"
#include "DcpOut.h"
#include "Crc16.h"
#include <bytevector.h>
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

DcpOut::DcpOut():pft(NULL),sock(NULL),ssock(NULL),
    m_type("xxx"), m_target(), m_proto(),
    m_use_crc(true), m_use_tist(true),m_file_framing(false),
    m_tcp_server(false),
    m_src_addr(0), m_dst_addr(0)
{
    tag="destination";
    clearConfig();
    m_type = "xxx";
}

DcpOut::DcpOut(const DcpOut& d)
    :Persist(d),pft(d.pft),sock(d.sock),
     ssock(d.ssock),m_type(d.m_type), m_target(d.m_target), m_proto(d.m_proto),
     m_use_crc(d.m_use_crc), m_use_tist(d.m_use_tist),m_file_framing(d.m_file_framing),
     m_tcp_server(d.m_tcp_server),
     m_src_addr(d.m_src_addr),
     m_dst_addr(d.m_dst_addr)
{
}

DcpOut& DcpOut::operator=(const DcpOut& d)
{
    tag = d.tag;
    pft = d.pft;
    sock = d.sock;
    ssock = d.ssock;
    m_type = d.m_type;
    m_target = d.m_target;
    m_proto = d.m_proto;
    m_use_crc = d.m_use_crc;
    m_use_tist = d.m_use_tist;
    m_file_framing = d.m_file_framing;
    m_tcp_server = d.m_tcp_server;
    m_src_addr = d.m_src_addr;
    m_dst_addr = d.m_dst_addr;
    return *this;
}

void DcpOut::clearConfig()
{
    m_target = "";
    m_src_addr = 0;
    m_dst_addr = 0;
    m_file_framing=false;
    m_use_tist=true;
    m_use_crc=true;
    m_tcp_server = false;
    if(pft) {
        delete pft;
        pft=NULL;
    }
    if(sock) {
        delete sock;
        sock=NULL;
    }
}

DcpOut::~DcpOut()
{
    clearConfig();
}

void DcpOut::ReConfigure(const string& uri)
{
    map<string,string> config;
    config.clear();
    parseDcpUri(config, uri);
    ReConfigure(config);
}


void DcpOut::ReConfigure(map<string,string>& config)
{
    clearConfig();
    for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++) {
        setParam(i->first.c_str(), i->second.c_str());
    }
    if(config.count("crc")>0) {
        const string & s = config["crc"];
        if(s=="0" || s == "false" || s=="f")
            m_use_crc = false;
    }
    if(config.count("pft")>0) {
        if(config.count("fec")>0) {
            const string & s = config["fec"];
            if(s=="0")
                pft = new SimplePftOut();
            else
                pft = new FecPftOut();
        } else {
            pft = new SimplePftOut();
        }
        pft->ReConfigure(config);
    }
    config["addr"] = string(&config["target"][2]);
    config["port"]=config["dst_addr"];
    if(m_type=="udp") {
        sock = new dgram_socket();
    } else if(m_type=="tcp") {
        if(config.count("end")>0) {
            string& end = config["end"];
            if(end == "server") {
                ssock = new server_socket();
                m_tcp_server = true;
            } else {
                sock = new client_socket();
            }
        } else {
            sock = new client_socket();
        }
    } else if(m_type=="file") {
        config["path"]=config["addr"];
        sock = new file_socket();
    } else if(m_type=="ser") {
        // not done yet
    }
    if(sock) {
        sock->ReConfigure(config);
        misconfiguration = false;
    } else if(m_tcp_server) {
        ssock->ReConfigure(config);
        misconfiguration = false;
    }
}

bool DcpOut::setParam(const char *param, const char *value)
{
    if(strcmp(param, "type")==0) {
        m_type = value;
    }
    if(strcmp(param, "target")==0) {
        m_target = value;
    }
    if(strcmp(param, "src_addr")==0) {
        m_src_addr = atoi(value);
    }
    if(strcmp(param, "dst_addr")==0) {
        m_dst_addr = atoi(value);
    }
    if(strcmp(param, "file_framing")==0) {
        m_file_framing = false;
        const string s = value;
        if(s=="1" || s == "true" || s=="t")
            m_file_framing = true;
    }
    if (strcmp(param, "proto")==0) {
        m_proto = value;
    }
    return true;
}

void DcpOut::PutParams(xmlTextWriterPtr writer)
{
    Persist::PutParams(writer);
    // scheme
    xmlTextWriterWriteString(writer, BAD_CAST "dcp.");
    xmlTextWriterWriteString(writer, BAD_CAST m_type.c_str());
    if(pft)
        xmlTextWriterWriteString(writer, BAD_CAST ".pft");
    // :
    xmlTextWriterWriteString(writer,BAD_CAST  ":");
    // target
    xmlTextWriterWriteString(writer,BAD_CAST  m_target.c_str());
    // optional src/dst addr
    if(m_dst_addr != 0) {
        xmlTextWriterWriteString(writer,BAD_CAST  ":");
        if(m_src_addr != 0) {
            xmlTextWriterWriteFormatString(writer, "%u:", m_src_addr);
        }
        xmlTextWriterWriteFormatString(writer, "%u", m_dst_addr);
    }
    // optional query
    if(pft) {
        char sep[2]="?";
        map<string,string> config;
        pft->config(config);
        for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++) {
            const char *param=i->first.c_str(), *value=i->second.c_str();
            xmlTextWriterWriteFormatString(writer, "%s%s=%s", sep, param, value);
            sep[0]='&';
        }
    }
}

bool DcpOut::sendFrame(const tagpacketlist& frame, const vector<string>& tag_tx_order, uint16_t af_seq)
{
    size_t packet_len, num_packets;
    if(sock) {
        if(sock && sock->handle==INVALID_SOCKET) {
            sock->open();
            if(sock->handle==INVALID_SOCKET)
                return true;
        }
    } else {
        if(m_tcp_server) {
            if(ssock) {
                if(ssock->handle==INVALID_SOCKET) {
                    ssock->open();
                }
                if(ssock->poll()) {
                    stream_socket *s = new stream_socket;
                    bool ok = ssock->fetch(*s);
                    if(ok)
                        sock = s;
                    else
                        return false; // should be true ?
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    vector<uint8_t> c;
    if(pft) {
        vector<uint8_t> d;
        makeAFpacket(d, frame, tag_tx_order, af_seq);
        pft->makePFT(d, c, packet_len);
        num_packets = c.size() / packet_len;
        if(num_packets*packet_len<c.size())
            num_packets++;
    } else {
        makeAFpacket(c, frame, tag_tx_order, af_seq);
        packet_len=c.size();
        num_packets=1;
    }
    size_t bytes = c.size();
    for(unsigned i=0; i<num_packets; i++) {
        size_t p,len;
        p=i*packet_len;
        if((p+packet_len)<=bytes)
            len = packet_len;
        else
            len = c.size()-p;
        if(m_file_framing) {
            vector<uint8_t> b;
            makeFFheader(b, len, true);
            sock->send(b);
        }
        sock->send(c, p, len);
    }
    return false;
}

bool DcpOut::sendFrameRaw(const vector<uint8_t>& data)
{
    if(sock) {
        if(sock && sock->handle==INVALID_SOCKET) {
            sock->open();
            if(sock->handle==INVALID_SOCKET)
                return true;
        }
    } else {
        if(m_tcp_server) {
            if(ssock) {
                if(ssock->handle==INVALID_SOCKET) {
                    ssock->open();
                }
                if(ssock->poll()) {
                    stream_socket *s = new stream_socket;
                    bool ok = ssock->fetch(*s);
                    if(ok)
                        sock = s;
                    else
                        return false; // should be true ?
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    sock->send(data, 0, data.size());

    return false;
}


void DcpOut::makeAFpacket(vector<uint8_t>& out,
                          const tagpacketlist& frame, const vector<string>& tag_tx_order, uint16_t af_seq)
{
    bytevector b;
    for(vector<string>::const_iterator tagp=tag_tx_order.begin(); tagp!=tag_tx_order.end(); tagp++) {
        tagpacketlist::const_iterator f = frame.find(*tagp);
        if(f!=frame.end()) {
            const bytevector payload=f->second;
            size_t n=payload.size();
            unsigned bits=payload.bits;
            b.put(*tagp);
            b.put(8*n+bits, 32);
            b.put(payload);
            if(payload.bits!=0) {
                b.put(payload.in_progress, 8);
            }
        }
    }
    size_t bs = b.size();
    bytevector tp;
    tp.put("AF");
    tp.put(bs, 32);
    tp.put(af_seq, 16);
    tp.put(1, m_use_crc?1:0); // crc flag
    tp.put(1, 3); // MAJor version
    tp.put(0, 4); // MINor version
    tp.put("T"); // protocol type tag packets
    tp.put(b);
    CCrc16 crc;
    for_each(tp.data().begin(), tp.data().end(), crc);
    uint16_t r = crc.result();
cout << "crc " << r << endl;
    tp.put(r, 16);
    out = tp.data();
}
/* File: ETSI TS 102 821 V0.0.2f (2003-10)

PFT Fragments or AF Packets may be stored in a file for offline distribution,
  archiving or any other purpose.  A standard mapping has been defined using
   the Hierarchical TAG Item option available,  however other mappings may be
    (or this one extended) in the future.  The top level TAG Item has the TAG
    Name fio_ and is used to encapsulate a TAG packet,  one part of which is
    the AF Packet or PFT Fragment in an afpf TAG Item.  Additional TAG Items
    have been defined to monitor the reception or control the replay of the packets.

B.3.1	File IO (fio_)
The fio_ TAG Item is the highest layer TAG Item in the file TAG Item hierarchy.

Tag name: fio_
Tag length: 8*n bits
Tag Value: This TAG Item acts as a container for an afpf TAG Item.  A time TAG Item
may optionally be present.

B.3.1.1	AF Packet / PFT Fragment (afpf)
The afpf TAG Item contains an entire AF Packet or PFT Fragment as the TAG Value.

Figure 16: AF Packet or PFT Fragment

B.3.1.2	Timestamp (time)
The time TAG Item may occur in the payload of any fio_ TAG Item.
It may record the time of reception of the payload,  or it may indicate
the intended time of replay.  The time value given in the TI_SEC and TI_NSEC
 fields may be relative to the start of the file,  or any other reference desired.

Figure 17: File timestamp
TI_SEC:  the number of whole SI seconds,  in the range 0-2^32-1)
TI_NSEC:  the number of whole SI nanoseconds,  in the range 0-999 999 999.
  Values outside of this range are not defined
*/

void DcpOut::makeFFheader(vector<uint8_t>& out, size_t packet_size, bool sendTime)
{
    timespec tp;
    clock_getrealtime(&tp);
    uint64_t bytesize_ff = 4+4+packet_size; // afpf
    if(sendTime)
        bytesize_ff += 4+4+4+4;
    bytevector b;
    // Tag Item fio_
    b.put("fio_");
    b.put(8*bytesize_ff, 32);
    // nested tag packets
    // Tag Item time
    if(sendTime) {
        b.put("time");
        b.put(64, 32);
        b.put(tp.tv_sec, 32);
        b.put(tp.tv_nsec, 32);
    }
    // Tag Item afpf
    b.put("afpf");
    b.put(8*packet_size, 32);
    out = b.data();
}
