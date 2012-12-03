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
int dprintf(const char *format, ...)
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
#  define dprintf(...) fprintf(stderr, __VA_ARGS__)
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
	return (time_t)((uli.QuadPart - 116444736000000000ULL)/10000000ULL);
}
#endif

// get next event
CScheduler::SEvent CScheduler::front()
{
	if(events.empty())
	{
		fill();
	}
//struct tm* dts = gmtime(&events.front().time); dprintf("%i %02i:%02i:%02i frequency=%i\n", (int)dts->tm_mday, (int)dts->tm_hour, (int)dts->tm_min, (int)dts->tm_sec, (int)events.front().frequency);
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
	time_t sod = timegm(&dts); // start of daytime
//dprintf("sod = %i\n", sod);
//{struct tm* gtm = gmtime(&sod); if (gtm) dprintf("%i %i %02i:%02i:%02i\n", (int)gtm->tm_year+1900, (int)gtm->tm_mday, (int)gtm->tm_hour, (int)gtm->tm_min, (int)gtm->tm_sec); else dprintf("gtm = NULL\n");}
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
//struct tm* dts = gmtime(&e.time); dprintf("%i %02i:%02i:%02i frequency=%i\n", (int)dts->tm_mday, (int)dts->tm_hour, (int)dts->tm_min, (int)dts->tm_sec, (int)e.frequency);
	}
}

int CScheduler::parse(string s)
{
	int hh,mm,ss;
	char c;
	istringstream iss(s);
	iss >> hh >> c >> mm >> c >> ss;
//dprintf("%02i:%02i:%02i\n", hh, mm, ss);
	return 60*(mm + 60*hh)+ss;
}
