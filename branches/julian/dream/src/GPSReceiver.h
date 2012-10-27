/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Andrew Murphy
 *
 * Description:
 *	See GPSReceiver.cpp
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

#ifndef _GPSRECEIVER_H_
#define _GPSRECEIVER_H_

#include "Parameter.h"
#include "util/Settings.h"
#ifdef QT_NETWORK_LIB
# include <QTcpSocket>
# include <QThread>
# include <QMutex>
# include <QTimer>
#else
# define slots
#endif

class CGPSReceiver
#ifdef QT_NETWORK_LIB
 : public QObject
{
	Q_OBJECT
#else
{
#endif
public:
	CGPSReceiver(CParameter&, const string& host, int port);
	virtual ~CGPSReceiver();
	/* dummy assignment operator to help MSVC8 */
	CGPSReceiver& operator=(const CGPSReceiver&)
	{ throw "should not happen"; return *this;}

protected:

	void open();
	void close();

	void DecodeGPSDReply(string Reply);
	void DecodeString(char Command, string Value);
	void DecodeO(string Value);
	void DecodeY(string Value);

	static const unsigned short c_usReconnectIntervalSeconds;

	CParameter&	Parameters;
#ifdef QT_NETWORK_LIB
	QTcpSocket*	m_pSocket;
	QTimer*		m_pTimer;
	QTimer*		m_pTimerDataTimeout;
#endif
	int		m_iCounter;
	string		m_sHost;
	int		m_iPort;

public slots:

	void slotInit();
	void slotConnected();
	void slotTimeout();
	void slotReadyRead();
	void slotSocketError(int);

};

#endif // !defined(_GPSRECEIVER_H_)
