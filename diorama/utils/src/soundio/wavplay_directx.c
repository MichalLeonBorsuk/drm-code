/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Andreas Dittrich                                       */
/*                                                                            */
/*  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  */
/*  Project start: 27.05.2004                                                 */
/*  Last change  : 20.02.2005                                                 */
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
/*                                                                            */
/*  wavplay_directx.c                                                         */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Description:                                                              */
/*  Sound playback using DirectX (5.0 and higher)                             */
/*  Usage:                                                                    */
/*                                                                            */
/* (1) wavplay_directx                                                        */
/* (2) wavplay_directx( in, Fs [, r] )                                        */
/* (3) playtime_delay_ms = wavplay_directx( in, Fs [, r] )                    */
/* (4) [playtime_delay_ms, delay_diff, delay_offset] =                        */
/*          wavplay_directx( in, Fs [, r] )                                   */
/*                                                                            */
/*  in = channels x no_of_samples - matrix                                    */
/*  Fs = sampling rate in Hz                                                  */
/*  r = resampling factor                                                     */
/*  playtime_delay_ms = time in ms, buffer is actual filled and will play     */
/*  delay_diff = no of samples which were added due to resampling             */
/*  delay_offset = fractional part of delay in samples due to resampling      */
/*                                                                            */
/*  A call without any argument stops all current playing (1).                */
/*  Use 'in' as input data vector of either double or int16 type. 'in' should */
/*  have two rows for stereo and one row for mono and as many columns as you  */
/*  have sound-samples to play. Calling wavplay_directx when sound is still   */
/*  playing adds the data to the play buffer and so enables continous playing.*/
/*  Using sample rate conversion, applying the factor r results in conversion */
/*  of N input samples into N*r soundcard samples.                            */
/*  By integrating (adding) only the output values 'delay_diff' and adding    */
/*  'delay_offset' to the integration result, you allways have the actual     */
/*  delay in samples between input data stream and the resampled data stream. */
/*                                                                            */
/******************************************************************************/


/*
 we need: user32.lib dsound.lib libcmt.lib
 mex wavplay_directx_c.c src_filter.c dynamicbuffer.c user32.lib dsound.lib libcmt.lib
*/


#include <math.h>
#include <stdlib.h>
#include "mex.h"

#include <process.h>
#include <windows.h>
#include <dsound.h>

#include "dynamicbuffer.h"
#include "src_filter.h"



//#define DEBUG
//#define DEBUG_OUTPUT_FILENAME "wavplay_directx_debug.txt"
#define DEBUG_ERROR_OUTPUT_FILENAME "wavplay_directx_error.txt"



#define PROGNAME "wavplay_directx"

#define TIMEOUT_THREADCLOSE_MS 3000

#define THREADCOMMAND_NONE 0
#define THREADCOMMAND_COPY 1
#define THREADCOMMAND_CLOSE 2
#define THREADCOMMAND_SYNC 3




#define MIN(a,b) ( (a)>(b) ? b : a )
#define MAX(a,b) ( (a)>(b) ? a : b )


typedef short int16_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;


// some global (and static) variables
FILE *debugOutFile;

int iFirstRun = 1;
int iThreadCommand = THREADCOMMAND_NONE;
int iThreadToken = 0;
int iBufferlevel = 0;

int ErrorFlag = 0;
char ErrorMsg[512];

int sample_rate_conversion_enable = 0;
int samplerate = 0;
int no_of_channels = 1;
int bytes_per_sample = 2;	// for now only 16 bit PCM

HANDLE hMyThread = 0;

dynamicbuffer_t dynbuf;

LPDIRECTSOUND lpDirectSound; 
LPDIRECTSOUNDBUFFER lpDsbPrimary;	// primary buffer
LPDIRECTSOUNDBUFFER lpDsb;	// secondary buffer

int16_t src_filter_state[4*SUBFILTER_LENGTH];
double t_fraction = 0;

#ifdef DEBUG
LARGE_INTEGER performance_counts;
LARGE_INTEGER performance_counts_start;
LARGE_INTEGER counts_per_second;
__int64 counts_per_ms;
int deltatime_ms;
#endif

int debugmsg( char *msg );
int debugerrormsg( char *msg );

void threadGetToken( int* ptoken );
void threadReleaseToken( int* ptoken );

int OpenDirectSound( int samplerate, int no_of_channels, int buffersize ) ;
int CloseDirectSound();
int CreatePrimaryBuffer( LPWAVEFORMATEX lpWfx );
int CreateSecondaryBuffer( LPWAVEFORMATEX lpWfx, int buffersize );
int PlayTest( uint32_t buffersize);
DWORD WINAPI MyThreadProc(LPVOID lpParameter );
void MyExitFunction( void );


/******************************************************************************/
/*  Gateway-Routine                                                           */
/******************************************************************************/

void mexFunction(
                  int nlhs,       mxArray *plhs[],
                  int nrhs, const mxArray *prhs[]
           		 )
{
	mxArray const *parg;
	unsigned long m,n;
	long N;

	double dummy;
	double *ptr_bufferlevel_ms = &dummy;			// these pointers are later redirected to the optional output arguments
	double *ptr_no_of_output_samples = &dummy;	// so, if these arguments are not available we write to the dummy-location
	double *ptr_t_fraction = &dummy;

	double *in;
	double r = 1.0;

	int16_t *buffer_temp = NULL;

	int samplerate_new;
	int no_of_channels_new;
	int sample_rate_conversion_enable_new;

	int no_of_input_samples;
	int data_to_process;
	int data_pos;

	DWORD dwExitCode = -1;
	DWORD dwThreadId;

	if (iFirstRun)
	{
		// create critical section object
		dynamicbuffer_init( &dynbuf );

		iFirstRun = 0;
		iThreadCommand = THREADCOMMAND_NONE;
		iThreadToken = 0;
		iBufferlevel = 0;
		ErrorFlag = 0;
		sample_rate_conversion_enable = 0;
		samplerate = 0;
		no_of_channels = 1;
		bytes_per_sample = 2;
		hMyThread = 0;
		t_fraction = 0;

		// register exit function
		mexAtExit( MyExitFunction );

#ifdef DEBUG
// for debugging
QueryPerformanceFrequency( &counts_per_second );
QueryPerformanceCounter( &performance_counts_start );
counts_per_ms = ( counts_per_second.QuadPart + 500 ) / 1000;
#endif
	}

	/* Check for proper number of arguments */
	if (nrhs==0)
	{
		MyExitFunction();	// stop all and go out
		plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
		*mxGetPr(plhs[0]) = 0;

		return;
	}

	if (nrhs < 2)
		mexErrMsgTxt(PROGNAME " requires at least 2 input arguments ");
	else if (nrhs > 3 )
		mexErrMsgTxt(PROGNAME " requires not more than 3 input arguments.");

	if ( (nlhs != 0) && (nlhs != 1) && (nlhs != 3) )
		mexErrMsgTxt(PROGNAME " requires 0, 1 or 3 output argument(s).");
  

   /* Check dimensions, check input values and assign pointer to the various parameters*/
	// in
	parg = prhs[0];
	m = mxGetM(parg);
	n = mxGetN(parg); 
	N = m*n;
	if ( !mxIsNumeric(parg) || mxIsComplex(parg) || 
		 mxIsSparse(parg)  || !( mxIsDouble(parg) || mxIsInt16(parg) ) || (N<1) )
		mexErrMsgTxt(PROGNAME " requires that in be a real vector.");
	if ((m>2) && (n!=1))
		mexErrMsgTxt(PROGNAME " requires that in be a 1-by-N (mono) or 2-by-N (stereo) matrix.");
	
	no_of_input_samples = n;
	no_of_channels_new = m;
	in = mxGetData(prhs[0]);

	if ( no_of_input_samples < (2*SUBFILTER_LENGTH) )
		mexErrMsgTxt(PROGNAME " requires that length(in)>=64");


	// samplerate (Fs)
	parg = prhs[1];
	m = mxGetM(parg);
	n = mxGetN(parg); 
	N = m*n;
	if ( !mxIsNumeric(parg) || mxIsComplex(parg) || 
		 mxIsSparse(parg)  || !mxIsDouble(parg) || (m!=1) || (n!=1) || (N<1) )
		mexErrMsgTxt(PROGNAME " requires that Fs is a scalar.");

	samplerate_new = (int)floor( mxGetScalar(prhs[1]) + 0.5 );
	if ( (samplerate_new<8000) || (samplerate_new>48000) )
		mexErrMsgTxt(PROGNAME " requires that samplerate is between 8000..48000");

	// r 
	if (nrhs>2)
	{
		parg = prhs[2];
		m = mxGetM(parg);
		n = mxGetN(parg); 
		N = m*n;
		if ( !mxIsNumeric(parg) || mxIsComplex(parg) || 
			 mxIsSparse(parg)  || !mxIsDouble(parg) || (m!=1) || (n!=1) || (N<1) )
			mexErrMsgTxt(PROGNAME " requires that r is a scalar.");

		r = mxGetScalar(prhs[2]);
		if ((r<0.125) || (r>2.0) )
			mexErrMsgTxt(PROGNAME " requires that r is between 0.125..2.0");

		sample_rate_conversion_enable_new = 1;
	}
	else
	{
		r=1.0;
		sample_rate_conversion_enable_new = 0;
	}



	/* Create a matrix for the return arguments */
	plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
	ptr_bufferlevel_ms = mxGetPr(plhs[0]);

	if (nlhs>2)
	{
		plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
		ptr_no_of_output_samples = mxGetPr(plhs[1]);

		plhs[2] = mxCreateDoubleMatrix(1, 1, mxREAL);
		ptr_t_fraction = mxGetPr(plhs[2]);
	}


	// assign return value: timestamp
	// lets predict the time from now, when this new data starts playing or the old data has finished
	*ptr_bufferlevel_ms = (double)( iBufferlevel + dynamicbuffer_getlevel(&dynbuf) )/(double)bytes_per_sample/(double)samplerate/(double)no_of_channels*1000.0;

	// if samplerate has changed we have to close the running thread 
	if ((samplerate_new!=samplerate)||(no_of_channels_new!=no_of_channels)||(sample_rate_conversion_enable_new!=sample_rate_conversion_enable))
	{
		int timeout_counter = TIMEOUT_THREADCLOSE_MS;

		// we have to wait until the thread has terminated, and then restart playing-thread with new samplerate
		iThreadCommand = THREADCOMMAND_CLOSE;	// force thread to quit
		GetExitCodeThread( hMyThread, &dwExitCode );
		while ( (dwExitCode==STILL_ACTIVE) && (timeout_counter>=0) )
		{
			Sleep(10);
			timeout_counter = timeout_counter - 10;
			GetExitCodeThread( hMyThread, &dwExitCode );
		}

		if ( (timeout_counter<0) && (dwExitCode==STILL_ACTIVE) )
		{
			debugerrormsg( "unable to close running thread failure\n" );
			mexErrMsgTxt(PROGNAME": unable to close running thread!");
		}

		// now we can start again with new sampling rate
		samplerate = samplerate_new;
		no_of_channels = no_of_channels_new;
		sample_rate_conversion_enable = sample_rate_conversion_enable_new;
	}
	
	threadGetToken( &iThreadToken );
	GetExitCodeThread( hMyThread, &dwExitCode );
	if ( dwExitCode!=STILL_ACTIVE ) 	//thread is not running 
	{
		// clear front of src_filter_state
		memset(src_filter_state, 0, SUBFILTER_LENGTH*no_of_channels*bytes_per_sample);		 
		t_fraction = 0;
	}
	threadReleaseToken( &iThreadToken );

	// create tempory buffer for double-to-int conversion if necessary
	if (mxIsDouble(prhs[0]))
		buffer_temp = (int16_t*)malloc( no_of_input_samples*no_of_channels*bytes_per_sample );

	// feed our buffer in small chunks in order to not stall our thread
	(*ptr_no_of_output_samples) = 0;
	data_to_process = no_of_input_samples*no_of_channels;
	data_pos = 0;
#define DATA_MAX_BLOCKSIZE 4096
	while (data_to_process>0) 
	{
		chainbuffer_t *pchainbuf;
		int16_t *samples_in;
		int16_t *samples_out;	// samples after sample-rate-conversion, to feed soundcard
		int no_of_output_samples;
		int index;

#ifdef DEBUG
{
char strbuf[100];
QueryPerformanceCounter( &performance_counts );
deltatime_ms = (int)( (performance_counts.QuadPart - performance_counts_start.QuadPart) / counts_per_ms );
sprintf( strbuf, "time: %i, dynamicbuffer_level: %i\n", 
		deltatime_ms, dynamicbuffer_getlevel(&dynbuf) );
debugmsg( strbuf );
}
#endif
		no_of_input_samples = MIN( DATA_MAX_BLOCKSIZE, data_to_process ) / no_of_channels;

		if (mxIsInt16(prhs[0]))
		{
			samples_in = (int16_t*)mxGetData(prhs[0]) + data_pos;
		}
		else if (mxIsDouble(prhs[0]))
		{
			int k;
			double *src;
			
			src = (double*)mxGetData(prhs[0]) + data_pos;
			samples_in = buffer_temp;

			for (k=0; k<(no_of_input_samples*no_of_channels); k++)
				samples_in[k] = (int16_t)( src[k]*32767 );
		}
		else
		{
			mexErrMsgTxt(PROGNAME " requires that input type is double or int16");
		}

		if (sample_rate_conversion_enable)
		{
			no_of_output_samples = (int)( ((double)no_of_input_samples - t_fraction)/(1.0/r) ) + 1;
			dynamicbuffer_newblock( &pchainbuf, &((uint8_t*)samples_out), no_of_output_samples*no_of_channels*bytes_per_sample );

			memcpy(src_filter_state + SUBFILTER_LENGTH*no_of_channels, samples_in, SUBFILTER_LENGTH*no_of_channels*bytes_per_sample );
				
			if (no_of_channels==1)
			{
				index = sample_rate_conv_int16_mono_mono( src_filter_state, 2*SUBFILTER_LENGTH, 
					samples_out, no_of_output_samples, &t_fraction, 1.0/r);

				no_of_output_samples = index + sample_rate_conv_int16_mono_mono( samples_in, no_of_input_samples, 
					samples_out + index, no_of_output_samples - index, &t_fraction, 1.0/r);
			}
			else if (no_of_channels==2)
			{
				index = sample_rate_conv_int16_stereo_stereo( src_filter_state, 2*SUBFILTER_LENGTH, 
					samples_out, no_of_output_samples, &t_fraction, 1.0/r);

				no_of_output_samples = index + sample_rate_conv_int16_stereo_stereo( samples_in, no_of_input_samples, 
					samples_out + no_of_channels*index, no_of_output_samples - index, &t_fraction, 1.0/r);
			}
			else
			{
				mexErrMsgTxt(PROGNAME ": multiple channels not supported");
			}

			memcpy(src_filter_state, samples_in+(no_of_input_samples-SUBFILTER_LENGTH)*no_of_channels, SUBFILTER_LENGTH*no_of_channels*bytes_per_sample );
		}
		else
		{
			no_of_output_samples = no_of_input_samples;
			dynamicbuffer_newblock( &pchainbuf, &((uint8_t*)samples_out), no_of_output_samples*no_of_channels*bytes_per_sample );
			memcpy(samples_out, samples_in, no_of_output_samples*no_of_channels*bytes_per_sample );
		}


		pchainbuf->level = no_of_output_samples*no_of_channels*bytes_per_sample; // possible size correction

		threadGetToken( &iThreadToken );
		dynamicbuffer_addblock( &dynbuf, pchainbuf );		
		threadReleaseToken( &iThreadToken );
		
		//assign values to matlab-output-arguments
		(*ptr_no_of_output_samples) += no_of_output_samples - no_of_input_samples;
		(*ptr_t_fraction) = -t_fraction;

		data_to_process = data_to_process - no_of_input_samples*no_of_channels;
		data_pos += no_of_input_samples*no_of_channels;
	} 

	// free temp buffer
	if (buffer_temp!=NULL) free(buffer_temp);
	
	
	threadGetToken( &iThreadToken );
	GetExitCodeThread( hMyThread, &dwExitCode );
	if ( dwExitCode!=STILL_ACTIVE ) 	//thread is not running -> we have to restart again
	{ 
		// create playing-thread 
		//hMyThread = CreateThread(
		hMyThread = (HANDLE)_beginthreadex(
			NULL,
			0,
			MyThreadProc,
			NULL,
			CREATE_SUSPENDED ,	// wait until ResumeThread()
			&dwThreadId
		);

		if (hMyThread==0)
		{
			debugerrormsg( "creating thread failure\n" );
		}

		iThreadCommand = THREADCOMMAND_NONE;

		// this thread controls the playing/recording of the soundcard, so its a little bit time critical
		SetThreadPriority( hMyThread, THREAD_PRIORITY_TIME_CRITICAL ); // THREAD_PRIORITY_HIGHEST

		// start Thread
		ResumeThread( hMyThread );

		*ptr_bufferlevel_ms = 0;
	}
	threadReleaseToken( &iThreadToken );

	Sleep(1);

	if (ErrorFlag!=0)
	{
		mexWarnMsgTxt( ErrorMsg );
		ErrorFlag=0;
	}

	return;
}


/******************************************************************************/
/* function                                                                   */
/******************************************************************************/


void MyExitFunction( void )
{
	int timeout_counter = TIMEOUT_THREADCLOSE_MS;
	DWORD dwExitCode = -1;

	iThreadCommand = THREADCOMMAND_CLOSE;	// force thread to quit
	GetExitCodeThread( hMyThread, &dwExitCode );
	while ( (dwExitCode==STILL_ACTIVE) && (timeout_counter>=0) )
	{
		Sleep(10);
		timeout_counter = timeout_counter - 10;
		GetExitCodeThread( hMyThread, &dwExitCode );
	}

	if ( (timeout_counter<0) && (dwExitCode==STILL_ACTIVE) )
	{
		debugerrormsg( "ExitFunction: unable to close running thread!\n" );
	}

	dynamicbuffer_deinit( &dynbuf );
	iFirstRun = 1;
}




#define THREADSTATE_CLOSING 0 
#define THREADSTATE_NORMAL 1
#define THREADSTATE_CLOSE_NEXT_TIME 0 
#define MINLEVEL_MS 40

DWORD WINAPI MyThreadProc(LPVOID lpParameter )
{
	int iSoundBufferSize;
	int iBlocksize;
	int iBufferlevel_dec;
	int iSleepTimeMs;
	int iMaxLevel;
	int iMinLevel;
	int iLowLevel;
	int iBytesPerMs;
	int iState;

	HRESULT hr; 
	DWORD dwStatus;
	DWORD dwWriteCursor;    
	DWORD dwWriteBytes;     
	DWORD dwBytesCopied;
	DWORD dwAudioBytes;
	LPVOID lpvAudioPtr1; 
	DWORD dwAudioBytes1;
	LPVOID lpvAudioPtr2;
	DWORD dwAudioBytes2;
	DWORD dwCurrentPlayCursor; 
	DWORD dwCurrentWriteCursor;
	DWORD dwCurrentPlayCursor_last;

	// get our working token, so we can do exclusive work on the global variables
	threadGetToken( &iThreadToken );
	iState = THREADSTATE_NORMAL;

	// do some initialization
	iBlocksize = 1024*no_of_channels*bytes_per_sample;	// 4 KByte, is about 20ms @ 48kHz, 16bit stereo
	iBytesPerMs = (samplerate*no_of_channels*bytes_per_sample)/1000+1;
	iSleepTimeMs = iBlocksize/(iBytesPerMs*2)+1;
	iSoundBufferSize = 32*iBlocksize;
	iMaxLevel = iSoundBufferSize - 3*iBlocksize;
	iMinLevel = ((MINLEVEL_MS*iBytesPerMs)/iBlocksize + 1)*iBlocksize;
	iLowLevel = iMinLevel + 3*iBlocksize;


	dwWriteCursor = 0;
	dwCurrentPlayCursor_last = 0;
	iBufferlevel = 0;

	// try to open sound device
	if ( (OpenDirectSound( samplerate, no_of_channels, iSoundBufferSize )) < 0 )
	{
		debugerrormsg( "play thread: OpenDirectSound() failure\n" );
		iThreadCommand = THREADCOMMAND_NONE;
		threadReleaseToken( &iThreadToken );
		return ERROR_SUCCESS;
	}
	
	// set cycletime to 1ms
	timeBeginPeriod(1);	// set system cycletime to 1ms

	// initial filling of sound buffer, copy data and/or silence into buffer.
	dwWriteBytes = iSoundBufferSize;
	hr = lpDsb->lpVtbl->Lock(lpDsb, dwWriteCursor, dwWriteBytes, 
		&lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, DSBLOCK_ENTIREBUFFER  );
	if (hr!=DS_OK) 
	{
		debugerrormsg( "first filling of buffer: Lock() failure\n" );
		iState = THREADSTATE_CLOSING;
	}
	else
	{

/*{	
char strbuf[500];
sprintf( strbuf, "lpvAudioPtr1: %i, dwAudioBytes1: %i, lpvAudioPtr2: %i, dwAudioBytes2: %i\n", 
		(int)lpvAudioPtr1, (int)dwAudioBytes1, (int)lpvAudioPtr2, (int)dwAudioBytes2 );
debugerrormsg( strbuf );
}*/

		// clear front of buffer
		memset( lpvAudioPtr1, 0, iLowLevel + iMinLevel );

		dwBytesCopied = dynamicbuffer_getbytes( &dynbuf, lpvAudioPtr1, iMaxLevel );

		iBufferlevel += dwBytesCopied;
		dwWriteCursor += dwBytesCopied;

		// wrap our writepointer if necessary
		if ((int)dwWriteCursor >= iSoundBufferSize)
			dwWriteCursor = dwWriteCursor - iSoundBufferSize;

		// unlock buffer
		hr = lpDsb->lpVtbl->Unlock(lpDsb, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2 );
		if (hr!=DS_OK) 
		{
			debugerrormsg( "playthread: Unlock() failure\n" );
			iState = THREADSTATE_CLOSING;
		}

		// start playing
		hr = lpDsb->lpVtbl->Play(lpDsb, 0, 0, DSBPLAY_LOOPING );
		if (hr!=DS_OK) 
		{
			debugerrormsg( "playthread: Play() failure\n" );
			iState = THREADSTATE_CLOSING;
		}

		// if bufferlevel is too low, we have to stop playing
		if ( (iBufferlevel<iLowLevel) && ( iState!=THREADSTATE_CLOSING ) )
		{
			Sleep( (iMinLevel/2 + iBufferlevel)/iBytesPerMs );
			iBufferlevel = 0;
			iState = THREADSTATE_CLOSING;
		}

		if ( iState!=THREADSTATE_CLOSING )
		{
			// sleep a little bit
			threadReleaseToken( &iThreadToken );
			Sleep( iSleepTimeMs );
			threadGetToken( &iThreadToken );
		}		
	}

	// now the main playing loop!
	// do processing, as long as there is data available
	while ( iState!=THREADSTATE_CLOSING )
	{
		//check for commands, change state
		if (iThreadCommand==THREADCOMMAND_CLOSE)
		{
			iState = THREADSTATE_CLOSING;
		}
		else
		{
			//check status
			hr = lpDsb->lpVtbl->GetStatus( lpDsb, &dwStatus );
			if (  (hr!=DS_OK) | ( (hr==DS_OK)&&((dwStatus&DSBSTATUS_PLAYING)==0) )  )
			{
				debugerrormsg( "inside main recording loop: soundbuffer is not playing\n" );
				iState = THREADSTATE_CLOSING;
			}
		}

		// get position and check our buffer
		if ( iState!=THREADSTATE_CLOSING )
		{
			hr = lpDsb->lpVtbl->GetCurrentPosition( lpDsb, &dwCurrentPlayCursor, &dwCurrentWriteCursor );
			if (hr!=DS_OK) 
			{
				debugerrormsg( "inside main recording loop: GetCurrentPosition() failure\n" );
				iState = THREADSTATE_CLOSING;
			}
			else
			{
/*{	
char strbuf[500];
sprintf( strbuf, "iBufferlevel: %i, dwCurrentPlayCursor: %i, dwCurrentWriteCursor: %i\n", 
		iBufferlevel, (int)dwCurrentPlayCursor, (int)dwCurrentWriteCursor );
debugerrormsg( strbuf );
}*/
				iBufferlevel_dec = (int)dwCurrentPlayCursor - (int)dwCurrentPlayCursor_last;
				dwCurrentPlayCursor_last = dwCurrentPlayCursor;

				//if (iBufferlevel_dec<0) 
				//	iBufferlevel_dec += iSoundBufferSize;

				// sometimes the PlayCursor seems not to behave as it should 
				if (iBufferlevel_dec<-(iSoundBufferSize/2)) 
					iBufferlevel_dec += iSoundBufferSize;
				else if (iBufferlevel_dec>(iSoundBufferSize/2))
					iBufferlevel_dec -= iSoundBufferSize;
				
				iBufferlevel -= iBufferlevel_dec;
				
				// check buffer
				if (iBufferlevel < iMinLevel )
				{
					// there is something wrong, we have to stop playing - this should normally not happen
					debugerrormsg( "while stopping play: buffer underrun failure\n" );
{	
char strbuf[500];
sprintf( strbuf, "iBufferlevel: %i, dwCurrentPlayCursor: %i, dwCurrentWriteCursor: %i, iBufferlevel_dec: %i\n", 
		iBufferlevel, (int)dwCurrentPlayCursor, (int)dwCurrentWriteCursor, (int)iBufferlevel_dec );
debugerrormsg( strbuf );
}

					iBufferlevel = -1;
					iState = THREADSTATE_CLOSING;
				}
			}
		}


		if (iThreadCommand==THREADCOMMAND_SYNC)
		{
			iThreadCommand=THREADCOMMAND_NONE;
		}

#ifdef DEBUG
{
char strbuf[100];
QueryPerformanceCounter( &performance_counts );
deltatime_ms = (int)( (performance_counts.QuadPart - performance_counts_start.QuadPart) / counts_per_ms );
sprintf( strbuf, "time: %i, copy *** iState: %i, dynamicbuffer_level: %i, iBufferlevel: %i, iMinLevel: %i\n", 
		deltatime_ms, iState, dynamicbuffer_getlevel(&dynbuf), iBufferlevel, iMinLevel );
debugmsg( strbuf );
}
#endif

		// add new data if available and buffer is not too full
		if ( iState!=THREADSTATE_CLOSING )
		{
			dwWriteBytes = MIN( iMaxLevel - iBufferlevel, dynamicbuffer_getlevel(&dynbuf) );
			dwWriteBytes = MIN( (int)dwWriteBytes, iBlocksize ); // fill buffer in small chunks

			if ( (dwWriteBytes > 0)&&((iMaxLevel-iBufferlevel)>=iBlocksize) )
			{
				// lock sound buffer
				hr = lpDsb->lpVtbl->Lock(lpDsb, dwWriteCursor, dwWriteBytes, 
					&lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0 );
				//	check if lock was successful
				if (hr==DS_OK) 
				{
					// start with first part of buffer and then second
					dwBytesCopied = dynamicbuffer_getbytes( &dynbuf, lpvAudioPtr1, dwAudioBytes1 );
					dwBytesCopied = dynamicbuffer_getbytes( &dynbuf, lpvAudioPtr2, dwAudioBytes2 );
					dwAudioBytes = dwAudioBytes1 + dwAudioBytes2;
					iBufferlevel += dwAudioBytes;
					dwWriteCursor += dwAudioBytes;
					// wrap our writepointer if necessary
					if ((int)dwWriteCursor >= iSoundBufferSize)
						dwWriteCursor = dwWriteCursor - iSoundBufferSize;
					// unlock buffer
					hr = lpDsb->lpVtbl->Unlock(lpDsb, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2 );
				}
				else
				{
					debugerrormsg( "inside main playing loop: Lock() failure\n" );
					iState = THREADSTATE_CLOSING;
				}
			}
		}

		// if bufferlevel is too low, we have to add silence and stop playing
		if ( (iBufferlevel<iLowLevel) && (iState!=THREADSTATE_CLOSING) )
		{
			// final filling of sound buffer with silence
			dwWriteBytes = (iLowLevel-iBufferlevel) + iMinLevel + 4*iBlocksize;
			hr = lpDsb->lpVtbl->Lock(lpDsb, dwWriteCursor, dwWriteBytes, 
				&lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0 );
			if (hr==DS_OK) 
			{
				memset( lpvAudioPtr1, 0, dwAudioBytes1 );
				memset( lpvAudioPtr2, 0, dwAudioBytes2 );
				// unlock buffer
				hr = lpDsb->lpVtbl->Unlock(lpDsb, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2 );

				Sleep( (iMinLevel/2 + iLowLevel)/iBytesPerMs );
				iBufferlevel = 0;
			}
			else
			{
				debugerrormsg( "inside main playing loop, filling zeros: Lock() failure\n" );
				iState = THREADSTATE_CLOSING;
			}
				
			iState = THREADSTATE_CLOSING;
		}

		if ( iState!=THREADSTATE_CLOSING )
		{
			// sleep a little bit
			threadReleaseToken( &iThreadToken );
			Sleep( iSleepTimeMs );
			threadGetToken( &iThreadToken );
		}		
	}


	CloseDirectSound();	// this stops playing and deinit directsound

	// free buffer
	dynamicbuffer_flush( &dynbuf );

	iThreadCommand = THREADCOMMAND_NONE;
	threadReleaseToken( &iThreadToken );

	timeEndPeriod(1);

	return ERROR_SUCCESS;
}





int OpenDirectSound(int samplerate, int no_of_channels, int buffersize) 
{
	WAVEFORMATEX wfx;

	// Set up wave format structure. 
    memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
    wfx.wFormatTag = WAVE_FORMAT_PCM; 
    wfx.nSamplesPerSec = samplerate; 
    wfx.nChannels = no_of_channels; 
    wfx.wBitsPerSample = 16; 
    wfx.nBlockAlign = wfx.nChannels*wfx.wBitsPerSample/8; 
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; 

	if ( CreatePrimaryBuffer(&wfx)>0 )
	{
		if ( CreateSecondaryBuffer(&wfx, buffersize)==buffersize )
		{
			return 1;
		}
		else
		{
			debugerrormsg( "OpenDirectSound: CreateSecondaryBuffer() failure\n" );
		}
	}
	else
	{
		debugerrormsg( "OpenDirectSound: CreatePrimaryBuffer() failure\n" );
	}

	return -1;
}


int CloseDirectSound()
{
	HRESULT hResult;
	DWORD dwWriteCursor = 0;    
	DWORD dwWriteBytes = 0;     
	LPVOID lpvAudioPtr1; 
	DWORD dwAudioBytes1;
	LPVOID lpvAudioPtr2;
	DWORD dwAudioBytes2;

	// stop playing
	hResult = lpDsb->lpVtbl->Stop(lpDsb);

	// clear secondary buffer (dont know if this is necessary)
	if ( DS_OK == lpDsb->lpVtbl->Lock(lpDsb, dwWriteCursor, dwWriteBytes, 
	&lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, DSBLOCK_ENTIREBUFFER ) )
	{
		if (lpvAudioPtr1 != NULL) memset(lpvAudioPtr1, 0, dwAudioBytes1); 
		if (lpvAudioPtr2 != NULL) memset(lpvAudioPtr2, 0, dwAudioBytes2); 
		lpDsb->lpVtbl->Unlock(lpDsb, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2 );
	}

	// release secondary buffer
	if (lpDsb)
        lpDsb->lpVtbl->Release(lpDsb); 

	// release primary buffer
	if (lpDsbPrimary)
        lpDsbPrimary->lpVtbl->Release(lpDsbPrimary); 

	// shut down directsound
	if (lpDirectSound)
	{
        lpDirectSound->lpVtbl->Release(lpDirectSound); 
		return 1;
	}

	return 0;
}



int CreatePrimaryBuffer( LPWAVEFORMATEX lpWfx ) 
{ 
    DSBUFFERDESC dsbdesc; 
    DSBCAPS dsbcaps; 
    HRESULT hr; 
	HWND hwnd;

   // Create DirectSound
    if FAILED( DirectSoundCreate(NULL, &lpDirectSound, NULL) ) return -1;
 
	// get a window handle - dont know why, but it is needed for SetCooperativeLevel(...)
	hwnd = GetForegroundWindow();
	if (hwnd == NULL) hwnd = GetDesktopWindow();

    // Set up DSBUFFERDESC structure. 
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
    dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER; 
    dsbdesc.dwBufferBytes = 0; // Buffer size is determined by sound hardware. 
    dsbdesc.lpwfxFormat = NULL; // Must be NULL for primary buffers. 
 
    // Obtain write-primary cooperative level. 
    hr = lpDirectSound->lpVtbl->SetCooperativeLevel(lpDirectSound, hwnd, DSSCL_PRIORITY ); //DSSCL_WRITEPRIMARY	); 
    if SUCCEEDED(hr) 
    { 
        // Try to create buffer. 
        hr = lpDirectSound->lpVtbl->CreateSoundBuffer(lpDirectSound, &dsbdesc, &lpDsbPrimary, NULL); 
		if SUCCEEDED(hr) 
        { 
            // Set primary buffer to desired format. 
            hr = lpDsbPrimary->lpVtbl->SetFormat(lpDsbPrimary, lpWfx); 
            if SUCCEEDED(hr) 
            { 
				// If you want to know the buffer size, call GetCaps. 
                dsbcaps.dwSize = sizeof(DSBCAPS); 
                lpDsbPrimary->lpVtbl->GetCaps(lpDsbPrimary, &dsbcaps); 
                return dsbcaps.dwBufferBytes; 
            } 
        } 
    } 

    // Failure. 
    return -1; 
}


int CreateSecondaryBuffer( LPWAVEFORMATEX lpWfx, int buffersize) 
{ 
    DSBUFFERDESC dsbdesc; 
    HRESULT hr; 
 
    // set up DSBUFFERDESC structure. 
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
    dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		//| DSBCAPS_CTRLPOSITIONNOTIFY 
		//| DSBCAPS_GETCURRENTPOSITION2; 
    dsbdesc.dwBufferBytes = buffersize; 
    dsbdesc.lpwfxFormat = lpWfx; 
 
    // try to create secondary buffer. 
    hr = lpDirectSound->lpVtbl->CreateSoundBuffer(lpDirectSound, &dsbdesc, &lpDsb, NULL); 
	if SUCCEEDED(hr) 
	{
		return dsbdesc.dwBufferBytes;
		/*
		// get notification interface
		hr = lpDsb->lpVtbl->QueryInterface(lpDsb, IID_IDirectSoundNotify, (VOID**)&lpDsNotify); 
		if (SUCCEEDED(hr)) 
		{ 
			return dsbdesc.dwBufferBytes;
		} 
		*/
	}

    // Failure. 
    return -1; 
}


#define TIMEOUT_THREADGETTOKEN 5000
#define THREADLOCK_SLEEPTIME 1

void threadGetToken( int* ptoken )
{
	int isLockedOld;
	int timeout_counter = TIMEOUT_THREADGETTOKEN;

	isLockedOld = (int)InterlockedCompareExchange( (PVOID*)(ptoken), (PVOID)1, (PVOID)0 );
	while ( (isLockedOld==1) && (timeout_counter>=0) )
	{
		Sleep( THREADLOCK_SLEEPTIME );
		timeout_counter -= THREADLOCK_SLEEPTIME;
		isLockedOld = (int)InterlockedCompareExchange( (PVOID*)(ptoken), (PVOID)1, (PVOID)0 );
	}

	if ( (timeout_counter<0) && (isLockedOld==1) )
	{
		debugerrormsg( "threadGetToken: unable to get token!\n" );
	}

	//return (isLockedOld==0);
}

void threadReleaseToken( int* ptoken )
{
	//int isLockedOld;
	//isLockedOld = (int)InterlockedCompareExchange( (PVOID*)(ptoken), (PVOID)0, (PVOID)1 );

	//return (isLockedOld==1);

	*ptoken = 0;
}






int debugmsg( char *msg )
{
#ifdef DEBUG_OUTPUT_FILENAME
	FILE *debugOutFile;

	if ((debugOutFile = fopen(DEBUG_OUTPUT_FILENAME,"a"))!=NULL)
	{
		fprintf(debugOutFile, msg);
		fclose(debugOutFile);
	}
#endif

	return 0;
}

int debugerrormsg( char *msg )
{
#ifdef DEBUG_ERROR_OUTPUT_FILENAME
	FILE *debugErrorOutFile;

	if ((debugErrorOutFile = fopen(DEBUG_ERROR_OUTPUT_FILENAME,"a"))!=NULL)
	{
		fprintf(debugErrorOutFile, msg);
		fclose(debugErrorOutFile);
	}
#endif

	ErrorFlag=1;
	strcpy( ErrorMsg, msg );

	return 0;
}


/*
clear all;
mex wavplay_directx.c src_filter.c dynamicbuffer.c user32.lib dsound.lib libcmt.lib

X1 = randn( 2^16, 1 ) + j*randn( 2^16, 1 );
Xmask = zeros( size(X1) );
Xmask(1000:1100)=1;
Xmask(2050:2150)=0.5;
X1 = X1.*Xmask;
X = [X1;flipud(X1)];
x = real( ifft(X) );
x = x./max(x)*0.9;
x_int16 = int16( x*(2^15-1) );

wavplay_directx(x_int16',48000,1.0)
wavplay_directx(x_int16',48000,1.0)
wavplay_directx(x,48000,1.0)

%are there spikes at start and end?
x(1:5000) = x(1:5000).*[1:5000]'/5000;
x(end+[-5000:0]) = x(end+[-5000:0]).*[5000:-1:0]'/5000;
wavplay_directx_c(x,48000)
*/

