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

#ifndef _SERVICE_H
#define _SERVICE_H

#include "persist.h"
#include "Announcement.h"

using namespace std;
  /* The FAC service parameters are as follows:
     • Service identifier 24 bits
     • Short identifier 2 bits
     • Audio CA indication 1 bit
     • Language 4 bits
     • Audio/Data flag 1 bit
     • Service descriptor 5 bits
     • Data CA indication 1 bit
     • Rfa 6 bits

  */

class Service: public Persist
{
public:
    Service();
    Service(const Service& s);
    Service& operator=(const Service& s);
    virtual ~Service();
    virtual void clearConfig();
    virtual void GetParams(xmlNodePtr n);
	virtual void PutParams(xmlTextWriterPtr writer);

    static const char* LANGUAGES[];  
    static const char* PROGRAMMETYPES[];  

    string service_label;
    int service_descriptor;
    bytev service_identifier;
    string country;
    int language;
    string language_long;
    string audio_ref, data_ref;
    vector<string> afs_ref;
    bool conditional_access;
    int ca_system_identifier;
    bytev ca_data;
    vector<Announcement> announcement;

protected:
};


#endif
