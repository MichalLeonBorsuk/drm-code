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

#ifndef _DATAGROUPENCODER_H
#define _DATAGROUPENCODER_H

#include <bytevector.h>

// Data Groups from ETSI EN 300 401 V1.3.3 (2001-05)

class DataGroupEncoder
{
public:
  DataGroupEncoder():use_crc(false),segment(true),use_tid(true),user_address() {}

  void Configure(bool crc, bool seg, bool tid, const bytevector& ua);
  void Configure(bool crc, bool seg, bool tid);

  void putDataGroupSegment(bytevector& out, uint16_t transport_id, const bytevector& in,
                 uint8_t type, uint8_t cont, uint16_t segment, bool last) const;

  void putDataGroup(bytevector& out, uint16_t transport_id, const bytevector& in,
                 uint8_t type, uint8_t cont, uint16_t segment_num, bool last) const;

  void putDataGroup(uint8_t type, bytevector& out, const bytevector& in, uint8_t cont);

  void putDataGroupHeader(bytevector& out, uint8_t type, uint8_t cont,
                 uint8_t rep, bool x=false, uint16_t ef=0) const;

  void putSessionHeader(bytevector& out, uint16_t transport_id,
                 uint16_t segment_num, bool last) const;

  void putSegmentationHeader(bytevector& out, uint16_t rep, size_t size) const;

private:

  bool use_crc, segment, use_tid;
  bytevector user_address;
};

#endif
