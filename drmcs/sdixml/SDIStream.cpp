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

#include "SDIStream.h"
#include <iostream>
using namespace std;

const char* Stream::PROTECTION_LEVELS[]= {"eep","eep_high","uep","hierarchical", NULL};
const char* Stream::TYPES[]= {"audio", "data_stream", "data_packet_mode", NULL};

Stream::Stream()
    :  Persist(),
       bytes_per_frame(-1),
       bytes_better_protected(-1),
       packet_size(-1),
       error_protection(-1),
       stream_type(unspecified),
       component()
{
    tag="stream";
}

Stream::Stream(const Stream& s)
    :Persist(s),
     bytes_per_frame(s.bytes_per_frame),
     bytes_better_protected(s.bytes_better_protected),
     packet_size(s.packet_size),
     error_protection(s.error_protection),
     stream_type(s.stream_type),
     component(s.component)
{
}

Stream& Stream::operator=(const Stream& s)
{
    *reinterpret_cast<Persist *>(this) = Persist(s);
    bytes_per_frame = s.bytes_per_frame;
    bytes_better_protected = s.bytes_better_protected;
    packet_size = s.packet_size;
    error_protection = s.error_protection;
    stream_type = s.stream_type;
    component = s.component;
    return *this;
}

Stream::~Stream()
{
}

void Stream::clearConfig()
{
    Persist::clearConfig();
    bytes_per_frame=-1;
    bytes_better_protected=-1;
    packet_size=-1;
    error_protection=-1;
    component.clear();
}

void Stream::ReConfigure(xmlNodePtr config)
{
    Persist::ReConfigure(config);
    xmlChar *type = xmlGetProp(config, BAD_CAST "type");
    if(component.size()==0) {
        cerr << "stream with no components" << endl;
        misconfiguration = true;
        return;
    }
    if(xmlStrEqual(type, BAD_CAST TYPES[0])) {
        stream_type=audio;
        switch (component.size()) {
        case 1:
            if(component[0].type!=ServiceComponent::audio_sce) {
                cerr << "audio stream with one component which is not audio" << endl;
                misconfiguration = true;
            }
            break;
        case 2:
            // see if we need to swap the order of components - audio must be first
            if(component[0].type==ServiceComponent::text_sce) {
                ServiceComponent txt = component[0];
                component[0] = component[1];
                component[1] = txt;
            }
            if(component[0].type!=ServiceComponent::audio_sce) {
                cerr << "audio stream with two components and no audio" << endl;
                misconfiguration = true;
            }
            if(component[1].type!=ServiceComponent::text_sce) {
                cerr << "audio stream with two components and no text" << endl;
                misconfiguration = true;
            }
            break;
        default:
            cerr << "audio stream with more than two components" << endl;
            misconfiguration = true;
        }
    }
    else if(xmlStrEqual(type, BAD_CAST TYPES[1])) {
        stream_type=data_stream;
        if(component.size()!=1 || component[0].type!=ServiceComponent::data_stream_sce) {
            cerr << "data stream with no data components" << endl;
            misconfiguration = true;
        }
    }
    else if(xmlStrEqual(type, BAD_CAST TYPES[2])) {
        stream_type=data_packet_mode;
        for(size_t i=0; i<component.size(); i++) {
            misconfiguration |= component[i].type!=ServiceComponent::data_packet_mode_sce;
        }
    }
    else {
        cerr << "unknown stream type " << type << endl;
        misconfiguration = true;
    }
    if(bytes_per_frame==-1) {
        cerr << "missing bytes_per_frame" << endl;
        misconfiguration = true;
    }
    if(bytes_better_protected==-1) {
        cerr << "missing bytes_better_protected" << endl;
        misconfiguration = true;
    }
    if(error_protection==-1) {
        cerr << "missing error_protection field" << endl;
        misconfiguration = true;
    }
    if(id.length()==0) {
        cerr << "missing id" << endl;
        misconfiguration = true;
    }
    xmlFree(type);
}

void Stream::PostReConfigure()
{
    switch(stream_type) {
    case audio:
        if(component.size()==1) { // audio only
            component[0].bytes_per_frame = bytes_per_frame;
            component[0].bytes_better_protected = bytes_better_protected;
        } else { // audio + text
            component[0].bytes_per_frame = bytes_per_frame-4;
            component[0].bytes_better_protected = bytes_better_protected;
            component[1].bytes_per_frame = 4;
            component[1].bytes_better_protected = 0;
        }
        break;
    case data_stream:
        component[0].bytes_per_frame = bytes_per_frame;
        component[0].bytes_better_protected = bytes_better_protected;
        component[0].packet_mode = 0;
        break;
    case data_packet_mode:
        for(size_t i=0; i<component.size(); i++) {
            component[i].bytes_per_frame = bytes_per_frame;
            component[i].bytes_better_protected = bytes_better_protected;
            component[i].packet_mode = 1;
            component[i].packet_size = packet_size;
            component[i].packet_id = static_cast<int>(i);
        }
        break;
    case unspecified:
        ;
    }
}

void Stream::GetParams(xmlNodePtr n)
{
    if(xmlStrEqual(n->name, BAD_CAST "components")) {
        for(xmlNodePtr c=n->children; c; c=c->next) {
            if(c->type==XML_ELEMENT_NODE) {
                ServiceComponent s;
                s.ReConfigure(c);
                component.push_back(s);
            }
        }
    }
    parseUnsigned(n, "bytes_per_frame", &bytes_per_frame);
    parseUnsigned(n, "bytes_better_protected", &bytes_better_protected);
    parseUnsigned(n, "packet_length", &packet_size);
    parseEnum(n, "error_protection", &error_protection, PROTECTION_LEVELS);
}

void Stream::PutParams(xmlTextWriterPtr writer)
{
    xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "type", "%s", TYPES[stream_type]);
    Persist::PutParams(writer);
    xmlTextWriterStartElement(writer, BAD_CAST "components");
    for(size_t i=0; i<component.size(); i++) {
        component[i].Configuration(writer);
    }
    xmlTextWriterEndElement(writer);
    PutEnum(writer, "error_protection", PROTECTION_LEVELS, error_protection);
    xmlTextWriterStartComment(writer);
    xmlTextWriterWriteString(writer, BAD_CAST
                             "The value of the bytes_per_frame tag represents the total number of bytes");
    xmlTextWriterWriteString(writer, BAD_CAST
                             " in the MSC for this stream, including for audio streams, 4 bytes for");
    xmlTextWriterWriteString(writer, BAD_CAST " text messages if these are active.");
    xmlTextWriterEndComment(writer);
    PutUnsigned(writer, "bytes_per_frame", bytes_per_frame);
    PutUnsigned(writer, "bytes_better_protected", bytes_better_protected);
    if(packet_size!=-1)
        PutUnsigned(writer, "packet_length", packet_size);
}
