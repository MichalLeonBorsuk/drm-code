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

#include <iostream>
#include <fstream>
#include <sstream>
#ifdef WIN32
# include <io.h>
#else
# include <unistd.h>
# include <dirent.h>
# include <syslog.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <zlib.h>

#include <timestamp.h>
#include <packetencoder.h>
#include <platform.h>
#include "MOTEncoder.h"

MOTEncoder::MOTEncoder(): packet_queue(), in_dir(),
    block_size(0), packet_encoder(), directory(),  dge(), current_object(),
    flags(), in_file(-1),max_queue_depth(100), segment(0), dir_interval(120.0e3),
    monitored_directories()
#ifndef WIN32
    ,fc()
#endif
{
}

MOTEncoder::MOTEncoder(const MOTEncoder& o): packet_queue(), in_dir(o.in_dir),
    block_size(o.block_size), packet_encoder(), directory(),  dge(), current_object(),
    flags(o.flags), in_file(-1), max_queue_depth(100), segment(0), dir_interval(30.0e3),
    monitored_directories()
#ifndef WIN32
    ,fc()
#endif
{
}

MOTEncoder& MOTEncoder::operator=(const MOTEncoder& o)
{
    in_dir = o.in_dir;
    block_size = o.block_size;
    flags = o.flags;
    in_file = -1;
    return *this;
}

MOTEncoder::~MOTEncoder()
{
    if(in_file>=0)
    {
        lockf(in_file, F_ULOCK, 0);
        close(in_file);
    }
#ifndef WIN32
    FAMClose(&fc);
#endif
}

/* MOTEncoder state:
x  string in_dir, out_dir;
?x  uint16_t next_transport_id;
x  bool use_crc, use_compression, use_directory;
x  uint16_t block_size;
  PacketEncoder packet_encoder;
x  MotDirectory directory;
  DataGroupEncoder dge;
x  map<string, MotObject>::iterator current_object;
*/

void MOTEncoder::ReConfigure(
    const string& encoder_id_param,
    const string & in,
    uint16_t block_sizep, uint16_t packet_id,
    uint16_t packet_size,
    Flags& flagsp,
    vector<string>& compressible,
    map<uint8_t,string>& profile_index
)
{
#ifndef WIN32
    if(FAMCONNECTION_GETFD(&fc)>0)
    {
        (void)FAMClose(&fc);
    }
#endif
    monitored_directories.clear();
    encoder_id = encoder_id_param;
    in_dir = in;
    block_size = block_sizep;
    packet_encoder.packet_id = packet_id;
    packet_encoder.packet_size = packet_size;
    flags = flagsp;
    directory.clear();
    directory.setSortedHeaderInformation();
    directory.setAlwaysSendMimeType(flags.always_send_mime_type);
    //directory.next_transport_id = 0; // should this be continuous across reconfigurations?
    for (map<uint8_t,string>::iterator i = profile_index.begin();
            i != profile_index.end(); i++)
    {
        directory.setDirectoryIndex(i->first, i->second);
    }
    // work out the DGE overhead
    dge.Configure(flags.crc, true, true);
    {
        crcbytevector dg;
        bytevector data;
        dge.putDataGroupSegment(dg, 0, data, 4U, 0, 0, true);
        dg_payload_size = block_size - dg.size();
    }
    // clear the output queue
    while(!packet_queue.empty()) packet_queue.pop();
    if(in_file>=0)
    {
        close(in_file);
        in_file = -1;
    }
    current_object = directory.objects.end();
    last_sent_dir = 0.0;
#ifndef WIN32
    if(FAMOpen(&fc)==0) {
        FAMRequest fr;
        if(FAMMonitorDirectory(&fc, in_dir.c_str(), &fr, NULL)==0)
            monitored_directories[fr.reqnum] = "";
        else
            cerr << "can't monitor " << in_dir << endl;
    } else {
        cerr << "can't connect to file alteration monitor " << endl;
    }
#endif
}

void MOTEncoder::next_packet(bytevector &out, size_t max, double stoptime)
{
    if(scan_input_directory() && flags.directory_mode)
        code_MOTdirectory();
    if(packet_queue.empty())
        fill(stoptime);
    if(packet_queue.empty())
    {
        //cout << "MOTEncoder: queue empty" << endl;
        return;
    } else {
        //cout << "MOTEncoder: queue OK" << endl;
    }
    if(packet_queue.front().size()<=max) {
        out.put(packet_queue.front());
        packet_queue.pop();
    } else {
        cerr << "MOTEncoder: packet queue size mismatch with packet mux" << endl;
        cerr.flush();
    }
}

void MOTEncoder::fill(double stoptime)
{
    timespec t;
    clock_getrealtime(&t);
    double now_ms = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
    if(flags.directory_mode && (now_ms-last_sent_dir)>dir_interval)
    {
        code_MOTdirectory();  // sending compressed and/or uncompressed?
        last_sent_dir = now_ms;
    }
    while(packet_queue.size() < max_queue_depth && now_ms < stoptime)
    {
        if(in_file<0) {
            segment = 0;
            move_directory_iterator();
            if (current_object == directory.objects.end()) {
                return; // nothing we can do
            }
            MotObject& m = current_object->second;
            string inp = in_dir + "/" + m.file_name;
            int mode = O_RDWR; // need RW for locking
#ifdef WIN32
            mode |= O_BINARY;
#endif
            in_file = open(inp.c_str(), mode);
            lockf(in_file, F_TLOCK, 0);
            if(flags.directory_mode==false) {
                codefileheader(m);
            }
            //cout << "sending " << m.file_name << " as tid " << m.transport_id << endl;
        }
        if (in_file<0) {
            return; // nothing we can do
        }
        get_one_data_unit(current_object->second.transport_id);
        clock_getrealtime(&t);
        now_ms = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
    }
}

void MOTEncoder::move_directory_iterator()
{
    if(current_object != directory.objects.end()) {
        current_object++;
    }
    if (current_object == directory.objects.end()) {
        current_object = directory.objects.begin();
    }
}

void MOTEncoder::get_one_data_unit(uint16_t transport_id)
{
    crcbytevector dg;
    bytevector data;
    bool last;
    data.resize(dg_payload_size);
    size_t r = read(in_file, (char*)data.data(), dg_payload_size);
    if(r<data.size())
        data.resize(r);
    if(r==0 || r < dg_payload_size) {
        lockf(in_file, F_ULOCK, 0);
        close(in_file);
        in_file = -1;
        last = true;
    } else {
        last = false;
    }
    dge.putDataGroupSegment(dg, transport_id, data, 4U, segment & 0x0f, segment, last);
    packet_encoder.makeDataUnit(packet_queue, dg);
    segment++;
    //cout << "put segment, queue length is " << packet_queue.size() << endl;
}

void MOTEncoder::compress(crcbytevector& out, const crcbytevector& in)
{
    Bytef* o = new Bytef[2*in.size()];
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef*)in.data();
    strm.next_out = o;
    strm.avail_in = in.size();
    strm.avail_out = 2*in.size();
    if(deflateInit2(&strm, 9, Z_DEFLATED, 16+15, 8, Z_DEFAULT_STRATEGY)!=Z_OK)
        throw(string("zlib init error"));
    if(deflate(&strm, Z_FINISH)!=Z_STREAM_END)
        throw(string("zlib process error"));
    if(deflateEnd(&strm)!=Z_OK)
        throw(string("zlib finish error"));
    out.putbytes((char*)o, strm.total_out);
    delete[] o;
}

void MOTEncoder::gzip_file(const string& dst, const string & src)
{
    //cout << "gzip open: " << src << " -> " << dst << endl;
    gzFile out = gzopen(dst.c_str(), "wb");
    if (!out) {
        cerr << "gzip File open fail for file descriptor" << dst << endl;
        cerr.flush();
        return;
    }
    ifstream in(src.c_str(), ios::in | ios::binary);
    if (!in.is_open()) {
        gzclose(out);
        cerr << "gzip File open fail for " << src << endl;
        cerr.flush();
        return;
    }
    char buf[255];
    memset(buf, 0, sizeof(buf));
    while (in.good()) {
        in.read(buf, 200);
        gzwrite(out, buf, in.gcount());
    }
    gzclose(out);
    in.close();
    //cout << "gzip closed: " << src << " -> " << dst << endl;
}

void MOTEncoder::code_MOTdirectory()
{
    crcbytevector dir;
    directory.put_to(dir);
    if(flags.send_uncompressed_dir) {
        crcbytevector dg;
        dge.putDataGroupSegment(dg, directory.transport_id, dir, 6U, 0, 0, true);
        packet_encoder.makeDataUnit(packet_queue, dg);
        //cout << "sending " << dg.size() << " byte directory on packet id " << packet_encoder.packet_id << " tid " << directory.transport_id << endl;
    }

    if(flags.send_compressed_dir) {
        crcbytevector cdir, dircompressed, dg;
        compress(dircompressed, dir);
        cdir.put(1, 1); // CF compression flag
        cdir.put(0, 1); // rfu
        cdir.put(9+dircompressed.size(), 30); // entity size
        cdir.put(1, 8);
        cdir.put(0, 2); // rfu
        cdir.put(dir.size(), 30); // uncompressed data size
        cdir.put(dircompressed);
        dge.putDataGroupSegment(dg, directory.transport_id, cdir, 7U, 0, 0, true);
        packet_encoder.makeDataUnit(packet_queue, dg);
        //cout << "sending " << dg.size() << " byte compressed directory on packet id " << packet_encoder.packet_id << endl;
    }
}

void MOTEncoder::codefileheader(const MotObject & m)
{
    crcbytevector dg;
    bytevector data;
    m.putHeader(data);
    dge.putDataGroupSegment(dg, m.transport_id, data, 3U, m.object_version, 0, true);
    packet_encoder.makeDataUnit(packet_queue, dg);
}

bool MOTEncoder::scan_input_directory()
{
    bool changed = false;
#ifdef WIN32
    // TODO
#else
    while(FAMPending(&fc))
    {
        FAMEvent fe;
        FAMNextEvent(&fc, &fe);
        FAMRequest fr;
        string& dir = monitored_directories[fe.fr.reqnum];
        struct stat s;
        string f;
        if(dir=="")
            f = fe.filename;
        else
            f = dir + '/' + fe.filename;
        string p = in_dir + '/' + f;
        switch(fe.code)
        {
        case FAMDeleted:
            if(current_object != directory.objects.end() && current_object->first == f)
            {
                if(in_file>=0) { // it might have already been closed on a read error
                    lockf(in_file, F_ULOCK, 0);
                    close(in_file);
                    in_file = -1;
                }
            }
            {
                map<int,string>::iterator d=monitored_directories.end();
                for(map<int,string>::iterator i=monitored_directories.begin(); i!=monitored_directories.end(); i++)
                    if(i->second == f)
                        d = i;
                if(d==monitored_directories.end()) {
                    directory.delete_file(f);
                    current_object = directory.objects.end();
                    //cout << "deleted file " << fe.filename << " in " << dir << endl;
                } else {
                    FAMRequest fr;
                    fr.reqnum = d->first;
                    //cout << "removing directory monitor " << (FAMCancelMonitor(&fc, &fr)?"failed":"OK") << endl;
                    monitored_directories.erase(d);
                }
            }
            changed = true;
            break;
        case FAMChanged:
            if(current_object != directory.objects.end() && current_object->first == f)
            {
                if(in_file>=0) {
                    lockf(in_file, F_ULOCK, 0);
                    close(in_file);
                    in_file = -1;
                }
            }
            //cout << "changed file " << fe.filename << endl; cout.flush();
            if (stat(p.c_str(), &s) == 0) {
                directory.change_file(f, s.st_size);
            }
            changed = true;
            break;
        case FAMCreated:
        case FAMExists:
            if (stat(p.c_str(), &s) == 0) {
                if (S_ISDIR(s.st_mode)) {
                    //cout << "new dir " << fe.filename << endl;
                    if(FAMMonitorDirectory(&fc, p.c_str(), &fr, NULL))
                        throw "MOTEncoder: can't monitor directory";
                    monitored_directories[fr.reqnum] = f;
                } else {
                    //cout << "new file " << fe.filename << " size " << s.st_size << endl;
                    directory.add_file(f, s.st_size);
                }
            }
            changed = true;
            break;
        case FAMEndExist:
            cout << "FAM initialised " << fe.filename << endl;
            break;
        case FAMAcknowledge:
            cout << "FAM cancel acknowledged " << fe.filename << endl;
            break;
        case FAMStartExecuting:
        case FAMStopExecuting:
        case FAMMoved:
            cout << "unexpected fam event " << fe.code << " '" << fe.filename << "'" << endl;
            break;
        default:
            cout << "unknown fam event " << fe.code << " '" << fe.filename << "'" << endl;
        }
    }
#endif
    return changed;
}
