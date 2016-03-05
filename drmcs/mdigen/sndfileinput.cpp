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

#include "sndfileinput.h"
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <sstream>
using namespace std;

CSndFile::CSndFile():handle(NULL),sfinfo(),num_channels(0)
{
    cout << "sndfile default constructor" << endl;
}

CSndFile::CSndFile(const CSndFile& e):handle(e.handle),sfinfo(e.sfinfo),num_channels(e.num_channels)
{
}

CSndFile& CSndFile::operator=(const CSndFile& e)
{
    handle = e.handle;
    sfinfo = e.sfinfo;
    num_channels = e.num_channels;
    return *this;
}

CSndFile::~CSndFile()
{
    close();
}

bool CSndFile::is_open()
{
    return (handle)?true:false;
}

void CSndFile::open(const string& device, int channels)
{
    memset(&sfinfo, 0, sizeof(SF_INFO));
    num_channels = channels;
    stringstream s(device);
    string sys, dev;
    s >> sys >> dev;
    cout << "sndfile: " << dev << channels << endl;
    cout.flush();
    handle = sf_open(dev.c_str(), SFM_READ, &sfinfo);
    if (handle == NULL)
        throw string("audio open error ") + sf_strerror(0);
    if(sfinfo.channels != num_channels)
    {
        //num_channels = -1;
        //throw string("audio open error wrong num channels file");
    }
    if(sfinfo.samplerate != 48000)
    {
        num_channels = -1;
        throw string("audio open error wrong sample rate in file");
    }
    cout << "sndfile audio open " <<  dev << endl;
    cout.flush();
}

void CSndFile::close()
{
    if(handle) {
        ::sf_close(handle);
        handle = NULL;
    }
}

void CSndFile::read(vector<float>& buffer)
{
    if(num_channels == -1)
        return;
    sf_count_t wanted = buffer.size()/num_channels;
    sf_count_t to_read = wanted;
    if(sfinfo.channels == 2 && num_channels == 1) {
        to_read = 2 * wanted;
        float buf[to_read];
        sf_count_t c = sf_readf_float(handle, &buf[0], wanted);
        if(c != wanted)
        {
            sf_seek(handle, 0, SEEK_SET);
            (void)sf_readf_float(handle, &buf[0], wanted);
        }
        for(int i=0; i<wanted; i++)
        {
            buffer[i] = (buf[2*i]+buf[2*i+1])/2.0;
        }
        return;
    }
    if(sfinfo.channels == 1 && num_channels == 2) {
        to_read = wanted / 2;
        sf_count_t c = sf_readf_float(handle, &buffer[to_read], wanted);
        if(c != wanted)
        {
            sf_seek(handle, 0, SEEK_SET);
            (void)sf_readf_float(handle, &buffer[to_read], wanted);
        }
        for(int i=0; i<to_read; i++)
        {
            buffer[i] = buffer[2*i]/2.0;
            buffer[i+1] = buffer[2*i]/2.0;
        }
        return;
    }
    sf_count_t c = sf_readf_float(handle, &buffer[0], wanted);
    if(c != wanted)
    {
        sf_seek(handle, 0, SEEK_SET);
        (void)sf_readf_float(handle, &buffer[0], wanted);
    }
}
