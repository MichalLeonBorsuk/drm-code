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

#include "AMSSConfig.h"
#include <iostream>
#include <sstream>

const char* AMSSConfig::CC_MODES[]={"normal","reserved","AMC -3dB","AMC -6dB","reserved","DAM +3dB","DAM +6dB","reserved", NULL};

AMSSConfig::AMSSConfig():
  Persist(),
  afs_mux_ref(),
  transmission_offset(0.0),
  carrier_control_mode(0),
  send_sdc_time(-1),
  asdi_destination(),
  service(), afs()
{
  tag = "amss_configuration";
}
  
AMSSConfig::AMSSConfig(const AMSSConfig& a):
  Persist(a),
  afs_mux_ref(a.afs_mux_ref),
  transmission_offset(a.transmission_offset),
  carrier_control_mode(a.carrier_control_mode),
  send_sdc_time(-1),
  asdi_destination(a.asdi_destination),
  service(a.service), afs(a.afs)
{
}

AMSSConfig& AMSSConfig::operator=(const AMSSConfig& a)
{
  *reinterpret_cast<Persist *>(this) = Persist(a);
  afs_mux_ref = a.afs_mux_ref;
  transmission_offset = a.transmission_offset;
  carrier_control_mode = a.carrier_control_mode;
  send_sdc_time = a.send_sdc_time;
  asdi_destination = a.asdi_destination;
  service = a.service;
  afs = a.afs;
  return *this;
}

AMSSConfig::~AMSSConfig()
{
}

void AMSSConfig::clearConfig()
{
  misconfiguration = false;
  afs_mux_ref.clear();
  transmission_offset=-1.0;
  send_sdc_time=-1;
  asdi_destination.clear();
  carrier_control_mode = 0;
  service.clear();
  afs.clearConfig();
}

void AMSSConfig::ReConfigure(xmlNodePtr config)
{
  Persist::ReConfigure(config);
  if(misconfiguration)
    return;

// resolve the schedule & region references
  afs.PostReConfigure();
  // resolve the afs_ref references
  // services in other multiplexes
  for(size_t i=0; i<afs.afs_mux_list.size(); i++){
    AFSMuxlist& list = afs.afs_mux_list[i];
    // find the service to which this list applies
    for(size_t j=0; j<service.size(); j++) {
      for(size_t k=0; k<service[j].afs_ref.size(); k++) {
        if(service[j].afs_ref[k] == list.id) {
          list.carried_short_id.push_back(static_cast<uint8_t>(j));
        }
      }
    }
	if(service.size()>list.carried_short_id.size())
      list.restricted_services = true;
    else
      list.restricted_services = false;
    for(size_t l=0; l<afs_mux_ref.size(); l++){
      if(afs_mux_ref[l].ref == list.id) {
        list.sync = afs_mux_ref[l].sync;
        list.base_layer = afs_mux_ref[l].base_layer;
      }
    }
  }
  // other services

  for(size_t i=0; i<afs.afs_service_list.size(); i++){
    AFSServicelist& list = afs.afs_service_list[i];
    // find the service to which this list applies
    for(size_t j=0; j<service.size(); j++) {
      for(size_t k=0; k<service[j].afs_ref.size(); k++) {
        if(service[j].afs_ref[k] == list.id) {
          list.short_id = static_cast<uint8_t>(j);
        }
      }
    }
  }
}

/*
        <general>
         <transmission_offset>2.5</transmission_offset>
         <send_sdc_time>1</send_sdc_time>
         <carrier_control_mode>1</carrier_control_mode>
	</general>
	<services>
	</services>
	<afs>
          <regions>
	  <schedules>
	  <afs_mux_lists>
	  <afs_service_lists>
        </afs>
*/

void AMSSConfig::GetParams(xmlNodePtr n)
{
  if(xmlStrEqual(n->name,(const xmlChar*)"general")) {
    for(xmlNodePtr c=n->children; c; c=c->next){
      if(c->type==XML_ELEMENT_NODE) {
        parseDouble(c, "transmission_offset", &transmission_offset);
        if(!xmlStrcmp(c->name, BAD_CAST "asdi_destinations")) {
          for(xmlNodePtr d=c->children; d; d=d->next){
            if(!xmlStrcmp(d->name, BAD_CAST "asdi_destination")) {
              xmlChar* s = xmlNodeGetContent(d);
              if(s) {
	      cout << "Destination: " << (char*) s << endl;
                asdi_destination.push_back((char*)s);
                xmlFree(s);
              }
            }
          }
        }
        parseBool(c, "send_sdc_time", &send_sdc_time);
	parseUnsigned(c, "carrier_control_mode", &carrier_control_mode);
      }
    }
  }

  if(!xmlStrcmp(n->name, BAD_CAST "services")) {
    for(xmlNodePtr c=n->children; c; c=c->next){
      if(c->type==XML_ELEMENT_NODE) {
        Service s;
        s.ReConfigure(c);
        service.push_back(s);
        misconfiguration |= s.misconfiguration;
      }
    }
  }
  if(!xmlStrcmp(n->name, BAD_CAST "afs")) {
    afs.ReConfigure(n);
    misconfiguration |= afs.misconfiguration;
  }
}

void AMSSConfig::PutParams(xmlTextWriterPtr writer)
{
  xmlTextWriterWriteFormatAttributeNS(writer, 
    BAD_CAST "xsi",
    BAD_CAST "schemaLocation", 
    BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",
    "%s %s",
    "http://www.drm.org/schema/drm",
    "http://217.35.80.115/drm/drm-mux.xsd"
  );
  xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns", 
    BAD_CAST "http://www.drm.org/schema/drm"
  );
  Persist::PutParams(writer);
  xmlTextWriterStartComment(writer);
  xmlTextWriterWriteString(writer, BAD_CAST "$id$");
  xmlTextWriterEndComment(writer);

  xmlTextWriterStartElement(writer, BAD_CAST "general");
  PutDouble(writer, "transmission_offset", transmission_offset);
  if(asdi_destination.size()>0) {
    xmlTextWriterStartElement(writer, BAD_CAST "asdi_destinations");
    for(size_t i=0; i<asdi_destination.size(); i++)
      PutString(writer, "asdi_destination", asdi_destination[i]);
    xmlTextWriterEndElement(writer);
  }
  PutBool(writer, "send_sdc_time", send_sdc_time);
  PutUnsignedEnum(writer, "carrier_control_mode", CC_MODES, carrier_control_mode);

  xmlTextWriterEndElement(writer);

  xmlTextWriterStartElement(writer, BAD_CAST "services");
  for(size_t i=0; i<service.size(); i++)
    service[i].Configuration(writer);
  xmlTextWriterEndElement(writer);

  afs.Configuration(writer);
}

