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

#ifndef _SERVICECOMPONENTENCODER_H
#define _SERVICECOMPONENTENCODER_H

#include <ServiceComponent.h>
#include <bytevector.h>
#include <iostream>

class ServiceComponentEncoder
{
public:

  ServiceComponentEncoder():current(){}
  ServiceComponentEncoder(const ServiceComponentEncoder& e):current(e.current){}
  ServiceComponentEncoder& operator=(const ServiceComponentEncoder& e)
  { current=e.current; return *this; }
  virtual ~ServiceComponentEncoder() {}
  virtual void ReConfigure(const ServiceComponent& config) { 	 cerr << "ServiceComponent::ReConfigure" << endl;
 current = config; }
  virtual void NextFrame(bytevector& buf, size_t max, double stoptime=0) {}
  virtual string next_message() { return "";} // avoids cast for text messages in SDI

  ServiceComponent current;
};

#endif
