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

#ifndef _PFTIN_H
#define _PFTIN_H

#include <persist.h>
#include <libxml/xmlwriter.h>
#include <RSCodePFT.h>
#include <string>
#include <map>
#include <vector>
#include "Reassemble.h"

class PftIn : public Persist
{
public:
  int m_use_address;
  int m_source_address, m_dest_address;

  PftIn();
  virtual ~PftIn();
  virtual void GetParams(xmlNodePtr n){};
  virtual void ReConfigure(std::map<std::string,std::string>& config);
  virtual void config(std::map<std::string,std::string>& config);
  bool decodePFT(std::vector<uint8_t>& out, const std::vector<uint8_t>& data);

protected:
  std::map<int,CReassembler> mapFragments;
  CRSCodePFT code;

  void clearConfig();
  bool decodeSimplePFT(std::vector<uint8_t>& out, const std::vector<uint8_t>& data, uint16_t Pseq, uint16_t Plen, uint32_t Findex, uint32_t Fcount);
  bool decodePFTWithFEC(std::vector<uint8_t>& out, const std::vector<uint8_t>& data, uint16_t Pseq, uint16_t Plen, uint32_t Findex, uint32_t Fcount, uint16_t rsK, uint16_t rsZ);
  void deinterleave(const std::vector<uint8_t>& input, std::vector<uint8_t>& output, uint16_t plen, uint32_t fcount);
  bool rsCorrectData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output, uint32_t c_max, uint16_t rsk, uint16_t rsz);
};

#endif
