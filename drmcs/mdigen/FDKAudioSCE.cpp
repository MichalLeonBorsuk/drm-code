/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2016
 *
 * Author(s):  Julian Cable
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

#include "FDKAudioSCE.h"

void FDKAudioSCE::ReConfigure(const ServiceComponent& config)
{
    ServiceComponentEncoder::ReConfigure(config);
    current.misconfiguration = false;
    current.misconfiguration |= (config.audio_coding != 0);
    int SBR;
    int audio_mode;
    int audio_sampling_rate;
    int crc;
    int coder_field;

    if(current.misconfiguration) {
        fprintf(stderr, "misconfig in FDKAudio\n");
        return;
    }

    if(hAacEncoder!=NULL) {
        aacEncClose(&hAacEncoder);
        hAacEncoder = NULL;
    }
    int ErrorStatus;
    if ( (ErrorStatus = aacEncOpen(&hAacEncoder,0,0)) != AACENC_OK ) {
        fprintf(stderr, "can't open encoder in FDKAudio\n");
        return;
    }
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_AOT, 29);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_BITRATE, 29);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_BITRATEMODE, 0);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_SAMPLERATE, 12000);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_CHANNELMODE, 2);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_TRANSMUX, 0);
    ErrorStatus = aacEncEncode(hAacEncoder, NULL, NULL, NULL, NULL);
}

void FDKAudioSCE::NextFrame(bytevector& buf, size_t max, double)
{
    AACENC_BufDesc inBufDesc, outBufDesc;
    AACENC_InArgs inargs;
    AACENC_OutArgs outargs;
    int ErrorStatus = aacEncEncode(hAacEncoder, &inBufDesc, &outBufDesc, &inargs, &outargs);
}
