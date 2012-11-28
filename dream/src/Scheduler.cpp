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

// get next event
CScheduler::SEvent CScheduler::front()
{
	if(events.empty())
	{
		fill();
	}
//struct tm* dts = gmtime(&events.front().time); printf("%i %02i:%02i:%02i frequency=%i\n", (int)dts->tm_mday, (int)dts->tm_hour, (int)dts->tm_min, (int)dts->tm_sec, (int)events.front().frequency);
	return events.front();
}

// remove first event from queue and return the front
CScheduler::SEvent CScheduler::pop()
{
	events.pop();
	return front();
}

bool CScheduler::empty() const
{
	return events.empty();
}

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
	fill();
}

void CScheduler::fill()
{
	time_t dt;
//	if (events.empty())
//	{
		dt = time(NULL); // daytime
//	}
//	else
//	{
//		dt = events.back().time;
//	}
	struct tm dts;
	dts = *gmtime(&dt);
	dts.tm_hour = 0;
	dts.tm_min = 0;
	dts.tm_sec = 0;
//	time_t sod = mktime(&dts); // start of daytime // mktime is for localtime struct tm to time_t (UTC)
	time_t sod = timegm(&dts); // start of daytime
	// resolve schedule to absolute time
	map<time_t,int> abs_sched;
	for (map<time_t,int>::const_iterator i = schedule.begin(); i != schedule.end(); i++)
	{
		time_t st = sod + i->first;
		if (st < dt)
			st += 24 * 60 * 60; // want tomorrow's.
		abs_sched[st] = i->second;
	}
	for (map<time_t,int>::const_iterator i = abs_sched.begin(); i != abs_sched.end(); i++)
	{
		SEvent e;
		e.time = i->first;
		e.frequency = i->second;
		events.push(e);
//struct tm* dts = gmtime(&e.time); printf("%i %02i:%02i:%02i frequency=%i\n", (int)dts->tm_mday, (int)dts->tm_hour, (int)dts->tm_min, (int)dts->tm_sec, (int)e.frequency);
	}
}

int CScheduler::parse(string s)
{
	int hh,mm,ss;
	char c;
	istringstream iss(s);
	iss >> hh >> c >> mm >> c >> ss;
//printf("%02i:%02i:%02i\n", hh, mm, ss);
	return 60*(mm + 60*hh)+ss;
}
