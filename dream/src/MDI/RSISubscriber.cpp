/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Oliver Haffenden
 *
 * Description:
 *
 *	This class represents a particular consumer of RSI information and supplier of
 *  RCI commands. There could be several of these. The profile is a property of the
 *  particular subscriber and different subscribers could have different profiles.
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

#include "RSISubscriber.h"
#include "TagPacketGenerator.h"
#include "Pft.h"
#ifdef QT_NETWORK_LIB
# include "PacketSocketQT.h"
#else
# include "PacketSocketNull.h"
#endif
#include <iostream>

CRSISubscriber::CRSISubscriber(CPacketSink *pSink) : pPacketSink(pSink),
	cProfile('0'), iSubsamplingFactor(1), bNeedPft(false),
	fragment_size(0),
	TagPacketDecoderRSCIControl(),
	mapPresets(),
	pDRMReceiver(0),
	AFPacketGenerator(),
	bUseAFCRC(true),
	sequence_counter(0),
	iSubsamplingCounter(0)
{
	TagPacketDecoderRSCIControl.SetSubscriber(this);
}

void CRSISubscriber::SetReceiver(ReceiverInterface *pReceiver)
{
	pDRMReceiver = pReceiver;
	TagPacketDecoderRSCIControl.SetReceiver(pReceiver);
}

void CRSISubscriber::SetProfile(const char c)
{
	// special values from '1' to '9' refer to presets
	if (c >= '1' && c <= '9')
	{
		cProfile = mapPresets[int(c - '0')].cProfile;
		SetSubsamplingFactor(mapPresets[int(c - '0')].iSubsamplingFactor);
	}
	else
	{
		cProfile = c;
	}
}

void CRSISubscriber::SetSubsamplingFactor(const int i)
{
	iSubsamplingFactor = i;
}

void CRSISubscriber::DefinePreset(const int iPresetNum, const int cPro, const int iFactor)
{
	mapPresets[iPresetNum] = CRSIPreset(cPro, iFactor);
}

void CRSISubscriber::SetPFTFragmentSize(const int iFrag)
{
    if(iFrag>0)
    {
        fragment_size = iFrag;
        bNeedPft = true;
    }
    else
        bNeedPft = false;
}

void CRSISubscriber::TransmitPacket(CTagPacketGenerator& Generator)
{

	// Don't do anything if this is one of the frames to discard in the subsampling
	// This includes not incrementing the AF sequence number.
	// (The dlfc will increase though to show how many were discarded)
	if (++iSubsamplingCounter < iSubsamplingFactor)
	{
		return;
	}

	iSubsamplingCounter = 0;

	// Special profile '0' means no output packets will be generated
	if (cProfile == '0')
	{
		return;
	}

	if (pPacketSink != 0)
	{
	 	Generator.SetProfile(cProfile);
		vector<_BYTE> packet = AFPacketGenerator.GenAFPacket(bUseAFCRC, Generator);
		if(bNeedPft)
		{
			vector< vector<_BYTE> > packets;
			CPft::MakePFTPackets(packet, packets, sequence_counter, fragment_size);
			sequence_counter++;
			for(size_t i=0; i<packets.size(); i++)
				pPacketSink->SendPacket(packets[i]);
		}
		else
			pPacketSink->SendPacket(packet);
	}
}


/* implementation of function from CPacketSink interface - process incoming RCI commands */
void CRSISubscriber::SendPacket(const vector<_BYTE>& vecbydata, uint32_t, uint16_t)
{
	CVectorEx<_BINARY> vecbidata;
	vecbidata.Init(vecbydata.size()*BITS_BINARY);
	vecbidata.ResetBitAccess();
	for(size_t i=0; i<vecbydata.size(); i++)
		vecbidata.Enqueue(vecbydata[i], BITS_BINARY);
	CTagPacketDecoder::Error err = TagPacketDecoderRSCIControl.DecodeAFPacket(vecbidata);
	if(err != CTagPacketDecoder::E_OK)
		cerr << "bad RSCI Control Packet Received" << endl;
}


/* TODO wrap a sendto in a class and store it in pPacketSink */
CRSISubscriberSocket::CRSISubscriberSocket(CPacketSink *pSink):CRSISubscriber(pSink),pSocket(NULL)
,uIf(0),uAddr(0),uPort(0)
{
#ifdef QT_NETWORK_LIB
	pSocket = new CPacketSocketQT;
#else
	pSocket = new CPacketSocketNull;
#endif
	pPacketSink = pSocket;
}

CRSISubscriberSocket::~CRSISubscriberSocket()
{
	delete pSocket;
}

bool CRSISubscriberSocket::SetDestination(const string& str)
{
    return pSocket->SetDestination(str);
}

bool CRSISubscriberSocket::GetDestination(string& str)
{
	/* want the canonical version so incoming can match */
	if(pSocket)
		return pSocket->GetDestination(str);
	return false;
}

bool CRSISubscriberSocket::SetOrigin(const string& str)
{
	if(pSocket)
	{
		// Delegate to socket
		bool bOK = pSocket->SetOrigin(str);

		if (bOK)
			// Connect socket to the MDI decoder
			pSocket->SetPacketSink(this);
		return bOK;
	}
	return false;
}

CRSISubscriberFile::CRSISubscriberFile(): CRSISubscriber(NULL), pPacketSinkFile(NULL)
{
	/* override the subscriber back to NULL to prevent Cpro doing anything */
	TagPacketDecoderRSCIControl.SetSubscriber(NULL);
}

bool CRSISubscriberFile::SetDestination(const string& strFName)
{
    string dest = strFName;
	if(pPacketSink)
	{
		delete pPacketSink;
		pPacketSink = NULL;
		pPacketSinkFile = NULL;
	}
	string ext;
	size_t p = strFName.rfind('.');
	if (p != string::npos)
		ext = strFName.substr(p + 1);
	if (ext == "pcap")
		pPacketSinkFile = new CPacketSinkPcapFile;
	else if(ext == "ff")
	{
		pPacketSinkFile = new CPacketSinkFileFraming;
		dest.erase(p);
	}
	else
		pPacketSinkFile = new CPacketSinkRawFile;
	if(pPacketSinkFile)
	{
		pPacketSink = pPacketSinkFile;
		return pPacketSinkFile->SetDestination(dest);
	}
	return false;
}

bool CRSISubscriberFile::GetDestination(string& strFName)
{
	if(pPacketSinkFile)
		return pPacketSinkFile->GetDestination(strFName);
	return false;
}

void CRSISubscriberFile::StartRecording()
{
	if(pPacketSinkFile)
		pPacketSinkFile->StartRecording();
}

void CRSISubscriberFile::StopRecording()
{
	if(pPacketSinkFile)
		pPacketSinkFile->StopRecording();
}
