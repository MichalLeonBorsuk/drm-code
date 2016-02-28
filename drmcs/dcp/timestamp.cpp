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

#include <stdio.h>
#include "timestamp.h"
#include <time.h>
#include <iostream>
using namespace std;

void clock_getrealtime(struct timespec *tp)
{
#if _POSIX_C_SOURCE >= 199309L
    clock_gettime(CLOCK_REALTIME, tp);
#else
    struct timeval * tv;
    gettimeofday(&tv, NULL);
    tp->tv_sec = tv.tv_sec;
    tp->tv_nsec = 1000UL*tv_usec;
#endif
}

void DrmTime::initialise(int tist_leapseconds,  double tist_delay_s)
{
    // reference time is 2000-01-01 T 00:00 UTC which is currently 10957 seconds
    // after the unix epoch of 1970-01-01
    // The unix epoch drifts forwards with leap seconds, so DRM t0 will appear to drift backwards
    // wrt unix time
    uint64_t drmtzero_ms, tist_delay_ms, tist_ms;
    struct tm time_drmzero;
    time_drmzero.tm_year  = 2000 - 1900;
    time_drmzero.tm_mon   =    1 - 1;
    time_drmzero.tm_mday  =    1;
    time_drmzero.tm_hour  =    0;
    time_drmzero.tm_min   =    0;
    time_drmzero.tm_sec   =    0;
    time_drmzero.tm_isdst =    0;

    utco = uint16_t(tist_leapseconds);

    tist_delay_ms = uint64_t(1000.0*tist_delay_s);
    tist_delay_us = 1000ULL*tist_delay_ms;
    drm_t0 = mktime(&time_drmzero)-tist_leapseconds;
    drmtzero_ms = 1000ULL*drm_t0;
    mux_drm_usec = current_drm_usec();
    // the first frame should be delayed to be suitable for transmission
    // so that frames are transmitted on the minute boundaries, which
    // for 400 ms frames means on the even second boundaries
    // if time and date is to be sent, the SDC blocks must be arranged
    // so that block 0 is send just after the minute boundary at the
    // TRANSMITTER, which is related to the tist
    tist_ms = mux_drm_usec/1000ULL - drmtzero_ms + tist_delay_ms;
    // twosec_ms is the number of milliseconds since the last 2s boundary
    uint64_t twosec_ms = tist_ms % 2000ULL;
    // frame_ms is the number of milliseconds since the last 400 ms boundary
    /* TODO (jfbc#1#): delay the start of coding so that extratime is zero. This
                       will minimise the actual delay between coding and
                       listening. */
    uint64_t frame_ms = twosec_ms % 400ULL;
    // we want to slip enough to make the next tist on a 2s+n*400ms boundary
    uint64_t extra_ms = 400ULL - frame_ms;
    tist_ms += extra_ms;
    // local time at mux must delay by extra_ms too
    mux_drm_usec += 1000ULL*extra_ms;

    tist_minute = tist_ms / 60000ULL;
    uint64_t frac_ms = tist_ms - 60000ULL*tist_minute;
    tist_frame = uint32_t(frac_ms / 400ULL);
}

/* frame  second  ms
    0      0       0
    1      0     400
    2      0     800
    3      1     200
    4      1     600
    5      2       0
    ...
*/
uint64_t DrmTime::tist_second()
{
    return 60*tist_minute + (4*tist_frame)/10;
}

uint16_t DrmTime::tist_ms()
{
    return 100*(4*tist_frame-10*uint16_t((4*tist_frame)/10));
}

uint64_t DrmTime::current_utc_usec()
{
    timespec tn;
    clock_getrealtime(&tn);
    return 1000000ULL*uint64_t(tn.tv_sec)+uint64_t(tn.tv_nsec)/1000ULL;
}

uint64_t DrmTime::current_drm_usec()
{
    timespec tn;
    clock_getrealtime(&tn);
    return 1000000ULL*uint64_t(tn.tv_sec-utco)+uint64_t(tn.tv_nsec)/1000ULL;
}


void DrmTime::date_and_time(uint32_t& mjd, uint8_t& hh, uint8_t& mm, time_t secs)
{
    struct tm *t;
    t = gmtime(&secs);
    uint32_t year,month,day,hour,min,second;
    year = t->tm_year+1900;
    month = t->tm_mon+1;
    day = t->tm_mday;
    hour = t->tm_hour;
    min = t->tm_min;
    second = t->tm_sec;
    // http://www.answers.com/topic/julian-day
    uint32_t a = (14-month)/12;
    uint32_t y = year + 4800 - a;
    uint32_t m = month + 12*a - 3;
    uint32_t jdn = day + (153*m+2)/5 + 365*y + y/4 - y/100 +y/400 - 32045;
    double jd = jdn + (hour-12.0)/24.0 + min/1440.0 + second/86400.0;
    double dmjd = jd - 2400000.5;
    //cout << "mjd " << mjd << endl;
    mjd = uint32_t(dmjd);
    hh = uint8_t(hour);
    mm = uint8_t(min);
}

void DrmTime::date_and_time(uint32_t& mjd, uint8_t& hh, uint8_t& mm)
{
    time_t secs = time_t(tist_second()) + drm_t0 - utco;
    date_and_time(mjd, hh, mm, secs);
}

void DrmTime::wait(long usec)
{
#ifdef WIN32
    /* windows select doesn't work with no fds */
    DWORD t_ms = static_cast<DWORD>(usec/1000UL);
    Sleep(t_ms);
#else
    struct timeval t;
    int maxfd=0;
    t.tv_sec=0;
    t.tv_usec = usec;
    select(maxfd, NULL, NULL, NULL, &t);
#endif
}

void DrmTime::increment()
{
    // add 400 milliseconds
    tist_frame++;
    if(tist_frame>149) {
        tist_frame = 0;
        tist_minute++;
    }
    mux_drm_usec += 400000ULL;
}

void DrmTime::check_for_leap_second()
/* TODO (jfbc#1#): check if we have just had a leap second and adjust
                   m_utco. */
{
}
