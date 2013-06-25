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

#include "packetencoder.h"

/*

6.6 Packet mode
Data services generally consist of either streams of information, in either synchronous or asynchronous form, or files of
information. A generalized packet delivery system allows the delivery of asynchronous streams and files for various
services in the same data stream and allows the bit rate of the (synchronous) data stream to be shared on a
frame-by-frame basis between the various services. Services can be carried by a series of single packets or as a series of
data units. A data unit is a series of packets that are considered as one entity with regard to error handling - one received
errored packet within a data unit causes the whole data unit to be rejected. This mechanism can be used to transfer files
and also to allow simpler synchronization of asynchronous streams. The carriage of data applications is described in
TS 101 968 [8].
The size of a packet mode data logical frame shall be a multiple of the packet size. The maximum length of a data unit
is 8 215 bytes.

6.6.1 Packet structure
The packet is made up as follows:
- header 8 bits.
- data field n bytes.
- CRC 16 bits.
The header contains information to describe the packet.
The data field contains the data intended for a particular service. The length of the data field is indicated by use of data
entity 5, see clause 6.4.3.6.
Cyclic Redundancy Check (CRC): this 16-bit CRC shall be calculated on the header and the data field. It shall use the
generator polynomial G16 (x) = x16 + x12 + x5 + 1 (see annex D).
6.6.1.1 Header
The header consists of the following fields:
- first flag 1 bit.
- last flag 1 bit.
- packet Id 2 bits.
- Padded Packet Indicator (PPI) 1 bit.
- Continuity Index (CI) 3 bits.
The following definitions apply:
First flag, Last flag: these flags are used to identify particular packets which form a succession of packets. The flags
are assigned as follows:
First
flag
Last
flag
The packet is:
0 0 : an intermediate packet;
0 1 : the last packet of a data unit;
1 0 : the first packet of a data unit;
1 1 : the one and only packet of a data unit.
Packet Id: this 2-bit field indicates the Packet Id of this packet.
Padded Packet Indicator: this 1-bit flag indicates whether the data field carries padding or not, as follows:
0: no padding is present: all data bytes in the data field are useful;
1: padding is present: the first byte gives the number of useful data bytes in the data field.
Continuity index: this 3-bit field shall increment by one modulo-8 for each packet with this packet Id.
6.6.1.2 Data field
The data field contains the useful data intended for a particular service.
If the Padded Packet Indicator (PPI) field of the header is 0, then all bytes of the data field are useful bytes.
If the PPI is 1 then the first byte indicates the number of useful bytes that follow, and the data field is completed with
padding bytes of value 0x00.

Packets with no useful data are permitted if no packet data is available to fill the logical frame. The PPI shall be set to 1
and the first byte of the data field shall be set to 0 to indicate no useful data. The first and last flags shall be set to 1. The
continuity index shall be incremented for these empty packets. If less than 4 sub-streams are used within the data stream
then an unused packet id shall be used. Empty packets using a packet id of <p> shall not be inserted during the
transmission of a DRM data unit using the same packet id <p>.
6.6.2 Asynchronous streams
Asynchronous streams can be used to transport byte-oriented information. Both single packets and data units can be
used to transport asynchronous streams.
Applications that use the single packet transport mechanism shall be able to deal with missing data packets. The first
and last flags indicate intermediate packets.
Applications that use the data unit transport mechanism can carry a collection of bytes that are related in a data unit and
then make use of the error handling of data units for synchronization purposes.
6.6.3 Files
The file may be carried in a data unit.
Applications that use this transport mechanism shall provide a mechanism to identify each object.
The first and last flags are used to indicate the series of packets that make up the data unit. The continuity index is used
to determine whether any intermediate packets have been lost.
6.6.4 Choosing the packet length
A data stream for packet mode may contain one or more packets per logical frame, and the packets may belong to one
or more services. However, all packets contained in the stream shall have the same length to minimize the propagation
of errors. The choice of the packet length depends on various factors, but the following should be taken into account:
- The overhead of signalling the header and CRC is fixed per packet. Therefore the larger the packet, the lower
the ratio of overhead to useful data.
- The amount of padding carried in packets is related to the size of the files compared to the packet size or the
transit delay requirements for asynchronous streams. Large packets are less efficient at transporting many
small objects.

*/

void PacketEncoder::ReConfigure(const ServiceComponent& config)
{
  // note - packet encoder continuity index (ci) not reset if packet id not changed
  if(packet_id != config.packet_id)
    ci = 0;
  packet_id=config.packet_id;
  packet_size=config.packet_size;
}

void PacketEncoder::makePacket(crcbytevector& packet, bytevector& in)
{
  bytevector::iterator p  = in.begin(), q  = in.end();
  makePacket(packet, p, q, true, true);
}

void PacketEncoder::makePacket(crcbytevector& packet,
              bytevector::iterator& from, bytevector::iterator& to,
              bool first, bool last)
{
  int bytes = int(to-from);
  uint16_t padding=packet_size;
  if(bytes>=0 && bytes<=packet_size)
    padding -= bytes;
  else {
    return; // should throw?
  }
  packet.crc.reset();
  packet.put(first?1:0, 1); // first indicator
  packet.put(last?1:0, 1); // last indicator
  packet.put(packet_id, 2); // packet id
  if(padding==0) {
    packet.put(0, 1); // padding indicator
    packet.put(ci++, 3);
    while(from!=to) {
      uint64_t c = *from;
      from++;
      packet.put(c, 8);
    }
  } else {
    packet.put(1, 1); // padding indicator
    packet.put(ci++, 3);
    packet.put(packet_size-padding, 8);
    while(from!=to) {
      uint64_t c = *from++;
      packet.put(c, 8);
    }
    for(int i=1; i<padding; i++){
      packet.put(0, 8);
    }
  }
  packet.put(packet.crc.result(), 16);
}

void PacketEncoder::makeDataUnit(packetqueue& out, bytevector& in)
{
  bool first, last;
  first=true;
  last=false;
  bytevector::iterator p = in.begin();
  while(p<in.end())  
  {
    crcbytevector packet;
    bytevector::iterator q;
    if((in.end()-p)<=packet_size) {
      last=true;
      q=in.end();
	} else {
	  q = p+packet_size;
	}
    makePacket(packet, p, q, first, last);
    out.push(packet);
    first=false;
    p=q;
  }
}
