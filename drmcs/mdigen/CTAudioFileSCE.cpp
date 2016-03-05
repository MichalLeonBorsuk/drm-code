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


#include "CTAudioFileSCE.h"
#include <arpa/inet.h>
#include <iostream>
#include <algorithm>

using namespace std;

CCTAudioFileSCE::~CCTAudioFileSCE()
{
    if(file.is_open())
        file.close();
    if(buffer)
        delete[] buffer;
}

void CCTAudioFileSCE::ReConfigure(const ServiceComponent& config)
{
    unsigned short iVersionId, iUEP, iError;
    int iTotalFrameLength, iCoderSamplingRate, iCoderField, iCoderId,
        iCoderMode, iAudioFrameLength, iHiProtFrameLength;
    int bSBR;
    string oldfile = current.source_selector;
    ServiceComponentEncoder::ReConfigure(config);
    if(oldfile != current.source_selector) {
        if(file.is_open())
            file.close();
    }
    if(!file.is_open()) {
        file.open(current.source_selector.c_str(), ios::in|ios::binary);
        if(!file.is_open()) {
            throw string("CTAudioFile: can't open file ")+current.source_selector;
        }

        //m_loop = true;
        // read the header from the file
        iTotalFrameLength = ReadInt();
        iVersionId = static_cast<unsigned short>(ReadInt());
        iCoderSamplingRate = ReadInt();
        iCoderField = ReadInt();
        iCoderId = ReadInt();
        iCoderMode = ReadInt();
        bSBR = ReadInt();
        iAudioFrameLength = ReadInt();
        iHiProtFrameLength = ReadInt();
        iUEP = static_cast<unsigned short>(ReadInt());
        iError = static_cast<unsigned short>(ReadInt());
    (void)iVersionId; (void)iUEP; (void)iError; (void)iTotalFrameLength;

        // validate it against what the xml file asked for
        // we will allow the xml file to not specify the values
        if(current.audio_sampling_rate == -1)
            current.audio_sampling_rate = iCoderSamplingRate;
        if(current.audio_coding == -1)
            current.audio_coding = iCoderId%10;
        if(current.audio_mode == -1)
            current.audio_mode = iCoderMode; // not just for AAC!
        if(current.SBR == -1)
            current.SBR = bSBR;
        if(current.bytes_per_frame == -1)
            current.bytes_per_frame = iAudioFrameLength;
        if(current.bytes_better_protected == -1)
            current.bytes_better_protected = iHiProtFrameLength;
        if(current.coder_field == -1)
            current.coder_field = iCoderField;

        current.misconfiguration = false;
        if(iCoderSamplingRate != current.audio_sampling_rate)
            current.misconfiguration = true;
        if(iCoderField != current.coder_field) current.misconfiguration = true;
        if(iCoderId%10 != current.audio_coding)
            current.misconfiguration = true;
        if(iCoderMode != current.audio_mode)
            current.misconfiguration = true;
        if(bSBR != current.SBR)
            current.misconfiguration = true;
        /* defer these checks
        if(iAudioFrameLength != m_bytes_per_frame)
          m_misconfiguration = true;
        if(iHiProtFrameLength != m_bytes_better_protected)
          m_misconfiguration = true;
        */
        if(buffer)
            delete[] buffer;
        buffer = new char[current.bytes_per_frame];
    }
}

unsigned long CCTAudioFileSCE::ReadInt()
{
    unsigned long n;
    file.read((char*)&n, 4);
    return ntohl(n);
}

void CCTAudioFileSCE::NextFrame(vector<uint8_t>& buf, size_t max, double)
{
    if (!current.misconfiguration
            && max>=(unsigned)current.bytes_per_frame
            && file.is_open())
    {
        file.read(buffer, current.bytes_per_frame);
        copy(buffer, buffer+current.bytes_per_frame, buf.end());
        current.loop=true;
        // skip over next header - will fail at eof but so what?
        file.seekg(DRM_FILE_HEADER_SIZE, ios::cur);
        if(file.eof()) {
            file.clear();
            if( current.loop) {
                file.seekg(DRM_FILE_HEADER_SIZE);
            } else {
                buf.insert(buf.end(), current.bytes_per_frame, 0);
            }
        }
    }
}
