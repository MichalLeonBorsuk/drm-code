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

#include "alsa.h"
#include <alsa/asoundlib.h>
#include <iostream>
#include <sstream>
using namespace std;

template<typename T> ALSA<T>::ALSA():handle(NULL)
{
}

template<typename T> ALSA<T>::ALSA(const ALSA<T>& e):handle(e.handle)
{
}

template<typename T> ALSA<T>& ALSA<T>::operator=(const ALSA<T>& e)
{
    handle = e.handle;
    return *this;
}

template<typename T> ALSA<T>::~ALSA()
{
    close();
}

template<typename T> void ALSA<T>::open(const string& device, int channels)
{
    num_channels = channels;
    stringstream s(device);
    string sys, pcm;
    card_channels = 2;
    left_chan = 0;
    right_chan = 1;
    s >> sys;
    if(sys!="alsa")
        throw string("alsa input invoked but source selector is ")+device;
    s >> pcm;
    if(pcm!="") {
        s >> card_channels;
        if(s.goodbit) {
            s >> left_chan >> right_chan;
        }
        if(card_channels<2) {
            cerr << "strange config " << device << " setting to default 2 channel config" << endl;
            card_channels = 2;
            left_chan = 0;
            right_chan = 1;
        }
        if(left_chan>=card_channels) {
            cerr << "left chan " << left_chan << " << out of range, setting to 0" << endl;
            left_chan = 0;
        }
        if(right_chan>=card_channels) {
            cerr << "right chan " << right_chan << " out of range, setting to 1" << endl;
            right_chan = 1;
        }
    } else {
        pcm = "dsnoop";
    }
    cout << pcm << " " << card_channels << " " << left_chan << " " << right_chan << endl;
    int err;
    err = snd_pcm_open(&handle, pcm.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if(err < 0)
        throw string("audio open error ") + snd_strerror(err);

    snd_pcm_hw_params_t* hwparams=NULL;
    snd_pcm_hw_params_malloc(&hwparams);
    if(hwparams==NULL)
        throw "could not allocate alsa hw_params buffer";

    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0)
        throw string("could not load alsa hw_params ") + snd_strerror(err);

    err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_NONINTERLEAVED);
    if (err < 0)
        throw string("could not set alsa interleave ") + snd_strerror(err);

    snd_pcm_format_mask_t *mask;
    snd_pcm_format_mask_malloc(&mask);
    snd_pcm_hw_params_get_format_mask(hwparams, mask);
    if(snd_pcm_format_mask_test(mask, SND_PCM_FORMAT_S32)) {
        format = SND_PCM_FORMAT_S32;
        scale_factor = 65536.0*32768.0;
    } else if(snd_pcm_format_mask_test(mask, SND_PCM_FORMAT_S16)) {
        format = SND_PCM_FORMAT_S16;
        scale_factor = 32768.0;
    } else
        throw "MDI Generator and sound card don't have a common supported sample format";

    err = snd_pcm_hw_params_set_format(handle, hwparams, format);
    if (err < 0)
        throw string("audio format error ") + snd_strerror(err);

    err = snd_pcm_hw_params_set_channels(handle, hwparams, card_channels);
    if (err < 0)
        throw string("audio stereo error ") + snd_strerror(err);

    err = snd_pcm_hw_params_set_rate(handle, hwparams, 48000, 0);
    if (err < 0)
        throw string("audio rate error ") + snd_strerror(err);

    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0)
        throw string("audio set hwparams error ") + snd_strerror(err);

    if ((err = snd_pcm_start(handle)) < 0)
        throw string("audio can't start error ") + snd_strerror(err);

    cout << "alsa audio open " <<  pcm << endl;
    snd_pcm_hw_params_free(hwparams);
    snd_pcm_format_mask_free(mask);
}

template<typename T> void ALSA<T>::close()
{
    if((snd_pcm_t*)handle) {
        snd_pcm_close((snd_pcm_t*)handle);
        handle = NULL;
    }
}

template<typename T> void read1(snd_pcm_t *handle, T* left, T* right, size_t n,
                                size_t card_channels, size_t left_chan, size_t right_chan)
{
    T dummy_buf[n];
    T* bb[card_channels];
    for(size_t i=0; i<card_channels; i++)
        bb[i] = dummy_buf;
    bb[left_chan] = left;
    bb[right_chan] = right;
    snd_pcm_uframes_t want = n;
    long r;
    do {
        r = snd_pcm_readn(handle, (void**)bb, want);
        if (r > 0) {
            bb[left_chan]+=r;
            bb[right_chan]+=r;
            want -= r;
        }
    } while (r > 0 && want>0);
}

template<typename T1, typename T2> void read_mono(snd_pcm_t *handle, vector<T1>& buffer, T2* left, T2* right,
                                    size_t card_channels, size_t left_chan, size_t right_chan, double scale_factor)
{
    size_t n = buffer.size();
    read1(handle, left, right, n, card_channels, left_chan, right_chan);
    for(size_t i=0; i<n; i++) {
        double left_sample = double(left[i])/scale_factor;
        double right_sample = double(right[i])/scale_factor;
        buffer[i] = float((left_sample+right_sample)/2.0);
    }
}

template<typename T1, typename T2> void read_stereo(snd_pcm_t *handle, vector<T1>& buffer, T2* left, T2* right,
                                      size_t card_channels, size_t left_chan, size_t right_chan, double scale_factor)
{
    size_t n = buffer.size()/2;
    read1(handle, left, right, n, card_channels, left_chan, right_chan);
    for(size_t i=0; i<n; i++) {
        double left_sample = double(left[i])/scale_factor;
        double right_sample = double(right[i])/scale_factor;
        buffer[2*i] = float(left_sample);
        buffer[2*i+1] = float(right_sample);
    }
}

template<typename T> void ALSA<T>::read(vector<T>& buffer)
{
    size_t n = buffer.size();
    if(format == SND_PCM_FORMAT_S32) {
        int32_t left[n];
        int32_t right[n];
        if(num_channels==2)
            read_stereo(handle, buffer, left, right, card_channels, left_chan, right_chan, scale_factor);
        else
            read_mono(handle, buffer, left, right, card_channels, left_chan, right_chan, scale_factor);
    } else {
        int16_t left[n];
        int16_t right[n];
        if(num_channels==2)
            read_stereo(handle, buffer, left, right, card_channels, left_chan, right_chan, scale_factor);
        else
            read_mono(handle, buffer, left, right, card_channels, left_chan, right_chan, scale_factor);
    }
}

template<typename T> bool ALSA<T>::is_open()
{
    return handle != NULL;
}
