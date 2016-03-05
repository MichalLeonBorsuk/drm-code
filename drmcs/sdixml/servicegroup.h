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

#ifndef _SERVICEGROUP_H
#define _SERVICEGROUP_H

#include "frequencygroup.h"
#include <vector>

class ServiceGroup : public FrequencyGroup
{
public:

  ServiceGroup();
  ServiceGroup(const ServiceGroup& a);
  ServiceGroup& operator=(const ServiceGroup& a);
  virtual ~ServiceGroup();
  virtual void ReConfigure(xmlNodePtr config);
  virtual void clearConfig();
  
  virtual void GetParams(xmlNodePtr n);
  virtual void PutParams(xmlTextWriterPtr writer);
  void parse_frequency_group(xmlNodePtr n,
           const string& group, const string& item);
/*
  <afs_service_group>
   <system_id>dab</system_id>
    <same_service>1</same_service>
    <afs_service_identifier>E1C238</afs_service_identifier>
	<frequencies>
	<frequency>93</frequency>
	</frequencies>
      <region_ref>uk</region_ref>
   </afs_service_group>
*/
  enum e_system_type { drm, am, amss, fm_ena, fm_asia, dab} system_type;
  static const char* types[];
  int system_id;
  int same_service;
  std::vector<uint8_t> service_identifier;
};
#endif
