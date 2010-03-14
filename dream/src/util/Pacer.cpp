/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *
 *  This defines a concrete subclass of CPacketSink that writes to a file
 *  For the moment this will be a raw file but FF could be added as a decorator
 *  The writing can be stopped and started - if it is not currently writing,
 *  any packets it receives will be silently discarded
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

#include "Pacer.h"

#ifdef _WIN32

  /* The FILETIME structure is a 64-bit value representing
   * the number of 100-nanosecond intervals since January 1, 1601.
   */

CPacer::CPacer(uint64_t ns)
{
  FILETIME ft;
  hTimer = CreateWaitableTimer(NULL, true, TEXT("CPacerTimer"));
  if(hTimer==NULL)
  {
	throw "Create Timer failed ";
  }
  GetSystemTimeAsFileTime(&ft);
  interval = ns/100;
  timekeeper = *(uint64_t*)&ft;
  timekeeper += interval;
  LARGE_INTEGER liDueTime;
  liDueTime.QuadPart = timekeeper;
  if(!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
  {
	throw "Set Timer failed ";
  }
}

CPacer::~CPacer()
{
  CancelWaitableTimer(hTimer);
}

uint64_t CPacer::nstogo()
{
  FILETIME ft;
  LARGE_INTEGER liNow;
  GetSystemTimeAsFileTime(&ft);
  liNow.LowPart = ft.dwLowDateTime;
  liNow.HighPart = ft.dwHighDateTime;
  return (uint64_t)100*(timekeeper - liNow.QuadPart);
}

void CPacer::wait()
{
  if(WaitForSingleObject(hTimer, INFINITE) !=WAIT_OBJECT_0)
  {
	throw "Wait on Timer failed ";
  }
  /* and re-arm */
  timekeeper += interval;
  LARGE_INTEGER liDueTime;
  liDueTime.QuadPart = timekeeper;
  if(!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
  {
	throw "Set Timer failed ";
  }
}

#else

#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

CPacer::CPacer(uint64_t ns)
{
	timespec now;
#if _POSIX_TIMERS>0
	int r = clock_gettime(CLOCK_REALTIME, &now);
#else
	timeval t;
	int r = gettimeofday(&t, NULL);
	now.tv_sec = t.tv_sec;
	now.tv_nsec = 1000*t.tv_usec;
#endif
	if(r<0)
		perror("time");
	interval = ns;
	timekeeper = 1000000000LL*now.tv_sec+now.tv_nsec;
	timekeeper += interval;
}

CPacer::~CPacer()
{
	//cout << "CPacer destructor" << endl; cout.flush();
}

void CPacer::wait()
{
	uint64_t delay_ns = nstogo();
	if(delay_ns>20000000ULL)
	{
		timespec delay;
		delay.tv_sec = delay_ns / 1000000000ULL;
		delay.tv_nsec = delay_ns % 1000000000ULL;
		nanosleep(&delay, NULL);
	}
	timekeeper += interval;
}

uint64_t CPacer::nstogo()
{
	timespec now;
#if _POSIX_TIMERS>0
	(void)clock_gettime(CLOCK_REALTIME, &now);
#else
	timeval t;
	(void)gettimeofday(&t, NULL);
	now.tv_sec = t.tv_sec;
	now.tv_nsec = 1000*t.tv_usec;
#endif
	uint64_t now_ns = 1000000000LL*now.tv_sec+now.tv_nsec;
	if(timekeeper<=now_ns)
		return 0;
	return timekeeper - now_ns;
}
#endif
