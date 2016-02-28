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

#include <libxml/xmlwriter.h>
#include <string.h>
#include "ServiceComponent.h"
#include <iostream>
using namespace std;

ServiceComponent::ServiceComponent() :
    Persist(),
    label(),
    source_selector(),
    encoder_id(),
    implementor(),
    typestring(),
    type(),
    bytes_per_frame(-1),
    bytes_better_protected(-1),
    loop(false),
    enhancement(-1),
    audio_coding(UNDEFINED), SBR(-1), audio_mode(-1),
    audio_sampling_rate(-1),
    crc(-1),hvxc_rate(-1),
    coder_field(-1),
    application_domain(-1),
    data_unit_indicator(-1),
    application_data(),
    packet_mode(-1),
    packet_size(-1),
    packet_id(-1),
    target_bitrate(-1)
{
    tag="component";
}

ServiceComponent::ServiceComponent(const ServiceComponent& e) :
    Persist(e),
    label(e.label),
    source_selector(e.source_selector),
    encoder_id(e.encoder_id),
    implementor(e.implementor),
    typestring(e.typestring),
    type(e.type),
    bytes_per_frame(e.bytes_per_frame),
    bytes_better_protected(e.bytes_better_protected),
    loop(e.loop),
    enhancement(e.enhancement),
    audio_coding(e.audio_coding),
    SBR(e.SBR),
    audio_mode(e.audio_mode),
    audio_sampling_rate(e.audio_sampling_rate),
    crc(e.crc),
    hvxc_rate(e.hvxc_rate),
    coder_field(e.coder_field),
    application_domain(e.application_domain),
    data_unit_indicator(e.data_unit_indicator),
    application_data(e.application_data),
    packet_mode(e.packet_mode),
    packet_size(e.packet_size),
    packet_id(e.packet_id),
    target_bitrate(e.target_bitrate)
{
}

ServiceComponent& ServiceComponent::operator=(const ServiceComponent& e)
{
    *reinterpret_cast<Persist *>(this) = Persist(e);
    label=e.label;
    source_selector=e.source_selector;
    encoder_id=e.encoder_id;
    implementor=e.implementor;
    typestring=e.typestring;
    type=e.type;
    bytes_per_frame=e.bytes_per_frame;
    bytes_better_protected=e.bytes_better_protected;
    loop=e.loop;
    enhancement=e.enhancement;
    audio_coding=e.audio_coding;
    SBR=e.SBR;
    audio_mode=e.audio_mode;
    audio_sampling_rate=e.audio_sampling_rate;
    crc=e.crc;
    hvxc_rate=e.hvxc_rate;
    coder_field=e.coder_field;
    application_domain=e.application_domain;
    data_unit_indicator=e.data_unit_indicator;
    application_data=e.application_data;
    packet_mode=e.packet_mode;
    packet_size=e.packet_size;
    packet_id=e.packet_id;
    target_bitrate=e.target_bitrate;
    return *this;
}

ServiceComponent::~ServiceComponent()
{
}

void ServiceComponent::clearConfig()
{
    Persist::clearConfig();
    source_selector.clear();
    label.clear();
    encoder_id.clear();
    implementor.clear();
    // don't clear typestring - its part of the class definition!
    bytes_per_frame=-1;
    bytes_better_protected=-1;
    enhancement=-1;
    loop=true;
    AudioclearConfig();
    TextclearConfig();
    DataclearConfig();
}

void ServiceComponent::ReConfigure(xmlNodePtr config)
{
    // need this early
    xmlChar *s = xmlGetProp(config, BAD_CAST "type");
    if(s) {
        typestring = (char*)s;
        xmlFree(s);
    } else {
        typestring = "";
        cerr << "ServiceComponent has not type" << endl;
        misconfiguration = true;
    }
    Persist::ReConfigure(config);
    // override default - better to check for exceptions
    misconfiguration = false;
    s = xmlGetProp(config, BAD_CAST "implementor");
    if(s) {
        implementor = (char*)s;
        xmlFree(s);
    }
    s = xmlGetProp(config, BAD_CAST "label");
    if(s) {
        label = (char*)s;
        xmlFree(s);
    }
    //if(implementor.length()==0) misconfiguration = true;
    //if(encoder_id.length()==0) misconfiguration = true;
    if(source_selector.length()==0) misconfiguration = true;
    if(typestring == "audio") {
        type=audio_sce;
        AudioReConfigure(config);
    } else if(typestring == "text") {
        type=text_sce;
        //TextReConfigure(config);
    } else if(typestring == "data_packet") {
        type=data_packet_mode_sce;
        DataReConfigure(config);
    } else if(typestring == "data_stream") {
        type=data_stream_sce;
        DataReConfigure(config);
    }
}

void ServiceComponent::GetParams(xmlNodePtr n)
{
    string v;
    xmlChar *s = xmlNodeGetContent(n);
    if(s) {
        v = (char*)s;
        xmlFree(s);
    }
    parseBool(n, "enhancement_flag", &enhancement);
    if(!xmlStrcmp(n->name, BAD_CAST "encoder_id")) {
        encoder_id = v;
    }
    if(!xmlStrcmp(n->name, BAD_CAST "source_selector")) {
        source_selector = v;
    }
    parseUnsigned(n, "bytes_better_protected", &bytes_better_protected);
    parseUnsigned(n, "bytes_per_frame", &bytes_per_frame);
    if(typestring == "audio") {
        AudioGetParams(n);
    } else if(typestring == "text") {
        //TextGetParams(n);
    } else if(typestring == "data_packet") {
        DataGetParams(n);
    } else if(typestring == "data_stream") {
        DataGetParams(n);
    }
}

void ServiceComponent::PutParams(xmlTextWriterPtr writer)
{
    xmlTextWriterWriteAttributeNS(writer, BAD_CAST "xsi", BAD_CAST "type", NULL, BAD_CAST typestring.c_str());
    if(implementor.length()>0)
        xmlTextWriterWriteAttribute(writer, BAD_CAST "implementor", BAD_CAST implementor.c_str());
    if(label.length()>0)
        xmlTextWriterWriteAttribute(writer, BAD_CAST "label", BAD_CAST label.c_str());
    Persist::PutParams(writer);
    PutBool(writer, "enhancement_flag", enhancement);
    PutString(writer, "encoder_id", encoder_id);
    PutString(writer, "source_selector", source_selector);
    if(typestring == "audio") {
        AudioPutParams(writer);
    } else if(typestring == "text") {
        //TextPutParams(writer);
    } else if(typestring == "data_packet") {
        DataPutParams(writer);
    } else if(typestring == "data_stream") {
        DataPutParams(writer);
    }
#if 1
    xmlTextWriterStartComment(writer);
    xmlTextWriterWriteFormatString(writer, "bytes_per_frame %d", bytes_per_frame);
    xmlTextWriterWriteFormatString(writer, ", bytes_better_protected %d", bytes_better_protected);
    xmlTextWriterEndComment(writer);
#endif
}

const char* ServiceComponent::sAACMode[]= {"mono", "lc stereo", "stereo", "any", NULL};
const char* ServiceComponent::CoderType[]= {"aac", "celp", "hvxc", "any", NULL};

void ServiceComponent::AudioclearConfig()
{
    audio_coding=UNDEFINED;
    SBR=-1;
    audio_mode=-1;
    audio_sampling_rate=-1;
    enhancement=-1;
    hvxc_rate=-1;
    crc=-1;
    coder_field=-1;
}

void ServiceComponent::AudioReConfigure(xmlNodePtr config)
{
    // in case the file had indexes instead of rates
    switch(audio_sampling_rate) {
    case 0:
        audio_sampling_rate = 8000;
        break;
    case 1:
        audio_sampling_rate = 12000;
        break;
    case 2:
        audio_sampling_rate = 16000;
        break;
    case 3:
        audio_sampling_rate = 24000;
        break;
    }
    switch(audio_coding) {
    case AUDIO_CODING_AAC:
        if(audio_mode==-1) {
            cerr << "missing audio mode" << endl;
            misconfiguration=true;
        }
        if(audio_sampling_rate!=12000
                && audio_sampling_rate!=24000) {
            misconfiguration=true;
            cerr << "bad sample rate" << endl;
        }
        if(SBR==-1) {
            misconfiguration=true;
            cerr << "missing SBR flag" << endl;
        }
        break;
    case AUDIO_CODING_CELP:
        if(audio_sampling_rate!=8000
                && audio_sampling_rate!=16000)
            misconfiguration=true;
        if(coder_field==-1)
            misconfiguration=true;
        break;
    case AUDIO_CODING_HVXC:
        if(audio_sampling_rate==-1)
            audio_sampling_rate=8000;
        if(audio_sampling_rate!=8000) misconfiguration=true;
        if(coder_field==-1)
            misconfiguration=true;
        break;
    default:
        misconfiguration=true;
    }
    string msg;
    if(!AudioValidate(msg)) {
        misconfiguration = true;
        cerr << msg << endl;
    }
}

bool ServiceComponent::AudioValidate(string& message)
{
    return true;
}

void ServiceComponent::AudioGetParams(xmlNodePtr n)
{
    if(typestring.length()>0)
        parseEnum(BAD_CAST typestring.c_str(), &audio_coding, CoderType);
    parseUnsigned(n, "audio_coding", &audio_coding);
    parseBool(n, "sbr", &SBR);
    parseUnsigned(n, "sampling_rate", &audio_sampling_rate);
    parseUnsigned(n, "audio_mode", &audio_mode);
    parseUnsigned(n, "coder_field", &coder_field);
    //parseUnsigned(n, "hvxc_rate", &hvxc_rate);
    //parseBool(n, "crc", &crc);
}

void ServiceComponent::AudioPutParams(xmlTextWriterPtr writer)
{
    PutUnsignedEnum(writer, "audio_coding", CoderType, audio_coding);
    PutBool(writer, "sbr", SBR);
    PutUnsigned(writer, "sampling_rate", audio_sampling_rate);
    PutUnsigned(writer, "audio_mode", audio_mode);
    if(coder_field>=0)
        PutUnsigned(writer, "coder_field", coder_field);
    /*switch(audio_coding) {
    case AUDIO_CODING_AAC:
    	PutUnsignedEnum(writer, "audio_mode", sAACMode, audio_mode);
    	break;
    case AUDIO_CODING_CELP:
    	//PutBool(writer, "crc", crc);
    	PutUnsigned(writer, "coder_field", coder_field);
    	break;
    case AUDIO_CODING_HVXC:
    	//PutUnsigned(writer, "hvxc_rate", hvxc_rate);
    	//PutBool(writer, "crc", crc);
    	PutUnsigned(writer, "coder_field", coder_field);
    }*/
}

void ServiceComponent::TextclearConfig()
{
    bytes_per_frame=MSG_SUB_SEG_LEN;
    bytes_better_protected = 0;
}

void ServiceComponent::DataclearConfig()
{
    application_data.clear();
    application_domain=-1;
    data_unit_indicator=-1;
    target_bitrate=-1;
    enhancement=0;
}

void ServiceComponent::DataReConfigure(xmlNodePtr config)
{
    string msg;
    if(!DataValidate(msg)) {
        misconfiguration = true;
        cerr << msg << endl;
    }
}

bool ServiceComponent::DataValidate(string& message)
{
    switch(application_domain) {
    case -1: // not specified
        message = "missing application domain for component " + label;
        return false;
        break;
    case 0: // DRM
        if(application_data.size()<2) {
            message = "missing application id for component " + label;
            return false;
        }
        break;
    case 1: // DAB
        if(application_data.size()<2) {
            message = "missing user application type for component " + label;
        }
    default: // we don't know anything about the future!
        ;
    }
    return true;
}

void ServiceComponent::DataGetParams(xmlNodePtr n)
{
    misconfiguration=false;
    parseUnsigned(n, "application_domain", &application_domain);
    parseHexBinary(n, "application_data", application_data);
    parseUnsigned(n, "data_unit_indicator", &data_unit_indicator);
    parseUnsigned(n, "target_bitrate", &target_bitrate);
}

void ServiceComponent::DataPutParams(xmlTextWriterPtr writer)
{
    PutUnsigned(writer, "application_domain", application_domain);
    PutHexBinary(writer, "application_data", application_data);
    PutUnsigned(writer, "data_unit_indicator", data_unit_indicator);
    PutUnsigned(writer, "target_bitrate", target_bitrate);
    string msg;
    if(!DataValidate(msg)) {
        xmlTextWriterStartComment(writer);
        xmlTextWriterWriteFormatString(writer, "%s", msg.c_str());
        xmlTextWriterEndComment(writer);
    }
#if 1
    xmlTextWriterStartComment(writer);
    xmlTextWriterWriteFormatString(writer, "packet length %d", packet_size);
    xmlTextWriterWriteFormatString(writer, ", id %d", packet_id);
#endif
}
