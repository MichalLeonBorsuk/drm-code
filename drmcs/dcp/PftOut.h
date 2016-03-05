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

#ifndef _PFT_H
#define _PFT_H

#include <persist.h>
#include <libxml/xmlwriter.h>
#include <RSCodePFT.h>
#include <string>
#include <map>
#include <vector>
#include <crcbytevector.h>

class PftOut : public Persist
{
public:
  int m_mtu;
  int m_use_address;
  int m_source_address, m_dest_address;
  uint16_t m_sequence_counter;

  PftOut();
  virtual ~PftOut();
  virtual void GetParams(xmlNodePtr n){};
  virtual void ReConfigure(std::map<std::string,std::string>& config);
  virtual void config(std::map<std::string,std::string>& config);
  void makePFTheader(
    crcbytevector &out, size_t in_size,
    uint32_t Findex, uint32_t Fcount, 
    bool fec=false, uint16_t rsK=0, uint16_t rsZ=0);
  int headerLength(bool use_addr, bool use_fec);
  int makePFT(const std::vector<uint8_t>& in, std::vector<uint8_t>& out, 
    size_t header_bytesize, size_t payload_bytesize, uint16_t num_packets,
    bool fec, uint16_t rsK, uint16_t rsZ);
  virtual int makePFT(const std::vector<uint8_t>& afpacket, std::vector<uint8_t>& out, 
                      size_t &packet_bytesize) { return 0; }

protected:
  void clearConfig();
  
};

class SimplePftOut : public PftOut
{
public:
  virtual int makePFT(const std::vector<uint8_t>& afpacket, std::vector<uint8_t>& out, size_t &packet_bytesize);
};

class FecPftOut : public PftOut
{
public:
  FecPftOut():m_expected_packet_losses(0),code() {}
  int m_expected_packet_losses;
  CRSCodePFT code;
  virtual int makePFT(const std::vector<uint8_t>& afpacket, std::vector<uint8_t>& out, size_t &packet_bytesize);
  virtual void ReConfigure(std::map<std::string,std::string>& config);
  virtual void config(std::map<std::string,std::string>& config);
};
#endif
