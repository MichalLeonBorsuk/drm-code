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

#include "timestamp.h"
#include "DcpIn.h"
#include "Crc16.h"
#include <cstring>
#include <iostream>

DcpIn::DcpIn():pft(),sock(NULL),m_use_pft(false),m_use_crc(false), m_use_tist(false),m_file_framing(false)
{
    tag="destination";
    clearConfig();
    m_type = "xxx";
}

void DcpIn::clearConfig()
{
  m_target = "";
  m_src_addr = 0;
  m_dst_addr = 0;
  m_file_framing=false;
  m_use_pft=false;
  m_use_tist=true;
  m_use_crc=true;
  m_tcp_server = false;
  if(sock) {
    delete sock;
    sock=NULL;
  }
}

DcpIn::~DcpIn()
{
    clearConfig();
}

void DcpIn::ReConfigure(const string& uri)
{
  map<string,string> config;
  config.clear();
  parseDcpUri(config, uri);
  ReConfigure(config);
}


void DcpIn::ReConfigure(map<string,string>& config)
{
  clearConfig();
  for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++){
    setParam(i->first.c_str(), i->second.c_str());
  }
  if(config.count("crc")>0){
    const string & s = config["crc"];
    if(s=="0" || s == "false" || s=="f")
      m_use_crc = false;
  }
  config["addr"] = string(&config["target"][2]);
  config["port"]=config["dst_addr"];
  if(m_type=="udp") {
    sock = new dgram_socket();
  } else if(m_type=="tcp") {
    if(config.count("end")>0){
      string& end = config["end"];
      if(end == "server") {
        ssock = new server_socket();
        m_tcp_server = true;
      } else {
        sock = new client_socket();
      }
    } else {
      sock = new client_socket();
    }
  } else if(m_type=="file") {
    config["path"]=config["addr"];
    sock = new file_socket();
  } else if(m_type=="ser") {
    // not done yet
  }
  if(sock) {
    sock->ReConfigure(config);
    misconfiguration = false;
  } else if(m_tcp_server) {
    ssock->ReConfigure(config);
    misconfiguration = false;
  }
}

bool DcpIn::setParam(const char *param, const char *value)
{
  if(strcmp(param, "type")==0) {
    m_type = value;
  }
  if(strcmp(param, "target")==0) {
    m_target = value;
  }
  if(strcmp(param, "src_addr")==0) {
    m_src_addr = atoi(value);
  }
  if(strcmp(param, "dst_addr")==0) {
    m_dst_addr = atoi(value);
  }
  if(strcmp(param, "file_framing")==0) {
      m_file_framing = false;
      const string s = value;
      if(s=="1" || s == "true" || s=="t")
        m_file_framing = true;
  }
  return true;
}

void DcpIn::PutParams(xmlTextWriterPtr writer)
{
  Persist::PutParams(writer);
  // scheme
  xmlTextWriterWriteString(writer, BAD_CAST "dcp.");
  xmlTextWriterWriteString(writer, BAD_CAST m_type.c_str());
  if(m_use_pft)
    xmlTextWriterWriteString(writer, BAD_CAST ".pft");
  // :
  xmlTextWriterWriteString(writer,BAD_CAST  ":");
  // target
  xmlTextWriterWriteString(writer,BAD_CAST  m_target.c_str());
  // optional src/dst addr
  if(m_dst_addr != 0) {
    xmlTextWriterWriteString(writer,BAD_CAST  ":");
    if(m_src_addr != 0) {
      xmlTextWriterWriteFormatString(writer, "%u:", m_src_addr);
    }
    xmlTextWriterWriteFormatString(writer, "%u", m_dst_addr);
  }
  // optional query
  if(m_use_pft) {
    char sep[2]="?";
    map<string,string> config;
    pft.config(config);
    for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++){
      const char *param=i->first.c_str(), *value=i->second.c_str();
      xmlTextWriterWriteFormatString(writer, "%s%s=%s", sep, param, value);
      sep[0]='&';
    }
  }
}

bool DcpIn::getFrame(tagpacketlist& frame)
{
  if(sock) {
    if(sock && sock->handle==INVALID_SOCKET) {
      sock->open();
      if(sock->handle==INVALID_SOCKET)
        throw "can't open socket";
    }
  } else {
    if(m_tcp_server) {
      if(ssock) {
        if(ssock->handle==INVALID_SOCKET) {
          ssock->open();
        }
        if(ssock->poll()) {
          stream_socket *s = new stream_socket;
          bool ok = ssock->fetch(*s);
          if(ok)
            sock = s;
          else
            throw "can't open socket";
        } else {
          throw "can't open socket";
        }
      } else {
        throw "can't open socket";
      }
    } else {
      throw "can't open socket";
    }
  }
  crcbytevector b;
  b.resize(2048);
  try {
    sock->fetch(b);
  } catch(...) {
  }
  if(b.size()==0)
    return false;
  if(b[0]=='P') {
    crcbytevector out;
    if(pft.decodePFT(out, b))
	  return decodeAF(frame, out);
	return false;
  }
  return decodeAF(frame, b);
}

bool DcpIn::decodeAF(tagpacketlist& frame, crcbytevector& data)
{
  size_t p=0;
  char s0 = data[p++];
  char s1 = data[p++];
  if(s0!='A' || s1!='F')
    return false;
  uint32_t bits = ntohl(*(uint32_t*)&data[p]);
  p+=4;
  uint16_t af_seq = ntohs(*(uint16_t*)&data[p]);
  p+=2;
  uint8_t flags = data[p++];
  char pt = data[p++];
  if(pt!='T')
    return false;
  uint32_t bytes = bits/8;
  while(p<data.size()-2) {
    string tag;
	tag += data[p++];
	tag += data[p++];
	tag += data[p++];
	tag += data[p++];
    uint32_t bits = ntohl(*(uint32_t*)&data[p]);
    p+=4;
	bytevector b;
	b.resize(bits/8);
	for(bytev::iterator i=b.begin(); i!=b.end(); i++)
	  *i = data[p++];
	frame[tag] = b;
	cout << tag << " " << b.size() << endl;
  }
  return true;
}
