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

#include <cstring>
//#include <cstdio>
#include <ctime>
//#include <fcntl.h>
#include <cctype>
//#include <iostream>
#include <sstream>

#include "TickerTextSCE.h"

#define ETIKOK (0)
#define ETIKNOMEM (-2)
#define ETIKBADNAME (-3)
#define ETIKNOTFOUND (-4)

/* DAB/DRM end of header character */
#define EOH (0x0b)

CTickerTextSCE::CTickerTextSCE() :
  CTranslatingTextSCE(),
  hostname(),
  port(),
  host(0),
  timer(0),
  need_clear(false),
  state(init),
  ticker(),
  socket_config()
{
}

CTickerTextSCE::CTickerTextSCE(const CTickerTextSCE& t)
:
  CTranslatingTextSCE(t),
  hostname(t.hostname),
  port(t.port),
  host(t.host),
  timer(t.timer),
  need_clear(t.need_clear),
  state(t.state),
  ticker(t.ticker),
  socket_config(t.socket_config)
{
}

CTickerTextSCE& CTickerTextSCE::operator=(const CTickerTextSCE& t)
{
  *(CTranslatingTextSCE*)this = t;
  hostname = t.hostname;
  port = t.port;
  host = t.host;
  timer = t.timer;
  need_clear = t.need_clear;
  state = t.state;
  ticker = t.ticker;
  socket_config = t.socket_config;
  return *this;
}

string CTickerTextSCE::next_message()
{
  check_source();
  switch(ticker.texts.size()){
  case 0:
    return "";
    break;
  case 1:
    return ticker.texts.front();
    break;
  default:
    ticker.texts.pop();
    return ticker.texts.front();
  }
}

void CTickerTextSCE::parse_source_selector(const string& source_selector)
{
  if(source_selector != current.source_selector) {
    ticker.socket.close();
    state=init;
  }
  int parms;
  char h0[255];
  char h1[255];
  char chan[255];
  int p0, p1;
  const char *p=source_selector.c_str();
  while(*p==' ') p++;
  parms = sscanf(p, "%[^:]:%d %[^:]:%d %[ -~]", 
    h0, &p0, h1, &p1, chan);
  if(parms==5) {
    hostname.resize(2);
    hostname[0]=h0;
    hostname[1]=h1;
    port.resize(2);
    port[0]=p0;
    port[1]=p1;
  } else {
    parms = sscanf(p, "%[^:]:%d %[ -~]", h0, &p0, chan);
    if(parms==3) {
      hostname.resize(1);
      hostname[0]=h0;
      port.resize(1);
      port[0]=p0;
    } else {
      throw string("mis-configuration in TickerText: ") + source_selector;
    }
  }
  ticker.channel_name = chan;
  //cout << hostname[0] << ", " << port[0] << ", " << chan << endl; cout.flush();
}

void CTickerTextSCE::ReConfigure(const ServiceComponent& config)
{
  cout << "CTickerTextSCE::ReConfigure " << config.source_selector << endl; cout.flush();
  parse_source_selector(config.source_selector);
  CTranslatingTextSCE::ReConfigure(config);
  host = 0;
  cerr << "Ticker: " << ticker.channel_name << endl;
}

void CTickerTextSCE::clearConfig()
{
  CTranslatingTextSCE::clearConfig();
  //if(ticker.socket!=INVALID_SOCKET) {
  //  ticker.close_socket();
  //}
  ticker.socket.close();
  hostname.clear();
  port.clear();
  state = init;
  ticker.channel = 0;
  host=0;
}

void CTickerTextSCE::init_source()
{
  if(hostname.size()>1) {
    if(!open()) {
      host=1-host;
      if(!open()) {
        host=1-host;
        return;
      }
    }
  } else {
    if(!open())
      return;
  }
  state=ready;
  ticker.channel=0;
  time(&timer);
}

bool CTickerTextSCE::open()
{
  socket_config["host"] = hostname[host];
  stringstream s;
  s << port[host];
  socket_config["port"]= s.str();
  ticker.socket.ReConfigure(socket_config);
  if(ticker.socket.open()) {
    cerr << "Ticker Opened Connection to Server "
      << hostname[host] << ":" << port[host] << endl;
    return true;
  } else {
    cerr << "can't connect to "
		 << hostname[host] << ":" << port[host]
#ifdef WIN32
         << " error: " << WSAGetLastError()
#else
		 << " error: " << strerror(errno)
#endif
         << endl;
    return false;
  }
}

void CTickerTextSCE::check_source()
{
  switch(state) {
  case init:
    init_source();
    break;
  case ready:
    switch(ticker.socket.poll()) {
    case client_socket::timeout: // also happens on initialisation
      if(ticker.channel==0) {
        cerr << "Ticker timeout" << endl;
        if((time(NULL)-timer)>10L) {
          cerr << "Ticker waited too long for channel list" << endl;
          ticker.socket.close();
        }
      }
      break;
    case client_socket::data_avail:
      time(&timer);
      ticker.check_for_input(ticker.socket.handle);
      break;
    case client_socket::error:
      ticker.socket.close();
    }
  }
  if(ticker.socket.handle==INVALID_SOCKET){
    state=init;
    init_source();
  }
}

void CTickerTextSCE::CMyTicker::added(const char *name, int id)
{
  if(channel_name == name) {
    channel = id;
    request_channels(socket.handle, 1, &channel);
    cout << "ticker host has added our channel" << endl;
  }
  cout << "ticker host has added channel " << id << " for service " << name << endl;
}

void CTickerTextSCE::CMyTicker::deleted(int id)
{
  if(id == channel) {
    cout << "ticker host has deleted channel " << id
	    << " for service " << channel_name << endl;
  }
}

void CTickerTextSCE::CMyTicker::changed(int id, const char *text)
{
  string s(text);
  //cout << s << endl; cout.flush();
  size_t len = s.length();
  for(unsigned i=0; i<len; i++){
    if(s[i]=='^')
      s[i]=EOH;
  }
  // remove newline character
  if(s[len-1] == '\n') {
      s.erase(len-1, 1);
  }
  texts.push(s);
}

void CTickerTextSCE::CMyTicker::closed()
{
  cerr << "socket closed" << endl;
  socket.close();
}
