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

#ifndef _DRMMUXCONFIG_H
#define _DRMMUXCONFIG_H

#include "RFChannel.h"
#include "SDIStream.h"
#include "Service.h"
#include "afs.h"

using namespace std;

class DrmMuxConfig : public Persist
{
public:
  DrmMuxConfig();
  DrmMuxConfig(const DrmMuxConfig& a);
  virtual ~DrmMuxConfig();
  DrmMuxConfig& operator=(const DrmMuxConfig& a);

  virtual void clearConfig();
  virtual void ReConfigure(xmlNodePtr config);
  virtual void GetParams(xmlNodePtr config);
  virtual void PutParams(xmlTextWriterPtr writer);

  vector< string > mdi_destination;

  struct Ref { Ref():ref(),sync(true),base_layer(true){} string ref; bool sync, base_layer; };
  vector< Ref > afs_mux_ref;
  bool have_sync_base_layer_refs;
  bool have_sync_enhancement_layer_refs;
  bool have_async_base_layer_refs;
  string info;
  double transmission_offset;
  int send_sdc_time;
  RFChannel channel;
  vector<Stream> stream;
  vector<Service> service;
  AFS afs;

//protected:
  bool parseMuxRef(xmlNodePtr n,
	 const string& name, vector<Ref>& v, bool sync, bool base_layer);
  bool findStream(const string& ref, size_t&) const;
  bool findSubstream(const string& ref, size_t&, size_t&) const;

};
#endif
