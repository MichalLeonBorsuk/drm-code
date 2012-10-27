/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright(c) 2004
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden
 *
 * Description:
 *
 * This is an implementation of the CPacketSocket interface that wraps up a QT socket.
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or(at your option) any later
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

#include "PacketSocketQT.h"
#include <QStringList>
#include <QTimer>
#include <iostream>
#include <sstream>
#include <errno.h>

#ifdef _WIN32
/* Always include winsock2.h before windows.h */
# include <winsock2.h>
# include <ws2tcpip.h>
# include <windows.h>
#else
# include <netinet/in.h>
# include <arpa/inet.h>
typedef int SOCKET;
# define SOCKET_ERROR				(-1)
# define INVALID_SOCKET				(-1)
#endif

CPacketSocketQT::CPacketSocketQT():
	pPacketSink(NULL), HostAddrOut(), iHostPortOut(-1),Socket()
{
	/* Connect the "activated" signal */
	QObject::connect(&Socket,SIGNAL(readyRead()),this,SLOT(readPendingDatagrams()));
}

CPacketSocketQT::~CPacketSocketQT()
{
}

// Set the sink which will receive the packets
void
CPacketSocketQT::SetPacketSink(CPacketSink * pSink)
{
	pPacketSink = pSink;
}

// Stop sending packets to the sink
void
CPacketSocketQT::ResetPacketSink(void)
{
	pPacketSink = NULL;
}

// Send packet to the socket
void
CPacketSocketQT::SendPacket(const vector < _BYTE > &vecbydata, uint32_t addr, uint16_t port)
{
	int bytes_written;
	/* Send packet to network */
	if(addr==0)
		bytes_written = Socket.writeDatagram((char*)&vecbydata[0], vecbydata.size(), HostAddrOut, iHostPortOut);
	else
		bytes_written = Socket.writeDatagram((char*)&vecbydata[0], vecbydata.size(), QHostAddress(addr), port);
	/* should we throw an exception or silently accept? */
	/* the most likely cause is that we are sending unicast and no-one
	   is listening, or the interface is down, there is no route */
	if(bytes_written == -1)
	{
		QAbstractSocket::SocketError x = Socket.error();
		if(x != QAbstractSocket::NetworkError)
			qDebug("error sending packet");
	}
}

bool
CPacketSocketQT::SetDestination(const string & strNewAddr)
{
	/* syntax
	   1:  <port>                send to port on localhost
	   2:  <ip>:<port>           send to port on host or port on m/c group
	   3:  <ip>:<ip>:<port>      send to port on m/c group via interface
	 */
	/* Init return flag and copy string in QT-String "QString" */
	int ttl = 127;
	bool bAddressOK = true;
	bool portOK;
	QString addr(strNewAddr.c_str());
	QStringList parts = addr.split(":");
	switch(parts.count())
	{
	case 1:
		HostAddrOut.setAddress(QHostAddress::LocalHost);
		iHostPortOut = parts[0].toUInt(&portOK);
		bAddressOK = portOK;
		break;
	case 2:
		bAddressOK = HostAddrOut.setAddress(parts[0]);
		iHostPortOut = parts[1].toUInt(&portOK);
		bAddressOK &= portOK;
    	if(setsockopt(Socket.socketDescriptor(), IPPROTO_IP, IP_TTL,
				(char*)&ttl, sizeof(ttl))==SOCKET_ERROR)
			bAddressOK = false;
		break;
	case 3:
		{
			QHostAddress AddrInterface;
			AddrInterface.setAddress(parts[0]);
			bAddressOK = HostAddrOut.setAddress(parts[1]);
			iHostPortOut = parts[2].toUInt(&portOK);
			bAddressOK &= portOK;
			const SOCKET s = Socket.socketDescriptor();
			uint32_t mc_if = htonl(AddrInterface.toIPv4Address());
			if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF,
						(char *) &mc_if, sizeof(mc_if)) == SOCKET_ERROR)
				bAddressOK = false;
		if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL,
						(char*) &ttl, sizeof(ttl)) == SOCKET_ERROR)
				bAddressOK = false;
		}
		break;
	default:
		bAddressOK = false;
	}
	return bAddressOK;
}


bool
CPacketSocketQT::GetDestination(string & str)
{
	stringstream s;
	s << HostAddrOut.toString().toStdString() << ":" << iHostPortOut;
	str = s.str();
	return true;
}


bool
CPacketSocketQT::SetOrigin(const string & strNewAddr)
{
	/* syntax
	   1:  <port>
	   2:  <group ip>:<port>
	   3:  <interface ip>:<group ip>:<port>
	   4:  <interface ip>::<port>
	   5:  :<group ip>:<port>
	 */
	int iPort=-1;
	QHostAddress AddrGroup, AddrInterface;
	QString addr(strNewAddr.c_str());
	QStringList parts = addr.split(":");
	bool ok=true;
	switch(parts.count())
	{
	case 1:
		iPort = parts[0].toUInt(&ok);
		break;
	case 2:
		iPort = parts[1].toUInt(&ok);
		ok &= AddrGroup.setAddress(parts[0]);
		break;
	case 3:
		iPort = parts[2].toUInt(&ok);
		if(parts[0].length() > 0)
			ok &= AddrInterface.setAddress(parts[0]);
		if(parts[1].length() > 0)
			ok &= AddrGroup.setAddress(parts[1]);
		break;
	default:
		ok = false;
	}

	if(ok ==false)
	{
		return false;
	}

	/* Multicast ? */

	uint32_t gp = AddrGroup.toIPv4Address();

	if(gp == 0)
	{
		/* Initialize the listening socket. */
		Socket.bind(AddrInterface, iPort);
	}
	else if((gp & 0xe0000000) == 0xe0000000)	/* multicast! */
	{
		struct ip_mreq mreq;

		/* Initialize the listening socket. Host address is 0 -> "INADDR_ANY" */
		bool ok = Socket.bind(QHostAddress::Any, iPort,
			QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
		if(ok == false)
		{
			//QAbstractSocket::Error x = Socket.error();
			throw CGenErr("Can't bind to port to receive packets");
		}

		mreq.imr_multiaddr.s_addr = htonl(AddrGroup.toIPv4Address());
		mreq.imr_interface.s_addr = htonl(AddrInterface.toIPv4Address());

		const SOCKET s = Socket.socketDescriptor();
		int n = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char *) &mreq,
							sizeof(mreq));
		if(n == SOCKET_ERROR)
		{
			throw
				CGenErr(string
						("Can't join multicast group to receive packets: ") +
						 strerror(errno));
		}
	}
	else /* one address specified, but not multicast - listen on a specific interface */
	{
		/* Initialize the listening socket. */
		Socket.bind(AddrGroup, iPort);
	}
	return true;
}

void CPacketSocketQT::readPendingDatagrams()
{
	while (Socket.hasPendingDatagrams()) {
		vector<_BYTE> datagram;
		datagram.resize(Socket.pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		Socket.readDatagram((char*)&datagram[0], datagram.size(),
		&sender, &senderPort);

		if(pPacketSink != NULL)
		{
			pPacketSink->SendPacket(datagram, sender.toIPv4Address(), senderPort);
		}
	}
}
