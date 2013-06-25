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

#ifndef _TCPDATASCE_H
#define _TCPDATASCE_H

#include <libxml/encoding.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <ServiceComponentEncoder.h>
#include <queue>
#include <string>
#include <platform.h>
#include "sockets.h"

using namespace std;

class CTcpDataSCE : public ServiceComponentEncoder
{
public:
    CTcpDataSCE();
    void clearConfig();
	virtual void NextFrame(bytevector& buf, size_t max, double stoptime=0);
    virtual void ReConfigure(const ServiceComponent&);

    client_socket sock;
};
#endif
