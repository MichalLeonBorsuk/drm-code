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

#include "SdcElement.h"
#include <iostream>

void SdcElement::Type0(
    const bytevector& sdci,
    bool next)
{
  type=0;
  out.clear();
  // length of body
  out.put(sdci.size()-1, 7);
  out.put(next?1:0, 1); // version flag = reconfiguration
  out.put(sdci);
}

/*
service may be labelled. The label should be sent in every SDC block to enable fast display, although for data service
the repetition rate can be lowered. This data entity uses the unique mechanism for the version flag. The information is
as follows:
- Short Id 2 bits.
- rfu 2 bits.
- label n bytes.
The following definitions apply:
Short Id: this field contains the short Id that relates the information to the Service Id provided by the FAC.
rfu: these 2 bits are reserved for future use and shall be set to zero until they are defined.
label: this is a variable length field of up to 64 bytes containing character data for up to 16 characters using UTF-8
coding (ISO/IEC 10646-1 [4]).
NOTE: The length of the label (in bytes) is given by the length field of the header.
*/



void SdcElement::Type1(
    const string& label,
    size_t short_id)
{
  size_t len = label.length();
  type=1;
  out.clear();
  out.put(len, 7);
  out.put(0, 1); // version flag = 0 for unique
  out.put(type, 4);
  out.put(short_id, 2);
  out.put(0, 2); // rfu
  out.put(label);
}


/*
This data entity allows the conditional access parameters to be sent. This data entity uses the reconfiguration
mechanism for the version flag.
- Short Id 2 bits.
- Audio CA flag 1 bit.
- Data CA flag 1 bit.
- CA system identifier 8 bits.
- CA system specific information n bytes.
The following definitions apply:
Short Id: this field contains the short Id that relates the information to the Service Id provided by the FAC.
Audio CA flag: this 1-bit flag indicates whether the conditional access parameters refer to an audio stream as follows:
0: Parameters do not refer to an audio stream.
1: Parameters refer to an audio stream.
NOTE 1: In case of a data service this flag shall be 0.
Data CA flag: this 1-bit flag indicates whether the conditional access parameters refer to a data stream/sub-stream as
follows:
0: Parameters do not refer to a data stream/sub-stream.
1: Parameters refer to a data stream/sub-stream.
NOTE 2: In case of an audio service that does not have a data stream/sub-stream this flag shall be 0.
rfu: these 2 bits are reserved for future use and shall be set to zero until they are defined.
CA system identifier: this field indicates the CA system used by this service.
CA system specific information: this is a variable length field containing CA system specific data.
NOTE 3: An audio service can have a scrambled audio stream and a scrambled data stream/sub-stream and the
conditional access parameters can be different for each. In this case two Conditional access parameters
data entity - type 2 are needed. If the audio stream and the data stream/sub-stream use identical
conditional access parameters then one Conditional access parameters data entity - type 2 is sufficient;
both the Audio CA flag and the Data CA flag are set to 1.
*/

void SdcElement::Type2(
    const Service& s, bool next,
	size_t short_id)
{
    type=2;
    out.clear();
	out.put(1+s.ca_data.size(), 7);
    out.put(next?1:0, 1);
    out.put(type, 4);
	out.put(short_id, 2);
	bool is_audio_ca = s.audio_ref.length()>0 
      && s.conditional_access;
	out.put(is_audio_ca, 1);
	bool is_data_ca = s.data_ref.length()>0
      && s.conditional_access;
	out.put(is_data_ca, 1);
    out.put(s.ca_system_identifier, 8);
    out.put(s.ca_data);
}

/*
- Synchronous Multiplex flag 1 bit.
- Layer flag 1 bit.
- Service Restriction flag 1 bit.
- Region/Schedule flag 1 bit.
- Service Restriction field 0 or 8 bits.
- Region/Schedule field 0 or 8 bits.
- n frequencies n -- 16 bits.

  zero to 16 frequencies per entity; zero to 64 entities in total
*/

void SdcElement::Type3(const AFSMuxlist& l, int i, int version)
{
	size_t len=0;
    uint16_t sr_flag=0, rs_flag=0;
    const FrequencyGroup& group = l.fg[i];
    if(group.frequency.size()>16)
		throw "too many frequencies in a type 3 SDC element (16 allowed)";
    if(l.restricted_services) {
      sr_flag=1;
      len++;
    }
    if(group.region_id!=0 || group.schedule_id!=0) {
      len++;
      rs_flag=1;
    }
    len+=2*group.frequency.size();
    type=3;
    out.clear();
    out.put(len, 7);
    out.put(version, 1);
    out.put(type, 4);
    out.put(l.sync?1:0, 1);
    out.put(l.base_layer?0:1, 1);
    out.put(sr_flag, 1);
    out.put(rs_flag, 1);
    if(sr_flag){
	  // bit map of short ids carried in this other service
	  uint8_t service_restriction = 0;
      for(size_t i=0; i<l.carried_short_id.size(); i++) {
        service_restriction |= 1 << l.carried_short_id[i];
      }
      out.put(service_restriction, 4); 
      out.put(0, 4); // rfa
    }
    if(rs_flag) {
      out.put(group.region_id, 4);
      out.put(group.schedule_id, 4);
    }
    for(unsigned i=0; i<group.frequency.size(); i++){
      out.put(group.frequency[i], 16);
    }
}

/*
- Schedule Id 4 bits - all schedules restricting a set of frequencies share the
  same id
- Day Code 7 bits bit 0 = sunday, bit 6 = monday
- Start Time 11 bits
- Duration 14 bits
*/
void SdcElement::Type4(size_t id, const Schedule::Interval& v, int version)
{
    type=4;
    out.clear();
    out.put(4, 7);
    out.put(version, 1);
    out.put(type, 4);
    out.put(id, 4);
	uint8_t days = 0;
	for(size_t i=0; i<v.days.length(); i++) {
	    uint8_t d = v.days[i] - '0'; // Mon=1, Tue=2, ... Sun=7
        days |= 1<<(7-d);
	}
    out.put(days, 7);
    out.put(60*v.start_hour+v.start_minute, 11);
    out.put(v.duration, 14);
}

/*
- Short Id 2 bits.
- Stream Id 2 bits.
- Packet mode indicator 1 bit.
- descriptor 7 bits or 15 bits.
- application data n bytes.
*/
void SdcElement::Type5(
    const ServiceComponent& c,
	size_t short_id,
	size_t stream,
	bool next)
{
  type=5;
  out.clear();
  size_t len = 1+(c.packet_mode?1:0)+c.application_data.size();
  out.put(len, 7);
  out.put(next?1:0, 1); // version flag = reconfiguration
  out.put(type, 4);
  out.put(short_id, 2);
  out.put(stream, 2);
  out.put(c.packet_mode?1:0, 1);
  if(c.packet_mode) {
/*
- data unit indicator 1 bit.
- packet Id 2 bits.
- enhancement flag 1 bit.
- application domain 3 bits.
- packet length 8 bits.
*/
    out.put(c.data_unit_indicator, 1);
    out.put(c.packet_id, 2);
    out.put(c.enhancement, 1);
    out.put(c.application_domain, 3);
    out.put(c.packet_size, 8);
  } else {
  /*- rfa 3 bits.
  - enhancement flag 1 bit.
  - application domain 3 bits.*/
    out.put(0, 3);
    out.put(c.enhancement, 1);
    out.put(c.application_domain, 3);
  }
  out.put(c.application_data);
}

/*
- Short Id flags 4 bits.
- Same Multiplex/Other Service flag 1 bit.
- Short Id/Announcement Id 2 bits.
- rfa 1 bit.
- Announcement support flags 10 bits.
- Announcement switching flags 10 bits.
*/

void SdcElement::Type6(
    const Announcement& a,
	uint8_t short_id_flags, int version)
{
    type=6;
    out.clear();
    out.put(3, 7);
    out.put(version, 1); // list mechanism
    out.put(type, 4);
    out.put(short_id_flags, 4);
    out.put(0, 1);
    out.put(0, 2);
    out.put(0, 1); // rfa
    out.put(a.announcement_types, 10);
}

/*
- Region Id 4 bits.
- Latitude 8 bits.
- Longitude 9 bits.
- Latitude Extent 7 bits.
- Longitude Extent 8 bits.
- n CIRAF Zones n -- 8 bits.
*/
void SdcElement::Type7(size_t id, 
  const Region::Area& a, 
  const vector<int>& zone,
  int version)
{
    type=7;
    out.clear();
    out.put(4+zone.size(), 7);
    out.put(version, 1); // list mechanism
    out.put(type, 4);
    out.put(id, 4);
    out.put(a.latitude, 8);
    out.put(a.longitude, 9);
    out.put(a.latitude_extent, 7);
    out.put(a.longitude_extent, 8);
    for(size_t i=0; i<zone.size(); i++)
      out.put(zone[i], 8);
}

/*
- Modified Julian Date 17 bits.
- UTC (hours and minutes) 11 bits.
UTC: this field specifies the current UTC time expressed in hours (5 bits) and minutes (6 bits).
*/
void SdcElement::Type8(
    uint32_t mjd,
    uint8_t hh,
    uint8_t mm)
{
    type=8;
    out.clear();
    out.put(3, 7);
    out.put(0, 1); // unique mechanism
    out.put(type, 4);
    out.put(mjd, 17);
    out.put(hh, 5);
    out.put(mm, 6);
}

/*
- Short Id 2 bits.
- Stream Id 2 bits.
- audio coding 2 bits.
- SBR flag 1 bit.
- audio mode 2 bits.
- audio sampling rate 3 bits.
- text flag 1 bit.
- enhancement flag 1 bit.
- coder field 5 bits.
- rfa 1 bit.
*/
void SdcElement::Type9(
  const ServiceComponent& c,
  size_t short_id,
  size_t stream,
  bool has_text, bool next
)
{
  type=9;
  out.clear();
  out.put(2, 7);
  out.put(next?1:0, 1); // version flag = reconfiguration
  out.put(type, 4);
  out.put(short_id, 2);
  out.put(stream, 2);
  out.put(c.audio_coding, 2);
  out.put(c.SBR?1:0, 1);
  out.put(c.audio_mode, 2);
  switch(c.audio_sampling_rate) {
  case 8000:
    out.put(0, 3);
    break;
  case 12000:
    out.put(1, 3);
    break;
  case 16000:
    out.put(2, 3);
    break;
  case 24000:
    out.put(3, 3);
    break;
  }
  out.put(has_text?1:0, 1);
  out.put(c.enhancement, 1);
  out.put(c.coder_field, 5);
  out.put(0, 1); // rfu
}

/*
- Base/Enhancement flag 1 bit.
- Robustness mode 2 bits.
- Spectrum occupancy 4 bits.
- Interleaver depth flag 1 bit.
- MSC mode 2 bits.
- SDC mode 1 bit.
- number of service 4 bits.
- rfa 3 bits.
- rfu 2 bits.
*/

void SdcElement::Type10(
    const RFChannel& channel,
    bool enhanced,
    size_t service_pattern)
{
  type=10;
  out.clear();
  out.put(2, 7);
  out.put(1, 1); // version flag = reconfiguration, always 1 for type 10s!
  out.put(type, 4);
  // Channel parameters
  out.put(enhanced?1:0, 1); // base/enhancement flag
  out.put(channel.robustness_mode, 2);
  out.put(channel.spectral_occupancy, 4);
  out.put(channel.interleaver_depth, 1);
  out.put(channel.msc_mode, 2);
  out.put(channel.sdc_mode, 1);
  out.put(service_pattern, 4);
  out.put(0, 3); // rfa
  out.put(0, 2); // rfu
}

/*
- Short Id/Announcement Id flag 1 bit.
- Short Id/Announcement Id field 2 bits.
- Region/Schedule flag 1 bit.
- Same Service flag 1 bit.
- rfa 2 bits.
- System Id 5 bits.
- Region/Schedule field 0 or 8 bits.
- Other Service Id 0 or 16 or 24 or 32 bits.
- n = 0 to 16 frequencies n -- (8 or 16) bits.
*/
static const size_t idbytes[] = {3, 3, 0, 3, 2, 0, 3, 2, 0, 3, 2, 4};

void SdcElement::Type11(
   const ServiceGroup& group,
   bool is_announcement,
   size_t short_id, int version
)
{
  type=11;
  size_t len=1;
  uint16_t rs_flag=0;
  if(group.frequency.size()>16)
    throw "too many frequencies in a type 11 SDC element (16 allowed)";
  if(group.region_id>0 || group.schedule_id>0) {
    len++;
    rs_flag=1;
  }
  if(group.system_id<12)
    len += idbytes[group.system_id];
  else
    throw "invalid system id";
  if(group.service_identifier.size()!=idbytes[group.system_id])
    throw "Wrong size service id for system id in a type 11 SDC element";
  if(group.system_id<3){
    len+=2*group.frequency.size();
  } else {
    len+=group.frequency.size();
  }
  out.clear();
  out.put(len, 7);
  out.put(version, 1); // list mechanism
  out.put(type, 4);
  out.put(is_announcement?1:0, 1);
  out.put(short_id, 2);
  out.put(rs_flag, 1);
  out.put(group.same_service==1?1:0, 1);
  out.put(0, 2); // rfa
  out.put(group.system_id, 5);
  if(rs_flag) {
    out.put(group.region_id, 4);
    out.put(group.schedule_id, 4);
  }
  out.put(group.service_identifier);
  switch(group.system_id){
  case 0: // drm
  case 1: // am with ID
  case 2: // am
       for(size_t i=0; i<group.frequency.size(); i++){
         out.put(0, 1);
         out.put(group.frequency[i], 15);
       }
       break;
  case 3:
  case 4:
  case 5:
       for(size_t i=0; i<group.frequency.size(); i++){
         uint16_t f = (group.frequency[i]-87500)/100;
         out.put(f, 8);
       }
       break;
  case 6:
  case 7:
  case 8:
       for(size_t i=0; i<group.frequency.size(); i++){
         uint16_t f = (group.frequency[i]-76000)/100;
         out.put(f, 8);
       }
       break;
  default:
       for(size_t i=0; i<group.frequency.size(); i++){
         out.put(group.frequency[i], 8);
       }
       break;
  }
}

void SdcElement::Type12(
    const Service& s,
	size_t short_id)
{
  type=12;
  out.clear();
  out.put(5, 7);
  out.put(0, 1); // unique
  out.put(type, 4);
  out.put(short_id, 2);
  out.put(0, 2);
  if(s.language_long.length()==3)
    out.put(s.language_long);
  else
    out.put("---");
  if(s.country.length()==2)
    out.put(s.country);
  else
    out.put("--");
}
