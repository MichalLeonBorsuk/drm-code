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

#ifndef _DUFILESCE_H
#define _DUFILESCE_H

#include <fstream>
#include "PacketSCE.h"
#include "DataGroupEncoder.h"
#include "MonitoredFile.h"

class DUFileSCE : public PacketSCE
{
public:
    DUFileSCE():PacketSCE(),file_pos(0),f(),
    packet_queue(),max_queue_depth(20),
    packet_encoder(),dge(),
    data_unit(),next_data_unit()
    {
    }
	virtual void NextFrame(std::vector<uint8_t>& buf, size_t max, double stoptime=0);
    virtual void ReConfigure(const ServiceComponent&);

protected:

    long file_pos;
    MonitoredFile f;
    packetqueue packet_queue;
    size_t max_queue_depth;
    PacketEncoder packet_encoder;
    DataGroupEncoder dge;
    std::vector<bytevector> data_unit;
    std::vector<bytevector>::iterator next_data_unit;
    void read_file(double stoptime);
    void fill(double stoptime);

};
#endif

