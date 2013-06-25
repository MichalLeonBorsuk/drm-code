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

#include <DrmMuxConfig.h>
#include "bytevector.h"

#define ERR_NO_ERROR 0

class SdcElement
{
public:

  bytevector out;
  unsigned type;
  size_t size() { return out.size(); }
  // methods to generate the sdc element
  void Type0(const bytevector&, bool);
  void Type1(const string&, size_t );
  void Type2(const Service&, bool, size_t );
  void Type3(const AFSMuxlist&, int, int);
  void Type4(size_t, const Schedule::Interval&, int );
  void Type5(const ServiceComponent&, size_t, size_t, bool);
  void Type6(const Announcement&, uint8_t, int);
  void Type7(size_t, const Region::Area&, const vector<int>&, int);
  void Type8(uint32_t, uint8_t, uint8_t);
  void Type9(const ServiceComponent&, size_t, size_t, bool, bool);
  void Type10(const RFChannel&, bool, size_t);
  void Type11(const ServiceGroup&, bool, size_t, int);
  void Type12(const Service&, size_t );
};

