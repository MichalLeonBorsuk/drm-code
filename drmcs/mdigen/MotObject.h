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

#ifndef _MOTOBJECT_H
#define _MOTOBJECT_H

#include "DataGroupEncoder.h"
#include "map"

// MOT From EN 301 234 V2.1.1 (2005-07) 

class MotObject  
{
public:

  MotObject();
  MotObject(const MotObject& o);
  MotObject& operator=(const MotObject& o);

  virtual ~MotObject();

  // set field methods
  void setHeaders(const string& f);
  void setMimeType(const string& mt);
  void setContentName(const string& content_name);
  void setCompressionType(uint8_t compression_type);
  void setProfileSubset(const bytevector& profiles);
  void setScopeStart(time_t pscope_start, int8_t local_time_offset=0);
  void setScopeEnd(time_t pscope_end, int8_t local_time_offset=0);
  void setScopeId(uint32_t pscope_id);

  // methods to build PDUs
  void putHeader(bytevector& out) const;
  static void putExtensionParameterHeader(bytevector& out, 
                              uint8_t id, size_t len);
  static void putHeaderCore(bytevector& out,
     size_t body_size, size_t hdr_size, 
     uint16_t content_type, uint16_t content_subtype);
  /* put a string, if the optional prefix is present it gets sent as
   * a one octet prefix, e.g. character set in ContentName
   * if absent or negative no prefix is sent, e.g. for MimeType
   * if the string is too long, it gets truncated.
   */
  static void putStringParameter(bytevector& out, uint8_t param, 
                          const string& s, int prefix=-1);
  static void putDateTime(bytevector& out, uint32_t mjd, uint8_t hours, 
              uint8_t minutes, uint8_t seconds, int8_t lto);

  uint32_t file_size;
  uint8_t object_version;
  string mime_type;
  uint8_t content_type;
  uint16_t content_subtype;
  uint16_t transport_id;
  string content_name, file_name;
  uint8_t compression_type;
  bytevector profiles, scope_start, scope_end;
  int scope_id;
  bool compressed;
  bool always_send_mime_type;

protected:

  void find_type_by_mime(
    uint8_t& type, uint16_t& subtype,
    const string& major, const string& minor) const;
  void find_mime_by_ext(
    string& major, string& minor, const string& content_name) const;

};

#endif
