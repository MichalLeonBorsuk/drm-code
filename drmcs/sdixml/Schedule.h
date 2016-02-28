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

#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "persist.h"

class Schedule : public Persist
{
public:

  Schedule();
  Schedule(const Schedule& s);
  Schedule& operator=(const Schedule& s);
  virtual ~Schedule();
  virtual void ReConfigure(xmlNodePtr config);
  virtual void clearConfig();
  virtual void GetParams(xmlNodePtr n);
  virtual void PutParams(xmlTextWriterPtr writer);

  struct Interval {
  		Interval():days(),start_hour(-1),start_minute(-1),duration(-1){}
         string days;
         int start_hour, start_minute;
         int duration;
  };
  
  std::vector<Interval> interval;
};
#endif
