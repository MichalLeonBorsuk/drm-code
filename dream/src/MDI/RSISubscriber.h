/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Oliver Haffenden
 *
 * Description:
 *	see RSISubscriber.cpp
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

#ifndef RSI_SUBSCRIBER_H_INCLUDED
#define RSI_SUBSCRIBER_H_INCLUDED

#include "../GlobalDefinitions.h"
#include "TagPacketDecoderRSCIControl.h"
#include "PacketInOut.h"
#include "PacketSinkFile.h"
#include "PacketInOut.h"
#include "AFPacketGenerator.h"

class CPacketSink;
class ReceiverInterface;
class CTagPacketGenerator;

class CRSIPreset
{
public:
	CRSIPreset (const char c = '0', const int i = 1) {cProfile = c; iSubsamplingFactor = i;}
	char	cProfile;
	char	iSubsamplingFactor;
};

class CRSISubscriber : public CPacketSink
{
public:
	CRSISubscriber(CPacketSink *pSink = NULL);

	/* provide a pointer to the receiver for incoming RCI commands */
	/* leave it set to NULL if you want incoming commands to be ignored */
	void SetReceiver(ReceiverInterface *pReceiver);

	/* Set the profile for this subscriber - could be different for different subscribers */
	void SetProfile(const char c);
	char GetProfile(void) const {return cProfile;}

	/* Each subscriber can also do subsampling, i.e. only transmit one in N frames. This sets the ratio */
	void SetSubsamplingFactor(const int i);

	/* Store a profile and subsampling factor in a preset 1-9 */
	void DefinePreset(const int iPresetNum, const int cPro, const int iFactor);

	void SetPFTFragmentSize(const int iFrag=-1);

	/* Generate and send a packet */
	void TransmitPacket(CTagPacketGenerator& Generator);

	void SetAFPktCRC(const bool bNAFPktCRC) {bUseAFCRC = bNAFPktCRC;}


	/* from CPacketSink interface */
	virtual void SendPacket(const vector<_BYTE>& vecbydata, uint32_t addr=0, uint16_t port=0);

protected:
	CPacketSink *pPacketSink;
	char cProfile;
	int iSubsamplingFactor;
	bool bNeedPft;
    size_t fragment_size;

	CTagPacketDecoderRSCIControl TagPacketDecoderRSCIControl;

	map<int, CRSIPreset> mapPresets;

private:
	ReceiverInterface *pDRMReceiver;
	CAFPacketGenerator AFPacketGenerator;

	bool bUseAFCRC;
	uint16_t sequence_counter;
	int iSubsamplingCounter;
};


class CRSISubscriberSocket : public CRSISubscriber
{
public:
	CRSISubscriberSocket(CPacketSink *pSink = NULL);
	virtual ~CRSISubscriberSocket();

	bool SetOrigin(const string& str);
	bool SetDestination(const string& str);
	bool GetDestination(string& addr);

private:
	CPacketSocket* pSocket;
	string strDestination;
	uint32_t uIf, uAddr;
	uint16_t uPort;
};


class CRSISubscriberFile : public CRSISubscriber
{
public:
	CRSISubscriberFile();

	bool SetDestination(const string& strFName);
	void StartRecording();
	void StopRecording();

	bool GetDestination(string& addr);
private:
	CPacketSinkFile* pPacketSinkFile;
};

#endif

