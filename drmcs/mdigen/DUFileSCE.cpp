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

#include "DUFileSCE.h"
#include <platform.h>
#include <iostream>
#include "timestamp.h"
#include <fcntl.h>

// TODO - make the DataGroup part optional by private config
void DUFileSCE::ReConfigure(const ServiceComponent& config)
{
  bool ss_changed=false;
  if(current.source_selector != config.source_selector)
  {
    f.ignore();
    ss_changed = true;
  }
  PacketSCE::ReConfigure(config);
  next_data_unit=data_unit.end();
  packet_encoder.ReConfigure(config);
  dge.Configure(true, false, false);
  file_pos=0;
  if(ss_changed)
    f.monitor(config.source_selector);
}

void DUFileSCE::NextFrame(bytevector &out, size_t max, double stoptime)
{
  if(max<(unsigned int)payload_size)
  {
    return;
  }
  if(packet_queue.empty())
  {
      if(f.changed() || data_unit.size()==0)
        read_file(stoptime);
      fill(stoptime);
  }
  if(packet_queue.empty())
  {
      return;
  }
  if(packet_queue.front().size()<=max)
  {
    out.put(packet_queue.front());
    packet_queue.pop();
  } else
  {
    cerr << "JMLSCE: packet queue size mismatch with packet mux" << endl; cerr.flush();
  }
}

void DUFileSCE::fill(double stoptime)
{
    if(data_unit.size()==0)
        return;
    if(next_data_unit==data_unit.end())
    {
        next_data_unit = data_unit.begin();
    }
    crcbytevector out;
    //cout << "fill: " << next_data_unit->size() << endl;
    dge.putDataGroup(0, out, *next_data_unit, 0); // TODO make configurable
    packet_encoder.makeDataUnit(packet_queue, out);
    next_data_unit++;
    /*
    timespec t;
    clock_getrealtime(&t);
    double now_ms = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
    while(packet_queue.size() < max_queue_depth && now_ms < stoptime)
    {
        if(next_data_unit==data_unit.end())
        {
            next_data_unit = data_unit.begin();
        }
        crcbytevector out;
        dge.putDataGroup(0, out, *next_data_unit, 0); // TODO make configurable
        packet_encoder.makeDataUnit(packet_queue, out);
        next_data_unit++;
        clock_getrealtime(&t);
        now_ms = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
    }
    */
}

/* see if we want to open the file.
   if we do, empty the du vector, and read as many DUs as we can in
    the time available.
    if the file is already open, then carry on reading DUs while we have
    time.
*/
void DUFileSCE::read_file(double stoptime)
{
    if(!current.misconfiguration)
    {
        cout << "open " << current.source_selector << endl;
        int mode = O_RDWR; // need RW for locking
#ifdef WIN32
	mode |= O_BINARY;
#endif
        bool ok = f.open(mode);
        if(!ok)
        {
          cerr << "can't open " << current.source_selector << endl;
          return;
        }
        data_unit.clear();
    }
    f.seek(file_pos, SEEK_SET);
    timespec t;
    clock_getrealtime(&t);
    double now_ms = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
    bool end_of_file=false;
    while(now_ms < stoptime && !end_of_file)
    {
        uint16_t du_len=0;
        size_t n = f.read(&du_len, 2);
        if(n==2)
        {
            du_len = ntohs(du_len);
            bytevector du;
            du.resize(du_len);
            n += f.read(const_cast<uint8_t*>(du.data()), du_len);
            if(n==size_t(du_len+2))
            {
                data_unit.push_back(du);
                file_pos += n;
            }
        }
        if(n==0)
        {
            end_of_file = true;
        }
        clock_getrealtime(&t);
        now_ms = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
    }
    f.close();
    if(end_of_file)
        file_pos = 0;
    cout << "DU " << data_unit.size() << " objects" << endl;
    next_data_unit = data_unit.begin();
}
