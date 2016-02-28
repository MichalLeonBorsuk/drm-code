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

#include "udpTextSCE.h"
#include <iostream>
using namespace std;
#include <unistd.h>
#include <fcntl.h>

udpTextSCE::udpTextSCE():CTranslatingTextSCE(),
    message(),socket()
{
}

udpTextSCE::~udpTextSCE()
{
}

udpTextSCE::udpTextSCE(const udpTextSCE& p)
    :CTranslatingTextSCE(p), message(p.message), socket(p.socket)
{
}

udpTextSCE& udpTextSCE::operator=(const udpTextSCE& e)
{
    *reinterpret_cast<CTranslatingTextSCE*>(this) = e;
    message = e.message;
    socket = e.socket;
    return *this;
}

void udpTextSCE::ReConfigure(const ServiceComponent& config)
{
    CTranslatingTextSCE::ReConfigure(config);
    if(socket.handle!=INVALID_SOCKET)
        socket.close();
    map<string,string> params;
    size_t p = config.source_selector.find(':');
    if(p != string::npos)
    {
        string host = config.source_selector.substr(0, p);
        string port = config.source_selector.substr(p+1);
        cout << "udp: " << host << " " << port << endl;
        params["join"] = "true";
        params["addr"] = host;
        params["port"] = port;
    }
    else
    {
        params["port"] = config.source_selector;
    }
    socket.ReConfigure(params);
    socket.open();
    int buffsize = 500;
    (void)setsockopt(socket.handle,SOL_SOCKET,SO_RCVBUF,(char*)&buffsize,sizeof(buffsize));
}

string udpTextSCE::next_message()
{
    if(socket.poll()==dgram_socket::data_avail)
    {
        bytev buffer;
        buffer.resize(2048);
        socket.fetch(buffer);
        message = "";
        for(size_t i=0; i<buffer.size(); i++)
        {
            message += buffer[i];
        }
    }
    cout << message << endl;
    return message;
}
