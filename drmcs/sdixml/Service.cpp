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

#include "Service.h"
#include "ServiceComponent.h"
#include <iostream>

const char* Service::LANGUAGES[]= {
    "No language specified",
    "Arabic", "Bengali", "Chinese (Mandarin)",
    "Dutch", "English", "French", "German",
    "Hindi", "Japanese", "Javanese", "Korean",
    "Portuguese", "Russian", "Spanish", "Other language", NULL
};

// Programme Type
const char* Service::PROGRAMMETYPES[]= {
    "No programme type", "News", "Current Affairs", "Information",
    "Sport", "Education", "Drama", "Culture", "Science", "Varied",
    "Pop Music", "Rock Music", "Easy Listening Music", "Light Classical", "Serious Classical",
    "Other Music", "Weather/meteorology", "Finance/Business", "Children's programmes",
    "Social Affairs", "Religion", "Phone In", "Travel", "Leisure", "Jazz Music",
    "Country Music", "National Music", "Oldies Music", "Folk Music", "Documentary",
    "Not used1", "Not used2", NULL
};

Service::Service():Persist(),
    service_label(),
    service_descriptor(),
    service_identifier(),
    country(),
    language(),
    language_long(),
    audio_ref(),
    data_ref(),
    afs_ref(),
    conditional_access(),
    ca_system_identifier(),
    ca_data(),
    announcement()
{
    clearConfig();
    tag="service";
}

Service::Service(const Service& s)
    :Persist(s),
     service_label(s.service_label),
     service_descriptor(s.service_descriptor),
     service_identifier(s.service_identifier),
     country(s.country),
     language(s.language),
     language_long(s.language_long),
     audio_ref(s.audio_ref),
     data_ref(s.data_ref),
     afs_ref(s.afs_ref),
     conditional_access(s.conditional_access),
     ca_system_identifier(s.ca_system_identifier),
     ca_data(s.ca_data),
     announcement(s.announcement)
{
}

Service& Service::operator=(const Service& s)
{
    *reinterpret_cast<Persist *>(this) = Persist(s);
    service_label = s.service_label;
    service_descriptor = s.service_descriptor;
    service_identifier = s.service_identifier;
    country = s.country;
    language = s.language;
    language_long = s.language_long;
    audio_ref = s.audio_ref;
    data_ref = s.data_ref;
    afs_ref = s.afs_ref;
    conditional_access = s.conditional_access;
    ca_system_identifier = s.ca_system_identifier;
    ca_data = s.ca_data;
    announcement = s.announcement;
    return *this;
}

Service::~Service()
{
    clearConfig();
}

void Service::clearConfig()
{
    Persist::clearConfig();
    misconfiguration = false;
    service_label.clear();
    service_descriptor=-1;
    service_identifier.clear();
    country.clear();
    language = -1;
    language_long.clear();
    audio_ref.clear();
    data_ref.clear();
    afs_ref.clear();
    conditional_access = false;
    ca_system_identifier=-1;
    ca_data.clear();
    announcement.clear();
}

void Service::GetParams(xmlNodePtr n)
{
    xmlChar* s=xmlNodeGetContent(n);
    string v;
    if(s) {
        v = (char*)s;
        xmlFree(s);
    }
    if(!xmlStrcmp(n->name, BAD_CAST "label")) {
        // careful, UTF-8, max 16 visible characters!
        service_label = v;
    }
    if(!xmlStrcmp(n->name,BAD_CAST "service_identifier")) {
        parseHexBinary(n, "service_identifier", service_identifier);
        size_t len=service_identifier.size();
        if(len>3) {
            misconfiguration=true;
            cerr << "service identifier more than 3 characters" << endl;
        }
        if(len<3) {
            vector<uint8_t>::iterator theIterator = service_identifier.begin();
            service_identifier.insert( theIterator, 3-len, 0);
        }
    }
    if(!xmlStrcmp(n->name,BAD_CAST "country")) {
        country = v;
        if(country.size()!=2) {
            cerr << "country code not 2 characters" << endl;
            misconfiguration=true;
        }

    }
    parseUnsigned(n, "language", &language);
    if(!xmlStrcmp(n->name,BAD_CAST "language_long")) {
        language_long = v;
        if(language_long.size()!=3)
            misconfiguration=true;
    }
    parseUnsigned(n, "service_descriptor", &service_descriptor);
    if(!xmlStrcmp(n->name,BAD_CAST "conditional_access")) {
        for(xmlNodePtr d=n->children; d; d=d->next) {
            if(d->type==XML_ELEMENT_NODE) {
                parseUnsigned(d, "ca_system_identifer", &ca_system_identifier);
                parseHexBinary(d, "ca_data", ca_data);
                /* TODO (jfbc#2#): ca data can't be more than 126 bytes - check */
            }
        }
    }
    parseIDREF(n, "audio_ref", audio_ref);
    parseIDREF(n, "data_ref", data_ref);
    if(xmlStrEqual(n->name,BAD_CAST "afs_refs")) {
        for(xmlNodePtr d=n->children; d; d=d->next) {
            if(d->type==XML_ELEMENT_NODE) {
                string r;
                parseIDREF(d, "afs_ref", r);
                afs_ref.push_back(r);
            }
        }
    }
    if(xmlStrEqual(n->name,BAD_CAST "announcements")) {
        for(xmlNodePtr d=n->children; d; d=d->next) {
            if(d->type==XML_ELEMENT_NODE) {
                Announcement a;
                a.ReConfigure(d);
                announcement.push_back(a);
            }
        }
    }
}

void Service::PutParams(xmlTextWriterPtr writer)
{
    Persist::PutParams(writer);
    PutString(writer, "label", service_label);
    PutHexBinary(writer, "service_identifier", service_identifier);
    PutString(writer, "country", country);
    PutUnsignedEnum(writer, "language", LANGUAGES, language);
    PutString(writer, "language_long", language_long);
    // needed for DRM, not for AMSS
    if(service_descriptor != -1)
        PutUnsigned(writer, "service_descriptor", service_descriptor);
    if(audio_ref.length()>0)
        PutString(writer, "audio_ref", audio_ref);
    if(data_ref.length()>0)
        PutString(writer, "data_ref", data_ref);
    if(afs_ref.size()>0) {
        xmlTextWriterStartElement(writer, BAD_CAST "afs_refs");
        for(size_t i=0; i<afs_ref.size(); i++)
            PutString(writer, "afs_ref", afs_ref[i]);
        xmlTextWriterEndElement(writer);
    }
    if(conditional_access) {
        xmlTextWriterStartElement(writer, BAD_CAST "conditional_access");
        PutUnsigned(writer, "ca_system_identifer", ca_system_identifier);
        xmlTextWriterStartElement(writer, BAD_CAST "ca_data");
        for(size_t i=0; i<ca_data.size(); i++) {
            xmlTextWriterWriteFormatString(writer, "%02x", ca_data[i]);
        }
        xmlTextWriterEndElement(writer);
    }
    if(announcement.size()>0) {
        xmlTextWriterStartElement(writer, BAD_CAST "announcements");
        for(size_t i=0; i<announcement.size(); i++)
            announcement[i].Configuration(writer);
        xmlTextWriterEndElement(writer);
    }
}
