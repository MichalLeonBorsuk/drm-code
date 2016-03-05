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
#include <algorithm>

using namespace std;

void FDKAudioSCE::ReConfigure(const ServiceComponent& config)
{
    ServiceComponentEncoder::ReConfigure(config);
    current.misconfiguration = false;
    current.misconfiguration |= (config.audio_coding != 0);
    double fudge_factor = 0.93;
    int bitrate = int(fudge_factor*double(config.bytes_per_frame)/0.4*8.0);
    cerr << "aac bitrate " << bitrate << endl;

    if(current.misconfiguration) {
        fprintf(stderr, "misconfig in FDKAudio\n");
        return;
    }
    if(config.audio_mode!=0) {
        fprintf(stderr, "FDKAudio bad audio mode. 0 needed\n");
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
    if(config.audio_mode==0) {
        channels = 1;
        if(config.SBR) {
            ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_AOT, 5);
        }
        else {
            ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_AOT, 2);
        }
    }
    else {
        channels = 1;
        if(config.SBR) {
            ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_AOT, 29);
        }
        else {
            ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_AOT, 2);
        }
    }
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_CHANNELMODE, channels);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_BITRATE, bitrate);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_BITRATEMODE, 0);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_SAMPLERATE, config.audio_sampling_rate);
    ErrorStatus = aacEncoder_SetParam(hAacEncoder, AACENC_TRANSMUX, 0);
    ErrorStatus = aacEncEncode(hAacEncoder, NULL, NULL, NULL, NULL);

    int err = snd_pcm_open(&hAlsa, config.source_selector.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if(err < 0)
        throw string("audio open error ") + snd_strerror(err);
    snd_pcm_hw_params_t* hwparams=NULL;
    snd_pcm_hw_params_malloc(&hwparams);
    if(hwparams==NULL)
        throw "could not allocate alsa hw_params buffer";
    err = snd_pcm_hw_params_any(hAlsa, hwparams);
    if (err < 0)
        throw string("could not load alsa hw_params ") + snd_strerror(err);
    err = snd_pcm_hw_params_set_format(hAlsa, hwparams, SND_PCM_FORMAT_S16);
    if (err < 0)
        throw string("audio format error ") + snd_strerror(err);
    err = snd_pcm_hw_params_set_channels(hAlsa, hwparams, channels);
    if (err < 0)
        throw string("audio stereo error ") + snd_strerror(err);
    err = snd_pcm_hw_params_set_rate(hAlsa, hwparams, 48000, 0);
    if (err < 0)
        throw string("audio rate error ") + snd_strerror(err);
    err = snd_pcm_hw_params(hAlsa, hwparams);
    if (err < 0)
        throw string("audio set hwparams error ") + snd_strerror(err);
    if ((err = snd_pcm_start(hAlsa)) < 0)
        throw string("audio can't start error ") + snd_strerror(err);
    cout << "alsa audio open " <<  current.source_selector << endl;
}

void FDKAudioSCE::NextFrame(vector<uint8_t>& buf, size_t max, double)
{
    AACENC_BufDesc inBufDesc, outBufDesc;
    AACENC_InArgs inargs;
    AACENC_OutArgs outargs;
    void *ibp[1], *obp[1];
    int ibufid=IN_AUDIO_DATA, obufid=OUT_BITSTREAM_DATA;
    int ibs[1], obs[1];
    int ibes[1], obes[1];
    // fetch 10 frames
    int num_frames = 48*40; // 48 num_frames per ms * 400 ms per frame, divided by 10
    int16_t audiobuf[channels*num_frames];
    ibp[0] = &audiobuf[0];
    ibs[0]=channels*num_frames*sizeof(int16_t);
    ibes[0] = sizeof(int16_t);
    inBufDesc.numBufs=1;
    inBufDesc.bufs=ibp;
    inBufDesc.bufSizes=ibs;
    inBufDesc.bufElSizes = ibes;
    inBufDesc.bufferIdentifiers=&ibufid;
    uint8_t encoded[current.bytes_per_frame];
    obp[0] = encoded;
    obs[0]=current.bytes_per_frame;
    obes[0] = 1;
    outBufDesc.numBufs=1;
    outBufDesc.bufs=obp;
    outBufDesc.bufSizes=obs;
    outBufDesc.bufElSizes = obes;
    outBufDesc.bufferIdentifiers=&obufid;
    int n=0;
    for(int i=0; i<10; i++) {
        snd_pcm_readi(hAlsa, audiobuf, num_frames);
	inargs.numInSamples=channels*num_frames;
	inargs.numAncBytes=0;
        int ErrorStatus = aacEncEncode(hAacEncoder, &inBufDesc, &outBufDesc, &inargs, &outargs);
        (void)ErrorStatus;
	n+=outargs.numOutBytes;
	//cerr << "err " << ErrorStatus << " this: " << outargs.numOutBytes << " total: "<< n << " stored " << buf.size() << endl;
        if(buf.size()+outargs.numOutBytes<=max) {
	    copy(encoded, encoded+outargs.numOutBytes, buf.end());
        }
    }
}
