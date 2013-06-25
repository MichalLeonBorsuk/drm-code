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

#include "DrmMuxConfig.h"
#include <iostream>
#include <sstream>

DrmMuxConfig::DrmMuxConfig():
  Persist(),mdi_destination(),
  afs_mux_ref(),
  have_sync_base_layer_refs(false),
  have_sync_enhancement_layer_refs(false),
  have_async_base_layer_refs(false),
  info(), transmission_offset(0.0),
  send_sdc_time(-1), channel(),stream(),service(),afs()
{
  tag = "drm_multiplex_configuration";
}
  
DrmMuxConfig::DrmMuxConfig(const DrmMuxConfig& a):
  Persist(a),
  mdi_destination(a.mdi_destination),
  afs_mux_ref(a.afs_mux_ref),
  have_sync_base_layer_refs(a.have_sync_base_layer_refs),
  have_sync_enhancement_layer_refs(a.have_sync_enhancement_layer_refs),
  have_async_base_layer_refs(a.have_async_base_layer_refs),
  info(a.info),
  transmission_offset(a.transmission_offset),send_sdc_time(a.send_sdc_time),
  channel(a.channel),stream(a.stream),service(a.service),afs(a.afs)
{
}

DrmMuxConfig& DrmMuxConfig::operator=(const DrmMuxConfig& a)
{
  *reinterpret_cast<Persist *>(this) = Persist(a);
  mdi_destination = a.mdi_destination;
  afs_mux_ref = a.afs_mux_ref;
  have_sync_base_layer_refs = a.have_sync_base_layer_refs;
  have_sync_enhancement_layer_refs = a.have_sync_enhancement_layer_refs;
  have_async_base_layer_refs = a.have_async_base_layer_refs;
  info = a.info;
  transmission_offset = a.transmission_offset;
  send_sdc_time = a.send_sdc_time;
  channel = a.channel;
  stream = a.stream;
  service = a.service;
  afs = a.afs;
  return *this;
}

DrmMuxConfig::~DrmMuxConfig()
{
}

void DrmMuxConfig::clearConfig()
{
  misconfiguration = false;
  afs_mux_ref.clear();
  have_sync_base_layer_refs=false;
  have_sync_enhancement_layer_refs=false;
  have_async_base_layer_refs=false;
  info.clear();
  transmission_offset=-1.0;
  send_sdc_time=-1;
  channel.clearConfig();
  stream.clear();
  mdi_destination.clear();
  service.clear();
  afs.clearConfig();
}

void DrmMuxConfig::ReConfigure(xmlNodePtr config)
{
  Persist::ReConfigure(config);
  if(misconfiguration)
    return;
  // Main Service Channel
  int msc_bytes_per_frame = channel.max_spp+channel.max_vspp;
  int msc_bytes_used = 0;
  for(size_t i=0; i<stream.size(); i++) {
    msc_bytes_used += stream[i].bytes_per_frame;
  }
  if(msc_bytes_used>msc_bytes_per_frame) {
    cerr << "MSC Capacity exceeded! " 
      << msc_bytes_per_frame << " available and " 
      << msc_bytes_used << " requested" << endl;
    misconfiguration = true;
  } else {
    cerr << "MSC Capacity OK " 
      << msc_bytes_per_frame << " available and " 
      << msc_bytes_used << " requested" << endl;
  }
  cerr.flush();
  for(size_t i=0; i<stream.size(); i++) {
    int bytes = stream[i].bytes_per_frame;
    if(bytes==0) // unspecified - use all remaining - max 1 stream gets anything!
    {
      bytes = msc_bytes_per_frame - msc_bytes_used;
      msc_bytes_used = msc_bytes_per_frame;
      stream[i].bytes_per_frame = bytes;
    }
    stream[i].PostReConfigure();
  }
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
    for(size_t j=0; j<afs_mux_ref.size(); j++){
      if(afs_mux_ref[j].ref == list.id) {
        list.sync = afs_mux_ref[j].sync;
        list.base_layer = afs_mux_ref[j].base_layer;
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
    <mux_general>
	  <transmission_offset>2.5</transmission_offset>
	  <send_sdc_time>1</send_sdc_time>
	  <info>ddd</info>
	</mux_general>
	<channel>
	<streams>
	<services>
	<afs>
  	  <regions>
	  <schedules>
	  <afs_mux_lists>
	  <afs_service_lists>
     <afs>
*/

bool DrmMuxConfig::parseMuxRef(xmlNodePtr n, 
	 const string& name, vector<Ref>& v, bool sync, bool base_layer)
{
  bool are_some = false;
  if(!xmlStrcmp(n->name, BAD_CAST (name+"s").c_str())) {
    for(xmlNodePtr c=n->children; c; c=c->next){
      if(!xmlStrcmp(c->name, BAD_CAST name.c_str())) {
  	    Ref r;
  	    xmlChar* s = xmlNodeGetContent(c);
  	    if(s) {
  		  r.ref = (char*)s;
  		  r.sync = sync;
  		  r.base_layer = base_layer;
  		  v.push_back(r);
  		  xmlFree(s);
  		  are_some = true;
  	    }
  	  }
    }
  }
  return are_some;
}

void DrmMuxConfig::GetParams(xmlNodePtr n)
{
  if(xmlStrEqual(n->name,(const xmlChar*)"mux_general")) {
    for(xmlNodePtr c=n->children; c; c=c->next){
      if(c->type==XML_ELEMENT_NODE) {
        parseDouble(c, "transmission_offset", &transmission_offset);
        if(!xmlStrcmp(c->name, BAD_CAST "mdi_destinations")) {
          for(xmlNodePtr d=c->children; d; d=d->next){
            if(!xmlStrcmp(d->name, BAD_CAST "mdi_destination")) {
              xmlChar* s = xmlNodeGetContent(d);
              if(s) {
                mdi_destination.push_back((char*)s);
                xmlFree(s);
              }
            }
          }
        }
        parseBool(c, "send_sdc_time", &send_sdc_time);
		have_sync_base_layer_refs = parseMuxRef(c, "sync_base_layer_afs_ref", 
								  afs_mux_ref, true, true);
		have_async_base_layer_refs =  parseMuxRef(c, "async_base_layer_afs_ref",
								   afs_mux_ref, false, true);
		have_sync_enhancement_layer_refs = parseMuxRef(c, 
	        "sync_enhancement_layer_afs_ref", afs_mux_ref, true, false);
        if(!xmlStrcmp(c->name, BAD_CAST "info")) {
          xmlChar* s = xmlNodeGetContent(c);
          if(s) {
            info = (char*)s;
            xmlFree(s);
          }
        }
      }
    }
  }
  if(!xmlStrcmp(n->name, BAD_CAST "channel")) {
    channel.ReConfigure(n);
    misconfiguration |= channel.misconfiguration;
  }
  if(xmlStrEqual(n->name, BAD_CAST "streams")) {
    for(xmlNodePtr c=n->children; c; c=c->next){
      if(c->type==XML_ELEMENT_NODE) {
        Stream s;
        s.ReConfigure(c);
        stream.push_back(s);
        misconfiguration |= s.misconfiguration;
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

void DrmMuxConfig::PutParams(xmlTextWriterPtr writer)
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
  xmlTextWriterStartElement(writer, BAD_CAST "mux_general");
  PutDouble(writer, "transmission_offset", transmission_offset);
  if(mdi_destination.size()>0) {
    xmlTextWriterStartElement(writer, BAD_CAST "mdi_destinations");
    for(size_t i=0; i<mdi_destination.size(); i++)
      PutString(writer, "mdi_destination", mdi_destination[i]);
    xmlTextWriterEndElement(writer);
  }
  PutBool(writer, "send_sdc_time", send_sdc_time);
  if(have_sync_base_layer_refs) {
    xmlTextWriterStartElement(writer, BAD_CAST "sync_base_layer_afs_refs");
    for(size_t i=0; i<afs_mux_ref.size(); i++) {
	  if(afs_mux_ref[i].sync==true && afs_mux_ref[i].base_layer==true)
        PutString(writer, "mdi_destination", afs_mux_ref[i].ref);
    }
    xmlTextWriterEndElement(writer);
  }
  if(have_sync_enhancement_layer_refs) {
    xmlTextWriterStartElement(writer, BAD_CAST "sync_base_layer_afs_refs");
    for(size_t i=0; i<afs_mux_ref.size(); i++) {
	  if(afs_mux_ref[i].sync==true && afs_mux_ref[i].base_layer==false)
        PutString(writer, "mdi_destination", afs_mux_ref[i].ref);
    }
    xmlTextWriterEndElement(writer);
  }
  if(have_async_base_layer_refs) {
    xmlTextWriterStartElement(writer, BAD_CAST "sync_base_layer_afs_refs");
    for(size_t i=0; i<afs_mux_ref.size(); i++) {
	  if(afs_mux_ref[i].sync==false && afs_mux_ref[i].base_layer==true)
        PutString(writer, "mdi_destination", afs_mux_ref[i].ref);
    }
    xmlTextWriterEndElement(writer);
  }
  if(info.length()>0)
    PutString(writer, "info", info);
  xmlTextWriterEndElement(writer);
  channel.Configuration(writer);
  xmlTextWriterStartElement(writer, BAD_CAST "streams");
  for(size_t i=0; i<stream.size(); i++)
    stream[i].Configuration(writer);
  xmlTextWriterEndElement(writer);
  xmlTextWriterStartElement(writer, BAD_CAST "services");
  for(size_t i=0; i<service.size(); i++)
    service[i].Configuration(writer);
  xmlTextWriterEndElement(writer);
  afs.Configuration(writer);
}

// find the stream whose ID is the ref
bool DrmMuxConfig::findStream(const string& ref, size_t& out) const
{
  for(size_t i=0; i<stream.size(); i++) {
    if(stream[i].id == ref) {
      out = i;
      return true;
    }
  }
  return false;
}

bool DrmMuxConfig::findSubstream(
       const string& ref, size_t& maj, size_t& min) const
{
  for(size_t i=0; i<stream.size(); i++) {
    if(stream[i].stream_type==Stream::data_packet_mode) {
      for(size_t j=0; j<stream[i].component.size(); j++) {
        if(stream[i].component[j].id == ref) {
          maj = i;
          min = j;
          return true;
        }
      }
    }
  }
  return false;
}
