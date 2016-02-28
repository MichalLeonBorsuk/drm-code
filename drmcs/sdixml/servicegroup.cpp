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

#include "servicegroup.h"
#include <iostream>
using namespace std;

const char *ServiceGroup::types[]= {"drm", "am", "amss", "fm-ena", "fm-asia", "dab", NULL};

ServiceGroup::ServiceGroup()
    :FrequencyGroup(),system_type(),
     system_id(),
     same_service(-1),
     service_identifier()
{
    clearConfig();
    tag="afs_service_group";
}

ServiceGroup::ServiceGroup(const ServiceGroup& a)
    :FrequencyGroup(a),system_type(a.system_type),
     system_id(a.system_id),
     same_service(a.same_service),
     service_identifier(a.service_identifier)
{
}

ServiceGroup& ServiceGroup::operator=(const ServiceGroup& a)
{
    *reinterpret_cast<FrequencyGroup *>(this) = FrequencyGroup(a);
    system_type = a.system_type;
    system_id = a.system_id;
    same_service = a.same_service;
    service_identifier =a.service_identifier;
    return *this;
}

void ServiceGroup::clearConfig()
{
    FrequencyGroup::clearConfig();
    system_type = drm;
    service_identifier.clear();
    system_id=255;
    same_service=-1;
    service_identifier.clear();
}

ServiceGroup::~ServiceGroup()
{
    clearConfig();
}
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
void ServiceGroup::GetParams(xmlNodePtr n)
{
    misconfiguration = false;
    FrequencyGroup::GetParams(n);
    int i=-1;
    parseEnum(n, "system_id", &i, types);
    if(i>-1)
        system_type = e_system_type(i);
    parseBool(n, "same_service", &same_service);
    parseHexBinary(n, "afs_service_identifier", service_identifier);
}

/*
00000: DRM service
Other Service Id: 24 bits (DRM service identifier).
00001: reserved for future definition
(AM service with AM service identifier).
00010: AM service
Other Service Id: not present (AM service identifier not specified)
00011: FM-RDS service (Europe and North America grid)
Other Service Id: 24 bits (ECC+PI code).
00100: FM-RDS service (Europe and North America grid)
Other Service Id: 16 bits (PI code only).
00101: FM service (Europe and North America grid)
Other Service Id: not present (PI code not specified).
00110: FM-RDS service (Asia grid)
Other Service Id: 24 bits (ECC+PI code).
00111: FM-RDS service (Asia grid)
Other Service Id: 16 bits (PI code only).
01000: FM service (Asia grid)
Other Service Id: not present (PI code not specified).
01001: DAB service
Other Service Id: 24 bits (ECC + audio service identifier).
01010: DAB service
Other Service Id: 16 bits (audio service identifier only).
01011: DAB service
Other Service Id: 32 bits (data service identifier).
*/


void ServiceGroup::ReConfigure(xmlNodePtr config)
{
    Persist::ReConfigure(config);
    misconfiguration=false;
    switch(system_type) {
    case drm:
        system_id = 0; // DRM
        break;
    case am:
        system_id = 2; // AM
        break;
    case amss:
        system_id = 1; // AMSS
        break;
    case fm_ena: // fm, Europe Grid
    case fm_asia: // fm, /Asia Grid
        switch(service_identifier.size()) {
        case 0:
            system_id = 5; // FM-RDS Europe & NA, No Id
            break;
        case 2:
            system_id = 4; // FM-RDS Europe & NA, PI
            break;
        case 3:
            system_id = 3; // FM-RDS Europe & NA, ECC+PI
            break;
        default:
            misconfiguration = true;
        }
        if(system_type==fm_asia)
            system_id+=3;
        break;
    case dab: // dab
        switch(service_identifier.size()) {
        case 2:
            system_id = 10; // DAB Audio
            break;
        case 3:
            system_id = 9; // DAB Audio with ECC
            break;
        case 4:
            system_id = 11; // DAB Data
        default:
            misconfiguration = true;
        }
        break;
    default:
        misconfiguration=true;
    }
}

void ServiceGroup::PutParams(xmlTextWriterPtr writer)
{
    PutEnum(writer, "system_id", types, system_type);
    if(same_service!=-1)
        PutBool(writer, "same_service", same_service);
    if(service_identifier.size()>0)
        PutHexBinary(writer, "afs_service_identifier", service_identifier);
    FrequencyGroup::PutParams(writer);
}
