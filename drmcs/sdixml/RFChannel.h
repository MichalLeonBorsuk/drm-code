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

#ifndef _MODULATORCONFIG_H
#define _MODULATORCONFIG_H

#include "persist.h"

class RFChannel : public Persist
{
public:

  RFChannel();
  RFChannel(const RFChannel&);
  RFChannel& operator=(const RFChannel&);
  virtual ~RFChannel();
  virtual void ReConfigure(xmlNodePtr config);
  virtual void clearConfig();
  virtual void GetParams(xmlNodePtr n);
  virtual void PutParams(xmlTextWriterPtr writer);


  static const char* INTERLEAVINGS[];
  static const char* ROBMODES[];
  static const char* VSPPS[];
  static const char* PROT[];
  static const char* SDC_MODES[];
  static const char* MSC_MODES[];
  
  int protection_a, protection_b;
  int VSPP;
  int spectral_occupancy;
  int interleaver_depth;
  int msc_mode;
  int sdc_mode;
  int robustness_mode;

  int max_spp;
  int max_vspp;
  int max_bitrate;


protected:
  void parse_msc_protection_level(xmlNodePtr n);

};
#endif
