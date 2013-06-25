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

#ifndef _FAC_H
#define _FAC_H

#include "bytevector.h"
#include <RFChannel.h>
#include <Service.h>
#include <DrmMuxConfig.h>

class FAC
{

public:

  static const char* MUXPLAN[];
  vector<int> audio, data;
  const char* mux_plan;
  unsigned int rep, short_id, service_pattern;
  vector<Service> service;
  RFChannel channel;

  FAC();
  virtual ~FAC();

  void ReConfigure(const DrmMuxConfig&, uint8_t);
  void NextFrame(bytevector&, uint8_t, bool, uint8_t);
    
};


#endif
