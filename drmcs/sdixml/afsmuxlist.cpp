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

#include "afsmuxlist.h"

AFSMuxlist::AFSMuxlist():Persist(),fg(),carried_short_id(),
sync(false),base_layer(true),restricted_services(false)
{
  tag = "afs_multiplex_list";
  misconfiguration = false;
}

AFSMuxlist::AFSMuxlist(const AFSMuxlist& l):Persist(l),
fg(l.fg),carried_short_id(l.carried_short_id),
sync(l.sync),base_layer(l.base_layer),restricted_services(l.restricted_services)
{
}

AFSMuxlist& AFSMuxlist::operator=(const AFSMuxlist& l)
{
  *reinterpret_cast<Persist *>(this) = Persist(l);
  fg = l.fg;
  carried_short_id = l.carried_short_id;
  sync = l.sync;
  base_layer = l.base_layer;
  restricted_services = l.restricted_services;
  return *this;
}

AFSMuxlist::~AFSMuxlist()
{
}

void AFSMuxlist::clearConfig()
{
  fg.clear();
  carried_short_id.clear();
  base_layer=true;
  sync=false;
  restricted_services=false;
  misconfiguration = false;
}

void AFSMuxlist::GetParams(xmlNodePtr n)
{
  misconfiguration = false;
  if(xmlStrEqual(n->name,BAD_CAST "afs_mux_frequency_group")) {
    FrequencyGroup f;
    f.ReConfigure(n);
    fg.push_back(f);
    misconfiguration |= f.misconfiguration;
  }
}

void AFSMuxlist::PutParams(xmlTextWriterPtr writer)
{
  Persist::PutParams(writer);
  if(fg.size()>0) {
    for(size_t i=0; i<fg.size(); i++) {
      fg[i].Configuration(writer);
    }
  }
}
