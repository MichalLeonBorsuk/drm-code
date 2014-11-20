/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2005 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 01.11.2004                                                 */
/*  Last change  : 27.04.2005                                                 */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  This program is free software; you can redistribute it and/or modify      */
/*  it under the terms of the GNU General Public License as published by      */
/*  the Free Software Foundation; either version 2 of the License, or         */
/*  (at your option) any later version.                                       */
/*                                                                            */
/*  This program is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/*  GNU General Public License for more details.                              */ 
/*                                                                            */
/*  You should have received a copy of the GNU General Public License         */
/*  along with this program; if not, write to the Free Software               */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/*  Last change: 27.04.2005, 22:00                                            */
/*  By         : Torsten Schorr                                               */
/*  Description: unlocking mutex when canceling playback thread which wasn't  */
/*               fed with data yet                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*  wavio_linux.c                                                             */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  Sound recording and playback for Linux                                    */
/*  Usage:                                                                    */
/*                                                                            */
/*  samples = wavio_linux(0, N, Fs [, ch [, r]]) for recording N samples at   */
/*  sampling frequency Fs from ch channels (default: 1 = mono).               */
/*  Using sample rate conversion, applying the factor r results in            */
/*  conversion of N*r soundcard samples into N output samples.                */
/*                                                                            */
/*  wavio_linux(1, samples, Fs [, r]) for playback of samples at              */
/*  sampling frequency Fs. Using sample rate conversion, applying the         */
/*  factor r results in conversion of N input samples into N*r                */
/*  soundcard samples.                                                        */
/*                                                                            */
/******************************************************************************/


#include <linux/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef _ALSA_
#include <alsa/asoundlib.h>
#endif
#include <sys/soundcard.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "mex.h"

#define PROGNAME "wavio_linux"
#define MAX_DEV_STRING_LENGTH 256
#define DEFAULT_OSS_DEVICE "/dev/dsp"
#define DEFAULT_ALSA_DEVICE "plughw:0,0"
#define CHANNELS 2
#define BUFF_SEC 5	/* 5 seconds are buffered */
#define SAMPLING_RATE 48000 /*48 kHz sample rate */
#define BUFFER_SIZE (CHANNELS*BUFF_SEC*SAMPLING_RATE) /* number of samples that fit into buffer */
#define AUDIO_TIMEOUT 1 /* time to close audio_device after buffer underrun in seconds */

#include "src_filter.h"
#include "wavio_linux.h"

typedef struct _audio_frame {
	int16_t *buffer;
	int length;
	struct _audio_frame *next;
} audio_frame;

typedef enum {OSS, ALSA} Audio_Interface;

static Audio_Interface audio_interface;

static FILE *fid;

static int audio_fd = -1;
#ifdef _ALSA_
static snd_pcm_t *pcm_handle_read;
static snd_pcm_t *pcm_handle_write;
#endif

static int firstcall = 1;
static int out_thread_error = 0;
static double last_delay_play = 0.0;
static double last_delay_rec  = 0.0;
static int last_sampling_rate;
static int base_sampling_rate = SAMPLING_RATE;

static pthread_mutex_t fp_outmutex;		/* = PTHREAD_MUTEX_INITIALIZER; */
static int playing = 0;
static pthread_mutex_t fp_inmutex;		/* = PTHREAD_MUTEX_INITIALIZER; */
static int recording = 0;

static pthread_mutex_t audio_dev_mutex;		/* = PTHREAD_MUTEX_INITIALIZER; */
static int audio_dev_accesses = 0;
static int open_to_read = 0;
static int open_to_write = 0;

static int buffered_bytes = 0;
static audio_frame *last_pointer  = 0;
static audio_frame *frame_pointer = 0;
static int read_index = 0;
static int write_index = 2 * SUBFILTER_LENGTH;
static int16_t *record_buffer;

static pthread_cond_t data_available;		/* = PTHREAD_COND_INITIALIZER; */
static pthread_cond_t rec_samples_avail;	/* = PTHREAD_COND_INITIALIZER; */
static pthread_cond_t start_recording;		/* = PTHREAD_COND_INITIALIZER; */

static pthread_t output_thread;
static pthread_t input_thread;


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	int16_t *samples = 0, *samplesl, *samples_ptr, *buffer_ptr;
	double *double_samples = 0, double_sample;
	int channels = 1, sampling_rate;
	int blocklength = 256;
	int samples_to_read = 0;
	int read_write = 0;
	int m, n, N;
	double Td, ty, r = 1.0;
	int output_length, overall_length, index, index2;
	int mix, dims[2];
	
	/* Check for proper number of arguments */
	if ((nrhs != 1) && (nrhs != 3) && (nrhs != 4) && (nrhs != 5)) {
		mexErrMsgTxt("Usage: "PROGNAME "(0/1, N/samples, sampling_rate [, ch/r [, r]])\n");
	}
  
   /* Check dimensions */
#define	ARG_INDEX 0
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	
	if ((m != 1) || (n != 1)) {
		mexErrMsgTxt(PROGNAME " requires \"read_write\" to be 0 (recording) or 1 (playback).\n");
	}

	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !(mxIsDouble(prhs[ARG_INDEX])) ) {
		mexErrMsgTxt(PROGNAME " requires \"read_write\" to be 0 (recording) or 1 (playback).\n");
	}
	read_write = (int) mxGetScalar(prhs[ARG_INDEX]);
	if ((read_write != 0) && (read_write != 1)) {
		mexErrMsgTxt(PROGNAME " requires \"read_write\" to be 0 (recording) or 1 (playback).\n");
	}
#undef ARG_INDEX

	if (nrhs == 1) {
	
		if (!firstcall) {
		
			if (read_write == 1) {
				pthread_cancel (output_thread);
				pthread_join(output_thread,NULL);
				pthread_create( &output_thread, NULL, (void *) &wavplay_oss_thread, NULL);

			} else if (read_write == 0) {
				pthread_cancel (input_thread);
				pthread_join(input_thread,NULL);
				pthread_create( &input_thread, NULL, (void *) &wavrecord_oss_thread, NULL);
			}
		}	
		plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
		*mxGetPr(plhs[0]) = 0.0;
		return;
	}

#define	ARG_INDEX 1
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	

	if (read_write == 1) {
		if ((n != 1) && ((m != 1) && (m != 2))) {
			mexErrMsgTxt(PROGNAME " requires \"samples\" to be a 1xN (mono) or 2xN (stereo) matrix.\n");
		}	
		channels = 1;
		if ((n != 1) && (m == 1)) {
			N = n;
		} else if ((n != 1) && (m == 2)) {
			N = n;
			channels = 2;
		} else {
			N = m;
		}
		if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
			mxIsSparse(prhs[ARG_INDEX])  || !(mxIsDouble(prhs[ARG_INDEX]) || mxIsInt16(prhs[ARG_INDEX])) ) {
			mexErrMsgTxt(PROGNAME " requires \"samples\" to be a 1xN (mono) or 2xN (stereo) matrix.\n");
		}

		if (mxIsDouble(prhs[ARG_INDEX])) {
			double_samples = (double *)mxGetData(prhs[ARG_INDEX]);
		} else {
			samples = (int16_t *)mxGetData(prhs[ARG_INDEX]);
		}	
	} else {
		if ((m != 1) || (n != 1)) {
			mexErrMsgTxt(PROGNAME " requires \"N\"(number of samples) to be a double scalar >= 0.\n");
		}
		if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
			mxIsSparse(prhs[ARG_INDEX]) ) {
			mexErrMsgTxt(PROGNAME " requires \"N\"(number of samples) to be a double scalar >= 0.\n");
		}	
		
		if (mxIsDouble(prhs[ARG_INDEX])) {
			samples = 0;
		} else {
			samples = (int16_t *)1;
		}
		
		N = (int)mxGetScalar(prhs[ARG_INDEX]);
		if (N < 0) {
			mexErrMsgTxt(PROGNAME " requires \"N\"(number of samples) to be a double scalar >= 0.\n");
		}	
	}
#undef ARG_INDEX
#define	ARG_INDEX 2
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	
	if ((m != 1) || (n != 1)) {
		mexErrMsgTxt(PROGNAME " requires \"sampling_rate\" to be a double scalar.\n");
	}
		
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !(mxIsDouble(prhs[ARG_INDEX])) ) {
		mexErrMsgTxt(PROGNAME " requires \"sampling_rate\" to be a double scalar.\n");
	}
	sampling_rate = (int)mxGetScalar(prhs[ARG_INDEX]);
	
	if ((sampling_rate != 48000) && (sampling_rate != 24000) && (sampling_rate != 12000)) {
		mexErrMsgTxt(PROGNAME ": sampling-rates other than 12kHz, 24kHz and 48kHz not supported yet.\n");
	}
 	
#undef ARG_INDEX

	if (nrhs > 3) {
#define	ARG_INDEX 3
		m = mxGetM(prhs[ARG_INDEX]);
		n = mxGetN(prhs[ARG_INDEX]);
		if (read_write == 1) {
			if ((m != 1) || (n != 1)) {
				mexErrMsgTxt(PROGNAME " requires \"r\"(sampling interval factor r = Tx/Ty) to be a double scalar.\n");
			}

			if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
				mxIsSparse(prhs[ARG_INDEX])  || !(mxIsDouble(prhs[ARG_INDEX])) ) {
				mexErrMsgTxt(PROGNAME " requires \"r\"(sampling interval factor r = Tx/Ty) to be a double scalar.\n");
			}
			r = mxGetScalar(prhs[ARG_INDEX]);
		} else {
			if ((m != 1) || (n != 1)) {
				mexErrMsgTxt(PROGNAME " requires \"ch\"(number of channels) to be a double scalar (1 = mono, 2 = stereo).\n");
			}

			if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
				mxIsSparse(prhs[ARG_INDEX])  || !(mxIsDouble(prhs[ARG_INDEX])) ) {
				mexErrMsgTxt(PROGNAME " requires \"ch\"(number of channels) to be a double scalar (1 = mono, 2 = stereo).\n");
			}
			channels = (int)mxGetScalar(prhs[ARG_INDEX]);		
		}
		
#undef ARG_INDEX	
	}
	
	if (nrhs > 4) {
#define	ARG_INDEX 4
		m = mxGetM(prhs[ARG_INDEX]);
		n = mxGetN(prhs[ARG_INDEX]);
		if (read_write == 0) {
			if ((m != 1) || (n != 1)) {
				mexErrMsgTxt(PROGNAME " requires \"r\"(sampling interval factor r = Tx/Ty) to be a double scalar.\n");
			}

			if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
				mxIsSparse(prhs[ARG_INDEX])  || !(mxIsDouble(prhs[ARG_INDEX])) ) {
				mexErrMsgTxt(PROGNAME "  requires \"r\"(sampling interval factor r = Tx/Ty) to be a double scalar.\n");
			}
			r = mxGetScalar(prhs[ARG_INDEX]);
		} else {
			mexErrMsgTxt(PROGNAME "; playback mode has maximum 4 arguments\n");
		}	
#undef ARG_INDEX	
	}



	base_sampling_rate = SAMPLING_RATE;
	
	if (firstcall) {
		/* fid = fopen ("debug.txt","w"); */
		firstcall = 0;
		last_pointer = 0;
		frame_pointer = 0;
		last_sampling_rate = base_sampling_rate;

#ifdef _ALSA_
		audio_interface = ALSA;
#else
		audio_interface = OSS;
#endif
		
		/* Initialize some concurrency controls: */
		pthread_mutex_init(&fp_outmutex, NULL);
		pthread_mutex_init(&fp_inmutex, NULL);
		pthread_mutex_init(&audio_dev_mutex, NULL);
		pthread_cond_init(&data_available, NULL);
		pthread_cond_init(&rec_samples_avail, NULL);
		pthread_cond_init(&start_recording, NULL);
		/* start playback and recording threads */
		pthread_create( &output_thread, NULL, (void *) &wavplay_oss_thread, NULL);
		pthread_create( &input_thread, NULL, (void *) &wavrecord_oss_thread, NULL);		
		/* mex file destructor */
		mexAtExit(wavio_oss_exit);	
	}
	
	
	

	if (read_write == 1) {
		if (N * channels > 0) {

			if (samples != 0) {
				samplesl = samples;
			} else {
				samplesl = (int16_t *)malloc(N * channels * sizeof(int16_t));
				for (index = 0; index < N * channels; index++) {
					double_sample = 32768.0 * double_samples[index] + 0.5;

					/* Clipping */
					if (double_sample > 32767.0) {
						double_sample = 32767.0;
					} else if (double_sample < -32768.0) {
						double_sample = -32768.0;
					}

					samplesl[index] = (int16_t)(double_sample);
				}
			}

			pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&fp_outmutex);
			pthread_mutex_lock( &fp_outmutex );

			if (out_thread_error) {
				wavio_oss_exit();
				mexErrMsgTxt("Error in ouput thread!\n");
			}
			if (!playing) {			/* clear state memory after start and buffer underrun */
				memset(state, 0, 2 * SUBFILTER_LENGTH * sizeof(int16_t));
				last_sampling_rate = sampling_rate;
				playing = 1;			
			}
			pthread_cleanup_pop(1);/*pthread_mutex_unlock( &fp_outmutex ); */
			
			if (last_sampling_rate != sampling_rate) {
				memcpy(statel,state,2 * SUBFILTER_LENGTH * sizeof(int16_t));
				memset(state, 0, 2 * SUBFILTER_LENGTH * sizeof(int16_t));
			}
			if (N < SUBFILTER_LENGTH) samples_to_read = N; else samples_to_read = SUBFILTER_LENGTH;
			if (channels == 2) {
				memcpy(state + 2 * SUBFILTER_LENGTH, samplesl, 2 * samples_to_read * sizeof(int16_t));
			} else {	
				for (index = 0; index < samples_to_read; index++) {
					state[2 * index + 2 * SUBFILTER_LENGTH] = samplesl[index];
					state[2 * index + 1 + 2 * SUBFILTER_LENGTH] = samplesl[index];
				}
			}

			r *= (double)base_sampling_rate / (double)sampling_rate;
			index2 = 0;
			overall_length = 0;
			samples_ptr = samplesl;
			
			while (index2 < N) {

				samples_to_read = blocklength;
				if (samples_to_read > N - index2) samples_to_read = N - index2; 
				Td = last_delay_play;
				ty = Td;
				output_length = (int) (((double)samples_to_read - Td) * r) + 1;
				
				pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&fp_outmutex);
				pthread_mutex_lock( &fp_outmutex );

				if (last_pointer == NULL) {	/* initialize playing buffer */
					last_pointer = (audio_frame *)malloc(sizeof(audio_frame));
				} else {
					last_pointer->next = (audio_frame *)malloc(sizeof(audio_frame));
					last_pointer = last_pointer->next;
				}
				if ((!last_pointer) || (!(last_pointer->buffer = (int16_t *)malloc(output_length * 2 * sizeof(int16_t))))) {
					mexErrMsgTxt(PROGNAME ": error allocating memory\n");
				}
				last_pointer->length = output_length * 2 * sizeof(int16_t);	
				last_pointer->next = NULL;
				index = 0;
				if (index2 == 0) {	/* state flushing */
					if (samples_to_read >= SUBFILTER_LENGTH) {
						index = sample_rate_conv_int16_stereo_stereo (state, 2 * SUBFILTER_LENGTH, last_pointer->buffer, output_length, &ty, 1/r);
						samples_to_read -= SUBFILTER_LENGTH;
					} else {
						index = sample_rate_conv_int16_stereo_stereo (state, SUBFILTER_LENGTH + samples_to_read, last_pointer->buffer, output_length, &ty, 1/r);
						samples_to_read = 0;
					}
					
				}
				
				if (channels == 2) {
					output_length = index + sample_rate_conv_int16_stereo_stereo (samples_ptr, samples_to_read + SUBFILTER_LENGTH, last_pointer->buffer + 2 * index, output_length - index, &ty, 1/r);
				} else {
					output_length = index + sample_rate_conv_int16_mono_stereo (samples_ptr, samples_to_read + SUBFILTER_LENGTH, last_pointer->buffer + 2 * index, output_length - index, &ty, 1/r);
				}
				last_delay_play = ty;
				last_pointer->length = 2 * sizeof(int16_t) * output_length; /* possible size correction	*/
				overall_length += output_length;
				if ((index2 == 0) && (last_sampling_rate != sampling_rate)) { /* dieing away of samples with old sample rate */
					index = (int) (((double)SUBFILTER_LENGTH - Td) * r) + 1;
					if (index < output_length) output_length = index;
					buffer_ptr = (int16_t *) calloc(output_length * 2, sizeof(int16_t));
					ty = Td;
					output_length = sample_rate_conv_int16_stereo_stereo (statel, 2 * SUBFILTER_LENGTH, buffer_ptr, output_length, &ty, last_sampling_rate/(r*sampling_rate));
					for (index = 0; index < 2 * output_length; index ++) {
						mix = (int)last_pointer->buffer[index] + (int)buffer_ptr[index];
						if (mix > 32767)  mix = 32767;
						if (mix < -32768) mix = -32768;
						last_pointer->buffer[index] = (int16_t)mix;
					}
					free (buffer_ptr);
				}
				
				if (frame_pointer == NULL) {
					frame_pointer = last_pointer;
				}
				pthread_cond_signal( &data_available );
				buffered_bytes += last_pointer->length;
				pthread_cleanup_pop(1);/*pthread_mutex_unlock( &fp_outmutex ); */
				samples_ptr += samples_to_read * channels;
				if (index2 == 0) index2 = SUBFILTER_LENGTH;
				index2 += samples_to_read;
			}

			if (channels == 2) {
				if (N >= SUBFILTER_LENGTH) {
					memcpy(state, samplesl + 2 * (N - SUBFILTER_LENGTH), 2 * SUBFILTER_LENGTH * sizeof(int16_t));
				} else {
					memmove(state, state + 2 * N, 2 * SUBFILTER_LENGTH * sizeof(int16_t));
				}
			} else {
				if (N >= SUBFILTER_LENGTH) {
					for (index = 0; index < SUBFILTER_LENGTH; index++) {
						state[2 * index] = samplesl[index + N - SUBFILTER_LENGTH];
						state[2 * index + 1] = samplesl[index + N - SUBFILTER_LENGTH];
					}
				} else {
					for (index = 0; index < SUBFILTER_LENGTH; index++) {
						state[2 * index] = state[index + N];
						state[2 * index + 1] = state[index + N];
					}
				}
			}			
			if (samples == NULL) free (samplesl);
			last_sampling_rate = sampling_rate;

		}
		plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
		pthread_mutex_lock( &fp_outmutex );
		*mxGetPr(plhs[0]) = 1000.0 * (double)buffered_bytes/(2 * 2 * base_sampling_rate);
		pthread_mutex_unlock( &fp_outmutex );

		if (nlhs > 1) {
			plhs[1] = mxCreateDoubleMatrix(1,1,mxREAL);
			*mxGetPr(plhs[1]) = overall_length - N;
		}

		if (nlhs > 2) {
			plhs[2] = mxCreateDoubleMatrix(1,1,mxREAL);
			*mxGetPr(plhs[2]) = -last_delay_play;
		}

	} else {	/* read_write == 0 => recording */
		dims[0] = channels;
		dims[1] = N;
		if (samples == NULL) {
			plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
			double_samples = (double *)mxGetData(plhs[0]);
			samples = (int16_t *)malloc(N * channels * sizeof(int16_t));
		} else {
			plhs[0] = mxCreateNumericArray(2, dims, mxINT16_CLASS, mxREAL);
			samples = (int16_t *) mxGetData(plhs[0]);
		}
		if (N != 0) {
			Td = last_delay_rec;
			r *= (double)base_sampling_rate / (double)sampling_rate;
			output_length = (int) ((double)(N - 1) * r  + Td);
			if ((double)output_length < (double)(N - 1) * r  + Td) {
				output_length++;
			}
			pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&fp_inmutex);
			pthread_mutex_lock( &fp_inmutex );

			if (recording == 0) {
				pthread_cond_signal( &start_recording );
				recording = 1;
				pthread_cond_wait( &rec_samples_avail, &fp_inmutex );
			}
			ty = Td;
			overall_length = 0;
			index2 = 0;
			while (index2 < N) {
				index = (write_index - 2 * SUBFILTER_LENGTH + BUFFER_SIZE) % BUFFER_SIZE;
				if ((index - read_index + BUFFER_SIZE) % BUFFER_SIZE <= 2) {
					pthread_cond_wait( &rec_samples_avail, &fp_inmutex );
				}
				samples_to_read = blocklength;
				if ((read_index < index) && (index - read_index < blocklength + 2)) {
					samples_to_read = index - read_index - 2;
				} else if (read_index + samples_to_read > BUFFER_SIZE) {
					samples_to_read = BUFFER_SIZE - read_index - 2 * (index == 0);
				}
				
				if (2 * output_length < samples_to_read) samples_to_read = 2 * output_length;
				output_length -= samples_to_read/2;
				overall_length += samples_to_read/2;

				if (channels == 2) {
					m = sample_rate_conv_int16_stereo_stereo (record_buffer + read_index, samples_to_read/2 + SUBFILTER_LENGTH, samples + 2 * index2, N - index2, &ty, r);	
				} else {
					m = sample_rate_conv_int16_stereo_mono (record_buffer + read_index, samples_to_read/2 + SUBFILTER_LENGTH, samples + index2, N - index2, &ty, r);
				}
				read_index = (read_index + samples_to_read) % BUFFER_SIZE;
				
				index2 += m;
				if ((output_length == 0) && (index2 < N)) output_length++;
			}
			pthread_cleanup_pop(1);/* pthread_mutex_unlock( &fp_inmutex ); */
			
			last_delay_rec = ty;				
		}
		if (mxIsDouble(plhs[0])) {
			for (index = 0; index < channels * N; index++) {
				double_samples[index] = (double)samples[index] / 32768.0;
			}
			free (samples);
		}

		if (nlhs > 1) {
			plhs[1] = mxCreateDoubleMatrix(1,1,mxREAL);
			*mxGetPr(plhs[1]) = overall_length - N;
		}

		if (nlhs > 2) {
			plhs[2] = mxCreateDoubleMatrix(1,1,mxREAL);
			*mxGetPr(plhs[2]) = last_delay_rec;
		}

	}
	

	return;
}
	
void wavio_oss_exit (void) 
{
	pthread_cancel (output_thread);
	pthread_join(output_thread,NULL);
	pthread_cancel (input_thread);
	pthread_join(input_thread,NULL);

	firstcall = 1;
	/*if (fid) {
		fclose(fid);
	} */
}


void wavplay_oss_thread (void *ptr)
{
	int i;
	int internal_error = 0;
	audio_frame *help_pointer = 0;
	sigset_t   signal_mask;
	struct timeval now;
	struct timespec timeout;
	/* count_info info; */
		
	/* blocking of interrupt signal to prevent shutting down */
	/* wavplay_oss_thread with CTRL-C on Matlab command line */
	sigemptyset (&signal_mask);
	sigaddset (&signal_mask, SIGINT);
	sigaddset (&signal_mask, SIGTERM);
	pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
	
	pthread_cleanup_push(wavplay_oss_thread_cleanup, NULL);

	pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&fp_outmutex);
	pthread_mutex_lock( &fp_outmutex );
	if (frame_pointer == NULL) {
		pthread_cond_wait( &data_available, &fp_outmutex );
	}
	pthread_cleanup_pop(1); 		/* unlock fp_outmutex */
	/* pthread_mutex_unlock( &fp_outmutex ); */
	init_playback();
	
	while (1) {
		/*debug: */
		/*for (i = 0; i < frame_pointer->length/2; i++) fprintf(fid,"%i, ",frame_pointer->buffer[i]); 
		fflush (fid);
		ioctl(audio_fd, SNDCTL_DSP_GETOPTR,&info);
		fprintf(fid,"%i, %i, %i\n",info.bytes, info.blocks, info.ptr);*/ 

		if ((i = playback(frame_pointer->buffer, frame_pointer->length)) != frame_pointer->length) {
			internal_error = 1;
		}
		
		help_pointer = frame_pointer; /* ok here, because frame_pointer is not changed, if != NULL */
		
		pthread_cleanup_push(wavplay_delete_frame, (void *) &help_pointer);
		pthread_mutex_lock( &fp_outmutex );
		out_thread_error = internal_error;
		buffered_bytes -= frame_pointer->length;
		/*help_pointer = frame_pointer; */
		frame_pointer = frame_pointer->next;
		if (frame_pointer == NULL) { /* buffer underrun, not serious because of driver buffer */
			last_pointer = NULL;
			gettimeofday(&now);
			timeout.tv_sec = now.tv_sec + AUDIO_TIMEOUT;
			timeout.tv_nsec = now.tv_usec * 1000;
			if (pthread_cond_timedwait( &data_available, &fp_outmutex, &timeout ) == ETIMEDOUT) {
			/* buffer underrun after AUDIO_TIMEOUT seconds */
				if (frame_pointer == NULL) {
					playing = 0;		/* indicate playback stop (for state nulling) */
					last_delay_play = 0.0;
					stop_playback();
					pthread_cond_wait( &data_available, &fp_outmutex );
					init_playback();
				}
			}
		}
		pthread_cleanup_pop(1);		/* execute clean up */
	}
	pthread_cleanup_pop(1);
	return;
}


void wavplay_delete_frame (void *ptr)
{
	audio_frame **help_pointer = (audio_frame **) ptr;
	pthread_mutex_unlock( &fp_outmutex );
	
	if (*help_pointer) {
		if ((*help_pointer)->buffer != 0) {
			free((*help_pointer)->buffer);
		}		
		free (*help_pointer);
	}
	*help_pointer = 0;
	return;
}

void wavplay_oss_thread_cleanup (void *ptr)
{	
	audio_frame *help_pointer;

	while (frame_pointer != NULL) {
		if (frame_pointer->buffer != 0) {
			free(frame_pointer->buffer);
		}
		help_pointer = frame_pointer->next;
		free(frame_pointer);
		frame_pointer = help_pointer;
	}
	frame_pointer = 0;
	last_pointer = 0;
	out_thread_error = 0;
	buffered_bytes = 0; 
	stop_playback();
	return;
}

void wavrecord_oss_thread (void *ptr)
{
	int internal_error = 0;
	int blocklength = 256;
	int samples_to_read = 0;
	int recording_local = 0;
	int index;
	int16_t *temp_buffer = 0;
	struct timeval now;
     	struct timespec timeout;
	sigset_t   signal_mask;
		
	/* blocking of interrupt signal to prevent shutting down */
	/* wavrecord_oss_thread with CTRL-C on Matlab command line */
	sigemptyset (&signal_mask);
	sigaddset (&signal_mask, SIGINT);
	sigaddset (&signal_mask, SIGTERM);
	pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);

	pthread_cleanup_push(wavrecord_oss_thread_cleanup, temp_buffer);
	temp_buffer = (int16_t *)malloc(blocklength * sizeof(int16_t));
	
	while (1) {
		pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&fp_inmutex);
		pthread_mutex_lock( &fp_inmutex );
		if (!recording) {
			pthread_cond_wait( &start_recording, &fp_inmutex );
		}
		record_buffer = (int16_t *)calloc(BUFFER_SIZE + 2 * SUBFILTER_LENGTH, sizeof(int16_t));
		write_index = 2 * SUBFILTER_LENGTH;
		read_index = 0;
		pthread_cleanup_pop(1);
		init_recording();
		
		recording_local = 1;
		while (recording_local) {
			samples_to_read = blocklength;
			pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&fp_inmutex);
			pthread_mutex_lock( &fp_inmutex );
			if ((write_index < read_index) && (read_index - write_index < blocklength + 2)) {
				samples_to_read = (read_index - write_index) - 2;
			} else if (write_index > BUFFER_SIZE - blocklength) {
				if (read_index != 0) {
					samples_to_read = BUFFER_SIZE - write_index;
				} else {
					samples_to_read = BUFFER_SIZE - write_index - 2;
				}
			}
			pthread_cleanup_pop(1);
			if (record(temp_buffer, sizeof(int16_t) * samples_to_read) != sizeof(int16_t) * samples_to_read) {
				internal_error = 1;
			}
		
			pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&fp_inmutex);
			pthread_mutex_lock( &fp_inmutex );
			
			memcpy (record_buffer + write_index, temp_buffer, sizeof(int16_t) * samples_to_read);
			if (write_index < 2 * SUBFILTER_LENGTH) {
				memcpy(record_buffer + BUFFER_SIZE + write_index, record_buffer + write_index, (2 * SUBFILTER_LENGTH - write_index) * sizeof(int16_t));
			}
			write_index = (write_index + samples_to_read) % BUFFER_SIZE;
			
			pthread_cond_signal( &rec_samples_avail );
			
			if ((read_index - write_index + BUFFER_SIZE) % BUFFER_SIZE <= 2) {
				gettimeofday(&now);
				timeout.tv_sec = now.tv_sec + AUDIO_TIMEOUT;
				timeout.tv_nsec = now.tv_usec * 1000;
				
				if (pthread_cond_timedwait( &data_available, &fp_inmutex, &timeout ) == ETIMEDOUT) {
				/* buffer underrun after AUDIO_TIMEOUT seconds */
					if ((read_index - write_index + BUFFER_SIZE) % BUFFER_SIZE <= 2) {
						free (record_buffer);
						record_buffer = 0;
						recording = 0;
						recording_local = 0;
						last_delay_rec = 0.0;
						stop_recording();
					}
				}
			}
			pthread_cleanup_pop(1);
			internal_error = 0;
		}
	}
	pthread_cleanup_pop(1);
	return;
}


void wavrecord_oss_thread_cleanup (void *ptr)
{
	if (ptr) free (ptr);
	if (record_buffer) free (record_buffer);
	record_buffer = 0;
	recording = 0;
	last_delay_rec = 0.0;
	stop_recording();
	return;
}

int init_playback (void) {
	
	int current_channels = 0, current_speed = 0, current_format = 0, caps, return_value = 0;
#ifdef _ALSA_
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
    	snd_pcm_hw_params_t *hwparams;
#endif

	pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&audio_dev_mutex);				
	pthread_mutex_lock(&audio_dev_mutex);	

	if (audio_interface == OSS) {
			
		if (!audio_dev_accesses) {
		
			if ((audio_fd = open(DEFAULT_OSS_DEVICE, O_RDWR, 0)) == -1) {
				return_value = -1;
				goto return_point;
			}
			/* enable full duplex */
			if (ioctl(audio_fd,SNDCTL_DSP_SETDUPLEX, 0) == -1) {
				close(audio_fd);
				return_value = -2;
				goto return_point;
			}
			caps = 0;
			if (ioctl(audio_fd,SNDCTL_DSP_GETCAPS, &caps) == -1) {
				close(audio_fd);
				return_value = -3;
				goto return_point;
			}
			if (caps & DSP_CAP_DUPLEX == 0) {
				close(audio_fd);
				return_value = -3;
				goto return_point;
			}		
			/* set format to 16 bit	*/
			current_format = AFMT_S16_LE;
			if (ioctl(audio_fd,SNDCTL_DSP_SETFMT, &current_format) == -1) {
				close(audio_fd);
				return_value = -4;
				goto return_point;
			}
			if (current_format != AFMT_S16_LE) {
				close(audio_fd);
				return_value = -4;
				goto return_point;
			}
			/* enable stereo */
			current_channels = CHANNELS;		/* always stereo */
			if (ioctl(audio_fd,SNDCTL_DSP_CHANNELS, &current_channels) == -1) {
				close(audio_fd);
				return_value = -5;
				goto return_point;
			}
			if (current_channels != CHANNELS) {
				close(audio_fd);
				return_value = -5;
				goto return_point;
			}
			current_speed = base_sampling_rate;
			if (ioctl(audio_fd,SNDCTL_DSP_SPEED, &current_speed) == -1) {
				close(audio_fd);
				return_value = -6;
				goto return_point;
			}
			if (current_speed != base_sampling_rate) {
				close(audio_fd);
				return_value = -6;
				goto return_point;
			}
		}
		if (!open_to_write) {
			audio_dev_accesses++;
		}
		open_to_write = 1;

	}
#ifdef _ALSA_
	 else {
		if (!open_to_write) {
			snd_pcm_hw_params_alloca(&hwparams);
			if (snd_pcm_open(&pcm_handle_write, DEFAULT_ALSA_DEVICE, stream, 0) < 0) {
				return_value = -1;
				goto return_point;
			}
			if (snd_pcm_hw_params_any(pcm_handle_write, hwparams) < 0) {
				snd_pcm_close(pcm_handle_write);
				return_value = -7;
				goto return_point;
			}
			if (snd_pcm_hw_params_set_access(pcm_handle_write, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
				snd_pcm_close(pcm_handle_write);
				return_value = -7;
				goto return_point;
			}
	
			/* set format to 16 bit */
			if (snd_pcm_hw_params_set_format(pcm_handle_write, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
				snd_pcm_close(pcm_handle_write);
				return_value = -4;
				goto return_point;
			}
	
			current_speed = base_sampling_rate;
			if (snd_pcm_hw_params_set_rate_near(pcm_handle_write, hwparams, &current_speed, 0) < 0) {
				snd_pcm_close(pcm_handle_write);
				return_value = -6;
				goto return_point;
			}
			if (current_speed != base_sampling_rate) {
				snd_pcm_close(pcm_handle_write);
				return_value = -6;
				goto return_point;
			}
	
			if (snd_pcm_hw_params_set_channels(pcm_handle_write, hwparams, 2) < 0) {
				snd_pcm_close(pcm_handle_write);
				return_value = -5;
				goto return_point;
			}
			if (snd_pcm_hw_params(pcm_handle_write, hwparams) < 0) {
				snd_pcm_close(pcm_handle_write);
				return_value = -9;
				goto return_point;
			}	
		
			/*snd_pcm_hw_params_free (hwparams); */	/* if allocated with malloc */
			
			if (snd_pcm_prepare (pcm_handle_write) < 0) {
				snd_pcm_close(pcm_handle_write);
				return_value = -10;
				goto return_point;
			}	
		}
		open_to_write = 1;	
	}
#endif	

return_point:
	
	pthread_cleanup_pop(1);
	return return_value;

}

int init_recording (void) {
	
	int current_channels = 0, current_speed = 0, current_format = 0, caps, return_value = 0;
#ifdef _ALSA_
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
    	snd_pcm_hw_params_t *hwparams;
#endif

	pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&audio_dev_mutex);				
	pthread_mutex_lock(&audio_dev_mutex);	

	if (audio_interface == OSS) {
			
		if (!audio_dev_accesses) {
			if ((audio_fd = open(DEFAULT_OSS_DEVICE, O_RDWR, 0)) == -1) {
				return_value = -1;
				goto return_point;
			}
			/* enable full duplex */
			if (ioctl(audio_fd,SNDCTL_DSP_SETDUPLEX, 0) == -1) {
				close(audio_fd);
				return_value = -2;
				goto return_point;
			}
			caps = 0;
			if (ioctl(audio_fd,SNDCTL_DSP_GETCAPS, &caps) == -1) {
				close(audio_fd);
				return_value = -3;
				goto return_point;
			}
			if (caps & DSP_CAP_DUPLEX == 0) {
				close(audio_fd);
				return_value = -3;
				goto return_point;
			}		
			/* set format to 16 bit	*/
			current_format = AFMT_S16_LE;
			if (ioctl(audio_fd,SNDCTL_DSP_SETFMT, &current_format) == -1) {
				close(audio_fd);
				return_value = -4;
				goto return_point;
			}
			if (current_format != AFMT_S16_LE) {
				close(audio_fd);
				return_value = -4;
				goto return_point;
			}
			/* enable stereo */
			current_channels = CHANNELS;		/* always stereo */
			if (ioctl(audio_fd,SNDCTL_DSP_CHANNELS, &current_channels) == -1) {
				close(audio_fd);
				return_value = -5;
				goto return_point;
			}
			if (current_channels != CHANNELS) {
				close(audio_fd);
				return_value = -5;
				goto return_point;
			}
			current_speed = base_sampling_rate;
			if (ioctl(audio_fd,SNDCTL_DSP_SPEED, &current_speed) == -1) {
				close(audio_fd);
				return_value = -6;
				goto return_point;
			}
			if (current_speed != base_sampling_rate) {
				close(audio_fd);
				return_value = -6;
				goto return_point;
			}
		}	
		if (!open_to_read) {
			audio_dev_accesses++;
		}
		open_to_read = 1;

	}
#ifdef _ALSA_
	 else {
	 	if (!open_to_read) {
			snd_pcm_hw_params_alloca(&hwparams);
			if (snd_pcm_open(&pcm_handle_read, DEFAULT_ALSA_DEVICE, stream, 0) < 0) {
				return_value = -1;
				goto return_point;
			}
			if (snd_pcm_hw_params_any(pcm_handle_read, hwparams) < 0) {
				snd_pcm_close(pcm_handle_read);
				return_value = -7;
				goto return_point;
			}
			if (snd_pcm_hw_params_set_access(pcm_handle_read, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
				snd_pcm_close(pcm_handle_read);
				return_value = -7;
				goto return_point;
			}
	
			/* set format to 16 bit */
			if (snd_pcm_hw_params_set_format(pcm_handle_read, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
				snd_pcm_close(pcm_handle_read);
				return_value = -4;
				goto return_point;
			}
	
			current_speed = base_sampling_rate;
			if (snd_pcm_hw_params_set_rate_near(pcm_handle_read, hwparams, &current_speed, 0) < 0) {
				snd_pcm_close(pcm_handle_read);
				return_value = -6;
				goto return_point;
			}
			if (current_speed != base_sampling_rate) {
				snd_pcm_close(pcm_handle_read);
				return_value = -6;
				goto return_point;
			}
	
			if (snd_pcm_hw_params_set_channels(pcm_handle_read, hwparams, 2) < 0) {
				snd_pcm_close(pcm_handle_read);
				return_value = -5;
				goto return_point;
			}
			if (snd_pcm_hw_params(pcm_handle_read, hwparams) < 0) {
				snd_pcm_close(pcm_handle_read);
				return_value = -9;
				goto return_point;
			}	
		
			/*snd_pcm_hw_params_free (hwparams); /*	/* if allocated with malloc */
			
			if (snd_pcm_prepare (pcm_handle_read) < 0) {
				snd_pcm_close(pcm_handle_read);
				return_value = -10;
				goto return_point;
			}	
		}
		open_to_read = 1;	
	}
#endif	

return_point:
	
	pthread_cleanup_pop(1);
	return return_value;
	
}

int stop_playback (void) {

	if (audio_interface == OSS) {
	
		pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&audio_dev_mutex);				
		pthread_mutex_lock(&audio_dev_mutex);
		
		if (open_to_write) {
			if (audio_dev_accesses) audio_dev_accesses--;
			if ((audio_dev_accesses == 0) && (audio_fd >= 0)) {
				close(audio_fd);
				audio_fd = -1;
			}
		}	
		open_to_write = 0;
		pthread_cleanup_pop(1);
	}
#ifdef _ALSA_
	 else {
		pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&audio_dev_mutex);				
		pthread_mutex_lock(&audio_dev_mutex);
		
		if (open_to_write) {
			snd_pcm_close(pcm_handle_write);
		}
		
		open_to_write = 0;
		pthread_cleanup_pop(1);	
	}
#endif
}

int stop_recording (void) {

	if (audio_interface == OSS) {
	
		pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&audio_dev_mutex);				
		pthread_mutex_lock(&audio_dev_mutex);
		
		if (open_to_read) {
			if (audio_dev_accesses) audio_dev_accesses--;
			if ((audio_dev_accesses == 0) && (audio_fd >= 0)) {
				close(audio_fd);
				audio_fd = -1;
			}
		}
		open_to_read = 0;
		pthread_cleanup_pop(1);
	}
#ifdef _ALSA_
	 else {
		pthread_cleanup_push((void (* )(void*))pthread_mutex_unlock, (void *)&audio_dev_mutex);				
		pthread_mutex_lock(&audio_dev_mutex);
		
		if (open_to_read) {
			snd_pcm_close(pcm_handle_read);
		}
		
		open_to_read = 0;
		pthread_cleanup_pop(1);
	}
#endif
}

int playback (int16_t *samples, int no_of_bytes) {

	if (audio_interface == OSS) {	
		if (open_to_write)
			return write(audio_fd, samples, no_of_bytes);
		else
			return 0;
	}
#ifdef _ALSA_
	 else {	 
	 	if (open_to_write)
			return 4 * snd_pcm_writei(pcm_handle_write, samples, no_of_bytes / 4);
		else
			return 0;
	}
#endif
}

int record (int16_t *samples, int no_of_bytes) {

	if (audio_interface == OSS) {
		if (open_to_read)
			return read(audio_fd, samples, no_of_bytes);
		else
			return 0;
	}
#ifdef _ALSA_
	 else {
	 	if (open_to_read)
			return 4 * snd_pcm_readi(pcm_handle_read, samples, no_of_bytes / 4);
		else
			return 0;	
	}
#endif
}
