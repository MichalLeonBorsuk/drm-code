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

#ifndef _FREQUENCYGROUP_H
#define _FREQUENCYGROUP_H

#include "persist.h"

using namespace std;

class FrequencyGroup : public Persist
{
public:

  FrequencyGroup();
  FrequencyGroup(const FrequencyGroup& a);
  FrequencyGroup& operator=(const FrequencyGroup& a);
  virtual ~FrequencyGroup();
  virtual void clearConfig();
  
  virtual void GetParams(xmlNodePtr n);
  virtual void PutParams(xmlTextWriterPtr writer);
/*
    <afs_mux_frequency_group>
      <region_ref>nw_europe</region_ref>
      <schedule_ref>B06orf</schedule_ref>
      <frequencies>
       <frequency>1296</frequency>
      </frequencies>
	</afs_mux_frequency_group>

*/
  
  string region_ref, schedule_ref;
  int region_id, schedule_id;
  vector<uint32_t> frequency;
};
#endif
