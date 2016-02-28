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

#include "drmmux.h"
#include "PacketSCE.h"
#include <iostream>
using namespace std;

DrmMux::DrmMux():frame_count(0),
    fac(), sdc(), msc(),
    robustness_mode(0),
    fac_bytes(), sdc_bytes(), msc_bytes(),
    wanted(),
    reconfiguration_state(first_time),reconfiguration_index(0),
    service_pattern(0)
{
}

DrmMux::~DrmMux()
{
}

/*
6.4.6 Signalling of reconfigurations

Reconfiguration of the DRM multiplex shall be signalled in advance in order to permit receivers to make the best
decisions about how to handle the changes. There two types of reconfiguration: a service reconfiguration, which
concerns the reallocation of the data capacity between the services of the MSC; and a channel reconfiguration, which
concerns changes to the overall capacity of the MSC.

Both types of reconfiguration are signalled by setting the FAC reconfiguration index to a non-zero value. The index
then counts down on each subsequent transmission super frame. The reconfiguration index shall be identical for all
three transmission frames of a transmission super frame. The final transmission super frame corresponding to the
current configuration shall be that in which the reconfiguration index = 1. The new configuration takes effect for the
next transmission super frame and in which the reconfiguration index = 0.

All data entity types that use the reconfiguration mechanism for the version flag that are present in the current
configuration, and all data entity types that use the reconfiguration mechanism for the version flag that are required in
the new configuration, shall be sent during the period when the reconfiguration index is non-zero with the version flag
indicating the next configuration. This shall include data entity type 10 that signals the FAC channel parameters for the
new configuration.

6.4.6.1            Service reconfigurations
A service reconfiguration is one in which the data capacity of the MSC is reallocated between services. This happens
when the number of services in the multiplex is changed or the size of data streams is changed. A service
reconfiguration shall also be signalled if any of the content of the data entity types using the reconfiguration mechanism
of the version flag changes. The reconfiguration shall be signalled as far in advance as possible in order to provide the
greatest chance that the receiver gets all the information necessary for the next configuration. Therefore the
reconfiguration index shall first take the value 7.

When a new service is introduced, and the overall capacity of the MSC is not changed, then the receiver shall follow the
currently selected service through the reconfiguration. To facilitate this, the Service Identity and Short Id of all
continuing services shall remain the same. The new service shall use a Short Id that is not used in the current
configuration. The one exception to this rule is if there are four services in the current configuration and four services in the new configuration. In this case, if the currently selected service is discontinued, then the receiver follows to the new service with the same Short Id if it is of the same type (e.g. both are audio services). If the currently selected service is discontinued at the reconfiguration, then the receiver may try to find another source of that service on another frequency and/or system by using the information from data entity types 3 and 11.

6.4.6.2            Channel reconfigurations

A channel reconfiguration is one in which the following FAC channel parameters are altered: spectrum occupancy,
interleaver depth, MSC mode; and when the robustness mode is changed. In this case the receiver is unable to follow
the currently selected service without disruption to the audio output. The reconfiguration should be signalled as far in
advance as possible in order to provide the greatest chance that the receiver gets all the information necessary for the
next configuration. Ideally the reconfiguration index should first take the value 7, although a lower starting value may
be necessary for operational reasons.

If the transmission is discontinued on the tuned frequency, then a reconfiguration shall be signalled with data entity type
10 taking a special value (see clause 6.4.3.11). In this specific case, the other data entity types that use the
reconfiguration mechanism for the version flag shall not be signalled.
*/

void DrmMux::ReConfigure(const DrmMuxConfig& config, unsigned int initial_reconfiguration_index)
{
    if(config.misconfiguration)
        return;
    wanted = config;
    cout << "reconfigure requested for robm " << wanted.channel.robustness_mode << endl;
    cout.flush();
    uint8_t num_audio=0, num_data=0;
    for(size_t i=0; i<config.service.size(); i++) {
        if(config.service[i].audio_ref.length()>0) {
            num_audio++;
        }
        else if(config.service[i].data_ref.length()>0) {
            num_data++;
        }
    }
    if( (num_audio==4) && (num_data==0) )
        service_pattern = 0; // special case of 4 audio services
    if( (num_audio==0) && (num_data==4) )
        service_pattern = 0x0f; // special case of 4 data services
    else {
        service_pattern = (num_audio<<2)+num_data;
    }
    if(reconfiguration_state == first_time) {
        reconfiguration_state = running;
        ReConfigureNow();
    } else {
        if(initial_reconfiguration_index>0)
            reconfiguration_state = requested;
        else
            reconfiguration_state = signalled;
        reconfiguration_index = initial_reconfiguration_index;
    }
}

void DrmMux::ReConfigureNow()
{
    try {
        robustness_mode = wanted.channel.robustness_mode;
        cout << "reconfigure implemented for robm " << wanted.channel.robustness_mode << endl;
        cout.flush();
        // Fast Access Channel
        cout << "FAC" << endl;
        cout.flush();
        fac.ReConfigure(wanted, service_pattern);
        // Service Description Channel
        cout << "SDC" << endl;
        cout.flush();
        sdc.ReConfigure(wanted);
        // Main Service Channel
        cout << "MSC" << endl;
        cout.flush();
        msc.ReConfigure(wanted.stream);
    } catch(const char* e) {
        cerr << e << endl;
        exit(1);
    }
}

void DrmMux::NextSuperFrame()
{
    switch(reconfiguration_state) {
    case first_time:
        throw "bad reconfiguration state in DRM Multiplexer";
        break;
    case requested: // reconfiguration requested and this is the first opportunity
        cout << "Seamless reconfigure requested, sending new SDC for "
             << int(reconfiguration_index) << " superframes" << endl;
        cout.flush();
        sdc.AnnounceReConfigure(wanted, service_pattern);
        reconfiguration_state = signalled;
        break;
    case signalled:
        if(reconfiguration_index==0) {
            cout << "reconfiguration index is zero - reconfiguring" << endl;
            cout.flush();
            ReConfigureNow();
            reconfiguration_state = running;
        } else {
            reconfiguration_index--;
        }
        break;
    case running:
        ; // do nothing
    }
}

void DrmMux::NextFrame(DrmTime& timestamp)
{
    uint8_t fac_frame = frame_count % 3;
    if(fac_frame==0) {
        NextSuperFrame();
        // Service Description Channel
        sdc.NextFrame(sdc_bytes, timestamp);
        // Fast Access Channel - must be after SDC in-case afs_index_valid has changed
        fac.NextFrame(fac_bytes, fac_frame, sdc.afs_index_valid, reconfiguration_index);
    } else {
        // no Service Description Channel
        sdc_bytes.clear();
        // Fast Access Channel
        fac.NextFrame(fac_bytes, fac_frame, sdc.afs_index_valid, reconfiguration_index);
    }
    // Main Service Channel
    msc.NextFrame(msc_bytes);
    frame_count++;
}

