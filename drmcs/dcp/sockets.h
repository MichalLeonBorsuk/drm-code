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

#ifndef _SOCKETS_H
#define _SOCKETS_H

#include <map>
#include <vector>
# include <pcap.h>
typedef int SOCKET;
# define SOCKET_ERROR (-1)
# define INVALID_SOCKET (-1)
#include <netinet/in.h>
#include <arpa/inet.h>


class basic_socket
{
public:

  enum poll_result_t {timeout, data_avail, error };
  int mtu;
  int port;
  SOCKET handle;

  basic_socket();
  basic_socket(const basic_socket&);
  virtual ~basic_socket();
  basic_socket& operator=(const basic_socket& e);
  virtual void ReConfigure(std::map<std::string,std::string>& config);
  virtual void config(std::map<std::string,std::string>& config);
  virtual bool open(){return false;}
  virtual poll_result_t poll();
  virtual void close();
  virtual void send(const std::vector<uint8_t>&, size_t start=0, size_t count=0){}
  void fetch(std::vector<uint8_t>&);
protected:
  const std::string error_message(const std::string&);
};

class stream_socket : public basic_socket
{
public:
  virtual void send(const std::vector<uint8_t>&, size_t start, size_t count);
  void fetch(std::vector<uint8_t>&);
};

class server_socket : public basic_socket
{
public:

  server_socket(){}
  bool open();
  poll_result_t poll();
  bool fetch(stream_socket&);
};

class client_socket : public stream_socket
{
public:
  std::string host;
  client_socket():host(""){}
  void ReConfigure(std::map<std::string,std::string>& config);
  void config(std::map<std::string,std::string>& config);
  bool open();
};

class dgram_socket : public basic_socket
{
public:
  sockaddr_in sock_addr;
  std::string addr, iface;
  int ttl;
  bool join;

  dgram_socket();
  dgram_socket(const dgram_socket&);
  virtual ~dgram_socket();
  dgram_socket& operator=(const dgram_socket& e);

  void ReConfigure(std::map<std::string,std::string>& config);
  void config(std::map<std::string,std::string>& config);
  bool open();
  void send(const std::vector<uint8_t>&, size_t start=0, size_t count=0);
  void fetch(std::vector<uint8_t>&);
};

class file_socket : public basic_socket
{
public:
  std::string path;

  file_socket();
  file_socket(const file_socket& p);
  virtual ~file_socket();
  file_socket& operator=(const file_socket& e);
  void ReConfigure(std::map<std::string,std::string>& config);
  void config(std::map<std::string,std::string>& config);
  bool open();
  void close();
  void send(const std::vector<uint8_t>&, size_t start=0, size_t count=0);
  void fetch(std::vector<uint8_t>&);
protected:
  pcap_dumper_t *fp;
  bool pcap;
};

#endif
