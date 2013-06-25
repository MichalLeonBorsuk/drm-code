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

#include <platform.h>
#include "drmmux.h"
#include "mdiout.h"
#include "DcpOut.h"
#include "Cfg.h"
#include <map>

class DcpDestinations
{
public:
  DcpDestinations();
  virtual ~DcpDestinations();
  void add(const string& uri);
  void remove(const string& uri);
  bool exists(const string& uri);
  void sendFrame(const tagpacketlist&, const vector<string>&, uint16_t);

protected:
  map<string,DcpOut*> dests;
};

class Mdigen
{
public:

  Mdigen():sdc_initted(false),timestamp(),mux_config(),
  mux(),mdi(),transmitted_frames(0),utco(0),tnow_usec(0),
  dests(),af_seq(rand()),initial_reconfiguration_index(7) {}
  virtual ~Mdigen();
 
  void ReConfigure(const string& sdi_file);
  virtual void eachframe();
  virtual void eachminute();
  bool sdc_initted;
  DrmTime timestamp;
  DrmMuxConfig mux_config;
  DrmMux mux;
  MdiOut mdi;
  int transmitted_frames, utco;
  uint64_t tnow_usec;
  DcpDestinations dests;
  uint16_t af_seq;
  uint8_t initial_reconfiguration_index;
};
