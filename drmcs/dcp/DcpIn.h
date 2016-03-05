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

#ifndef _DCPIN_H
#define _DCPIN_H
#include "DcpUtil.h"
#include "PftIn.h"
#include "sockets.h"
#include <vector>


class DcpIn : public Persist
{
public:
    DcpIn();
    DcpIn(const DcpIn&);
    DcpIn& operator=(const DcpIn&);
    virtual ~DcpIn();
    virtual void ReConfigure(const std::string& uri);
    virtual void ReConfigure(std::map<std::string,std::string>& config);
    bool parseUri(std::map<std::string,std::string>& out, const std::string& uri);
    virtual bool setParam(const char *param, const char *value);
    virtual void GetParams(xmlNodePtr n) {}
    virtual void PutParams(xmlTextWriterPtr writer);
    virtual void clearConfig();
    virtual bool getFrame(tagpacketlist&);

protected:
    PftIn pft;
    basic_socket *sock;
    server_socket *ssock;
    std::string m_type, m_target;
    bool m_use_pft, m_use_crc, m_use_tist, m_file_framing, m_tcp_server;
    unsigned int m_src_addr, m_dst_addr;

	bool decodeAF(tagpacketlist& frame, std::vector<uint8_t>& input);


};

#endif
