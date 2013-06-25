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

#ifndef _SERVICE_COMPONENT_H
#define _SERVICE_COMPONENT_H

#include "persist.h"

class ServiceComponent : public Persist
{
public:
    ServiceComponent();
    ServiceComponent(const ServiceComponent&);
    ServiceComponent& operator=(const ServiceComponent&);
    virtual ~ServiceComponent();
    virtual void ReConfigure(xmlNodePtr config);
    virtual void clearConfig();
    virtual void GetParams(xmlNodePtr n);
    virtual void PutParams(xmlTextWriterPtr writer);

    string label;
    string source_selector;
    string encoder_id;
    string implementor;
    string typestring;
    enum  {audio_sce, text_sce, data_stream_sce, data_packet_mode_sce, unspecified_sce} type;
    int bytes_per_frame;
    int bytes_better_protected; // live encoding needs this!
    bool loop;
    int enhancement;

public: // audio SCEs
      /* SDC Type 9 info describes an audio stream
       Short Id 2 bits.
       Stream Id 2 bits.
       audio coding 2 bits.
       SBR flag 1 bit.
       audio mode 2 bits.
       audio sampling rate 3 bits.
       text flag 1 bit.
       enhancement flag 1 bit.
       coder field 5 bits.
       rfa 1 bit.
      */
    static const char* sAACMode[];
    static const char* CoderType[];
    static const char* SampleRate[];

    void AudioReConfigure(xmlNodePtr config);
    void AudioGetParams(xmlNodePtr n);
    void AudioPutParams(xmlTextWriterPtr writer);
    void AudioclearConfig();
    bool AudioValidate(string& message);
    
    enum eAudio {AUDIO_CODING_AAC, AUDIO_CODING_CELP, AUDIO_CODING_HVXC, UNDEFINED=-1};
    int audio_coding;
    int SBR;
    int audio_mode;
    int audio_sampling_rate;
    int crc;
    int hvxc_rate;
    int coder_field;

public: // text SCEs

    void TextclearConfig();

    static const unsigned int MAX_TEXT_MSG_LENGTH=128;
    static const unsigned int NUTEXT_REP=1;
    static const int MSG_SUB_SEG_LEN=4;

public: // data SCEs

    void DataReConfigure(xmlNodePtr config);
    void DataGetParams(xmlNodePtr n);
    void DataPutParams(xmlTextWriterPtr writer);
    void DataclearConfig();
    bool DataValidate(string& message);

    int application_domain;
    int data_unit_indicator;
    bytev application_data;
    int packet_mode;
    int	packet_size;
    int	packet_id; // to be filled in by the mux
    int target_bitrate;
};

#endif
