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

#include "afsservicelist.h"

AFSServicelist::AFSServicelist():Persist(),short_id(4),sg()
{
  tag = "afs_service_list";
  misconfiguration = false;
}

AFSServicelist::AFSServicelist(const AFSServicelist& l)
:Persist(l),short_id(l.short_id),sg(l.sg)
{
}

AFSServicelist::~AFSServicelist()
{
}

AFSServicelist& AFSServicelist::operator=(const AFSServicelist& l)
{
  sg = l.sg;
  short_id = l.short_id;
  return *this;
}

void AFSServicelist::clearConfig()
{
  sg.clear();
  misconfiguration = false;
}

void AFSServicelist::GetParams(xmlNodePtr n)
{
  misconfiguration = false;
  if(xmlStrEqual(n->name,BAD_CAST "afs_service_group")) {
    ServiceGroup s;
    s.ReConfigure(n);
    sg.push_back(s);
    misconfiguration |= s.misconfiguration;
  }
}

void AFSServicelist::PutParams(xmlTextWriterPtr writer)
{
  Persist::PutParams(writer);
  if(sg.size()>0) {
    for(size_t i=0; i<sg.size(); i++) {
      sg[i].Configuration(writer);
    }
  }
}

