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

#include "PacketFileSCE.h"
#include <platform.h>
#include <iostream>

void PacketFileSCE::ReConfigure(const ServiceComponent& config)
{
    PacketSCE::ReConfigure(config);
}

void PacketFileSCE::NextFrame(bytevector &out, size_t max, double stoptime)
{
    if(max<(unsigned int)payload_size)
        return;
    if(!current.misconfiguration && open==false) {
        file.open(current.source_selector.c_str(), ios::in|ios::binary);
        if(file.is_open()) {
            open=true;
        } else {
            cerr << "can't open " << current.source_selector << endl;
            cerr.flush();
            return;
        }
    }
    if (open)
    {
        bytevector packet;
        for(size_t i=0; i<max; i+=payload_size) {
            //bool done = file.eof();
            packet.resize(payload_size);
            file.read((char*)packet.data(), payload_size);
            size_t l=file.gcount();
            if(l<payload_size) {
                file.clear();
                file.seekg(0, ios::beg);
                if(l==0)
                    file.read((char*)packet.data(), payload_size);
                else
                    packet.resize(l);
            }
            out.put(packet);
        }
    }
}
