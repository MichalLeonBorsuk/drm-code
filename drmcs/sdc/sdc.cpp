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

#include "sdc.h"
#include "Crc16.h"
#include <iostream>
using namespace std;

const unsigned short SDC::sdc_length[4][2][6]
= {{{ 37,  43,  85,  97, 184, 207}, { 17,  20,  41,  47,  91, 102}},
    {{ 28,  33,  66,  76, 143, 161}, { 13,  15,  32,  37,  70,  79}},
    {{  0,   0,   0,  68,   0, 147}, {  0,   0,   0,  32,   0,  72}},
    {{  0,   0,   0,  33,   0,  78}, {  0,   0,   0,  15,   0,  38}}
};

SDC::SDC():sdci(),
    afs_index_valid(false),
    current(),afs_index(0),length(0),
    block(15),elements_in_every_block(),
    reconfiguration_version(false), afs_version(false), announcement_version(false),
    current_block(0),send_date_and_time(false),num_elements_in_every_block(0)
{
}

SDC::~SDC()
{
}

// Reconfigure everything later

void SDC::AnnounceReConfigure(const DrmMuxConfig& config, uint8_t new_service_pattern)
{
    ReConfigure(config, true, new_service_pattern);
}

// Reconfigure everything immediately

void SDC::ReConfigure(const DrmMuxConfig& config,
                      bool new_reconfiguration_version,
                      uint8_t service_pattern
                     )
{
    current = config;
    announcement_version = afs_version = false;
    reconfiguration_version = new_reconfiguration_version;

    // build the full list of static elements we want to send
    vector<SdcElement> static_element;
    elements_in_every_block.clear();
    num_elements_in_every_block = 0;

    // Configure elements using the reconfiguration mechanism for the version flag
    ConfigureChannel(static_element, current.channel);
    ConfigureService(static_element, current);
    // type 10 for seamless reconfigure only
    // we can tell if its needed by looking at reconfiguration_version,
    if(reconfiguration_version) {
        SdcElement e;
        e.Type10(current.channel, false, service_pattern);
        elements_in_every_block.put(e.out);
        num_elements_in_every_block++;
    }
    // Configure elements using the unique mechanism for the version flag
    //type 8 optional zero or one entity per minute - just remember whether to do it.
    send_date_and_time = (current.send_sdc_time==0)?false:true;
    // type 12 optional zero or one entity per service; zero to four entities in total
    for(size_t i=0; i<current.service.size(); i++)
    {
        ConfigureServiceUnique(static_element, current.service[i], i);
    }

    // Configure elements using the list mechanism for the version flag
    // Announcements - type 6 optional zero to eight entities in total
    size_t num_type6 = 0;
    for(size_t i=0; i<current.service.size(); i++)
    {
        num_type6 += current.service[i].announcement.size();
        if(num_type6>8)
            throw "too many announcements";
        ConfigureAnnouncement(static_element, current.service[i].announcement, i);
    }

    ConfigureAFS(static_element, current.afs);

    // then build a set of SDC blocks
    buildBlocks(static_element, 15);
    current_block = 0;
}

void SDC::NextFrame(vector<uint8_t> &out, DrmTime& timestamp)
{
    bytevector s;
    check_build_date_and_time(timestamp);
    afs_index_valid = block[current_block].NextFrame(s, afs_index);
    if(reconfiguration_version)
        afs_index_valid = false;
    block[current_block].build_sdc(s, afs_index, length);
    current_block++;
    if(current_block>afs_index)
        current_block=0;
    out = s.data();
}

// Allocate elements to blocks. If we are really stuck for space
// we might need to optimise which elements go in which blocks, but for now
// we will give up if the required AFS index exceeds 15.
// we fill up a block until the next element would overflow it or we are done.
// As long as we have unallocated elements, we add new blocks.
// for each static element

void SDC::buildBlocks(vector<SdcElement>& element, size_t max_blocks)
{

    block.clear();
    bytevector new_block;
    new_block.put(elements_in_every_block);
    cout << num_elements_in_every_block << " elements needed in every block" << endl;
    size_t z = 0;
    for(size_t i=0; i<element.size(); i++) {
        if(new_block.size()+element[i].size()>length) {
            cout << "block " << block.size() << " carries elements " << z << " to " << (i-1) << endl;
            z = i;
            block.push_back(SdcBlock(new_block));
            if(block.size()>max_blocks) {
                throw "too many SDC elements";
            }
            new_block.clear();
            new_block.put(elements_in_every_block);
        }
        new_block.put(element[i].out);
    }
    block.push_back(SdcBlock(new_block));
    cout << "block " << (block.size()-1) << " carries elements " << z << " to " << (element.size()-1) << endl;

    if(block.size()>0)
        afs_index = block.size()-1;
    else
        afs_index = 0;
}

// if its time to send type 8 element make it happen.
// We need to put the element in an SDC block which will be transmitted
// as soon as possible after the UTC minute boundary and contains the
// current minute at that time.
// utc minute boundary - tx offset - 3*num sdc blocks*.4

void SDC::check_build_date_and_time(DrmTime& timestamp)
{
    if(!send_date_and_time)
        return;
    //int frames_until_minute = 150 - timestamp.tist_frame;
    //int frames_per_sdc = 3*block.size();
    //int frames_of_tx_offset =
    uint64_t us_until_minute_edge = 400000ULL*(150 - timestamp.tist_frame);
    uint64_t us_per_superframe = 400000ULL*3;
    uint64_t us_per_sdc_cycle = us_per_superframe*block.size();
    uint64_t us_needed_in_advance = timestamp.tist_delay_us + us_per_sdc_cycle;
    if(us_until_minute_edge<us_needed_in_advance) // we are too late
        return;
    uint64_t us_avail = us_until_minute_edge - us_needed_in_advance;
    if(us_avail>us_per_superframe) // we are too early
        return;
    cout << "time to insert date & time element " << timestamp.tist_second() << endl;
    uint32_t mjd;
    uint8_t hh, mm;
    time_t t = (timestamp.current_utc_usec()/1000000ULL)+60;
    DrmTime::date_and_time(mjd, hh, mm, t);
    SdcElement element;
    element.Type8(mjd, hh, mm);
    element.out.put(elements_in_every_block);
    block[current_block].sendOnce(element.out);
}

void SDC::build_sdci(const DrmMuxConfig& mux)
{
    bytevector payload;
    payload.put(0, 4); // this only works because a type 0 has zero in the type field!
    payload.put(mux.channel.protection_a, 2);
    payload.put(mux.channel.protection_b, 2);

    // stream description for stream 0
    if (mux.channel.msc_mode==1 || mux.channel.msc_mode==2) {
        // hierarchical coding, first stream
        if (mux.stream[0].bytes_better_protected>0)
        {
            // When using hierarchical coding, the first service must not use unequal error protection
        }
        payload.put(mux.channel.VSPP, 2);
        payload.put(0, 10); // rfu
        payload.put(mux.stream[0].bytes_per_frame, 12);

    } else {
        payload.put(mux.stream[0].bytes_better_protected, 12);
        payload.put(mux.stream[0].bytes_per_frame - mux.stream[0].bytes_better_protected, 12);
    }
    // in case of hierarchical, datalenA and datalenB are interpreted differently
    // stream description for streams 1, 2 and 3 (if in use)
    for (size_t n=1; n<mux.stream.size(); n++)
    {
        payload.put(mux.stream[n].bytes_better_protected, 12);
        payload.put(mux.stream[n].bytes_per_frame - mux.stream[n].bytes_better_protected, 12);
    }
    sdci = payload.data();
}

void SDC::ConfigureServiceUnique(vector<SdcElement>& element,
                                 const Service& service, size_t short_id)
{
    // type 1 optional zero or one entity per service
    if(service.service_label.length()>0)
    {
        SdcElement e;
        e.Type1(service.service_label, short_id);
        elements_in_every_block.put(e.out);
        num_elements_in_every_block++;
    }
    // type 12 optional zero or one entity per service
    if(service.language_long.length()>0 || service.country.length()>0) {
        SdcElement e;
        e.Type12(service, short_id);
        element.push_back(e);
    }
}

void SDC::ConfigureService(vector<SdcElement>& element, const DrmMuxConfig& mux)
{

    // type 0 - mandatory one entity as (normal) for each configuration; two entities in total
    {
        build_sdci(mux);
        SdcElement e;
        e.Type0(sdci, reconfiguration_version);
        elements_in_every_block.put(e.out);
        num_elements_in_every_block++;
    }
    // type 2 mandatory for each service for which the FAC CA indication flag = 1 zero,
    // one or two entities per audio service; zero or one entity per data service;
    // zero to seven entities in total
    for(size_t i=0; i<mux.service.size(); i++)
    {
        if(mux.service[i].conditional_access)
        {
            SdcElement e;
            e.Type2(mux.service[i], reconfiguration_version, i);
            element.push_back(e); // ca
        }
    }

    // type 5 mandatory for each data service and data application
    // zero or one entity per audio service;
    // one entity per data service; zero to four entities in total
    for(size_t i=0; i<mux.service.size(); i++)
    {
        const string& ref = mux.service[i].data_ref;
        if(ref.length()>0)
        {
            const ServiceComponent *comp = NULL;
            size_t stream;
            if(mux.findStream(ref, stream)) { // its a reference to a data stream
                comp = &mux.stream[stream].component[0];
            } else {
                size_t sub_stream;
                if(mux.findSubstream(ref, stream, sub_stream)) {
                    comp = &mux.stream[stream].component[sub_stream];
                } else {
                    throw string("SDC can't associate service ")+ref+" with packet stream";
                }
            }
            SdcElement e;
            e.Type5(*comp, i, stream, reconfiguration_version);
            element.push_back(e);
        }
    }
    // type 9 mandatory for each audio service one entity per audio service
    // zero to four entities in total
    for(size_t i=0; i<mux.service.size(); i++)
    {
        const string& ref = mux.service[i].audio_ref;
        if(ref.length()>0)
        {
            SdcElement e;
            size_t stream;
            if(mux.findStream(ref, stream)) {
                const Stream& s = mux.stream[stream];
                e.Type9(s.component[0], i, stream, s.component.size()>1, reconfiguration_version);
                elements_in_every_block.put(e.out);
                num_elements_in_every_block++;
            } else {
                throw string("SDC can't associate service ")+ref+" with audio stream";
            }
        }
    }
}

void SDC::ConfigureChannel(vector<SdcElement>& element, const RFChannel& chan)
{
    // calculate the number of bytes we have available in an SDC block
    length = sdc_length[chan.robustness_mode][chan.sdc_mode][chan.spectral_occupancy];
}

void SDC::ConfigureAnnouncement(vector<SdcElement>& element,
                                const vector<Announcement>& a,
                                size_t short_id
                               )
{
    for(size_t j=0; j<a.size(); j++)
    {
        SdcElement e;
        e.Type6(a[j], short_id, announcement_version);
        element.push_back(e);
    }
}

void SDC::ConfigureAFS(vector<SdcElement>& afs_element, const AFS& afs)
{
    // type 3 optional zero to 16 frequencies per entity; zero to 64 entities in total
    // list mechanism
    size_t num_elements = 0;
    for(size_t i=0; i<afs.afs_mux_list.size(); i++)
    {
        const AFSMuxlist& l = afs.afs_mux_list[i];
        size_t n = l.fg.size();
        num_elements += n;
        if(num_elements>64)
            throw "too many type 3 elements (64 allowed)";
        for(size_t j=0; j<n; j++)
        {
            SdcElement e;
            e.Type3(l, j, afs_version);
            afs_element.push_back(e);
        }
    }
    //type 4 optional zero to 32 entities per Schedule Id; zero to 128 entities in total
    // list mechanism
    num_elements = 0;
    for(size_t schedule=1; schedule<afs.schedule.size(); schedule++)
    {
        const Schedule& s = afs.schedule[schedule];
        size_t n = s.interval.size();
        if(n>32)
            throw "too many intervals per schedule id (32 allowed)";
        num_elements += n;
        if(num_elements>128)
            throw "too many type 4 elements (128 allowed)";
        for(size_t j=0; j<n; j++)
        {
            SdcElement e;
            e.Type4(schedule, s.interval[j], afs_version);
            afs_element.push_back(e);
        }
    }
    // type 7 optional zero to four entities per Region Id; up to 16 CIRAF zones per Region Id
    // up to 16 CIRAF zones per entity.
    // zero to 32 entities in total
    // so - put all the CIRAF zones in one entity and have up to four areas!
    // list mechanism
    // TODO zones per region, NOT zones per area!!! review
    num_elements = 0;
    for(size_t region=1; region<afs.region.size(); region++)
    {
        const Region& r = afs.region[region];
        size_t areas = r.area.size();
        size_t zones = r.zone.size();
        SdcElement e;
        if(areas==0)
            throw "no lat/lng area defined for a region";
        if(areas>4)
            throw "too many lat/lng areas in a region (4 allowed)";
        if(zones>16)
            throw "too many CIRAF zones in a region (16 allowed)";
        num_elements += areas;
        if(num_elements>32)
            throw "too many type 7 elements (32 allowed)";
        e.Type7(region, r.area[0], r.zone, afs_version);
        afs_element.push_back(e);
        for(size_t j=1; j<areas; j++) {
            vector<int> z; // dummy length zero vector
            e.Type7(region, r.area[j], z, afs_version);
            afs_element.push_back(e);
        }
    }
    // type 11 optional zero to 16 frequencies per entity; zero to 256 entities in total
    // list mechanism
    size_t num_type11 = 0;
    for(size_t i=0; i<afs.afs_service_list.size(); i++)
    {
        const AFSServicelist& l = afs.afs_service_list[i];
        size_t n = l.sg.size();
        num_type11 += n;
        if(num_type11>256)
            throw "too many type 11 SDC entities (256 allowed)";
        for(size_t j=0; j<l.sg.size(); j++)
        {
            SdcElement e;
            e.Type11(l.sg[j], false, l.short_id, afs_version);
            afs_element.push_back(e);
        }
    }
    /*
     * scanning guide
    The fields of the SDC data entity type 11 are filled as follows:

      Short Id/Announcement Id flag: is set to 0 to indicate short id
      Short Id/Announcement Id field: is set to 03
      Region/Schedule flag: is set to 0 to indicate no schedule or region restriction
      Same Service flag: is set to 0 to indicate an alternative service
      rfa: set to 00
      System Id: set to 00000 to indicate DRM
      Region/Schedule field: is absent
      Other Service Id: is set to the Service Id of the indicated service
      n frequencies: a list of frequencies on which the indicated service may be receivable
    */
    const vector<ServiceGroup>& sg = afs.guide;
    size_t n = sg.size();
    num_type11 += n;
    if(num_type11>256)
        throw "too many type 11 SDC entities (256 allowed)";
    for(size_t i=0; i<sg.size(); i++)
    {
        SdcElement e;
        try {
            e.Type11(sg[i], false, 3, afs_version);
            afs_element.push_back(e);
        } catch(const char *ex)
        {
            throw (string("error in scanning guide data: ")+ex).c_str();
        }
    }
}
