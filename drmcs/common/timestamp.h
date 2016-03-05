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

#ifndef _TIMESTAMP_H
#define _TIMESTAMP_H

#include <ctime>
#include <cstdint>

void clock_getrealtime(struct timespec *tp);

class DrmTime
{
public:

    DrmTime():mux_drm_usec(0),tist_minute(0),tist_frame(0),tist_delay_us(0),
    utco(0),drm_t0(0){}

    static void date_and_time(uint32_t& mjd, uint8_t& hh, uint8_t& mm, time_t secs);

    // the time here at the mux when the next frame should be transmitted
    uint64_t mux_drm_usec;
    // the time at the transmitter when the next frame should be transmitted
    uint64_t tist_minute;
    uint32_t tist_frame;
    uint64_t tist_delay_us;
    uint16_t utco;
    time_t drm_t0;

    void initialise(int tist_leapseconds, double tist_delay_s);
    void increment();
    void wait(long usec);
    uint64_t current_utc_usec();
    uint64_t current_drm_usec();
    void check_for_leap_second();
    void date_and_time(uint32_t&, uint8_t&, uint8_t&);
    uint64_t tist_second();
    uint16_t tist_ms();
};
#endif
