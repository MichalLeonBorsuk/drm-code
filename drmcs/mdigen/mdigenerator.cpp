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

#include <iostream>
#include <iomanip>
#include <time.h>

#include "mdigenerator.h"

#include "BBCSceFactory.h"
#include "libxml/xinclude.h"
#include "libxml/xmlsave.h"

DcpDestinations::DcpDestinations()
{
}

DcpDestinations::~DcpDestinations()
{
    for(map<string,DcpOut*>::iterator i = dests.begin(); i!=dests.end(); i++)
    {
        if(i->second)
        {
            delete i->second;
            i->second = NULL;
        }
    }
}

void DcpDestinations::add(const string& uri)
{
    DcpOut* out = new DcpOut();
    out->ReConfigure(uri);
    dests[uri]=out;
    cout << "new dest " << uri << endl;
}

void DcpDestinations::remove(const string& uri)
{
    map<string,DcpOut*>::iterator d = dests.find(uri);
    if(d!=dests.end())
    {
        delete d->second;
        dests.erase(d);
    }
}

bool DcpDestinations::exists(const string& uri)
{
    return dests.find(uri)!=dests.end();
}

void DcpDestinations::sendFrame(const tagpacketlist& frame, const vector<string>&tag_tx_order, uint16_t af_seq)
{
    for(map<string,DcpOut*>::iterator i = dests.begin(); i!=dests.end(); i++)
        i->second->sendFrame(frame, tag_tx_order, af_seq);
}

Mdigen::~Mdigen()
{
}


void Mdigen::ReConfigure(const string& sdi_file)
{
    cout << "reading main config from " << sdi_file << endl;
    xmlDocPtr mdi_config_doc = xmlParseFile(sdi_file.c_str());
    if(mdi_config_doc == NULL) {
        throw string("missing configuration file ") + sdi_file;
    }
    int n = xmlXIncludeProcessTree(mdi_config_doc->children);
    if(n<0) {
        throw string("errors in XInclude processing");
    }
    cout << "XInclude processed " << n << " files" << endl;
    xmlSaveCtxtPtr ctxt = xmlSaveToFilename("processed.xml", "UTF-8", 0);
    xmlSaveTree(ctxt,	mdi_config_doc->children);
    xmlSaveClose(ctxt);

    mux_config.ReConfigure(mdi_config_doc->children);

    xmlFreeDoc(mdi_config_doc);
    xmlTextWriterPtr writer;
    writer=xmlNewTextWriterFilename("test.xml", 0);
    xmlTextWriterSetIndent(writer, 1);
    xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);
    mux_config.Configuration(writer);
    xmlTextWriterEndDocument(writer);
    xmlFreeTextWriter(writer);

    if(mux_config.misconfiguration==true) {
        throw string("error in configuration file ") + sdi_file;
    }
    mux.ReConfigure(mux_config, initial_reconfiguration_index);
    // there are a few things in mux_config mdi is interested in
    mdi.ReConfigure(mux_config);
    timestamp.initialise(utco, mux_config.transmission_offset);

    // make lists of continuing, lost and new destinations
    const vector<string>& next_dests = mux_config.mdi_destination;
    // continuing destinations
    vector<string> cont_dests;
    for(size_t i=0; i<next_dests.size(); i++)
    {
        if(dests.exists(next_dests[i])==false)
        {
            dests.add(next_dests[i]);
        }
    }
    // TODO remove old destinations
}

void Mdigen::eachminute()
{
    timestamp.check_for_leap_second();
    // once per minute output a stats line
    cout << "clock:" << tnow_usec/1000000
         << " count: " << timestamp.mux_drm_usec/1000000
         << " tist: " << timestamp.tist_second()
         << endl;
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = gmtime(&t);
    if (tmp) {
        char outstr[80];
        if (strftime(outstr, sizeof(outstr), "%FT%T", tmp))
            cout << outstr << endl;
    } else {
        cout << "can't read system time" << endl;
    }
}

void Mdigen::eachframe()
{
    mux.NextFrame(timestamp);
    mdi.buildFrame(mux, timestamp);
    tnow_usec = timestamp.current_drm_usec();
    if(timestamp.mux_drm_usec > tnow_usec) {
        //cout << "clock:" << tnow_usec << " count: " << timestamp.mux_drm_usec << endl;
        // we are early
        long usec = static_cast<long>(timestamp.mux_drm_usec - tnow_usec);
        if(usec>900000L) {
            // it might be the leap second
            timestamp.check_for_leap_second();
            tnow_usec = timestamp.current_drm_usec();
            if(timestamp.mux_drm_usec > tnow_usec) {
                // we really are early
                usec = static_cast<long>(timestamp.mux_drm_usec - tnow_usec);
                timestamp.wait(usec);
            }
        } else {
            if(usec>100000)
            {
                timestamp.wait(usec-100000);
                //cout << "delay " << double(usec-100000)/1000.0 << endl;
            }
        }
    }
    // if we are not early we send a frame, this should catch us up
    // if we ever get behind without doing anything special
    // unless we are live and the audio buffer got out of step
    dests.sendFrame(mdi.frame, mdi.tag_tx_order, af_seq);
    af_seq++;
    if(timestamp.tist_frame==0) {
        eachminute();
    }
    timestamp.increment();
    transmitted_frames++;
}
