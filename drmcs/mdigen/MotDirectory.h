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

#ifndef _MOTDIRECTORY_H
#define _MOTDIRECTORY_H

#include "MotObject.h"
#include "crcbytevector.h"
#include <map>

// MOT From EN 301 234 V1.2.1 (1999-02)

class MotDirectory
{
public:

    MotDirectory();
    ~MotDirectory();
    
    void clear()
    {
      extension.clear();
      SegmentSize = 0;
      DataCarouselPeriod = 0;
      objects.clear();
    }

  void add_file(const string& name, size_t size);
  void change_file(const string& name, size_t size);
  void delete_file(const string& name);

  // set field methods
  void setDataCarouselPeriod(uint32_t period);
  void setSegmentSize(uint16_t segmentSize);
  void setSortedHeaderInformation() ;
  void setDefaultPermitOutdatedVersions(uint8_t n);
  void setPriority(uint8_t priority) ;
  void setRetransmissionDistance(uint32_t d) ;
  void setProfileSubset(const bytevector& profiles) ;
  void setDirectoryIndex(uint8_t profile, const string& index);
  void setUniqueBodyVersion(uint32_t v) ;
  void setDefaultExpirationRelative(time_t t);
  void setDefaultExpirationAbsolute(timespec t);
  void setAlwaysSendMimeType(bool);

  // query methods

  // methods to build PDUs
  void put_to(crcbytevector& out);
  void put_compressed_to(crcbytevector& out);
  void putPermitOutdatedVersions(bytevector& out, uint8_t n) const;
  void putExpirationRelative(bytevector& out, time_t t) ;
  void putExpirationAbsolute(bytevector& out, timespec t);

  string next_object();

  /* this storage mechanism ensures that 
     the directory is sorted by content name */
  map<string, MotObject> objects;
  uint16_t next_transport_id;
  uint16_t transport_id;
  bool dirty;

protected:
  bool check_add_file(const string& name, uint32_t size);

  bytevector extension;
  uint16_t SegmentSize;
  uint32_t DataCarouselPeriod;
  bool always_send_mime_type;
};

#endif
