/******************************************************************************\
 *
 * Copyright (c) 2012
 *
 * Author(s):
 *	David Flamand
 *
 * Decription:
 *  PulseAudio sound interface with clock drift adjustment (optional)
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

#include <time.h>
#include <string.h>
#include <vector>
#include "drm_pulseaudio.h"

#ifndef PA_STREAM_ADJUST_LATENCY
# define PA_STREAM_ADJUST_LATENCY 0
#endif
#ifndef PA_STREAM_DONT_MOVE
# define PA_STREAM_DONT_MOVE 0
#endif

#define APP_NAME_RECEIVER    "Dream Receiver" // PulseAudio stream name
#define APP_NAME_TRANSMITTER "Dream Transmitter" // PulseAudio stream name
#define PLAYBACK_BUFFER  2000000 // us
#define PLAYBACK_LATENCY 1000000 // us
#define WAIT_PREBUFFER ((int)(PLAYBACK_LATENCY + 399999) / 400000) // 400000us = .4s (frame duration)
#define NUM_CHANNELS 2 // Stereo
#define BYTES_PER_SAMPLE ((int)sizeof(_SAMPLE))
#define PA_RECORD_MAXLENGTH -1//((int)((double)(NUM_CHANNELS * BYTES_PER_SAMPLE * iSampleRate) * ((double)RECORD_BUFFER)  / (double)1000000))
#define PA_RECORD_FRAGSIZE (2560*BYTES_PER_SAMPLE/2)
#define PA_PLAYBACK_TLENGTH ((int)((double)(NUM_CHANNELS * BYTES_PER_SAMPLE * iSampleRate) * ((double)PLAYBACK_BUFFER)  / (double)1000000))
#define PA_PLAYBACK_PREBUF  ((int)((double)(NUM_CHANNELS * BYTES_PER_SAMPLE * iSampleRate) * ((double)PLAYBACK_LATENCY) / (double)1000000))
#define STREAM_FLAGS (PA_STREAM_ADJUST_LATENCY | PA_STREAM_DONT_MOVE)


#define DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__)


#ifdef CLOCK_DRIFT_ADJ_ENABLED
/* TODO more optimized algorithm and/or parameters */
# define ALGO_ERROR_MULTIPLIER 256.0
# define ALGO_ERROR_EXPONENT 2.0
# define FIR_NTAPS 31
static const double fir_taps[FIR_NTAPS] = {
	3.101997697081398790e-04,	8.886432578895606680e-04,	2.034224618317520290e-03,	3.972795457854356310e-03,
	6.946703306811025623e-03,	1.115796813527233054e-02,	1.672148617456211198e-02,	2.362113883029164432e-02,
	3.167801425275264954e-02,	4.053912141968199490e-02,	4.969219159313521123e-02,	5.850780817682837975e-02,
	6.630498434957272924e-02,	7.243147253964839216e-02,	7.634661516740624820e-02,	7.769326590053561576e-02,
	7.634661516740624820e-02,	7.243147253964839216e-02,	6.630498434957272924e-02,	5.850780817682837975e-02,
	4.969219159313521123e-02,	4.053912141968199490e-02,	3.167801425275264954e-02,	2.362113883029164432e-02,
	1.672148617456211198e-02,	1.115796813527233054e-02,	6.946703306811025623e-03,	3.972795457854356310e-03,
	2.034224618317520290e-03,	8.886432578895606680e-04,	3.101997697081398790e-04
};
#endif


static int pa_c_sync(pa_mainloop *pa_m, pa_context *pa_c, int error)
{
	int retval;
	pa_context_state_t pa_c_s;
	if (error==PA_OK) {
		do {
			pa_mainloop_iterate(pa_m, 1, &retval);
			pa_c_s = pa_context_get_state(pa_c);
		} while (pa_c_s!=PA_CONTEXT_READY && pa_c_s!=PA_CONTEXT_FAILED);
		return pa_c_s==PA_CONTEXT_READY ? PA_OK : PA_ERR_MAX;
	}
	DEBUG_MSG("pa_c_sync failed, error %i\n", error);
	return error;
}
static int pa_o_sync(pa_mainloop *pa_m, pa_context *pa_c, pa_operation *pa_o)
{
	int retval, error;
	pa_operation_state pa_o_s;
	if (pa_o) {
		do {
			pa_mainloop_iterate(pa_m, 1, &retval);
			pa_o_s = pa_operation_get_state(pa_o);
		} while (pa_o_s==PA_OPERATION_RUNNING);
		pa_operation_unref(pa_o);
		return pa_o_s==PA_OPERATION_DONE ? PA_OK : PA_ERR_MAX;
	}
	error = pa_context_errno(pa_c);
	DEBUG_MSG("pa_o_sync failed, error %i\n", error);
	return error;
}
static int pa_s_sync(pa_mainloop *pa_m, pa_context *pa_c, pa_stream *pa_s, int error)
{
	(void)pa_c;
	int retval;
	pa_stream_state pa_s_s;
	if (error==PA_OK) {
		do {
			pa_mainloop_iterate(pa_m, 1, &retval);
			pa_s_s = pa_stream_get_state(pa_s);
		} while (pa_s_s==PA_STREAM_CREATING);
		return pa_s_s==PA_STREAM_READY ? PA_OK : PA_ERR_MAX;
	}
	DEBUG_MSG("pa_s_sync failed, error %i\n", error);
	return error;
}

static int pa_init(pa_mainloop **pa_m, pa_context **pa_c, int transmitter)
{
	int ret;
	pa_mainloop *pa_m_tmp;
	pa_context *pa_c_tmp;

	pa_m_tmp = pa_mainloop_new();
	if (!pa_m_tmp) {
		DEBUG_MSG("pa_mainloop_new failed\n");
		return PA_ERR_MAX;
	}

	pa_c_tmp = pa_context_new(pa_mainloop_get_api(pa_m_tmp), transmitter < 0 ? "" : (transmitter ? APP_NAME_TRANSMITTER : APP_NAME_RECEIVER));
	if (!pa_c_tmp) {
		DEBUG_MSG("pa_context_new failed\n");
		pa_mainloop_free(pa_m_tmp);
		return PA_ERR_MAX;
	}

	ret = pa_context_connect(pa_c_tmp, NULL, PA_CONTEXT_NOFLAGS, NULL);
	if (pa_c_sync(pa_m_tmp, pa_c_tmp, ret)!=PA_OK) {
		DEBUG_MSG("pa_context_connect failed\n");
		pa_context_unref(pa_c_tmp);
		pa_mainloop_free(pa_m_tmp);
		return PA_ERR_MAX;
	}

	*pa_m = pa_m_tmp;
	*pa_c = pa_c_tmp;
	return PA_OK;
}

static void pa_free(pa_mainloop **pa_m, pa_context **pa_c)
{
	if (*pa_c) {
		pa_context_disconnect(*pa_c);
		pa_context_unref(*pa_c);
		*pa_c = NULL;
	}
	if (*pa_m) {
		pa_mainloop_free(*pa_m);
		*pa_m = NULL;
	}
}

static void pa_s_free(pa_stream **pa_s)
{
	if (*pa_s) {
		pa_stream_disconnect(*pa_s);
		pa_stream_unref(*pa_s);
		*pa_s = NULL;
	}
}

static void pa_stream_notify_cb(pa_stream *p, void *userdata)
{
	(void)p;
	pa_stream_notify_cb_userdata_t* ud = (pa_stream_notify_cb_userdata_t*)userdata;
	if (!ud->bMute)
	{
		const char *type = ud->bOverflow ? "OVERFLOW" : "UNDERFLOW";
		if (!((CSoundOutPulse*)ud->SoundIO)->bPrebuffer) {
			DEBUG_MSG("*** playback %s\n", type);
		}
	}
	if (ud->bOverflow)
		((CSoundOutPulse*)ud->SoundIO)->bSeek = TRUE;
	else
		((CSoundOutPulse*)ud->SoundIO)->bPrebuffer = TRUE;
}

static void pa_stream_success_cb(pa_stream *s, int success, void *userdata)
{
	(void)s;
	if (userdata) {
		DEBUG_MSG("pa_stream_success_cb(%s) = %i\n", (char*)userdata, success);
	}
}

#ifdef CLOCK_DRIFT_ADJ_ENABLED
static void pa_set_sample_rate(pa_mainloop *pa_m, pa_context *pa_c, pa_stream *pa_s, int sample_rate)
{
	if (pa_o_sync(pa_m, pa_c, pa_stream_update_sample_rate(pa_s, sample_rate, pa_stream_success_cb, NULL)) != PA_OK)
		DEBUG_MSG("pa_set_sample_rate(%i): pa_stream_update_sample_rate failed\n", sample_rate);
}
#endif


/* Implementation *************************************************************/


void CSoundInPulse::Init_HW()
{
	int ret;
	pa_sample_spec ss;
	pa_buffer_attr pa_attr;
	const char *recdevice=NULL;

	DEBUG_MSG("CSoundInPulse::Init_HW()\n");

	ss.format = PA_SAMPLE_S16NE;
	ss.channels = NUM_CHANNELS;
	ss.rate = iSampleRate;

	/* record device */
	if(devices.size()==0)
		throw CGenErr("pulseaudio CSoundInPulse::Init_HW no record devices available!");

	if(iCurrentDevice < 0 || iCurrentDevice >= int(devices.size()))
		iCurrentDevice = 0;

	if (iCurrentDevice)
		recdevice = devices[iCurrentDevice].c_str();

	if (pa_m != NULL)
		throw CGenErr("pulseaudio CSoundInPulse::Init_HW already init");

	if (pa_init(&pa_m, &pa_c, !bBlockingRec)!=PA_OK)
		throw CGenErr("pulseaudio CSoundInPulse::Init_HW pa_init failed");

	pa_s = pa_stream_new(pa_c,	// The context to create this stream in
		"input",				// A name for this stream
		&ss,					// Our sample format.
		NULL					// Use default channel map
		);
	if (!pa_s)
		throw CGenErr("pulseaudio CSoundInPulse::Init_HW pa_stream_new failed");

	pa_attr.maxlength = PA_RECORD_MAXLENGTH;	// Maximum length of the buffer.
	pa_attr.tlength   = -1;						// Playback only: target length of the buffer.
	pa_attr.prebuf    = -1; 					// Playback only: pre-buffering.
	pa_attr.minreq    = -1;						// Playback only: minimum request.
	pa_attr.fragsize  = iBufferSize*BYTES_PER_SAMPLE;//PA_RECORD_FRAGSIZE;		// Recording only: fragment size.

	ret = pa_stream_connect_record(pa_s,	// The stream to connect to a source 
		recdevice,							// Name of the source to connect to, or NULL for default
		&pa_attr,							// Buffer attributes, or NULL for default
#ifdef CLOCK_DRIFT_ADJ_ENABLED
		(pa_stream_flags_t)(STREAM_FLAGS | (!bBlockingRec ? PA_STREAM_VARIABLE_RATE : 0)) // Additional flags, or 0 for default
#else
		(pa_stream_flags_t)STREAM_FLAGS		// Additional flags, or 0 for default
#endif
		);
	if (pa_s_sync(pa_m, pa_c, pa_s, ret) != PA_OK)
		throw CGenErr("pulseaudio CSoundInPulse::Init_HW pa_stream_connect_record failed");

	remaining_nbytes = 0;
	remaining_data   = NULL;

	DEBUG_MSG("pulseaudio input device '%s', init done\n", devices[iCurrentDevice].c_str());
}

int CSoundInPulse::Read_HW(void *recbuf, int size)
{
	int ret, retval, filled, chunk;
	const char *data;
	size_t nbytes;

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	if (!bBlockingRec && cp) {
		if (bClockDriftComp != cp->bClockDriftComp) {
			bClockDriftComp = cp->bClockDriftComp;
			DEBUG_MSG("CSoundInPulse::Read_HW(): bClockDriftComp=%i\n", bClockDriftComp);
			if (!bClockDriftComp)
				pa_set_sample_rate(pa_m, pa_c, pa_s, iSampleRate);
		}
		int sample_rate = iSampleRate - cp->sample_rate_offset;
		if (record_sample_rate != sample_rate) {
			record_sample_rate = sample_rate;
			pa_set_sample_rate(pa_m, pa_c, pa_s, sample_rate);
		}
	}
#endif

	filled = 0;
	size *= BYTES_PER_SAMPLE;

	while (size) {
		if (!remaining_nbytes) {
			ret = pa_mainloop_iterate(pa_m, 1, &retval);
			if (ret < 0) break;
			nbytes = 0;
			data   = NULL;
			ret = pa_stream_peek(pa_s, (const void **)&data, &nbytes);
			if (ret != PA_OK) break;
		}
		else {
			nbytes = remaining_nbytes;
			data   = remaining_data;
		}
		if (data) {
/*			int negative;
			pa_usec_t pa_usec;
			pa_usec = 0;
			if (pa_o_sync(pa_m, pa_c, pa_stream_update_timing_info(pa_s, pa_stream_success_cb, NULL))>=0) {
				ret = pa_stream_get_latency(pa_s, &pa_usec, &negative);
				if (ret == PA_OK) {
					DEBUG_MSG("record latency: %i us\n", (int)pa_usec);
				}
			}
*/			if (nbytes > (size_t)size) {
				chunk = size;
				remaining_nbytes = nbytes - chunk;
				remaining_data   = data   + chunk;
//				DEBUG_MSG("pa_stream_peek frag %6i %6i\n", (int)nbytes, chunk);
				memcpy(recbuf, data, chunk);
			}
			else {
				chunk = (int)nbytes;
				remaining_nbytes = 0;
				remaining_data   = NULL;
//				DEBUG_MSG("pa_stream_peek full %6i %6i\n", (int)nbytes, chunk);
				memcpy(recbuf, data, chunk);
				pa_stream_drop(pa_s); // <- after memcpy
			}
			filled += chunk;
			size -= chunk;
			recbuf = ((char*)recbuf)+chunk;
		}
	};

//	DEBUG_MSG("CSoundInPulse::read_HW filled %6i\n", filled);
	return filled / BYTES_PER_SAMPLE;
}

void CSoundInPulse::Close_HW()
{
	DEBUG_MSG("CSoundInPulse::close_HW()\n");
	remaining_nbytes = 0;
	remaining_data   = NULL;
	pa_s_free(&pa_s);
	pa_free(&pa_m, &pa_c);
}

void CSoundInPulse::SetBufferSize_HW()
{
#ifdef PA_STREAM_FIX_RATE /* used to check for at least version 0.9.8 */
	pa_buffer_attr pa_attr;
	pa_attr.maxlength = PA_RECORD_MAXLENGTH;			// Maximum length of the buffer.
	pa_attr.tlength   = -1;								// Playback only: target length of the buffer.
	pa_attr.prebuf    = -1; 							// Playback only: pre-buffering.
	pa_attr.minreq    = -1;								// Playback only: minimum request.
	pa_attr.fragsize  = iBufferSize*BYTES_PER_SAMPLE;	// Recording only: fragment size.
	if (pa_o_sync(pa_m, pa_c, pa_stream_set_buffer_attr(pa_s, &pa_attr, pa_stream_success_cb, NULL)) != PA_OK)
	{
		DEBUG_MSG("CSoundInPulse::SetBufferSize_HW() error\n");
	}
	else {
		DEBUG_MSG("CSoundInPulse::SetBufferSize_HW() success\n");
	}
#endif
}

void CSoundOutPulse::Init_HW()
{
	int ret;
	pa_sample_spec ss;
	pa_buffer_attr pa_attr;
	const char *playdevice=NULL;

	DEBUG_MSG("CSoundOutPulse::Init_HW()\n");

	ss.format = PA_SAMPLE_S16NE;
	ss.channels = NUM_CHANNELS;
	ss.rate = iSampleRate;

	/* playback device */
	if(devices.size()==0)
		throw CGenErr("CSoundOutPulse::Init_HW no playback devices available!");

	if(iCurrentDevice < 0 || iCurrentDevice >= int(devices.size()))
		iCurrentDevice = 0;

	if (iCurrentDevice)
		playdevice = devices[iCurrentDevice].c_str();
	
	if (pa_m != NULL)
		throw CGenErr("CSoundOutPulse::Init_HW already init");

	if (pa_init(&pa_m, &pa_c, bBlockingPlay)!=PA_OK)
		throw CGenErr("CSoundOutPulse::Init_HW pa_init failed");

	pa_s = pa_stream_new(pa_c,	// The context to create this stream in
		"output",				// A name for this stream
		&ss,					// Our sample format.
		NULL					// Use default channel map
		);
	if (!pa_s)
		throw CGenErr("CSoundOutPulse::Init_HW pa_stream_new failed");

	pa_attr.maxlength = PA_PLAYBACK_TLENGTH; // Maximum length of the buffer.
	pa_attr.tlength   = PA_PLAYBACK_TLENGTH; // Playback only: target length of the buffer.
	pa_attr.prebuf    = PA_PLAYBACK_PREBUF;  // Playback only: pre-buffering.
	pa_attr.minreq    = -1; // Playback only: minimum request.
	pa_attr.fragsize  = -1; // Recording only: fragment size.

	ret = pa_stream_connect_playback(pa_s,	// The stream to connect to a sink
		playdevice,							// Name of the source to connect to, or NULL for default
		&pa_attr,							// Buffer attributes, or NULL for default
#ifdef CLOCK_DRIFT_ADJ_ENABLED
		(pa_stream_flags_t)(STREAM_FLAGS | (!bBlockingPlay ? PA_STREAM_VARIABLE_RATE : 0)), // Additional flags, or 0 for default
#else
		(pa_stream_flags_t)STREAM_FLAGS,	// Additional flags, or 0 for default
#endif
		NULL,								// Initial volume, or NULL for default
		NULL								// Synchronize this stream with the specified one, or NULL for a standalone stream
		);
	if (pa_s_sync(pa_m, pa_c, pa_s, ret) != PA_OK)
		throw CGenErr("CSoundOutPulse::Init_HW pa_stream_connect_playback failed");

	pa_stream_notify_cb_userdata_underflow.SoundIO = this;
	pa_stream_notify_cb_userdata_underflow.bOverflow = FALSE;
	pa_stream_notify_cb_userdata_underflow.bMute = FALSE;
	pa_stream_set_underflow_callback(pa_s, &pa_stream_notify_cb, (void*)&pa_stream_notify_cb_userdata_underflow);
	pa_stream_notify_cb_userdata_overflow.SoundIO = this;
	pa_stream_notify_cb_userdata_overflow.bOverflow = TRUE;
	pa_stream_notify_cb_userdata_overflow.bMute = FALSE;
	pa_stream_set_overflow_callback(pa_s, &pa_stream_notify_cb, (void*)&pa_stream_notify_cb_userdata_overflow);

//	pa_o_sync(pa_m, pa_c, pa_stream_update_sample_rate(pa_s, iSampleRate+750, pa_stream_success_cb, NULL));
//	playback_latency_usec = PLAYBACK_LATENCY;
//	playback_sample_rate = iSampleRate;

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	X.Init(1);
	A.Init(1, 1.0);
	B.Init(FIR_NTAPS);
	for (int i = 0; i < FIR_NTAPS; i++)
		B[i] = fir_taps[i];
#endif

	DEBUG_MSG("pulseaudio output device '%s', init done\n", devices[iCurrentDevice].c_str());
}

int CSoundOutPulse::Write_HW(void *playbuf, int size)
{
	int ret;
	int retval;
//	if (!bTransmitter)
//		size += 4 * 200;

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	_BOOLEAN bInitClockDriftComp = FALSE;
	if (cp.bClockDriftComp != bNewClockDriftComp) {
		cp.bClockDriftComp = bNewClockDriftComp;
		DEBUG_MSG("CSoundOutPulse::write_HW(): bClockDriftComp=%i\n", cp.bClockDriftComp);
		if (cp.bClockDriftComp)
			bInitClockDriftComp = TRUE;
		else
			if (!bBlockingPlay)
				pa_set_sample_rate(pa_m, pa_c, pa_s, iSampleRate);
	}
#endif

	if (bPrebuffer) {
//		pa_o_sync(pa_m, pa_c, pa_stream_prebuf(pa_s, pa_stream_success_cb, NULL);
//		DEBUG_MSG("CSoundOutPulse::write_HW(): prebuffering ...\n");
		if (pa_o_sync(pa_m, pa_c, pa_stream_prebuf(pa_s, pa_stream_success_cb, NULL)) != PA_OK)
			DEBUG_MSG("CSoundOutPulse::write_HW(): prebuffering failed\n");
//		else
//			DEBUG_MSG("CSoundOutPulse::write_HW(): prebuffering success\n");
		bPrebuffer = FALSE;
		bSeek = FALSE;
#ifdef CLOCK_DRIFT_ADJ_ENABLED
		bInitClockDriftComp = TRUE;
#endif
	}

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	if (cp.bClockDriftComp) {
		if (bInitClockDriftComp) {
			playback_usec_smoothed = PLAYBACK_LATENCY;
			target_latency = PLAYBACK_LATENCY;
			wait_prebuffer = WAIT_PREBUFFER;
			cp.sample_rate_offset = 0;
			clock = 0;
//			filter_stabilized = 16;
			filter_stabilized = FIR_NTAPS;
			Z.Init(FIR_NTAPS, CReal(PLAYBACK_LATENCY));
		}

		if (!bBlockingPlay)
/*Receiver*/	DEBUG_MSG("playback latency: %07i us, smoothed %07i us, %i, %02i, %02i, %i\n", (int)playback_usec, (int)playback_usec_smoothed, iSampleRate + cp.sample_rate_offset, wait_prebuffer, filter_stabilized, ++clock);
		else
/*Transmitter*/	DEBUG_MSG("playback latency: %07i us, smoothed %07i us, %i, %02i, %02i, %i\n", (int)playback_usec, (int)playback_usec_smoothed, iSampleRate - cp.sample_rate_offset, wait_prebuffer, filter_stabilized, ++clock);

		if (wait_prebuffer > 0) {
			wait_prebuffer--;
		}
		else {
			if (!filter_stabilized) {
				/****************************************************************************************************************/
				/* The Clock Drift Adjustment Algorithm                                                                        */
				int offset;
				double error = (playback_usec_smoothed - (double)target_latency) / (double)target_latency * ALGO_ERROR_MULTIPLIER;
//				error = error * error * (error >= 0.0 ? 1.0 : -1.0);
				error = pow(fabs(error), ALGO_ERROR_EXPONENT) * (error >= 0.0 ? 1.0 : -1.0);
				if (error >= 0.0) offset = (int)floor(error + 0.5);
				else              offset = (int) ceil(error - 0.5);
				if      (offset>iMaxSampleRateOffset) offset=iMaxSampleRateOffset;
				else if (offset<-iMaxSampleRateOffset) offset=-iMaxSampleRateOffset;
				/****************************************************************************************************************/
				if (!bBlockingPlay && cp.sample_rate_offset != offset) {
					pa_set_sample_rate(pa_m, pa_c, pa_s, iSampleRate + offset);
				}
				cp.sample_rate_offset = offset;
			}
		}
	}
#endif

	ret = pa_stream_write(pa_s,		// The stream to use
		playbuf,					// The data to write
		size * BYTES_PER_SAMPLE,	// The length of the data to write in bytes
		NULL,						// A cleanup routine for the data or NULL to request an internal copy
		bSeek ? -(PA_PLAYBACK_PREBUF/2) : 0,	// Offset for seeking, must be 0 for upload streams
		PA_SEEK_RELATIVE			// Seek mode, must be PA_SEEK_RELATIVE for upload streams
		);

#ifdef CLOCK_DRIFT_ADJ_ENABLED
	if (cp.bClockDriftComp) {
		if (!wait_prebuffer) {
			if (pa_o_sync(pa_m, pa_c, pa_stream_update_timing_info(pa_s, pa_stream_success_cb, NULL)) == PA_OK) {
				const pa_timing_info *ti;
				ti = pa_stream_get_timing_info(pa_s);
				if (ti && !ti->write_index_corrupt && !ti->read_index_corrupt) {
					uint64_t samples = abs(ti->write_index - ti->read_index) / (NUM_CHANNELS*BYTES_PER_SAMPLE);
					uint64_t usec = samples * (uint32_t)1000000 / (uint32_t)iSampleRate;
					playback_usec = usec;
					X[0] = CReal(usec);
					playback_usec_smoothed = Filter(B, A, X, Z)[0];
//					playback_usec_smoothed = (playback_usec_mean * (ALGO_MEAN_SAMPLE-1) + usec) / ALGO_MEAN_SAMPLE;
					if (filter_stabilized > 0) {
						filter_stabilized--;
						if(!filter_stabilized) {
							target_latency = playback_usec_smoothed;
						}
					}
				}
			}
		}
	}
#endif

/*
//	if (bBlockingPlay) {
		if (pa_o_sync(pa_m, pa_c, pa_stream_update_timing_info(pa_s, pa_stream_success_cb, NULL)) == PA_OK) {
			pa_usec_t pa_usec = 0;
			int negative;
			ret = pa_stream_get_latency(pa_s, &pa_usec, &negative);
			if (ret == PA_OK) {
				DEBUG_MSG("pa_usleep: pa_usec %i us\n", (int)pa_usec);
//				if (bSeek)
//					pa_usec_mean = PLAYBACK_LATENCY;
//				DEBUG_MSG("playback latency: %i us, mean %i us, %i %i\n", (int)pa_usec, (int)pa_usec_mean, bPrebuffer, bSeek);
				//if (bBlockingPlay && pa_usec > PLAYBACK_LATENCY) {
				//	pa_usleep(pa_usec - PLAYBACK_LATENCY);
				//	DEBUG_MSG("pa_usleep: r_usec %i, usleep %i us\n", (int)pa_usec, (int)(pa_usec - PLAYBACK_LATENCY));
				//}
//				if (pa_usec > 0) {
//					playback_latency_usec = pa_usec;
//					if (!skip)
//						pa_usec_mean = (pa_usec_mean * 15 + pa_usec) / 16;
//				}
			}
		}
//	}
*/
//	if (bSeek) {
//		bTransmitter = TRUE;
//	}
	bSeek = FALSE;

	if (ret == PA_OK) {
		do {
			ret = pa_mainloop_iterate(pa_m, 0, &retval);
		} while (ret>0);
		return size;
	}

	return -1;
}

void CSoundOutPulse::Close_HW()
{
	DEBUG_MSG("CSoundOutPulse::close_HW()\n");
	pa_stream_notify_cb_userdata_underflow.bMute = TRUE;
	pa_stream_notify_cb_userdata_overflow.bMute = TRUE;
	if (bBlockingPlay && pa_m!=NULL && pa_c!=NULL && pa_s!=NULL)
	{
		if (pa_o_sync(pa_m, pa_c, pa_stream_drain(pa_s, pa_stream_success_cb, NULL)) != PA_OK)
		{
			DEBUG_MSG("CSoundOutPulse::close_HW() pa_stream_drain failed\n");
		}		
	}
	pa_s_free(&pa_s);
	pa_free(&pa_m, &pa_c);
}


/****************/
/* devices list */

typedef struct _USERDATA {
	vector < string > *names;
	vector < string > *devices;
} USERDATA;

void my_pa_source_info_cb_t(pa_context *c, const pa_source_info *i, int eol, void *userdata)
{
	(void)c;
	if (!eol)
	{
		((USERDATA*)userdata)->names->push_back(string(i->name) + " [" + string(i->description) + "]");
		((USERDATA*)userdata)->devices->push_back(i->name);
	}
}
void my_pa_sink_info_cb_t(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
	(void)c;
	if (!eol)
	{
		((USERDATA*)userdata)->names->push_back(string(i->name) + " [" + string(i->description) + "]");
		((USERDATA*)userdata)->devices->push_back(i->name);
	}
}

void
getdevices(vector < string > &names, vector < string > &devices,
		   bool playback)
{
	pa_mainloop *pa_m;
	pa_context *pa_c;
	pa_operation *pa_o;
	USERDATA userdata;

	names.clear();
	devices.clear();
	names.push_back("[default]");
	devices.push_back("[default]");

	if (pa_init(&pa_m, &pa_c, -1) != PA_OK)
	{
		DEBUG_MSG("pa_init failed\n");
		return;
	}

	userdata.names = &names;
	userdata.devices = &devices;

	if (playback)
		pa_o = pa_context_get_sink_info_list(pa_c, my_pa_sink_info_cb_t, &userdata);
	else
		pa_o = pa_context_get_source_info_list(pa_c, my_pa_source_info_cb_t, &userdata);

	if (pa_o_sync(pa_m, pa_c, pa_o) != PA_OK)
		DEBUG_MSG("pa_context_get_(sink/source)_info_list failed\n");

	pa_free(&pa_m, &pa_c);
}


/*********************************************************************************************************************/


/* Wave in ********************************************************************/

CSoundInPulse::CSoundInPulse():
	iBufferSize(0),devices(),names(),bChangDev(TRUE),iCurrentDevice(-1),
	pa_m(NULL),pa_c(NULL),pa_s(NULL),
	remaining_nbytes(0),remaining_data(NULL)
#ifdef CLOCK_DRIFT_ADJ_ENABLED
	,record_sample_rate(0),bClockDriftComp(FALSE),cp(NULL)
#endif
{
	/* Get devices list */
	getdevices(names, devices, false);
}

void CSoundInPulse::Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	DEBUG_MSG("initrec iNewBufferSize=%i bNewBlocking=%i\n", iNewBufferSize, bNewBlocking);

	/* Save samplerate and blocking mode */
	iSampleRate = iNewSampleRate;
	bBlockingRec = bNewBlocking;

	/* Check if device must be opened or reinitialized */
	if (bChangDev == TRUE)
	{
		/* Save buffer size */
		iBufferSize = iNewBufferSize;

		/* Close the previous input */
		Close_HW();

		/* Open the new input */
		Init_HW();

		/* Reset flag */
		bChangDev = FALSE;
	}
	else {
		if (iBufferSize != iNewBufferSize)
		{
			/* Save buffer size */
			iBufferSize = iNewBufferSize;

			/* Set buffer size */
			SetBufferSize_HW();
		}
	}
}

_BOOLEAN CSoundInPulse::Read(CVector<_SAMPLE>& psData)
{
	/* Check if device must be opened or reinitialized */
	if (bChangDev == TRUE)
	{
		/* Reinit sound interface */
		Init(iBufferSize, bBlockingRec);

		/* Reset flag */
		bChangDev = FALSE;
	}

	/* Read from 'hardware' */
	if (Read_HW(&psData[0], iBufferSize) != iBufferSize)
	{
	    DEBUG_MSG("CSoundInPulse::Read(): read_HW error\n");
	}

	return FALSE;
}

void CSoundInPulse::Close()
{
	DEBUG_MSG("stoprec\n");

	/* Close the input */
	Close_HW();

	/* Set flag to open devices the next time it is initialized */
	bChangDev = TRUE;
}

void CSoundInPulse::SetDev(int iNewDevice)
{
	/* Change only in case new device id is not already active */
	if (iNewDevice != iCurrentDevice)
	{
		iCurrentDevice = iNewDevice;
		bChangDev = TRUE;
	}
}

int CSoundInPulse::GetDev()
{
	return iCurrentDevice;
}


/* Wave out *******************************************************************/

CSoundOutPulse::CSoundOutPulse():
	iBufferSize(0),devices(),names(),bChangDev(TRUE),iCurrentDevice(-1),
	pa_m(NULL),pa_c(NULL),pa_s(NULL)
#ifdef CLOCK_DRIFT_ADJ_ENABLED
//	,bNewClockDriftComp(TRUE),cp()
	,bNewClockDriftComp(FALSE),cp()
#endif
{
	/* Get devices list */
	getdevices(names, devices, true);
}

void CSoundOutPulse::Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	DEBUG_MSG("initplay iNewBufferSize=%i bNewBlocking=%i\n", iNewBufferSize, bNewBlocking);

	/* Save samplerate, blocking mode and buffer size */
	iSampleRate = iNewSampleRate;
	bBlockingPlay = bNewBlocking;
	iBufferSize = iNewBufferSize;

	/* Check if device must be opened or reinitialized */
	if (bChangDev == TRUE)
	{
		/* Close the previous input */
		Close_HW();

		/* Open the new input */
		Init_HW();

		/* Reset flag */
		bChangDev = FALSE;
	}

	/* Set prebuffer flag */
	bPrebuffer = TRUE;
}

_BOOLEAN CSoundOutPulse::Write(CVector<_SAMPLE>& psData)
{
	/* Check if device must be opened or reinitialized */
	if (bChangDev == TRUE)
	{
		/* Reinit sound interface */
		Init(iBufferSize, bBlockingPlay);

		/* Reset flag */
		bChangDev = FALSE;
	}

	/* Write to 'hardware' */
	if (Write_HW(&psData[0], iBufferSize) != iBufferSize)
	{
		DEBUG_MSG("CSoundOutPulse::Write(): write_HW error\n");
	}

	return FALSE;
}

void CSoundOutPulse::Close()
{
	DEBUG_MSG("stopplay\n");

	/* Close the output */
	Close_HW();

	/* Set flag to open devices the next time it is initialized */
	bChangDev = TRUE;
}

void CSoundOutPulse::SetDev(int iNewDevice)
{
	/* Change only in case new device id is not already active */
	if (iNewDevice != iCurrentDevice)
	{
		iCurrentDevice = iNewDevice;
		bChangDev = TRUE;
	}
}

int CSoundOutPulse::GetDev()
{
	return iCurrentDevice;
}
