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

#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <cctype>
#include "TcpDataSCE.h"

using namespace std;

CTcpDataSCE::CTcpDataSCE()
{
}

void CTcpDataSCE::clearConfig()
{
    sock.close();
}

void CTcpDataSCE::ReConfigure(const ServiceComponent& config)
{
    ServiceComponentEncoder::ReConfigure(config);
    map<string, string> mapconfig;
    int parms;
    char host[255], port[255];
    const char *p=current.source_selector.c_str();
    while(*p==' ') p++;
    parms = sscanf(p, "%[^:]:%s", host, port);
    if(parms==2) {
        mapconfig["host"]=host;
        mapconfig["port"]=port;
        sock.ReConfigure(mapconfig);
    } else {
        throw "TCPDATASCE error";
    }
}

void CTcpDataSCE::NextFrame(vector<uint8_t>& buf, size_t max, double)
{
    if(sock.handle==INVALID_SOCKET)
        sock.open();
    if(sock.poll()) {
        vector<uint8_t> b;
        sock.fetch(b);
        if(b.size()<=max)
            buf.insert(buf.end(), b.begin(), b.end());
    }
}
