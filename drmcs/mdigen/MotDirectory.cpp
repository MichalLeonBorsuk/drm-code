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

#include "MotDirectory.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <zlib.h>
#include <timestamp.h>

using namespace std;

MotDirectory::MotDirectory(): next_transport_id(0), dirty(true),
    SegmentSize(0), DataCarouselPeriod(0), always_send_mime_type(false)
{
}

MotDirectory::~MotDirectory()
{
}

void MotDirectory::setDataCarouselPeriod(uint32_t period)
{
    DataCarouselPeriod = period;
}

void MotDirectory::setSegmentSize(uint16_t segmentSize)
{
    SegmentSize = segmentSize;
}

void MotDirectory::setSortedHeaderInformation()
{
    MotObject::putExtensionParameterHeader(extension, 0x00, 0);
}

void MotDirectory::setAlwaysSendMimeType(bool val)
{
    always_send_mime_type = val;
}

void MotDirectory::putPermitOutdatedVersions(bytevector& out, uint8_t n) const
{
    MotObject::putExtensionParameterHeader(out, 0x01, 1);
    out.put(n, 8);
}

void MotDirectory::setDefaultPermitOutdatedVersions(uint8_t n)
{
    putPermitOutdatedVersions(extension, n);
}

void MotDirectory::putExpirationRelative(bytevector& out, time_t t)
{
    MotObject::putExtensionParameterHeader(out, 0x09, 1);
    time_t mins = t / 60;
    if(mins<=63) {
        out.put(0, 2);
        out.put(mins/2, 6);
    } else {
        time_t half_hours = mins / 30;
        if(half_hours<=63) {
            out.put(1, 2);
            out.put(half_hours, 6);
        } else {
            time_t bi_hours = half_hours / 4;
            if(bi_hours<=63) {
                out.put(2, 2);
                out.put(bi_hours, 6);
            } else {
                time_t days = bi_hours / 12;
                if(days<=63) {
                    out.put(3, 2);
                    out.put(days, 6);
                }
            }
        }
    }
}

void MotDirectory::putExpirationAbsolute(bytevector& out, timespec t)
{
    uint8_t hours=0, mins=0;
    bool secs=false; // TODO
    uint32_t mjd=0;
    DrmTime::date_and_time(mjd, hours, mins, t.tv_sec);
    MotObject::putExtensionParameterHeader(out, 0x09, 4+(secs?2:0));
    out.put(1, 1); // time valid
    out.put(mjd, 17);
    out.put(0, 2); //  rfu
    out.put(secs, 1);
    out.put(hours, 5);
    out.put(mins, 6);
    if(secs) {
        uint16_t seconds=0; // TODO
        out.put(seconds, 6);
        out.put(t.tv_nsec/1000000L, 10);
    }
}

void MotDirectory::setDefaultExpirationRelative(time_t t)
{
    putExpirationRelative(extension, t);
}

void MotDirectory::setDefaultExpirationAbsolute(timespec t)
{
    putExpirationAbsolute(extension, t);
}

void MotDirectory::setPriority(uint8_t priority)
{
    MotObject::putExtensionParameterHeader(extension, 0x0a, 1);
    extension.put(priority, 8);
}

void MotDirectory::setRetransmissionDistance(uint32_t d)
{
    MotObject::putExtensionParameterHeader(extension, 0x07, 4);
    extension.put(0, 8);
    extension.put(d, 24);
}

void MotDirectory::setProfileSubset(const bytevector& profiles)
{
    MotObject::putExtensionParameterHeader(extension, 0x21, profiles.size());
    extension.put(profiles);
}

void MotDirectory::setDirectoryIndex(uint8_t profile, const string& index)
{
    MotObject::putStringParameter(extension, 0x22, index, profile);
}

void MotDirectory::setUniqueBodyVersion(uint32_t v)
{
    MotObject::putExtensionParameterHeader(extension, 0x0d, 4);
    extension.put(v, 32);
}

void MotDirectory::put_compressed_to(crcbytevector& out)
{
    crcbytevector in;
    put_to(in);
    Bytef* o = new Bytef[2*in.size()];
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef*)&in.data()[0];
    strm.next_out = o;
    strm.avail_in = in.size();
    strm.avail_out = 2*in.size();
    if(deflateInit2(&strm, 9, Z_DEFLATED, 16+15, 8, Z_DEFAULT_STRATEGY)!=Z_OK)
        throw(string("zlib init error"));
    if(deflate(&strm, Z_FINISH)!=Z_STREAM_END)
        throw(string("zlib process error"));
    if(deflateEnd(&strm)!=Z_OK)
        throw(string("zlib finish error"));
    out.put(1, 1); // CF compression flag
    out.put(0, 1); // rfu
    out.put(9+strm.total_out, 30); // entity size
    out.put(1, 8);
    out.put(0, 2); // rfu
    out.put(strm.total_in, 30); // uncompressed data size
    out.putbytes((char*)o, strm.total_out);
    delete[] o;
}

void MotDirectory::put_to(crcbytevector& out)
{
    bytevector ob;
    if(dirty)
    {
        transport_id = next_transport_id++;
        dirty = false;
        cout << "MOT Directory with tid " << transport_id << endl;
    }
    for(map<string,MotObject>::iterator i=objects.begin(); i!=objects.end(); i++) {
        const MotObject& m = i->second;
        ob.put(m.transport_id, 16);
        m.putHeader(ob);
    }
    out.put(0, 1); // CF compression flag, must be zero for known applications
    out.put(0, 1); // rfu
    out.put(13+extension.size()+ob.size(), 30);
    out.put(objects.size(), 16);
    out.put(DataCarouselPeriod, 24);
    out.put(0,1);  // rfu
    out.put(0,2);  // rfa
    out.put(SegmentSize, 13);
    out.put(extension.size(), 16);
    out.put(extension);
    out.put(ob);
}

void MotDirectory::add_file(const string& name, size_t size)
{
    objects[name].file_size = size;
    objects[name].transport_id = next_transport_id++;
    objects[name].setHeaders(name);
    objects[name].always_send_mime_type = always_send_mime_type;
    dirty = true;
}

void MotDirectory::change_file(const string& name, size_t size)
{
    objects[name].file_size = size;
    objects[name].object_version++;
    dirty = true;
}

void MotDirectory::delete_file(const string& name)
{
    objects.erase(name);
    dirty = true;
}

bool MotDirectory::check_add_file(const string& name, uint32_t size)
{
    bool add = false;
    if (objects.count(name) == 0) {
        // its a new file
        objects[name].object_version = 0;
        add = true;
        /*
          } else if (objects[name].last_modified < info.last_modified) {
            // its a changed file
            objects[name].object_version++;
        	add = true;
        */
    }
    if (add) {
        add_file(name, size);
    }
    return add;
}
