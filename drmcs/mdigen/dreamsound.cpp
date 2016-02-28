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
void CSound::InitRecording(int iNewBufferSize, _BOOLEAN bNewBlocking)
{

    /* Save < */
    SoundBufR.lock();
    iInBufferSize = iNewBufferSize;
    bBlockingRec = bNewBlocking;
    SoundBufR.unlock();

    int err, dir;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    unsigned int rrate;
    snd_pcm_uframes_t period_size = FRAGSIZE * NUM_IN_OUT_CHANNELS/2;
    snd_pcm_uframes_t buffer_size;
    int periods = 2;
    snd_pcm_t *  handle;


    static const char *recdevice = "hw:0,0";

    if (rhandle != NULL)
        return;

    err = snd_pcm_open( &rhandle, recdevice, SND_PCM_STREAM_CAPTURE, 0 );
    if ( err != 0)
    {
    }
    handle = rhandle;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    /* Choose all parameters */
    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0) {
    }
    /* Set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);

    if (err < 0) {

    }
    /* Set the sample format */
    err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16);
    if (err < 0) {
    }
    /* Set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, hwparams, NUM_IN_OUT_CHANNELS);
    if (err < 0) {
    }
    /* Set the stream rate */
    rrate = SOUNDCRD_SAMPLE_RATE;
    err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rrate, &dir);
    if (err < 0) {

    }
    if (rrate != SOUNDCRD_SAMPLE_RATE) {
    }

    dir=0;
    unsigned int buffer_time = 500000;              /* ring buffer length in us */
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
    if (err < 0) {
    }
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
    if (err < 0) {
    }
    /* set the period time */
    unsigned int period_time = 100000;              /* period time in us */
    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
    if (err < 0) {
    }
    err = snd_pcm_hw_params_get_period_size(hwparams, &period_size, &dir);
    if (err < 0) {
    }

    /* Write the parameters to device */
    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0) {
    }
    /* Get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
    }
    /* Start the transfer when the buffer immediately */
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 0);
    if (err < 0) {
    }
    /* Allow the transfer when at least period_size samples can be processed */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
    if (err < 0) {
    }
    /* Align all transfers to 1 sample */
    err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1);
    if (err < 0) {
    }
    /* Write the parameters to the record/playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
    }
    snd_pcm_start(handle);

    if ( RecThread1.running() == FALSE ) {
        SoundBufR.Init( SOUNDBUFLEN );
        SoundBufR.unlock();
        RecThread1.start();
    }
}

void CSound::Close()
{

    // stop the recording and playback threads

    if (RecThread1.running() ) {
        SoundBufR.keep_running = FALSE;
        // wait 1sec max. for the threads to terminate
        RecThread1.wait(1000);
    }

    if (PlayThread1.running() ) {
        SoundBufP.keep_running = FALSE;
        PlayThread1.wait(1000);
    }

    if (rhandle != NULL)
        snd_pcm_close( rhandle );

    rhandle = NULL;
}

_BOOLEAN CSound::Read(CVector< _SAMPLE >& psData)
{
    CVectorEx<_SAMPLE>*	p;

    SoundBufR.lock();	// we need exclusive access


    while ( SoundBufR.GetFillLevel() < iInBufferSize ) {


        // not enough data, sleep a little
        SoundBufR.unlock();
        usleep(1000); //1ms
        SoundBufR.lock();
    }

    // copy data

    p = SoundBufR.Get( iInBufferSize );
    for (int i=0; i<iInBufferSize; i++)
        psData[i] = (*p)[i];

    SoundBufR.unlock();

    return FALSE;
}


class CSoundBuf : public CCyclicBuffer<_SAMPLE> {

public:
    CSoundBuf() : keep_running(TRUE) {}
    void lock (void) {
        data_accessed.lock();
    }
    void unlock (void) {
        data_accessed.unlock();
    }

    bool keep_running;
protected:
    QMutex	data_accessed;
} SoundBufR;


class RecThread : public QThread {
public:
    virtual void run();
protected:
    _SAMPLE	tmprecbuf[NUM_IN_OUT_CHANNELS * FRAGSIZE];
} RecThread1;


void RecThread::run()
{
    while (SoundBufR.keep_running) {

        int fill;

        SoundBufR.lock();
        fill = SoundBufR.GetFillLevel();
        SoundBufR.unlock();

        if (  (SOUNDBUFLEN - fill) > (FRAGSIZE * NUM_IN_OUT_CHANNELS) ) {
            // enough space in the buffer

            int size = snd_pcm_readi(rhandle, recbuf, size);

            int size = CSound::read_HW( tmprecbuf, FRAGSIZE);

            // common code
            if (size > 0) {
                CVectorEx<_SAMPLE>*	ptarget;

                /* Copy data from temporary buffer in output buffer */
                SoundBufR.lock();

                ptarget = SoundBufR.QueryWriteBuffer();

                for (int i = 0; i < size * NUM_IN_OUT_CHANNELS; i++)
                    (*ptarget)[i] = tmprecbuf[i];

                SoundBufR.Put( size * NUM_IN_OUT_CHANNELS );
                SoundBufR.unlock();
            }
        } else {
            msleep( 1 );
        }
    }
}
