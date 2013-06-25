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

#include "PacketStreamMux.h"
using namespace std;
#include <iostream>
#include <time.h>
#include <timestamp.h>

PacketStreamMux::PacketStreamMux()
:StreamMux(),last_packet_component(0),dummy_sce(),tokens_per_packet(0)
{
}

PacketStreamMux::PacketStreamMux(const PacketStreamMux& s):
StreamMux(s),last_packet_component(s.last_packet_component),
dummy_sce(s.dummy_sce),tokens_per_packet(s.tokens_per_packet)
{
  for(size_t i=0; i<sce.size(); i++) {
    tokens_per_frame[i] = s.tokens_per_frame[i];
    tokens[i] = s.tokens[i];
  }
}

PacketStreamMux& PacketStreamMux::operator=(const PacketStreamMux& s)
{
  *this = s;
  last_packet_component = s.last_packet_component;
  dummy_sce = s.dummy_sce;
  for(size_t i=0; i<sce.size(); i++) {
    tokens_per_frame[i] = s.tokens_per_frame[i];
    tokens[i] = s.tokens[i];
  }
  tokens_per_packet = s.tokens_per_packet;
  return *this;
}

PacketStreamMux::~PacketStreamMux()
{
}

/*  if the target bit rate is specified and non-zero then give it this bit rate.
 *  If the total available bit rate is exceeded then scale the bit rates down to fit.
 * once this is done, share any remaining capacity equally between
 * those components with zero or unspecified target bit rates.
 * At the end, if rounding errors leave any spare capacity, share it out
 * Only an integral number of packets can be put in a frame,
 * so don't allocate the wasted space at the end of the frame.
 */
void PacketStreamMux::ReConfigure(const Stream& config)
{
  cout << "PacketStreamMux::ReConfigure" << endl;
  StreamMux::ReConfigure(config);
  int payload_size = current.packet_size+3;
  int packets_per_frame = current.bytes_per_frame / payload_size;
  int bytes_per_frame = packets_per_frame * payload_size;
  int total_bits_in_100_frames = bytes_per_frame * 8 * 100;
  int free_bits_in_100_frames = total_bits_in_100_frames;
  size_t num_components = current.component.size();
  size_t num_unspecified_components = 0;
  // add up the requests
  for(size_t i=0; i<num_components; i++) {
    int target_bits_in_100_frames = 40 * current.component[i].target_bitrate;
    if(target_bits_in_100_frames>0) {
	  // 1 token represents 1 bit in 100 frames
      tokens_per_frame[i] = target_bits_in_100_frames;
      free_bits_in_100_frames -= target_bits_in_100_frames;
    } else {
      tokens_per_frame[i] = 0;
      num_unspecified_components++;
    }
  }
  // scale if needed
  if(free_bits_in_100_frames<0) {
    double scaling = double(total_bits_in_100_frames)/double(total_bits_in_100_frames-free_bits_in_100_frames);
    free_bits_in_100_frames = total_bits_in_100_frames;
    for(size_t i=0; i<num_components; i++) {
      int bits = int(tokens_per_frame[i] * scaling);
      tokens_per_frame[i] = bits;
      free_bits_in_100_frames -= bits;
    }
  }
  // share out spare capacity
  if(free_bits_in_100_frames>0) {
    if(num_unspecified_components>0) {
      int share = free_bits_in_100_frames/num_unspecified_components;
      for(size_t i=0; i<num_components; i++) {
        if(tokens_per_frame[i] == 0) {
          tokens_per_frame[i] = share;
          free_bits_in_100_frames -= share;
        }
      }
    }
  }
  // share out remaining spare
  while(free_bits_in_100_frames>0) {
    for(size_t i=0; i<num_components; i++) {
      if(free_bits_in_100_frames == 0)
	    break;
      tokens_per_frame[i]++;
      free_bits_in_100_frames--;
    }
  }
  for(size_t i=0; i<num_components; i++) {
	tokens[i] = 0;
  }
  ServiceComponent dummy = current.component[0];
  if(num_components<4)
    dummy.packet_id = num_components;
  else
    dummy.packet_id = 3;
  dummy_sce.ReConfigure(dummy);
  last_packet_component=0;
  tokens_per_packet = 100*8*payload_size; // 1 token represents 1 bit in 100 frames
}

/*
Packets with no useful data are permitted if no packet data is available to 
fill the logical frame. The PPI shall be set to 1 and the first byte of the 
data field shall be set to 0 to indicate no useful data. The first and last
flags shall be set to 1. The continuity index shall be incremented for these
empty packets. If less than 4 sub-streams are used within the data stream
then an unused packet id shall be used. Empty packets using a packet id of 
<p> shall not be inserted during the transmission of a DRM data unit using 
the same packet id <p>
*/

void PacketStreamMux::NextFrame(bytevector& out)
{
  size_t max = static_cast<size_t>(current.bytes_per_frame);
  size_t avail = max;
  size_t payload_size = current.packet_size+3;
  // put a token to each component
  for(size_t i=0; i<sce.size(); i++) {
    tokens[i] += tokens_per_frame[i];
  }
  out.clear();
  // don't take more than 50ms for this stream
  timespec t;
  clock_getrealtime(&t);
  double now = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
  double stoptime = now + 50.0;
  // add packets until the output is full or all sce's are exhausted.
  // start from where we left off last frame
  size_t comp = last_packet_component+1;
  size_t num_failed = 0;
  while(now<stoptime)
  {
    if(comp>=sce.size()) { comp=0; }
	if(out.size()<=max)
      avail = max - out.size();
	else {
	  //cout << avail << " " << max << " " << out.size() << endl;
	  throw "output vector too big in packetstreammux";
	}
    // is there enough space to add a packet ?
    if(avail<payload_size)
      break;
    // have all sce's failed to add a packet ?
    if(num_failed==sce.size())
      break;
	if(tokens[comp]>=tokens_per_packet) {
	  size_t s = out.size();
      sce[comp]->NextFrame(out, avail, stoptime);
      // did the SCE go wild ?
	  if(out.size()>max) {
	    cerr << "output vector too big in packetstreammux component " << comp << endl;
		// and discard this rubbish
		out.resize(s);
	  }
      // did the sce add a packet?
      if(s == out.size()) {
        num_failed++;
      } else {
	    //cout << comp << ": tokens " << tokens[comp] << endl;
	    tokens[comp] -= tokens_per_packet;
        num_failed=0;
	  }
	} else {
      num_failed++;
	}
    comp++;
    clock_getrealtime(&t);
    now = 1000.0*double(t.tv_sec) + double (t.tv_nsec) / 1.0e6;
  }
  last_packet_component = comp;
  // see if we need to insert dummy packets
  if(out.size()<max) {
    avail = max - out.size();
    while(avail>=payload_size) {
      // get dummy SCE to add a packet
      dummy_sce.NextFrame(out, avail);
      avail = max - out.size();
	  //cout << "dummy" << endl;
    }
  }
  // pad out with nulls (payload not a submultiple of frame size)
  if(avail>0){
    out.insert(out.end(), avail, 0);
  }
}
