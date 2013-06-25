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

#ifndef _MOTDIRSCE_H
#define _MOTDIRSCE_H

#include <fstream>
#include "PacketSCE.h"
#include "MOTEncoder.h"

class MOTSCE : public PacketSCE, public Persist
{
public:
  MOTSCE();
  MOTSCE(const MOTSCE&);
  MOTSCE& operator=(const MOTSCE&);
  virtual ~MOTSCE();

  virtual void NextFrame(bytevector& buf, size_t max, double stoptime=0);
  virtual void ReConfigure(const ServiceComponent&);
  void NextFile();

protected:

  MOTEncoder::Flags flags;
  map<uint8_t,string> profile_index;
  MOTEncoder mot_encoder;

  void GetParams(xmlNodePtr n);
  void PutPrivateParams(xmlTextWriterPtr writer);

};
#endif
