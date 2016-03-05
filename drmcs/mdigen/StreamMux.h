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

#ifndef _STREAMMUX_H
#define _STREAMMUX_H

#include <SDIStream.h>
#include "BBCSceFactory.h"

class StreamMux
{
public:

  StreamMux();
  StreamMux(const StreamMux&);
  StreamMux& operator=(const StreamMux&);
  virtual ~StreamMux();
  virtual void ReConfigure(const Stream&);
  virtual void NextFrame(std::vector<uint8_t>&);
  Stream::StreamType Class() { return Stream::unspecified; }

protected:

  vector<ServiceComponentEncoder*> sce;
  BBCSceFactory factory;
  Stream current;
};
#endif
