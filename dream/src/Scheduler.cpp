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
#ifdef _WIN32
# include <windows.h>
#endif

#if 0
# ifdef _WIN32
# include <stdarg.h>
int qDebug(const char *format, ...)
{
	char buffer[1024];
	va_list vl;
	va_start(vl, format);
	int ret = vsprintf(buffer, format, vl);
	va_end(vl);
	OutputDebugStringA(buffer);
	return ret;
}
# else
#  define qDebug(...) fprintf(stderr, __VA_ARGS__)
# endif
#endif

#ifdef _WIN32
time_t timegm(struct tm *tm)
{
	SYSTEMTIME st;
	st.wYear = tm->tm_year+1900;
	st.wMonth = tm->tm_mon+1;
	st.wDay = tm->tm_mday;
	st.wHour = 0;
	st.wMinute = 0;
	st.wSecond = 0;
	st.wMilliseconds = 0;
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	ULARGE_INTEGER uli;
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	return (time_t)(uli.QuadPart/10000000 - 11644473600);
}
#endif

// get next event
CScheduler::SEvent CScheduler::front()
{
	if(events.empty())
	{
		fill();
	}
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
SaveIni(cerr);
	for(int i=1; i<999; i++)
	{
		ostringstream ss;
		ss << "Freq" << i;
		string f = GetIniSetting("Settings", ss.str());
qDebug("freq %s", f.c_str());
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
	time_t dt = time(NULL); // daytime
	struct tm dts;
	dts = *gmtime(&dt);
	dts.tm_hour = 0;
	dts.tm_min = 0;
	dts.tm_sec = 0;
	time_t sod = timegm(&dts); // start of daytime
	// resolve schedule to absolute time
	map<time_t,int> abs_sched;
	map<time_t,int>::const_iterator i;
	for(i = schedule.begin(); i != schedule.end(); i++)
	{
		time_t st = sod + i->first;
		if (st < dt)
			st += 24 * 60 * 60; // want tomorrow's.
		abs_sched[st] = i->second;
	}
	for (i = abs_sched.begin(); i != abs_sched.end(); i++)
	{
		SEvent e;
		e.time = i->first;
		e.frequency = i->second;
		events.push(e);
	}
}

int CScheduler::parse(string s)
{
	int hh,mm,ss;
	char c;
	istringstream iss(s);
	iss >> hh >> c >> mm >> c >> ss;
qDebug("%s %02i:%02i:%02i\n", s.c_str(), hh, mm, ss);
	return 60*(mm + 60*hh)+ss;
}
