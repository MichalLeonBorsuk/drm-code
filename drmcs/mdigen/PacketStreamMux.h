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

#ifndef _PACKETSTREAMMUX_H
#define _PACKETSTREAMMUX_H

#include "StreamMux.h"
#include "PacketSCE.h"

class PacketStreamMux:public StreamMux
{
public:

  PacketStreamMux();
  PacketStreamMux(const PacketStreamMux&);
  PacketStreamMux& operator=(const PacketStreamMux&);
  virtual ~PacketStreamMux();
  void ReConfigure(const Stream&);
  void NextFrame(bytevector&);
  Stream::StreamType Class() { return Stream::data_packet_mode; }

protected:
  size_t last_packet_component;
  PacketSCE dummy_sce;
  uint32_t tokens_per_frame[4];
  uint32_t tokens[4];
  uint32_t tokens_per_packet;
};
#endif
