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

#ifndef _SDI_H
#define _SDI_H

#include "ServiceComponentEncoder.h"
#include "DcpOut.h" // for tagpacketlist
#include "timestamp.h"

class SdiOut
{
public:

  tagpacketlist frame;
  vector<string> tag_tx_order;
  uint32_t frame_count;
  bool reconfiguration_version;

  SdiOut();
  virtual ~SdiOut();

  virtual void ReConfigure();
  virtual void buildFrame(ServiceComponentEncoder& sce, DrmTime& timestamp);

};
#endif
