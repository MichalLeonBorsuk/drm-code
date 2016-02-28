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

#include "Announcement.h"

Announcement::Announcement():service_ref(),afs_ref(),announcement_types(0)
{
    clearConfig();
    tag="announcement_flags_carried";
}

void Announcement::clearConfig()
{
    service_ref="";
    afs_ref="";
    announcement_types=0;
}

Announcement::~Announcement()
{
    clearConfig();
}

void Announcement::GetParams(xmlNodePtr n)
{
    if(xmlStrEqual(n->name, BAD_CAST "announcement_types")) {
        for(xmlNodePtr d=n->children; d; d=d->next) {
            if(d->type==XML_ELEMENT_NODE) {
                int i=-1;
                parseUnsigned(d, "announcement_type", &i);
                if(i>=0 && i<10)
                    announcement_types |= 1<<i;
                if(i>9)
                    misconfiguration = true;
            }
        }
    }
    parseIDREF(n, "service_ref", service_ref);
    parseIDREF(n, "afs_ref", afs_ref);
}

void Announcement::ReConfigure(xmlNodePtr config)
{
    Persist::ReConfigure(config);
}

void Announcement::PutParams(xmlTextWriterPtr writer)
{
    Persist::PutParams(writer);
    xmlTextWriterStartElement(writer, BAD_CAST "announcement_types");
    for(int i=0; i<10; i++)
        if(announcement_types & (1<<i))
            PutUnsigned(writer, "announcement_type", i);
    xmlTextWriterEndElement(writer);
    if(service_ref.length()>0)
        PutString(writer, "service_ref", service_ref);
    if(afs_ref.length()>0)
        PutString(writer, "afs_ref", afs_ref);
}
