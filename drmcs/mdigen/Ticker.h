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

#ifndef _TICKER_H
#define _TICKER_H

#include "sockets.h"

class CTicker
{
public:

  CTicker() {}
  virtual ~CTicker() {}
  void request_channels(SOCKET s , int networks, int *networklist);
  void check_for_input(SOCKET s);

  virtual void added(const char *name, int id);
  virtual void deleted(int id);
  virtual void changed(int id, const char *text);
  virtual void closed();
};
#endif
