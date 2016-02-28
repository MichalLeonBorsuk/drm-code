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

#include<iostream>

#include "Region.h"

Region::Region()
    :Persist(),zone(),area()
{
    clearConfig();
    tag="region";
}

Region::Region(const Region& a)
    :Persist(a),zone(a.zone),area(a.area)
{
}

Region& Region::operator=(const Region& r)
{
    *reinterpret_cast<Persist *>(this) = Persist(r);
    zone = r.zone;
    area = r.area;
    return *this;
}

void Region::clearConfig()
{
    zone.clear();
    area.clear();
}

Region::~Region()
{
    clearConfig();
}

void Region::GetParams(xmlNodePtr n)
{
    if(xmlStrEqual(n->name, BAD_CAST "zones")) {
        for(xmlNodePtr c=n->children; c; c=c->next) {
            if(c->type==XML_ELEMENT_NODE) {
                int i=-1;
                parseUnsigned(c, "ciraf", &i);
                if(i>=0) {
                    zone.push_back(i);
                }
            }
        }
    }
    if(xmlStrEqual(n->name, BAD_CAST "areas")) {
        for(xmlNodePtr c=n->children; c; c=c->next) {
            if(xmlStrEqual(c->name, BAD_CAST "area")) {
                Area a;
                for(xmlNodePtr d=c->children; d; d=d->next) {
                    if(d->type==XML_ELEMENT_NODE) {
                        parseSigned(d, "latitude", &a.latitude);
                        parseSigned(d, "longitude", &a.longitude);
                        parseUnsigned(d, "latitude_extent", &a.latitude_extent);
                        parseUnsigned(d, "longitude_extent", &a.longitude_extent);
                    }
                }
                area.push_back(a);
            }
        }
    }
}

void Region::ReConfigure(xmlNodePtr config)
{
    Persist::ReConfigure(config);
    misconfiguration=false;
    for(size_t i=0; i<zone.size(); i++)
        if(zone[i]<1 || zone[i]>85)
            misconfiguration=true;
    if(area.size()>15)
        misconfiguration=true;
    for(size_t i=0; i<area.size(); i++) {
        if(area[i].longitude<-180 || area[i].longitude>179)
            misconfiguration=true;
        if(area[i].latitude<-90 || area[i].latitude>90)
            misconfiguration=true;
        if(area[i].latitude+area[i].latitude_extent>90)
            misconfiguration=true;
    }
}

void Region::PutParams(xmlTextWriterPtr writer)
{
    Persist::PutParams(writer);
    if(zone.size()>0) {
        xmlTextWriterStartElement(writer, BAD_CAST "zones");
        for(size_t i=0; i<zone.size(); i++)
            PutUnsigned(writer, "ciraf", zone[i]);
        xmlTextWriterEndElement(writer);
    }
    xmlTextWriterStartElement(writer, BAD_CAST "areas");
    for(size_t i=0; i<area.size(); i++) {
        xmlTextWriterStartElement(writer, BAD_CAST "area");
        PutSigned(writer, "longitude", area[i].longitude);
        PutUnsigned(writer, "longitude_extent", area[i].longitude_extent);
        PutSigned(writer, "latitude", area[i].latitude);
        PutUnsigned(writer, "latitude_extent", area[i].latitude_extent);
        xmlTextWriterEndElement(writer);
    }
    xmlTextWriterEndElement(writer);
}
