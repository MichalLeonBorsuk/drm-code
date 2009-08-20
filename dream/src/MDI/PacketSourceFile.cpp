/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable, Oliver Haffenden
 *
 * Description:
 *
 * see PacketSourceFile.h
 *
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
#ifdef _WIN32
# include <winsock2.h>
#else
# include <arpa/inet.h> // for ntohs, ntohl
#endif

#include "PacketSourceFile.h"
#include <iostream>
#include <errno.h>
#include <QTimer>
#include <QStringList>

#ifdef HAVE_LIBPCAP
# include <pcap.h>
#endif

const size_t iMaxPacketSize = 4096;
const size_t iAFHeaderLen = 10;
const size_t iAFCRCLen = 2;

CPacketSourceFile::CPacketSourceFile():pPacketSink(NULL),
pacer(NULL), last_packet_time(0),
 pf(NULL), bRaw(true)
{
}

bool
CPacketSourceFile::SetOrigin(const string& str)
{
	if(str.rfind(".pcap") == str.length()-5)
	{
#ifdef HAVE_LIBPCAP
		char errbuf[PCAP_ERRBUF_SIZE];
		pf = pcap_open_offline(str.c_str(), errbuf);
#else
        cerr << "sorry - pcap files not supported in this build" << endl;
        pf = NULL;
#endif
		bRaw = false;
	}
	else
	{
		pf = fopen(str.c_str(), "rb");
	}
	if ( pf != NULL)
	{
		pacer = new CPacer(uint64_t(400000000));
	}
	return pf != NULL;
}

CPacketSourceFile::~CPacketSourceFile()
{
	if(bRaw && pf)
		fclose((FILE*)pf);
	else if(pf)
	{
#ifdef HAVE_LIBPCAP
		pcap_close((pcap_t*)pf);
#endif
	}
}

// Set the sink which will receive the packets
void
CPacketSourceFile::SetPacketSink(CPacketSink * pSink)
{
	pPacketSink = pSink;
}

// Stop sending packets to the sink
void
CPacketSourceFile::ResetPacketSink()
{
	pPacketSink = NULL;
}

bool
CPacketSourceFile::Poll()
{
	if(pf==NULL)
		return false;
	if(pacer) pacer->wait();
	vector<_BYTE> vecbydata (iMaxPacketSize);
	int interval;
	if(bRaw)
		 readRawOrFF(vecbydata, interval);
	else
		 readPcap(vecbydata, interval);

	/* Decode the incoming packet */
	if (pPacketSink != NULL)
	{
		pPacketSink->SendPacket(vecbydata);
	}
    return true;
}

void
CPacketSourceFile::readRawOrFF(vector<_BYTE>& vecbydata, int& interval)
{
	char header[8];
	size_t len2;

	vecbydata.resize(0); // in case we don't find anything

	// get the sync bytes
	fread(header, sizeof(header), 1, (FILE *) pf);
	// guess file framing
	size_t len = ntohl(*(uint32_t*)&header[4])/8;
	char c = header[4];
	header[4]=0;
	if(strcmp("fio_", header)==0)
	{
		 fread(header, sizeof(header), 1, (FILE *) pf);
		 len2 = ntohl(*(uint32_t*)&header[4])/8;
		 header[4]=0;
		 if(strcmp("time", header)==0)
		 {
				if(len2 != 8)
				{
					cout << "weird length in FF " << len2 << " expected 8" << endl;
					fclose((FILE *) pf);
					pf = 0;
					return;
				}
				len -= 16;
				// read the time tag packet payload
				fread(header, sizeof(header), 1, (FILE *) pf);
				// read the next tag packet header
				fread(header, sizeof(header), 1, (FILE *) pf);
				len2 = ntohl(*(uint32_t*)&header[4])/8;
				header[4]=0;
		 }

		 if(strcmp("afpf", header)==0)
		 {
				// get the first 8 bytes of the payload
				fread(header, sizeof(header), 1, (FILE *) pf);
				len -= 8;
				len -= len2;
		 }
		 else
		 {
				cout << "bad tag packet in FF " << header << endl;
				fclose((FILE *) pf);
				pf = 0;
				return;
		 }
	}
	else
	{
		 len = 0;
		 header[4]=c;
	}
	// if we get here, either its not FF or we read the FF headers
	// TODO: add PF and re-synch on AF bytes
	if (header[0] != 'A' || header[1] != 'F') // Not an AF file - return.
	{
		 fclose((FILE *) pf);
		 pf = 0;
		 return;
	}

	// get the length
	size_t iAFPayloadLen = ntohl(*(uint32_t*)&header[2]);
	size_t iAFPacketLen = iAFPayloadLen + iAFHeaderLen + iAFCRCLen;

	if (iAFPacketLen > iMaxPacketSize)
	{
		 // throw?
		 fclose((FILE *) pf);
		 pf = 0;
		 return;
	}

	// initialise the output vector
	vecbydata.resize(iAFPacketLen);

	size_t i;

	// Copy header into output vector
	for (i=0; i<sizeof(header); i++)
	{
		 vecbydata[i] = header[i];
	}

	// Copy payload into output vector
	_BYTE data;
	for (i=sizeof(header); i<iAFPacketLen; i++)
	{
		 fread(&data, 1, sizeof(_BYTE), (FILE *)pf);
		 vecbydata[i] = data;
	}
	// skip any other nested tag packets (e.g. time)
	while(len > 0)
	{
		 fread(header, 8, sizeof(_BYTE), (FILE *) pf);
		 len2 = ntohl(*(uint32_t*)&header[4])/8;
		 header[4] = 0;
		 fseek((FILE*)pf, len2, SEEK_CUR);
		 if(len>len2+8)
				len -= len2+8;
	}
	interval = 400;
}

void
CPacketSourceFile::readPcap(vector<_BYTE>& vecbydata, int& interval)
{
	int link_len = 0;
	const _BYTE* pkt_data = NULL;
	timeval packet_time = { 0, 0 };
#ifdef HAVE_LIBPCAP
	struct pcap_pkthdr *header;
	int res;
	const u_char* data;
	/* Retrieve the packet from the file */
	if((res = pcap_next_ex( (pcap_t*)pf, &header, &data)) != 1)
	{
		 pcap_close((pcap_t*)pf);
		 pf = NULL;
		 return;
	}
	int lt = pcap_datalink((pcap_t*)pf);
	pkt_data = (_BYTE*)data;
	/* 14 bytes ethernet header */
	if(lt==DLT_EN10MB)
	{
		link_len=14;
	}
	/* raw IP header ? */
	if(lt==DLT_RAW)
	{
		link_len=0;
	}
	packet_time = header->ts;
#endif
	if(pkt_data == NULL)
		return;

	/* 4n bytes IP header, 8 bytes UDP header */
	uint8_t proto = pkt_data[link_len+9];
	if(proto != 0x11) // UDP
	{
		 /* not a UDP datagram, skip it */
		 return;
	}
	int udp_ip_hdr_len = 4*(pkt_data[link_len] & 0x0f) + 8;
	int ip_packet_len = ntohs(*(uint16_t*)&pkt_data[link_len+2]);
	int data_len = ip_packet_len - udp_ip_hdr_len;
	vecbydata.resize (data_len);
	for(int i=0; i<data_len; i++)
		 vecbydata[i] = pkt_data[link_len+udp_ip_hdr_len+i];
	uint64_t pt;
	pt = 1000*uint64_t(packet_time.tv_sec);
	pt += uint64_t(packet_time.tv_usec)/1000;
	if(last_packet_time == 0)
	{
		 last_packet_time = pt - 400;
	}
	//pacer->changeInterval(1000000ULL*(pt - last_packet_time));
	interval = pt - last_packet_time;
	last_packet_time = pt;
}
