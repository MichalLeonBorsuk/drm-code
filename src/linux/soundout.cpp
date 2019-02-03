/******************************************************************************\
* Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
* Copyright (c) 2001-2014
*
* Author(s):
*   Alexander Kurpiers
*
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

#include "soundout.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

/*****************************************************************************/

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>

CSoundOut::CSoundOut() : handle(NULL), bChangDev(true), sCurrentDevice("")
{
    PlayThread.pSoundOut = this;
}

void CSoundOut::Enumerate(vector<string>& choices, vector<string>& descriptions)
{
    choices.resize(0);
    descriptions.resize(0);

    char **hints;
    int err = snd_device_name_hint(-1, "pcm", (void***)&hints);
    if (err != 0)
       return;//Error! Just return

    char** n = hints;
    while (*n != nullptr) {

        char *ioid = snd_device_name_get_hint(*n, "IOID");
        if (ioid == nullptr || 0 == strcmp("Output", ioid)) {
            char *name = snd_device_name_get_hint(*n, "NAME");
            if (name != nullptr && 0 != strcmp("null", name)) {
                char *desc = snd_device_name_get_hint(*n, "DESC");
                if (desc != nullptr && 0 != strcmp("null", desc)) {
                    descriptions.push_back(desc);
                    free(desc);
                }
                else {
                    descriptions.push_back("");
                }
                choices.push_back(name);
                free(name);
            }
        }
        free(ioid);
        n++;
    }

    //Free hint buffer too
    snd_device_name_free_hint((void**)hints);
}

void CSoundOut::CPlayThread::run()
{
    while ( SoundBuf.keep_running ) {
        int fill;

        SoundBuf.lock();
        fill = SoundBuf.GetFillLevel();
        SoundBuf.unlock();

        if ( fill > (FRAGSIZE * NUM_OUT_CHANNELS) ) {

            // enough data in the buffer

            CVectorEx<_SAMPLE>* p;

            SoundBuf.lock();
            p = SoundBuf.Get( FRAGSIZE * NUM_OUT_CHANNELS );

            for (int i=0; i < FRAGSIZE * NUM_OUT_CHANNELS; i++)
                tmpplaybuf[i] = (*p)[i];

            SoundBuf.unlock();

            pSoundOut->write_HW( tmpplaybuf, FRAGSIZE );

        } else {

            do {
                msleep( 1 );

                SoundBuf.lock();
                fill = SoundBuf.GetFillLevel();
                SoundBuf.unlock();

            } while ((SoundBuf.keep_running) && ( fill < SOUNDBUFLEN/2 ));  // wait until buffer is at least half full
        }
    }
    qDebug("Play Thread stopped");
}

bool CSoundOut::Init(int iSampleRate, int iNewBufferSize, bool bNewBlocking)
{
    qDebug("initplay %d", iNewBufferSize);

    this->iSampleRate = iSampleRate;

    /* Save buffer size */
    PlayThread.SoundBuf.lock();
    iBufferSize = iNewBufferSize;
    bBlockingPlay = bNewBlocking;
    PlayThread.SoundBuf.unlock();

    /* Check if device must be opened or reinitialized */
    if (bChangDev == true)
    {

        Init_HW( );

        /* Reset flag */
        bChangDev = false;
    }

    if ( PlayThread.isRunning() == false ) {
        PlayThread.SoundBuf.lock();
        PlayThread.SoundBuf.Init( SOUNDBUFLEN );
        PlayThread.SoundBuf.unlock();
        PlayThread.start();
    }

    return true;
}


bool CSoundOut::Write(CVector< _SAMPLE >& psData)
{
    /* Check if device must be opened or reinitialized */
    if (bChangDev == true)
    {
        /* Reinit sound interface */
        Init(iBufferSize, bBlockingPlay);

        /* Reset flag */
        bChangDev = false;
    }

    if ( bBlockingPlay ) {
        // blocking write
        while ( PlayThread.SoundBuf.keep_running ) {
            PlayThread.SoundBuf.lock();
            int fill = SOUNDBUFLEN - PlayThread.SoundBuf.GetFillLevel();
            PlayThread.SoundBuf.unlock();
            if ( fill > iBufferSize) break;
        }
    }

    PlayThread.SoundBuf.lock(); // we need exclusive access

    if ( ( SOUNDBUFLEN - PlayThread.SoundBuf.GetFillLevel() ) > iBufferSize) {

        CVectorEx<_SAMPLE>* ptarget;

        // data fits, so copy
        ptarget = PlayThread.SoundBuf.QueryWriteBuffer();
        for (int i=0; i < iBufferSize; i++)
        {
            (*ptarget)[i] = psData[i];
        }

        PlayThread.SoundBuf.Put( iBufferSize );
    }

    PlayThread.SoundBuf.unlock();

    return false;
}

void CSoundOut::Close()
{
    qDebug("stopplay");

    // stop the playback thread
    if (PlayThread.isRunning() ) {
        PlayThread.SoundBuf.keep_running = false;
        PlayThread.wait(1000);
    }

    close_HW();

    /* Set flag to open devices the next time it is initialized */
    bChangDev = true;
}

void CSoundOut::SetDev(string sNewDevice)
{
    /* Change only in case new device id is not already active */
    if (sNewDevice != sCurrentDevice)
    {
        sCurrentDevice = sNewDevice;
        bChangDev = true;
    }
}

string CSoundOut::GetDev()
{
    return sCurrentDevice;
}

void CSoundOut::Init_HW()
{

    int err, dir;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t period_size = FRAGSIZE * NUM_OUT_CHANNELS/2;
    snd_pcm_uframes_t buffer_size;

    if (handle != NULL)
        return;

    vector<string> names;
    vector<string> descriptions;
    Enumerate(names, descriptions);

    /* Default ? */
    if (sCurrentDevice == "")
    {
        int n = int(names.size())-1;
	sCurrentDevice = names[n];
    }
    /* might be invalid due to command line parameter or USB device unplugged */
    bool found = false;
    for(size_t i=0; i<names.size(); i++) {
        if(names[i] == sCurrentDevice) found = true;
    }
    if(!found) sCurrentDevice = names[names.size()-1];
    err = snd_pcm_open( &handle, sCurrentDevice.c_str(), SND_PCM_STREAM_PLAYBACK, 0 );
    if ( err != 0)
    {
        qDebug("open error: %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW playback, can't open "+sCurrentDevice);
    }

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    /* Choose all parameters */
    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0) {
        qDebug("Broken configuration : no configurations available: %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {
        qDebug("Access type not available : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");

    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        qDebug("Sample format not available : %s", snd_strerror(err));
        throw CGenErr(string("alsa CSoundOut::Init_HW ")+snd_strerror(err));
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, hwparams, NUM_OUT_CHANNELS);
    if (err < 0) {
        qDebug("Channels count (%i) not available s: %s", NUM_OUT_CHANNELS, snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Set the stream rate */
    dir=0;
    err = snd_pcm_hw_params_set_rate(handle, hwparams, iSampleRate, dir);
    if (err < 0) {
        qDebug("Rate %iHz not available : %s", iSampleRate, snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    dir=0;
    unsigned int buffer_time = 500000;              /* ring buffer length in us */
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
    if (err < 0) {
        qDebug("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
    if (err < 0) {
        qDebug("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    // qDebug("buffer size %d", buffer_size);
    /* set the period time */
    unsigned int period_time = 100000;              /* period time in us */
    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
    if (err < 0) {
        qDebug("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    err = snd_pcm_hw_params_get_period_size_min(hwparams, &period_size, &dir);
    if (err < 0) {
        qDebug("Unable to get period size for playback: %s\n", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    // qDebug("period size %d", period_size);

    /* Write the parameters to device */
    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0) {
        qDebug("Unable to set hw params : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        qDebug("Unable to determine current swparams : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    /* Write the parameters to the playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        qDebug("Unable to set sw params : %s", snd_strerror(err));
        throw CGenErr("alsa CSoundOut::Init_HW ");
    }
    snd_pcm_start(handle);
    qDebug("alsa init done");

}

int CSoundOut::write_HW( _SAMPLE *playbuf, int size )
{

    int start = 0;
    int ret;

    while (size) {

        ret = snd_pcm_writei(handle, &playbuf[start], size );
        if (ret < 0) {
            if (ret ==  -EAGAIN) {
                if ((ret = snd_pcm_wait (handle, 100)) < 0) {
                    qDebug ("poll failed (%s)", snd_strerror (ret));
                    break;
                }
                continue;
            } else if (ret == -EPIPE) {   /* under-run */
                qDebug("underrun");
                ret = snd_pcm_prepare(handle);
                if (ret < 0)
                    qDebug("Can't recover from underrun, prepare failed: %s", snd_strerror(ret));
                continue;
            } else if (ret == -ESTRPIPE) {
                qDebug("strpipe");
                while ((ret = snd_pcm_resume(handle)) == -EAGAIN)
                    sleep(1);       /* wait until the suspend flag is released */
                if (ret < 0) {
                    ret = snd_pcm_prepare(handle);
                    if (ret < 0)
                        qDebug("Can't recover from suspend, prepare failed: %s", snd_strerror(ret));
                }
                continue;
            } else {
                qDebug("Write error: %s", snd_strerror(ret));
                throw CGenErr("Write error");
            }
            break;  /* skip one period */
        }
        size -= ret;
        start += ret;
    }
    return 0;
}

void CSoundOut::close_HW( void )
{

    if (handle != NULL)
        snd_pcm_close( handle );

    handle = NULL;
}
