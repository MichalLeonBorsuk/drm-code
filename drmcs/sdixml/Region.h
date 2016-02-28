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

#ifndef _REGION_H
#define _REGION_H

#include "persist.h"

using namespace std;

class Region : public Persist
{
public:

  Region();
  Region(const Region& a);
  Region& operator=(const Region& a);
  virtual ~Region();
  virtual void ReConfigure(xmlNodePtr config);
  virtual void clearConfig();
  virtual void GetParams(xmlNodePtr n);
  virtual void PutParams(xmlTextWriterPtr writer);

  struct Area {
    int latitude, longitude, latitude_extent, longitude_extent;
  };
  
  vector<int> zone;
  vector<Area> area;

};
#endif
