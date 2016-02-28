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

#include "frequencygroup.h"
#include <iostream>
using namespace std;

FrequencyGroup::FrequencyGroup():Persist(),
   region_ref(""), schedule_ref(""),
   region_id(-1), schedule_id(-1),frequency(0)

{
  clearConfig();
  tag="afs_mux_frequency_group";
}

FrequencyGroup::FrequencyGroup(const FrequencyGroup& a)
:Persist(a),
   region_ref(a.region_ref), schedule_ref(a.schedule_ref),
   region_id(a.region_id), schedule_id(a.schedule_id),frequency(a.frequency)
{
}

FrequencyGroup& FrequencyGroup::operator=(const FrequencyGroup& a)
{
  *reinterpret_cast<Persist *>(this) = Persist(a);
   region_ref = a.region_ref;
   schedule_ref = a.schedule_ref;
   region_id = a.region_id;
   schedule_id = a.schedule_id;
   frequency = a.frequency;
  return *this;
}

void FrequencyGroup::clearConfig()
{
  Persist::clearConfig();
   region_ref.clear();
   schedule_ref.clear();
   region_id = -1;
   schedule_id = -1;
   frequency.clear();
}

FrequencyGroup::~FrequencyGroup()
{
  clearConfig();
}

void FrequencyGroup::GetParams(xmlNodePtr n)
{
  misconfiguration = false;
  parseIDREF(n, "region_ref", region_ref);
  parseIDREF(n, "schedule_ref", schedule_ref);
  if(xmlStrEqual(n->name, BAD_CAST "frequencies")) {
    for(xmlNodePtr e=n->children; e; e=e->next){
      if(e->type==XML_ELEMENT_NODE){
        int i=-1;
        parseUnsigned(e, "frequency", &i);
        if(i>0) {
          frequency.push_back(i);
        }
      }
    }
  }
}

void FrequencyGroup::PutParams(xmlTextWriterPtr writer)
{
      if(region_ref.length()>0)
        PutString(writer, "region_ref", region_ref);
      if(schedule_ref.length()>0)
        PutString(writer, "schedule_ref", schedule_ref);
      xmlTextWriterStartElement(writer, BAD_CAST "frequencies");
      for(size_t j=0; j<frequency.size(); j++) {
        PutUnsigned(writer, "frequency", frequency[j]);
      }
      xmlTextWriterEndElement(writer);
}
