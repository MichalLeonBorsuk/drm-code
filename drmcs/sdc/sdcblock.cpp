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

#include "sdcblock.h"
#include <iostream>
using namespace std;

bool SdcBlock::NextFrame(crcbytevector &out, uint8_t afs_index)
{
  if(sent()) {
    if(more_queued()) { // this has been sent and a different one is required
      if(change_allowed) {
        getNext();
        change_allowed = true;
      } else { // this has been sent, and is sent again, but next one will be different
        change_allowed = true;
      }
    } else { // this has been sent and we can go back to the static data
      transmit_buffer = static_data;
      change_allowed = false;
    }
  } else {
    if(more_queued()) { // this has not been sent and will only be sent once
      getNext();
      change_allowed = true;
    } else { // no dynamic blocks available, send the static data at least twice
      transmit_buffer = static_data;
      change_allowed = false;
    }
  }
  return !change_allowed;
}

void SdcBlock::build_sdc(crcbytevector &out, uint8_t afs_index, uint16_t sdc_length)
{
  out.crc.reset();
  out.put(0, 4); // rfu for MDI byte alignment
  out.put(afs_index, 4);
  out.put(transmit_buffer);
  // pad out with zeroes. The CRC and AFS_Index byte are not included in the length
  for(size_t i=out.size(); i<sdc_length+1U; i++) {
    out.put(0, 8);
  }
  out.put(out.crc.result(), 16);
}
