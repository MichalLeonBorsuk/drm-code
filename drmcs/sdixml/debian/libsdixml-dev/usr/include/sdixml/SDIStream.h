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

#ifndef _SDISTREAM_H
#define _SDISTREAM_H

#include "ServiceComponent.h"

using namespace std;

class Stream : public Persist
{
public:
  Stream();
  Stream(const Stream& s);
  Stream& operator=(const Stream& s);
  virtual ~Stream();
  virtual void clearConfig();
  virtual void ReConfigure(xmlNodePtr config);
  virtual void PostReConfigure();
  virtual void GetParams(xmlNodePtr n);
  virtual void PutParams(xmlTextWriterPtr writer);

  static const char* PROTECTION_LEVELS[];
  static const char* TYPES[];

  int bytes_per_frame;
  int bytes_better_protected;
  int packet_size;
  int error_protection;
  enum StreamType {audio, data_stream, data_packet_mode, unspecified} stream_type;
  vector<ServiceComponent> component;

};
#endif
