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

#ifndef _PACKETSCE_H
#define _PACKETSCE_H

#include "packetencoder.h"

class PacketSCE : public ServiceComponentEncoder
{
public:

  PacketSCE():ServiceComponentEncoder(),packet_encoder(),payload_size(0){}
  PacketSCE(const PacketSCE& e):ServiceComponentEncoder(e),packet_encoder(e.packet_encoder),
  payload_size(e.payload_size) {}
  PacketSCE& operator=(const PacketSCE& e)
  {
    *reinterpret_cast<ServiceComponentEncoder*>(this) = e;
    packet_encoder = e.packet_encoder;
    payload_size = e.payload_size;
    return *this;
  }
  virtual ~PacketSCE() {}
  void NextFrame(std::vector<uint8_t>& buf, size_t max, double stoptime=0);
  void ReConfigure(const ServiceComponent&);

  PacketEncoder packet_encoder;
  size_t payload_size;
  std::string encoder_id;
};

#endif
