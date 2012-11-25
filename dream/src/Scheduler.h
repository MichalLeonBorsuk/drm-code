/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2012
 *
 * Author(s):
 *      Julian Cable
 *
 * Description:
 *
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

#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <string>
#include <map>
#include "util/Settings.h"

using namespace std;

class CScheduler: public CIniFile
{
public:
	CScheduler():schedule(){}
	void LoadSchedule(const string& filename);
	time_t next(time_t); // get next event time after input time
	int frequency(time_t); // get frequency we should be tuned to at input time
private:
	map<time_t,int> schedule; // map seconds from start of day to schedule event, frequency or -1 for off
	map<time_t,int> abs(time_t dt);
	int parse(string);
};

#endif
