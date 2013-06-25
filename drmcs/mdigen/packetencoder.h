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

#ifndef _PACKETENCODER_H
#define _PACKETENCODER_H

#include <queue>
#include <crcbytevector.h>
#include <ServiceComponentEncoder.h>

typedef queue<bytevector> packetqueue;

class PacketEncoder
{
public:
  uint8_t ci;
  uint16_t packet_id, packet_size;
    
  PacketEncoder():ci(0),packet_id(0),packet_size(0) {}
  PacketEncoder(uint16_t id, uint16_t size):
       ci(0),packet_id(id),packet_size(size) {}
  PacketEncoder(const PacketEncoder& e):
       ci(e.ci),packet_id(e.packet_id),packet_size(e.packet_size) {}
  PacketEncoder& operator=(const PacketEncoder& e)
  {
    ci = e.ci; packet_id = e.packet_id; packet_size = e.packet_size;
    return *this;
  }
  void ReConfigure(const ServiceComponent& config);
  void makeDataUnit(packetqueue& out, bytevector& in);
  void makePacket(crcbytevector& packet, bytevector& in);
  void makePacket(crcbytevector& packet, 
             bytevector::iterator& from, bytevector::iterator& to, bool, bool);
};

#endif
