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

#include "sockets.h"

#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <sstream>
#include "timestamp.h"
#ifndef WIN32
#include <unistd.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#endif

/*
#ifdef WIN32
extern "C" {
int install_bpf_program(pcap_t *, struct bpf_program *){ return 0;}
void pcap_freecode(struct bpf_program *) {}
char* PacketGetVersion() { return "3.0"; }
};
#endif
*/

basic_socket::basic_socket():mtu(0),port(0),handle(INVALID_SOCKET)
{
}

basic_socket::basic_socket(const basic_socket& s):mtu(s.mtu),port(s.port),handle(INVALID_SOCKET)
{
}

basic_socket::~basic_socket()
{
  close();
}

basic_socket& basic_socket::operator=(const basic_socket& s)
{
  mtu = s.mtu;
  port = s.port;
  handle = INVALID_SOCKET;
  return *this;
}

const string basic_socket::error_message(const string& where)
{
#ifdef WIN32
  stringstream s;
  s << where << " " << WSAGetLastError();
  return s.str();
#else
  return where + strerror(errno);
#endif
}

void basic_socket::ReConfigure(map<string,string>& config)
{
  for(map<string,string>::iterator i = config.begin(); i!=config.end(); i++){
    string param=i->first, value=i->second;
    if(param=="port") {
      port = atoi(value.c_str());
    }
    if(param=="mtu") {
      mtu = atoi(value.c_str());
    }
  }
}

void basic_socket::config(map<string,string>& config)
{
  if(mtu>0) {
    stringstream s;
    s << mtu;
    config["mtu"]= s.str();
  }
  if(port>0) {
    stringstream s;
    s << port;
    config["port"]= s.str();
  }
}

basic_socket::poll_result_t basic_socket::poll()
{
    struct timeval timeout_struct;
    fd_set read_fds, write_fds, except_fds;
    int rv;
    timeout_struct.tv_sec = 0;
    timeout_struct.tv_usec = 1;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    if(handle!=INVALID_SOCKET) {
        FD_SET(handle, &except_fds);
        FD_SET(handle, &read_fds);
    }
    rv = select( FD_SETSIZE, &read_fds, &write_fds, &except_fds, &timeout_struct);
    switch(rv) {
    case SOCKET_ERROR:
		cerr << error_message("select") << endl;
        cerr.flush();
        break;
    case 0:
  		//cout << "socket poll " << handle << " timeout " << endl;
		return timeout;
        break;
    default:
      if(handle!=INVALID_SOCKET && FD_ISSET(handle, &read_fds)) {
  		//cout << "socket poll " << handle << " data avail " << endl;
        return data_avail;
      }
      if(handle!=INVALID_SOCKET && FD_ISSET(handle, &except_fds)) {
  		//cout << "socket poll " << handle << " error " << strerror(error) << endl;
        return error;
      }
    }
    return error;
}

void basic_socket::close()
{
#ifdef WIN32
  closesocket(handle);
#else
  ::close(handle);
#endif
  handle=INVALID_SOCKET;
}

void stream_socket::send(const bytev&, size_t start, size_t count)
{

}

void stream_socket::fetch(bytev&)
{
}

bool server_socket::open()
{
	struct  sockaddr_in sin;
	SOCKET s;
	memset(&sin, 0, sizeof(sin));
	s = socket(AF_INET, SOCK_STREAM, 0);
	handle = socket(AF_INET, SOCK_STREAM, 0);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	if(bind(s, (struct sockaddr*)&sin, sizeof(sin))==-1) {
		cerr << error_message("bind") << endl;
        cerr.flush();
		return false;
	}
#ifdef WIN32
#else
    fcntl(s, F_SETFL, O_NONBLOCK);
#endif
   	listen(s, 5); // length of queue for pending
   	handle = s;
   	return true;
}

/*
 * determine if an accept would provide a new connection
*/
basic_socket::poll_result_t server_socket::poll()
{
     return basic_socket::poll();
}

bool server_socket::fetch(stream_socket& sock)
{
	struct  sockaddr_in clnt;
#ifdef WIN32
	int clen=sizeof(clnt);
#else
	socklen_t clen=sizeof(clnt);
#endif
    SOCKET s;
    int n,optval = 1;
    struct protoent *pe;
	s=accept(handle, (struct sockaddr*)&clnt, &clen);
	if(handle==INVALID_SOCKET){
		cerr << error_message("accept") << endl;
        cerr.flush();
		return false;
	}
    n=sizeof(optval);
    pe=getprotobyname("tcp");
    if(pe==NULL) {
	  cerr << "getproto error" << endl;
      cerr.flush();
      return false;
    }
    setsockopt (handle, pe->p_proto, TCP_NODELAY, (char*)&optval, n);
	sock.handle = s;
	return true;
}

void client_socket::ReConfigure(map<string,string>& config)
{
  basic_socket::ReConfigure(config);
  for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++){
    string param=i->first, value=i->second;
    if(param=="host") {
      host = value;
    }
  }
}

void client_socket::config(map<string,string>& config)
{
  basic_socket::config(config);
  config["host"]=host;
}

bool client_socket::open()
{
    SOCKET s;
	int n,optval;
    struct sockaddr_in server;
    struct hostent *h;
    char *haddr;
    int h_length;
    unsigned long addr;
    struct protoent *pe;
    handle = INVALID_SOCKET;
    if (isalpha(host[0])) {
      h = gethostbyname (host.c_str());
      if (h == NULL) {
	      cerr << "Invalid hostname" << endl;
	      errno = 0;
	      return false;
      }
      haddr = h->h_addr;
      h_length = 4;
    }
    else {
        if(host=="")
            addr = inet_addr("127.0.0.1");
        else
            addr = inet_addr (host.c_str());
        haddr = (char *) &addr;
        h_length = 4;
    }
    s=socket(AF_INET, SOCK_STREAM, 0);
    if(s<0){
      return false;
    }
    memset ((char *)&server, 0, sizeof server);
    memcpy ((char *)&server.sin_addr, haddr, h_length);
    server.sin_family= AF_INET;
    server.sin_port= htons (port);
    optval = 1;
    n=sizeof(optval);
    pe=getprotobyname("tcp");
    if(pe==NULL) {
      cerr << "getproto error" << endl;
	  return false;
    }
#ifdef WIN32
#else
    setsockopt (s, SOL_SOCKET, O_NONBLOCK, (char*)&optval, sizeof (optval));
#endif
    if (::connect(s,(struct sockaddr*)&server, sizeof(struct sockaddr_in))!=0){
	  ::close(s);
	  cerr << "can't connect to host " << host << endl;
	  return false;
    }
    setsockopt (s, pe->p_proto, TCP_NODELAY, (char*)&optval, n);
    setsockopt (s, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, sizeof (optval));
    handle = s;
    return true;
}

dgram_socket::dgram_socket():basic_socket(),ttl(127),join(false)
{
}

dgram_socket::dgram_socket(const dgram_socket& s):basic_socket(s),ttl(s.ttl),join(s.join)
{
}

dgram_socket::~dgram_socket()
{
 close();
}

dgram_socket& dgram_socket::operator=(const dgram_socket& s)
{
  *reinterpret_cast<basic_socket*>(this) = s;
  ttl = s.ttl;
  join = s.join;
  return *this;
}

void dgram_socket::ReConfigure(map<string,string>& config)
{
  basic_socket::ReConfigure(config);
  addr = "";
  iface = "";
  join = false;
  for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++){
    string param=i->first, value=i->second;
    cout << "dgram: " << param << "=" << value << endl;
    if(param=="addr") {
      addr = value;
    }
    if(param=="interface") {
      iface = value;
    }
    if(param=="ttl") {
      ttl = atoi(value.c_str());
    }
    if(param=="join") {
      join = false;
      if(value=="t") join=true;
      if(value=="true") join=true;
      if(value=="1") join=true;
    }
  }
}

void dgram_socket::config(map<string,string>& config)
{
  basic_socket::config(config);
  config["addr"]=addr;
  if(iface!="")
    config["interface"]=iface;
  if(ttl>0) {
    stringstream s;
    s << ttl;
    config["ttl"]= s.str();
  }
  if(join)
    config["join"]="1";
}

bool dgram_socket::open()
{
  handle = socket(AF_INET, SOCK_DGRAM, 0);
  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = htons(port);
  if(handle == INVALID_SOCKET)
    throw string("can't create dgram socket");
  // allow use for ordinary incoming sockets
  cout << "open dgram port " << addr << ":" << port << endl;
  try
  {
  if(addr=="") {
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(handle,(struct sockaddr *)&sock_addr, sizeof(sock_addr))==SOCKET_ERROR)
      throw string("dgram bind ")+strerror(errno);
  } else {
    sock_addr.sin_addr.s_addr = inet_addr(addr.c_str());
    if(sock_addr.sin_addr.s_addr==INADDR_NONE)
      throw string("dgram: bad address ")+addr;
  }
  // multicast ?
  if ((ntohl (sock_addr.sin_addr.s_addr) & 0xe0000000) == 0xe0000000) {
    if(setsockopt(handle, IPPROTO_IP, IP_MULTICAST_TTL,
             (char*) &ttl, sizeof(ttl))==SOCKET_ERROR)
    {
      throw error_message("m/cast ttl");
    }
    in_addr_t mc_if = INADDR_ANY;
    if(iface!="") {
      mc_if = inet_addr(iface.c_str());
      if(setsockopt(handle, IPPROTO_IP, IP_MULTICAST_IF,
             (char*) &mc_if, sizeof(mc_if))==SOCKET_ERROR)
        throw string("dgram: bad interface address ")+addr;
    }
    if(join) {
      struct ip_mreq mreq;
      mreq.imr_multiaddr.s_addr = sock_addr.sin_addr.s_addr;
      mreq.imr_interface.s_addr = mc_if;

      if(setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq,
		sizeof(mreq)) == SOCKET_ERROR)
        throw error_message("dgram mcast join");
	cerr << "joined" << endl;
    }
  }
  } catch(string e)
  {
    close();
    throw e;
  }
  return false;
}

void dgram_socket::send(const bytev& packet, size_t start, size_t count)
{
  int n = sendto(handle, (char*)packet.data()+start, static_cast<int>((count==0)?packet.size():count), 0,
		(sockaddr*) &sock_addr, sizeof(sockaddr_in));
  //cerr << "dgram_socket::send wanted " << count << " sent " << n << endl;
}

void dgram_socket::fetch(bytev& packet)
{
  socklen_t l = sizeof(sockaddr_in);
  if(&packet[0] != packet.data())
    throw "BUG BUG BUG in dgram_socket::fetch";
  ssize_t n = recvfrom(handle, reinterpret_cast<char*>(&packet[0]),
                         static_cast<int>(packet.size()), MSG_TRUNC,
		(sockaddr*) &sock_addr, &l);
  if(n==SOCKET_ERROR)
    throw error_message("dgram recvfrom");
  // won't happen on windows
  if(size_t(n)>packet.size())
    throw "packet truncated on input";
  packet.resize(n);
}

file_socket::file_socket():basic_socket(),path(),fp(NULL),pcap(false)
{
}

file_socket::file_socket(const file_socket& p):basic_socket(p),path(p.path),fp(NULL),pcap(p.pcap)
{
}

file_socket::~file_socket()
{
  close();
}

file_socket& file_socket::operator=(const file_socket& s)
{
  *reinterpret_cast<basic_socket*>(this) = s;
  path = s.path;
  fp = NULL;
  pcap = s.pcap;
  return *this;
}

void file_socket::ReConfigure(map<string,string>& config)
{
  basic_socket::ReConfigure(config);
  for(map<string,string>::iterator i=config.begin(); i!=config.end(); i++){
    string param=i->first, value=i->second;
    if(param=="path") {
      path = value;
    }
  }
  if(config.count("file_framing")>0){
    const string & s = config["file_framing"];
    if(s=="pcap")
	  pcap=true;
  }
}

void file_socket::config(map<string,string>& config)
{
  basic_socket::config(config);
  config["path"]=path;
  if(pcap)
    config["file_framing"]="pcap";
}

bool file_socket::open()
{
  if(pcap)
  {
#ifndef WIN32
    pcap_t *p = pcap_open_dead(DLT_RAW, 65536);
	fp = pcap_dump_open(p, path.c_str());
#endif
	if(fp) {
	 handle=1;
	 return true;
    } else {
      return false;
    }
  } else {
    int h = ::open(path.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 00660);
    if(h==-1) {
      cerr << "file socket open: " << strerror(errno) << endl;
      cerr.flush();
      return false;
    }
    handle = SOCKET(h);
    return true;
  }
}

void file_socket::close()
{
  if(pcap)
  {
#ifndef WIN32
    pcap_dump_close(fp);
#endif
  }
  else
  {
    ::close(handle);
  }
  handle=INVALID_SOCKET;
}

void file_socket::send(const bytev& packet, size_t start, size_t count)
{
  char *p = (char*)packet.data()+start;
  size_t n = (count==0)?packet.size():count;
  if(pcap)
  {
    bytevector out;
	int c = n+20+8;
    // Ip header - fields in network byte order
    out.put(0x4500, 16);
    out.put(c, 16);
    out.put(0x08d8, 16);
    out.put(0x4000, 16);
    out.put(0x7f, 8);
    out.put(0x11, 8); // udp
    out.put(0x0000, 16);
    out.put(0x0000, 32);
    out.put(0x0000, 32);
    // udp header - fields in network byte order
    out.put(0, 16);
    out.put(9999, 16);
    out.put(8+n, 16);
    out.put(01, 16);
	out.put(packet);
#ifndef WIN32
    pcap_pkthdr hdr;
    timespec tp;
    clock_getrealtime(&tp);
    hdr.ts.tv_sec = tp.tv_sec;
    hdr.ts.tv_usec = tp.tv_nsec/1000UL;
	hdr.caplen = c;
	hdr.len = c;
    pcap_dump((u_char*)fp, &hdr, (u_char*)out.data()+start);
    pcap_dump_flush(fp);
#endif
  } else {
    ::write(handle, p, n);
#ifdef WIN32
    ::_commit(handle);
#else
    ::fsync(handle);
#endif
  }
}

void file_socket::fetch(bytev&)
{
}
