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

#ifndef _DCPOUT_H
#define _DCPOUT_H
#include <persist.h>
#include <libxml/xmlwriter.h>
#include <libxml/uri.h>
#include "PftOut.h"
#include "DcpUtil.h"
#include "sockets.h"

class DcpOut : public Persist
{
public:
    DcpOut();
    virtual ~DcpOut();
    virtual void ReConfigure(const string& uri);
    virtual void ReConfigure(map<string,string>& config);
    virtual bool setParam(const char *param, const char *value);
    virtual void GetParams(xmlNodePtr n) {}
    virtual void PutParams(xmlTextWriterPtr writer);
    virtual void clearConfig();
    virtual bool sendFrame(const tagpacketlist&, const vector<string>&, uint16_t);
    virtual bool sendFrameRaw(const bytevector& data); // for AMSS
    void makeAFpacket(crcbytevector&, const tagpacketlist&, const vector<string>&, uint16_t);
    void makeFFheader(bytevector& out, size_t packet_size, bool sendTime);
    void makePcapFileHeader(bytevector& out);
    void makePcapPacketHeader(bytevector& out, size_t packet_size);

    PftOut *pft;
    basic_socket *sock;
    server_socket *ssock;
    string m_type, m_target, m_proto;
    bool m_use_crc, m_use_tist, m_file_framing, m_tcp_server;
    unsigned int m_src_addr, m_dst_addr;
};

#endif
