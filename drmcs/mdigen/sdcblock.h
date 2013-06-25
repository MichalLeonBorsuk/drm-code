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

#ifndef _SDCBLOCK_H
#define _SDCBLOCK_H

#include <crcbytevector.h>
#include <queue>

class SdcBlock
{
public:

  SdcBlock():transmit_buffer(),static_data(),requested(),change_allowed(true) {}
  SdcBlock(const bytevector& data):
    transmit_buffer(),requested(),change_allowed(true)
  { sendByDefault(data); }
  virtual ~SdcBlock() {}
  void sendByDefault(const bytevector& data)
  { 
    static_data.clear(); static_data.put(data);
  }
  void sendOnce(const bytevector& data) { requested.push(data); }
  bool NextFrame(crcbytevector &out, uint8_t afs_index);
  void build_sdc(crcbytevector &out, uint8_t afs_index, uint16_t sdc_length);

protected:

  bool sent() { return !transmit_buffer.empty(); }
  bool more_queued() { return !requested.empty(); }
  void getNext() { transmit_buffer = requested.front(); requested.pop(); }

  bytevector transmit_buffer, static_data;
  queue<bytevector> requested;
  bool change_allowed;
};

#endif
