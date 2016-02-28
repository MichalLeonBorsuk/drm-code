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

#include "afs.h"
#include <iostream>
using namespace std;


AFS::AFS():Persist(),
    schedule(),
    region(),
    afs_mux_list(),
    afs_service_list(),
    guide()
{
    tag = "afs";
    misconfiguration = false;
}

AFS::AFS(const AFS& a):Persist(a),
    schedule(a.schedule),
    region(a.region),
    afs_mux_list(a.afs_mux_list),
    afs_service_list(a.afs_service_list),
    guide(a.guide)
{
}

AFS& AFS::operator=(const AFS& a)
{
    schedule = a.schedule;
    region = a.region;
    afs_mux_list = a.afs_mux_list;
    afs_service_list = a.afs_service_list;
    guide = a.guide;
    return *this;
}


AFS::~AFS()
{
    clearConfig();
}

void AFS::clearConfig()
{
    // regions and schedules arrays are indexed from 1, not 0
    region.resize(1);
    schedule.resize(1);
    afs_mux_list.clear();
    afs_service_list.clear();
    guide.clear();
    misconfiguration = false;
}

/*
  	  <regions>
	  <schedules>
	  <afs_multiplex_lists>
	  <afs_service_lists>
*/

void AFS::GetParams(xmlNodePtr n)
{
    if(xmlStrEqual(n->name,BAD_CAST "regions")) {
        for(xmlNodePtr c=n->children; c; c=c->next) {
            if(c->type==XML_ELEMENT_NODE) {
                Region r;
                r.ReConfigure(c);
                region.push_back(r);
                misconfiguration |= r.misconfiguration;
            }
        }
    }
    if(xmlStrEqual(n->name,BAD_CAST "schedules")) {
        for(xmlNodePtr c=n->children; c; c=c->next) {
            if(c->type==XML_ELEMENT_NODE) {
                Schedule s;
                s.ReConfigure(c);
                schedule.push_back(s);
                misconfiguration |= s.misconfiguration;
            }
        }
    }
    if(xmlStrEqual(n->name,BAD_CAST "afs_multiplex_lists")) {
        for(xmlNodePtr c=n->children; c; c=c->next) {
            if(c->type==XML_ELEMENT_NODE) {
                AFSMuxlist l;
                l.ReConfigure(c);
                afs_mux_list.push_back(l);
                misconfiguration |= l.misconfiguration;
            }
        }
    }
    if(xmlStrEqual(n->name,BAD_CAST "afs_service_lists")) {
        for(xmlNodePtr c=n->children; c; c=c->next) {
            if(c->type==XML_ELEMENT_NODE) {
                AFSServicelist l;
                l.ReConfigure(c);
                afs_service_list.push_back(l);
                misconfiguration |= l.misconfiguration;
            }
        }
    }
    if(xmlStrEqual(n->name,BAD_CAST "guide")) {
        for(xmlNodePtr c=n->children; c; c=c->next) {
            if(c->type==XML_ELEMENT_NODE) {
                ServiceGroup s;
                s.ReConfigure(c);
                guide.push_back(s);
                misconfiguration |= s.misconfiguration;
            }
        }
    }
}

void AFS::PutParams(xmlTextWriterPtr writer)
{
    Persist::PutParams(writer);
    if(region.size()>1) {
        xmlTextWriterStartElement(writer, BAD_CAST "regions");
        for(size_t i=1; i<region.size(); i++)
            region[i].Configuration(writer);
        xmlTextWriterEndElement(writer);
    }
    if(schedule.size()>1) {
        xmlTextWriterStartElement(writer, BAD_CAST "schedules");
        for(size_t i=1; i<schedule.size(); i++)
            schedule[i].Configuration(writer);
        xmlTextWriterEndElement(writer);
    }
    if(afs_mux_list.size()>0) {
        xmlTextWriterStartElement(writer, BAD_CAST "afs_multiplex_lists");
        for(size_t i=0; i<afs_mux_list.size(); i++) {
            afs_mux_list[i].Configuration(writer);
        }
        xmlTextWriterEndElement(writer);
    }
    if(afs_service_list.size()>0) {
        xmlTextWriterStartElement(writer, BAD_CAST "afs_service_lists");
        for(size_t i=0; i<afs_service_list.size(); i++) {
            afs_service_list[i].Configuration(writer);
        }
        xmlTextWriterEndElement(writer);
    }
    if(guide.size()>0) {
        xmlTextWriterStartElement(writer, BAD_CAST "guide");
        for(size_t i=0; i<guide.size(); i++) {
            guide[i].Configuration(writer);
        }
        xmlTextWriterEndElement(writer);
    }
}

// resolve the schedule & region references
void AFS::PostReConfigure()
{
    for(size_t i=0; i<afs_mux_list.size(); i++) {
        AFSMuxlist& nl = afs_mux_list[i];
        for(size_t j=0; j<nl.fg.size(); j++) {
            FrequencyGroup& fg = nl.fg[j];
            if(fg.region_ref.empty()) {
                fg.region_id = 0; // unspecified
            } else {
                for(size_t l=1; l<region.size(); l++) {
                    if(fg.region_ref==region[l].id)
                        fg.region_id = int(l);
                }
            }
            if(fg.schedule_ref.empty()) {
                fg.schedule_id = 0; // unspecified
            } else {
                for(size_t l=1; l<schedule.size(); l++) {
                    if(fg.schedule_ref==schedule[l].id)
                        fg.schedule_id = int(l);
                }
            }
        }
    }
    for(size_t i=0; i<afs_service_list.size(); i++) {
        AFSServicelist& nl = afs_service_list[i];
        for(size_t j=0; j<nl.sg.size(); j++) {
            ServiceGroup& sg = nl.sg[j];
            if(sg.region_ref.empty()) {
                sg.region_id = 0; // unspecified
            } else {
                for(size_t l=1; l<region.size(); l++) {
                    if(sg.region_ref==region[l].id)
                        sg.region_id = int(l);
                }
            }
            if(sg.schedule_ref.empty()) {
                sg.schedule_id = 0; // unspecified
            } else {
                for(size_t l=1; l<schedule.size(); l++) {
                    if(sg.schedule_ref==schedule[l].id)
                        sg.schedule_id = int(l);
                }
            }
        }
    }
}
