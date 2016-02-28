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

#include "oss.h"
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <sstream>
using namespace std;

#ifdef WIN32
// this is just to help test syntax when developing on Windows
#  include <io.h>
extern "C" {
    int ioctl (int, int, ...)
    {
        return -1;
    }
};
#  define AFMT_S16_NE 1
#  define SNDCTL_DSP_SETFMT 1
#  define SNDCTL_DSP_CHANNELS 1
#  define SNDCTL_DSP_SPEED 1
#else
#  include <sys/ioctl.h>
#  include <sys/soundcard.h>
#  include <unistd.h>
#  include <cstdio>
#  include <cstdlib>
#endif

OSS::OSS():handle(-1)
{
    cout << "oss default constructor" << endl;
}

OSS::OSS(const OSS& e):handle(e.handle),num_channels(e.num_channels)
{
}

OSS& OSS::operator=(const OSS& e)
{
    handle = e.handle;
    num_channels = e.num_channels;
    return *this;
}

OSS::~OSS()
{
    close();
}

bool OSS::is_open()
{
    return (handle==-1)?false:true;
}

void OSS::open(const string& device, int channels)
{
    num_channels = channels;
    stringstream s(device);
    string sys, dev;
    s >> sys >> dev;
    cout << "oss: " << dev << endl;
    if(dev[0]!='/')
        dev = "/dev/dsp";
    handle = ::open(dev.c_str(), O_RDONLY, 0);
    if(handle == -1)
        throw string("audio open error ") + strerror(errno);

    int format = AFMT_S16_NE;
    if (ioctl(handle, SNDCTL_DSP_SETFMT, &format) == -1)
        throw string("audio format error ") + strerror(errno);
    if(format != AFMT_S16_NE)
        throw string("audio format error");

    int chan = 2; // always capture stereo, for some reason!
    if (ioctl(handle, SNDCTL_DSP_CHANNELS, &chan) == -1)
        throw string("audio stereo error");

    int rate = 48000;
    if (ioctl(handle, SNDCTL_DSP_SPEED, &rate)==-1)
        throw string("audio rate error");
    if(rate != 48000)
        throw string("audio rate error");

    cout << "oss audio open " <<  dev << " fd " << handle << endl;
}

void OSS::close()
{
    if(handle>=0) {
        ::close(handle);
        handle = -1;
    }
}

void OSS::read(vector<float>& buffer)
{
    float* buf = &buffer[0];
    int want = buffer.size();
    do {
        short b[2]; // always read 16 bit stereo interleaved
again:
        int r = ::read(handle, b, sizeof(b));
        if(r < 0) {
            if(errno == EINTR)
                goto again;
            throw string("audio read error ") + strerror(errno);
        }
        if (r > 0) {
            if(num_channels == 2) {
                *buf++ = float(double(b[0])/32768.0);
                *buf++ = float(double(b[1])/32768.0);
                want -= 2;
            } else {
                double s = (b[0]+b[1])/2;
                *buf++ = s/32768.0; // scale to -1.0 .. +1.0
                want--;
            }
        }
    } while (want > 0);
}
