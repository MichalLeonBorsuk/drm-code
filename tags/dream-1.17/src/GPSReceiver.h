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

#if !defined(_GPSRECEIVER_H_)
#define _GPSRECEIVER_H_

#include "Parameter.h"
#include "util/Settings.h"
#if QT_VERSION < 0x040000
# include <qsocket.h>
#else
# include <QTcpSocket>
#endif
#include <qthread.h>
#if QT_VERSION >= 0x030000
# include <qmutex.h>
#endif
#include <qtimer.h>

class CGPSReceiver : public QObject
{
    Q_OBJECT
public:
    CGPSReceiver(CParameter&, CSettings&);
    virtual ~CGPSReceiver();

protected:

    void open();
    void close();

    void DecodeGPSDReply(string Reply);
    void DecodeString(char Command, string Value);
    void DecodeO(string Value);
    void DecodeY(string Value);

    static const unsigned short c_usReconnectIntervalSeconds;

    CParameter&	Parameters;
    CSettings&	m_Settings;
#if QT_VERSION < 0x040000
    QSocket*	m_pSocket;
#else
    QTcpSocket*	m_pSocket;
#endif
    QTimer*		m_pTimer;
    QTimer*		m_pTimerDataTimeout;
    int			m_iCounter;
    string		m_sHost;
    int			m_iPort;

public slots:

    void slotInit();
    void slotConnected();
    void slotTimeout();
    void slotReadyRead();
    void slotSocketError(int);

};

#endif // !defined(_GPSRECEIVER_H_)
