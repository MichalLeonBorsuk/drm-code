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

#include "MOTSCE.h"
#include <platform.h>
#include <iostream>
#include "MOTEncoder.h"
#include <sys/stat.h>
#ifdef WIN32
# include <io.h>
# include <direct.h>
#endif

MOTSCE::MOTSCE():PacketSCE(),flags(),profile_index(), mot_encoder()
{
}

MOTSCE::MOTSCE(const MOTSCE& e):PacketSCE(e),
  flags(e.flags),
  profile_index(e.profile_index),
  mot_encoder(e.mot_encoder)
{
}

MOTSCE& MOTSCE::operator=(const MOTSCE& e)
{
  *reinterpret_cast<PacketSCE *>(this) = PacketSCE(e);
  flags = e.flags;
  profile_index = e.profile_index;
  mot_encoder = e.mot_encoder;
  return *this;
}

MOTSCE::~MOTSCE()
{
}

/*  MOTSCE state consist of:
x    PacketSCE,
xx  packetqueue packet_queue;
x  MOTEncoder mot_encoder;
x  ifstream in_file;
x  enum {open,no_files} state;
x  string out_dir,;
xx  string current_file;
xx  int packet_counter;
x  bool interrupt;
*/

void MOTSCE::ReConfigure(const ServiceComponent& config)
{
  PacketSCE::ReConfigure(config);
  cout << "MOT::ReConfigure arg packet size " << config.packet_size << endl; cout.flush();
  //cout << "MOT::ReConfigure current packet size " << current.packet_size << endl; cout.flush();
  uint16_t uat = 0;
  flags.directory_mode = true;
  flags.always_send_mime_type = true;
  flags.crc = false;
  flags.send_compressed_dir = false;
  flags.send_uncompressed_dir = true;
  profile_index.clear();
  if( (current.application_domain == 1) && current.application_data.size()>1) {
    int msb = current.application_data[0];
    int lsb = current.application_data[1];
    uat = (msb << 8) + lsb;
  }
  xmlDocPtr private_config_doc = 
            xmlParseMemory(
			        current.private_config.c_str(), 
                    current.private_config.length()
		    );
  if(private_config_doc != NULL) {
    GetParams(private_config_doc->children);
    xmlFreeDoc(private_config_doc);
  }
  vector<string> compressible;
  mot_encoder.ReConfigure(
    current.encoder_id,
    current.source_selector,
    current.packet_size, // block size
    current.packet_id, // packet id
    current.packet_size, // packet size
    flags,
    compressible,
    profile_index
  );
}

void MOTSCE::NextFrame(bytevector &out, size_t max, double stoptime)
{
  if(max>=payload_size) {
	mot_encoder.next_packet(out, max, stoptime);
  }
}

void MOTSCE::GetParams(xmlNodePtr n)
{
  if(xmlStrEqual(n->name, BAD_CAST "private")) {
    for(xmlNodePtr c=n->children; c; c=c->next){
      if(c->type==XML_ELEMENT_NODE) {
		int n;
		n=-1;
		parseBool(c, "directory_mode", &n);
		if(n!=-1) flags.directory_mode = (n==0)?false:true;
		n=-1;
		parseBool(c, "always_send_mime_type", &n);
		if(n!=-1) flags.always_send_mime_type = (n==0)?false:true;
		n=-1;
		parseBool(c, "use_crc", &n);
		if(n!=-1) flags.crc = (n==0)?false:true;
		n=-1;
		parseBool(c, "send_compressed_directory", &n);
		if(n!=-1) flags.send_compressed_dir = (n==0)?false:true;
		n=-1;
		parseBool(c, "send_uncompressed_directory", &n);
		if(n!=-1) flags.send_uncompressed_dir = (n==0)?false:true;
        if(xmlStrEqual(c->name, BAD_CAST "profiles")){
          for(xmlNodePtr d=c->children; d; d=d->next){
            if(d->type==XML_ELEMENT_NODE) {
              if(!xmlStrcmp(d->name, BAD_CAST "profile")){
			    int profile;
                string index;
                for(xmlNodePtr e=d->children; e; e=e->next){
                  if(e->type==XML_ELEMENT_NODE) {
                    if(xmlStrEqual(e->name, BAD_CAST "id")){
                      xmlChar *f = xmlNodeGetContent(e);
                      profile = atoi((char*)f);
                      xmlFree(f);
                    }
                    if(xmlStrEqual(e->name, BAD_CAST "index")){
                      xmlChar *f = xmlNodeGetContent(e);
                      index = (char*)f;
                      xmlFree(f);
                    }
                  }
                }
                if(profile>0 && index.length()>0)
                  profile_index[profile] = index;
              }
            }
          }
        }
      }
    }
  }
}

void MOTSCE::PutPrivateParams(xmlTextWriterPtr writer)
{
  //PutParams(writer);
  //xmlTextWriterWriteString(writer, BAD_CAST "dcp.");
}
