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

#ifndef _MOT_ENCODE_H
#define _MOT_ENCODE_H

#ifndef WIN32
#include <fam.h>
#endif
#include <string>
#include <map>
#include <queue>
#include "packetencoder.h"
#include "MotDirectory.h"

using namespace std;

class MOTEncoder
{
public:

  struct Flags { bool directory_mode;
           bool crc; bool send_compressed_dir; bool send_uncompressed_dir;
           bool always_send_mime_type; };

  MOTEncoder();
  MOTEncoder(const MOTEncoder&);
  MOTEncoder& operator=(const MOTEncoder&);
  ~MOTEncoder();

  void ReConfigure(
    const string& encoder_id_param,
    const string& in,
    uint16_t block_sizep,
    uint16_t packet_id, uint16_t packet_size,
    Flags& flagsp,
    vector<string>& compressible,
    // next should be const as soon as find iterator syntax for const map
     map<uint8_t,string>& profile_index
  );

  void next_packet(vector<uint8_t>& out, size_t max, double stoptime=0);

protected:

  void fill(double stoptime);
  void get_one_data_unit(uint16_t transport_id);
  void codefileheader(const MotObject& m);
  void code_MOTdirectory();
  void move_directory_iterator();

  void compress(bytevector& out, const bytevector& in);
  void gzip_file(const string& dst, const string& src);
  bool scan_input_directory();

  packetqueue packet_queue;

  string encoder_id;
  string in_dir;
  uint16_t block_size;
  PacketEncoder packet_encoder;
  MotDirectory directory;
  DataGroupEncoder dge;
  map<string, MotObject>::iterator current_object;
  Flags flags;
  int in_file;
  size_t dg_payload_size;
  size_t max_queue_depth;
  int segment;
  double last_sent_dir;
  double dir_interval;
  map<int,string> monitored_directories;;
  size_t file_bytes;
#ifndef WIN32
  FAMConnection fc;
  struct DL { DL(const FAMRequest& r, const string&p):fr(r),path(p){} FAMRequest fr; string path;};
#endif

};
#endif
