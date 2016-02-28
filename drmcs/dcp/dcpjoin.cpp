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

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <iostream>
#include <sstream>
#include "sockets.h"
#include "timestamp.h"

using namespace std;

const string error_message(const string& where)
{
    return where + strerror(errno);
}

int main(int argc, char** argv)
{
    bool join = true;
    sockaddr_in sock_addr;
    string addr = argv[1];
    int port = atoi(argv[2]);
    string iface = argv[3];
    int ttl = 127;
    int handle = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    if(handle == INVALID_SOCKET)
        throw string("can't create dgram socket");
    // allow use for ordinary incoming sockets
    cout << "open dgram port " << addr << ":" << port << endl;
    try
    {
        if(addr=="") {
            sock_addr.sin_addr.s_addr = INADDR_ANY;
            if(bind(handle,(struct sockaddr *)&sock_addr, sizeof(sock_addr))==SOCKET_ERROR)
                throw string("dgram bind ")+strerror(errno);
        } else {
            sock_addr.sin_addr.s_addr = inet_addr(addr.c_str());
            if(sock_addr.sin_addr.s_addr==INADDR_NONE)
                throw string("dgram: bad address ")+addr;
        }
        // multicast ?
        if ((ntohl (sock_addr.sin_addr.s_addr) & 0xe0000000) == 0xe0000000) {
            if(setsockopt(handle, IPPROTO_IP, IP_MULTICAST_TTL,
                          (char*) &ttl, sizeof(ttl))==SOCKET_ERROR)
            {
                throw error_message("m/cast ttl");
            }
            in_addr_t mc_if = INADDR_ANY;
            if(iface!="") {
                mc_if = inet_addr(iface.c_str());
                if(setsockopt(handle, IPPROTO_IP, IP_MULTICAST_IF,
                              (char*) &mc_if, sizeof(mc_if))==SOCKET_ERROR)
                    throw string("dgram: bad interface address ")+addr;
                cerr << "bound " << iface << endl;
            }
            if(join) {
                struct ip_mreq mreq;
                mreq.imr_multiaddr.s_addr = sock_addr.sin_addr.s_addr;
                mreq.imr_interface.s_addr = mc_if;

                if(setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq,
                              sizeof(mreq)) == SOCKET_ERROR)
                    throw error_message("dgram mcast join");
                cerr << "joined" << endl;
            }
        }
    } catch(string e)
    {
        close(handle);
        throw e;
    }
    while(true)
    {
        sleep(86400);
    }
    return 0;
}
