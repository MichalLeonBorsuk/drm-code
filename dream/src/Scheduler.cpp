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

#include "Scheduler.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>

void CScheduler::LoadSchedule(const string& filename)
{
	LoadIni(filename.c_str());
	for(int i=1; i<999; i++)
	{
		ostringstream ss;
		ss << "Freq" << i;
		string f = GetIniSetting("Settings", ss.str());
		ss.str("");
		ss << "StartTime" << i;
		string starttime = GetIniSetting("Settings", ss.str());
		ss.str("");
		ss << "EndTime" << i;
		string endtime = GetIniSetting("Settings", ss.str());
		if(starttime == endtime)
			break;
		time_t start = parse(starttime);
		time_t end = parse(endtime);
		schedule[start] = int(atol(f.c_str()));
		schedule[end] = -1;
	}
}

// get next event time and frequency after input time
time_t CScheduler::next(time_t dt)
{
	map<time_t,int> abs_sched = abs(dt);
	// find last event before dt
	for(map<time_t,int>::const_iterator i = abs_sched.begin(); i != abs_sched.end(); i++)
	{
		if(i->first>dt)
			return i->first;
	}
	return 0;
}

// get frequency we should be tuned to at input time
int CScheduler::frequency(time_t dt)
{
	map<time_t,int> abs_sched = abs(dt);
	// find last event before dt
	for(map<time_t,int>::const_iterator i = abs_sched.begin(); i != abs_sched.end(); i++)
	{
		if(dt>=i->first)
			return i->second;
	}
	return -1;
}

map<time_t,int> CScheduler::abs(time_t dt)
{
	struct tm dts;
	dts = *gmtime(&dt);
	dts.tm_hour = 0;
	dts.tm_min = 0;
	dts.tm_sec = 0;
	time_t sod = mktime(&dts);
	// resolve schedule to absolute time
	map<time_t,int> abs_sched;
	for(map<time_t,int>::const_iterator i = schedule.begin(); i != schedule.end(); i++)
	{
		time_t st = sod+i->first;
		if(st < dt)
			st += 86400; // want tomorrow's.
		abs_sched[st] = i->second;
	}
	return abs_sched;
}

int CScheduler::parse(string s)
{
	int hh,mm,ss;
	char c;
	istringstream iss(s);
	iss >> hh >> c >> mm >> c >> ss;
	return 60*(mm + 60*hh)+ss;
}
