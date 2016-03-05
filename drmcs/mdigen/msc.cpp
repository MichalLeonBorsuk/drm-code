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

#include "msc.h"
#include <iostream>
#include "TDMStreamMux.h"
#include "PacketStreamMux.h"
using namespace std;

MSC::MSC()
{
    for(size_t i=0; i<stream.size(); i++)
        stream[i] = NULL;
}

MSC::MSC(const MSC& m)
{
    for(size_t i=0; i<m.stream.size(); i++) {
        stream.push_back(m.stream[i]);
    }
}
MSC& MSC::operator=(const MSC&)
{
    return *this;
}

MSC::~MSC()
{
    for(size_t i=0; i<stream.size(); i++)
        delete stream[i];
}

void MSC::ReConfigure(const vector<Stream>& config)
{
    cout << "MSC::ReConfigure: " << stream.size() << endl;
    cout.flush();
    // delete any streams no longer in use
    for(size_t i=config.size(); i<stream.size(); i++)
        delete stream[i];
    if(stream.size()>config.size())
        stream.resize(config.size());
    // make new streams
    for(size_t i=stream.size(); i<config.size(); i++) {
        if(config[i].stream_type == Stream::data_packet_mode)
            stream.push_back(new PacketStreamMux());
        else
            stream.push_back(new TDMStreamMux());
    }
    // reconfigure streams
    for(size_t i=0; i<stream.size(); i++) {
        if((stream[i]->Class() == Stream::data_packet_mode) && (config[i].stream_type != Stream::data_packet_mode)) {
            delete stream[i];
            stream[i] = new TDMStreamMux();
        }
        if((stream[i]->Class() != Stream::data_packet_mode) && (config[i].stream_type == Stream::data_packet_mode)) {
            delete stream[i];
            stream[i] = new PacketStreamMux();
        }
        stream[i]->ReConfigure(config[i]);
    }
}

void MSC::NextFrame(vector<vector<uint8_t> >& out)
{
    out.resize(stream.size());
    for(size_t i=0; i<stream.size(); i++)
        stream[i]->NextFrame(out[i]);
}
