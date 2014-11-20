/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Andreas Dittrich                                       */
/*                                                                            */
/*  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  */
/*  Project start: 27.05.2004                                                 */
/*  Last change: 21.04.2005, 16:30                                            */
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
/*  Last change: 21.04.2005, 16:30                                            */
/*  By         : Andreas Dittrich                                             */
/*  Description: workaround: negative record cursor increment detection       */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*  wavrecord__directx.c                                                      */
/*                                                                            */
/******************************************************************************/
/* out = wavrecord__directx( no_of_samples, samplerate, no_of_channels [,r])  */
/* [out, delay_diff, delay_offset] =                                          */
/*       wavrecord_c( no_of_samples, samplerate, no_of_channels [,r] )        */
/******************************************************************************/


// we need: user32.lib dsound.lib libcmt.lib
// mex wavrecord_directx.c src_filter.c user32.lib dsound.lib libcmt.lib

#include <math.h>
#include <stdlib.h>
#include "mex.h"

#include <process.h>
#include <windows.h>
#include <dsound.h>

#include "src_filter.h"



int ErrorFlag = 0;
char ErrorMsg[512];
//#define DEBUG_OUTPUT_FILENAME "wavrecord_debug.txt"
#define DEBUG_ERROR_OUTPUT_FILENAME "wavrecord_error.txt"



#define NARGS_RHS_STR "3"
#define NARGS_LHS_STR "1"
#define PROGNAME "wavrecord_directx_c"

#define RECORD_BUFFER_SECONDS 8

#define THREADCOMMAND_NONE 0
#define THREADCOMMAND_COPY 1
#define THREADCOMMAND_CLOSE 2
#define THREADCOMMAND_SYNC 3


typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned int uint32_t;


// some global (and static) variables
HANDLE hMyThread;
int iFirstRun = 1;
int iThreadCommand = THREADCOMMAND_NONE;
int iThreadToken = 0;
int iBufferlevel;

int samplerate = 0;
int no_of_channels = 1;
int bytes_per_sample = 2;	// for now only 16 bit PCM
int no_of_samples;
int reset_src_filter;
int sample_rate_conversion_enable = 0;
double ty_src_filter;
double r_src_filter=1.0;

int delay_diff;
double delay_offset;

double *out;


LPDIRECTSOUNDCAPTURE lpDsc; 
LPDIRECTSOUNDCAPTUREBUFFER lpDscb;


int debugmsg( char *msg );
int debugerrormsg( char *msg );

void threadGetToken( int* ptoken );
void threadReleaseToken( int* ptoken );

int OpenDirectSoundCapture(int samplerate, int no_of_channels, int buffersize);
int CloseDirectSoundCapture();


DWORD WINAPI MyThreadProc( LPVOID lpParameter );
void MyExitFunction( void );


void sample_rate_conversion_int16_mono_mono_ex( int16_t *pwInPtr, unsigned int *uiInSamples, 
										  int16_t *pwOutPtr, unsigned int *uiOutSamples, 
										  double *ty, double r, int *iReset );
void sample_rate_conversion_int16_stereo_stereo_ex( int16_t *pwInPtr, unsigned int *uiInSamples, 
										  int16_t *pwOutPtr, unsigned int *uiOutSamples, 
										  double *ty, double r, int *iReset );


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

	int samplerate_new;
	int no_of_channels_new;
	int soundbuffer_size = 0;
	int soundbuffer_size_new = 0;
	int sample_rate_conversion_enable_new;
	int close_recording_thread;

	DWORD dwExitCode = -1;
	DWORD dwThreadId;

	if (iFirstRun)
	{
		iThreadCommand = THREADCOMMAND_NONE;
		iFirstRun = 0;
		iThreadToken = 0;

		// register exit function
		mexAtExit( MyExitFunction );
	}

	/* Check for proper number of arguments */
	if (nrhs==0)
	{
		MyExitFunction();	// stop all and go out
		plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
		*mxGetPr(plhs[0]) = 0;

		return;
	}

	if ((nrhs!=3)&&(nrhs!=4)) {
		mexErrMsgTxt(PROGNAME " requires at least 3 input arguments.");
	} else if ( (nlhs!=0) && (nlhs!=1) && (nlhs!=3) )
		mexErrMsgTxt(PROGNAME " requires " NARGS_LHS_STR " output argument(s).");
   
	/* Check dimensions */
	// no_of_samples
	parg = prhs[0];
	m = mxGetM(parg);
	n = mxGetN(parg); 
	N = m*n;
	if ( !mxIsNumeric(parg) || mxIsComplex(parg) || 
		 mxIsSparse(parg)  || !mxIsDouble(parg) || (m!=1) || (n!=1) || (N<1) )
  		mexErrMsgTxt(PROGNAME " requires that no_of_samples is a scalar.");

	// samplerate
	parg = prhs[1];
	m = mxGetM(parg);
	n = mxGetN(parg); 
	N = m*n;
	if ( !mxIsNumeric(parg) || mxIsComplex(parg) || 
		 mxIsSparse(parg)  || !mxIsDouble(parg) || (m!=1) || (n!=1) || (N<1) )
		mexErrMsgTxt(PROGNAME " requires that samplerate is a scalar.");

	// no_of_channels
	parg = prhs[2];
	m = mxGetM(parg);
	n = mxGetN(parg); 
	N = m*n;
	if ( !mxIsNumeric(parg) || mxIsComplex(parg) || 
		 mxIsSparse(parg)  || !mxIsDouble(parg) || (m!=1) || (n!=1) || (N<1) )
		mexErrMsgTxt(PROGNAME " requires that no_of_channels is a scalar.");

	// r_src_filter 
	if (nrhs>3)
	{
		double r_src_filter_new;

		parg = prhs[3];
		m = mxGetM(parg);
		n = mxGetN(parg); 
		N = m*n;
		if ( !mxIsNumeric(parg) || mxIsComplex(parg) || 
			 mxIsSparse(parg)  || !mxIsDouble(parg) || (m!=1) || (n!=1) || (N<1) )
			mexErrMsgTxt(PROGNAME " requires that r is a scalar.");

		r_src_filter_new = mxGetScalar(prhs[3]);
		if ((r_src_filter_new<0.125) || (r_src_filter_new>2.0) )
			mexErrMsgTxt(PROGNAME " requires that r is between 0.125..2.0");
		else
			r_src_filter = r_src_filter_new;

		sample_rate_conversion_enable_new = 1;
	}
	else
	{
		r_src_filter=1.0;
		sample_rate_conversion_enable_new = 0;
	}



	/* check input values and assign pointer to the various parameters*/
	// no_of_samples
	no_of_samples = (int)floor( mxGetScalar(prhs[0]) + 0.5 );
	if ( no_of_samples<0 )
		close_recording_thread = 1;
	else
		close_recording_thread = 0;

	// samplerate
	samplerate_new = (int)floor( mxGetScalar(prhs[1]) + 0.5 );
	if ( (samplerate_new<8000) || (samplerate_new>48000) )
		mexErrMsgTxt(PROGNAME " requires that samplerate is between 8000..48000");
	// no_of_channels
	no_of_channels_new = (int)floor( mxGetScalar(prhs[2]) + 0.5 );
	if ( (no_of_channels_new<1) || (no_of_channels_new>2) )
		mexErrMsgTxt(PROGNAME " requires that no_of_channels is 1 or 2");

	/* Create a matrix for the return arguments */
	if (nlhs==3)
	{
		plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
		plhs[2] = mxCreateDoubleMatrix(1, 1, mxREAL);
	}

	// if samplerate has changed we have to close the running thread 
	if ((samplerate_new!=samplerate)||(no_of_channels_new!=no_of_channels)||
		(sample_rate_conversion_enable_new!=sample_rate_conversion_enable)||(close_recording_thread==1))
	{
		// we have to wait until the thread has terminated, and then restart playing-thread with new samplerate
		iThreadCommand = THREADCOMMAND_CLOSE;	// force thread to quit
		GetExitCodeThread( hMyThread, &dwExitCode );
		while (dwExitCode==STILL_ACTIVE)
		{
			Sleep(10);
			GetExitCodeThread( hMyThread, &dwExitCode );
		}

		// now we can start again with new sampling rate
		samplerate = samplerate_new;
		no_of_channels = no_of_channels_new;
		sample_rate_conversion_enable = sample_rate_conversion_enable_new;

		if (close_recording_thread==1) 
		{
			plhs[0] = mxCreateDoubleMatrix(0, 0, mxREAL);
			return;
		}
	}
	
	/* Create a matrix for the return arguments */
	plhs[0] = mxCreateDoubleMatrix(no_of_channels, no_of_samples, mxREAL);
	out = (double*)mxGetPr(plhs[0]);

	//threadGetToken( &iThreadToken );
	GetExitCodeThread( hMyThread, &dwExitCode );
	if ( dwExitCode!=STILL_ACTIVE )	//thread is not running -> we have to restart again
	{ 

		// create record-control-thread 
		hMyThread = (HANDLE)_beginthreadex(
			NULL,
			0,
			MyThreadProc,
			NULL,
			CREATE_SUSPENDED,	// wait until ResumeThread()
			&dwThreadId
		);

/*{	
char strbuf[500];
sprintf( strbuf, "create thread, return value: %i\n", (int)hMyThread );
debugmsg( strbuf );
}*/
	
		iThreadCommand = THREADCOMMAND_NONE;

		// this thread controls the recording of the soundcard, so its a little bit time critical
		SetThreadPriority( hMyThread, THREAD_PRIORITY_TIME_CRITICAL ); // THREAD_PRIORITY_HIGHEST

		// reset internal state variables 
		reset_src_filter=1;
		ty_src_filter=0;
		delay_diff = 0;
		delay_offset = 0;

		// start Thread
		ResumeThread( hMyThread );
	}
	//threadReleaseToken( &iThreadToken );

	// wait until data is copied
	iThreadCommand = THREADCOMMAND_COPY;
	GetExitCodeThread( hMyThread, &dwExitCode );
	while( (iThreadCommand!=THREADCOMMAND_NONE) && (dwExitCode==STILL_ACTIVE) )
	{
		Sleep( 1 );
		GetExitCodeThread( hMyThread, &dwExitCode );
	}

	if (nlhs==3)
	{
		threadGetToken( &iThreadToken );
		*mxGetPr(plhs[1]) = (double)delay_diff;
		*mxGetPr(plhs[2]) = delay_offset;
		delay_diff = 0;
		threadReleaseToken( &iThreadToken );
	}

	if (ErrorFlag!=0)
	{
		mexWarnMsgTxt( ErrorMsg );
		Sleep( 1000 );
		ErrorFlag=0;
	}

	return;
}


/******************************************************************************/
/* function                                                                   */
/******************************************************************************/

void MyExitFunction( void )
{
	DWORD dwExitCode = -1;

	iThreadCommand = THREADCOMMAND_CLOSE;	// force thread to quit
	GetExitCodeThread( hMyThread, &dwExitCode );
	while (dwExitCode==STILL_ACTIVE)
	{
		Sleep(10);
		GetExitCodeThread( hMyThread, &dwExitCode );
	}

	iFirstRun = 1;
	CloseHandle(hMyThread);
}



#define THREADSTATE_CLOSING 0 
#define THREADSTATE_NORMAL 1
#define THREADSTATE_COPYING 2

#define MIN(a,b) ( (a)>(b) ? b : a )

#define BLOCKSIZE_SRC_BYTES 4096

DWORD WINAPI MyThreadProc(LPVOID lpParameter )
{
	HRESULT hr; 

	double *dst;

	int16_t dst_buffer[BLOCKSIZE_SRC_BYTES/2];

	int iState;
	int iSamplesToCopy;
	int iSoundBufferSize;
	int iBlocksize;
	int iBufferlevel_inc;
	int iMinLevel;
	int iMaxLevel;
	int iSleepTimeMs;
	int iBytesPerMs;
	int iBlocksProcessed;

	DWORD dwCapturePosition;
	DWORD dwCapturePosition_last;
	DWORD dwReadPosition;

	DWORD dwReadCursor;     
	DWORD dwReadBytes;  
    
	LPVOID lpvAudioPtr1;
	DWORD dwAudioBytes1;  
	LPVOID lpvAudioPtr2;  
	DWORD dwAudioBytes2;  
	DWORD dwAudioBytes;  

//debugmsg( "thread start\n");
	// get our working token, so we can do exclusive work on the global variables
	threadGetToken( &iThreadToken );

	// do some initialization
	iBlocksize = 1024*no_of_channels*bytes_per_sample; // 4 KByte, is about 20ms @ 48kHz, 16bit stereo
	iBytesPerMs = (samplerate*no_of_channels*bytes_per_sample)/1000+1;
	iSleepTimeMs = MIN(10, iBlocksize/(iBytesPerMs*2)+1);
	iSoundBufferSize = ((RECORD_BUFFER_SECONDS*(samplerate*no_of_channels*bytes_per_sample))/iBlocksize+1+10)*iBlocksize;	// RECORD_BUFFER_SECONDS seconds
	iMaxLevel = iSoundBufferSize - 12*iBlocksize;	// we stop thread if this level is reached
	iMinLevel = 4*iBlocksize;	// that is our minimal recording delay

	if ( (OpenDirectSoundCapture( samplerate, no_of_channels, iSoundBufferSize )) < 0 )
	{
		debugerrormsg( "recording thread: unable to open soundcapture device\n" );

		iThreadCommand = THREADCOMMAND_NONE;
		threadReleaseToken( &iThreadToken );
		return ERROR_SUCCESS;
	}
	
	// set cycletime to 1ms
	timeBeginPeriod(1);	// set system cycletime to 1ms

	//start recording
	hr = lpDscb->lpVtbl->Start(lpDscb, DSCBSTART_LOOPING );
	if (hr!=DS_OK) 
	{
		debugerrormsg( "start recording failure: Start()\n" );
		iState = THREADSTATE_CLOSING;
	}

	// get our start position
	hr = lpDscb->lpVtbl->GetCurrentPosition( lpDscb, &dwCapturePosition, &dwReadPosition );
	if (hr!=DS_OK) 
	{
		debugerrormsg( "firt call of GetCurrentPosition() failure\n" );
		iState = THREADSTATE_CLOSING;
	}
	else
	{
		dwCapturePosition_last = dwCapturePosition;
		dwReadCursor = 0;//dwReadPosition%4;	// we have to start aligned to samples
		iBufferlevel = 0;

		iState = THREADSTATE_NORMAL;
	}

	// now the main recording loop!
	// do processing, as long as there is enough place for data
	while ( iState!=THREADSTATE_CLOSING )
	{
//debugmsg( "---loop---\n");
		// sleep a little bit
		threadReleaseToken( &iThreadToken );
		Sleep( iSleepTimeMs/3 );
		if (iThreadCommand==THREADCOMMAND_NONE) 
			Sleep( iSleepTimeMs/3 );
		if (iThreadCommand==THREADCOMMAND_NONE) 
			Sleep( iSleepTimeMs/3 );
		threadGetToken( &iThreadToken );

		//check for commands, change state
		if (iThreadCommand==THREADCOMMAND_CLOSE)
		{
			iState = THREADSTATE_CLOSING;
		}
		else if ( (iThreadCommand==THREADCOMMAND_COPY) && (iState==THREADSTATE_NORMAL) )
		{
			if (no_of_samples>0)
			{
				iState = THREADSTATE_COPYING;
				iSamplesToCopy = no_of_samples;
				dst = out;
			}
			else
			{
				iThreadCommand = THREADCOMMAND_NONE;
				iState = THREADSTATE_NORMAL;
			}
//debugmsg( "copy command\n");
		}

		// check buffer
		if ( iState!=THREADSTATE_CLOSING )
		{
			// get position and check our buffer
			hr = lpDscb->lpVtbl->GetCurrentPosition( lpDscb, &dwCapturePosition, &dwReadPosition );
			if (hr!=DS_OK) 
			{
				debugerrormsg( "inside main recording loop: GetCurrentPosition() failure\n" );
				iState = THREADSTATE_CLOSING;
			}
			else
			{
				iBufferlevel_inc = (int)dwCapturePosition - (int)dwCapturePosition_last;
				dwCapturePosition_last = dwCapturePosition;

				// sometimes the RecordCursor seems not to behave as it should and jumps back in time! 
				if (iBufferlevel_inc<-(iSoundBufferSize/2)) 
					iBufferlevel_inc += iSoundBufferSize;
				else if (iBufferlevel_inc>(iSoundBufferSize/2))
					iBufferlevel_inc -= iSoundBufferSize;

				iBufferlevel += iBufferlevel_inc;

				if (iBufferlevel>iMaxLevel) 
					iState = THREADSTATE_CLOSING;
			}
		}

		if ( iThreadCommand==THREADCOMMAND_SYNC )
		{
			iThreadCommand = THREADCOMMAND_NONE;
		}

		// do we have data to copy
		iBlocksProcessed = 0;
		while ( (iState==THREADSTATE_COPYING) && (iBufferlevel>(iMinLevel+iBlocksize)) )
		{
/*{	
char strbuf[500];
sprintf( strbuf, "copy *** iState: %i, iSamplesToCopy: %i, iBufferlevel: %i, iMinLevel: %i\n", 
		iState, iSamplesToCopy, iBufferlevel, iMinLevel );
debugmsg( strbuf );
}*/
			// lock buffer of dwReadBytes Bytes
			dwReadBytes = iBlocksize;
			hr = lpDscb->lpVtbl->Lock(lpDscb, dwReadCursor, dwReadBytes, 
				&lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0 );
			if (hr!=DS_OK) 
			{
				debugerrormsg( "inside main recording loop: Lock() failure\n" );
				iState = THREADSTATE_CLOSING;
			}
			else
			{
				int k, N;
				int dst_buffer_index = 0;
				int N_src = dwAudioBytes1/(bytes_per_sample*no_of_channels);
				int N_dst = MIN( iSamplesToCopy, BLOCKSIZE_SRC_BYTES/(bytes_per_sample*no_of_channels) );
				int N_src_processed = N_src;
				int N_dst_processed = N_dst;

				if (sample_rate_conversion_enable==1)
				{
					if( no_of_channels==1 )
						sample_rate_conversion_int16_mono_mono_ex( lpvAudioPtr1, &N_src_processed,
										  dst_buffer, &N_dst_processed, &ty_src_filter, r_src_filter, &reset_src_filter );
					else
						sample_rate_conversion_int16_stereo_stereo_ex( lpvAudioPtr1, &N_src_processed,
										  dst_buffer, &N_dst_processed, &ty_src_filter, r_src_filter, &reset_src_filter );

					delay_diff = delay_diff + (N_src_processed-N_dst_processed);
					delay_offset = ty_src_filter;
				}
				else
				{
					N_src_processed = MIN(N_src, N_dst);
					N_dst_processed = N_src_processed;
					memcpy( dst_buffer, lpvAudioPtr1, no_of_channels*N_src_processed*sizeof(int16_t) );
				}
/*{
char buf[500];
sprintf( buf, "N_src: %i, N_src_proc: %i, N_dst: %i, N_dst_proc: %i, dwAudioB1: %i, dwAudioB2: %i, level: %i, ty: %e\n", 
		N_src, N_src_processed, N_dst, N_dst_processed, dwAudioBytes1, dwAudioBytes2, iBufferlevel, ty_src_filter );
debugmsg( buf );
}*/

				iSamplesToCopy = iSamplesToCopy - N_dst_processed;
				dst_buffer_index = N_dst_processed;
				N_dst = N_dst - N_dst_processed;

				dwAudioBytes = N_src_processed*(bytes_per_sample*no_of_channels);

				if ((N_src_processed==N_src)&&(N_dst>0)&&(dwAudioBytes2>0))
				{
					N_src = dwAudioBytes2/(bytes_per_sample*no_of_channels);
					N_src_processed = N_src;
					N_dst_processed = N_dst;

					if (sample_rate_conversion_enable==1)
					{
						if( no_of_channels==1 )
							sample_rate_conversion_int16_mono_mono_ex( lpvAudioPtr2, &N_src_processed,
												  dst_buffer+dst_buffer_index*no_of_channels, &N_dst_processed, &ty_src_filter, r_src_filter, &reset_src_filter );
						else
							sample_rate_conversion_int16_stereo_stereo_ex( lpvAudioPtr2, &N_src_processed,
												  dst_buffer+dst_buffer_index*no_of_channels, &N_dst_processed, &ty_src_filter, r_src_filter, &reset_src_filter );

						delay_diff = delay_diff + (N_src_processed-N_dst_processed);
						delay_offset = ty_src_filter;
					}
					else
					{
						N_src_processed = MIN(N_src, N_dst);
						N_dst_processed = N_src_processed;
						memcpy( dst_buffer+dst_buffer_index*no_of_channels, lpvAudioPtr2, no_of_channels*N_src_processed*sizeof(int16_t) );
					}

					iSamplesToCopy = iSamplesToCopy - N_dst_processed;
					dst_buffer_index = dst_buffer_index + N_dst_processed;
					N_dst = N_dst - N_dst_processed;

					dwAudioBytes = dwAudioBytes + N_src_processed*(bytes_per_sample*no_of_channels);
				}

				// increase our readpointer 
				dwReadCursor += dwAudioBytes;
				if ((int)dwReadCursor >= iSoundBufferSize)
					dwReadCursor = (int)dwReadCursor - iSoundBufferSize;
				iBufferlevel -= (int)dwAudioBytes;

				// copy data to matlab-double-vector
				N = dst_buffer_index*no_of_channels;
				for( k=0; k<N; k++)
					dst[k] = _scalb( (double)(dst_buffer[k]), -15 );

				dst = dst + N;

				if (iSamplesToCopy<=0)
				{
					iThreadCommand = THREADCOMMAND_NONE;
					iState = THREADSTATE_NORMAL;
				}

				// unlock buffer
				hr = lpDscb->lpVtbl->Unlock(lpDscb, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2 );
			}

			iBlocksProcessed++;
			if (iBlocksProcessed>8)
			{
				Sleep(1);	// let other threads with highest priority do their job
				iBlocksProcessed = 0;
			}
		}

	}

	CloseDirectSoundCapture();	// this stops recording and deinit directsoundbuffer

	iThreadCommand = THREADCOMMAND_NONE;
	threadReleaseToken( &iThreadToken );

	timeEndPeriod(1);

	return ERROR_SUCCESS;
}






int OpenDirectSoundCapture(int samplerate, int no_of_channels, int buffersize) 
{
	WAVEFORMATEX wfx;
    DSCBUFFERDESC dscbdesc; 

	// Set up wave format structure. 
    memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
    wfx.wFormatTag = WAVE_FORMAT_PCM; 
    wfx.nSamplesPerSec = samplerate; 
    wfx.nChannels = no_of_channels; 
    wfx.wBitsPerSample = 16; 
    wfx.nBlockAlign = wfx.nChannels*wfx.wBitsPerSample/8; 
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; 

    // Set up DSBUFFERDESC structure. 
    memset(&dscbdesc, 0, sizeof(DSCBUFFERDESC));
    dscbdesc.dwSize = sizeof(DSCBUFFERDESC); 
    dscbdesc.dwFlags = 0; 
    dscbdesc.dwBufferBytes = buffersize; 
	dscbdesc.dwReserved = 0;
	dscbdesc.lpwfxFormat = &wfx;

   // Create DirectSoundCapture and DirectSoundCaptureBuffer
    if ( DirectSoundCaptureCreate(NULL, &lpDsc, NULL) != DS_OK ) 
	{
		debugerrormsg( "DirectSoundCaptureCreate() failure\n" );
		return -1;
	}
	else
	{
		if ( lpDsc->lpVtbl->CreateCaptureBuffer(lpDsc, &dscbdesc, &lpDscb, NULL) != DS_OK )
		{
			debugerrormsg( "CreateCaptureBuffer() failure\n" );
			if (lpDsc)
				lpDsc->lpVtbl->Release(lpDsc); 

			return -1;
		}
	}

	return 0;
}


int CloseDirectSoundCapture()
{
	HRESULT hResult;

	// stop recording
	if (lpDscb)
	{
		hResult = lpDscb->lpVtbl->Stop(lpDscb);
        lpDscb->lpVtbl->Release(lpDscb); 
	}

	// release
	if (lpDsc)
        lpDsc->lpVtbl->Release(lpDsc); 

	return 0;
}






#define GETTOKEN_SLEEPTIME 1

void threadGetToken( int* ptoken )
{
	int isLockedOld;

	isLockedOld = (int)InterlockedCompareExchange( (PVOID*)(ptoken), (PVOID)1, (PVOID)0 );
	while (isLockedOld==1)
	{
		Sleep( GETTOKEN_SLEEPTIME );
		isLockedOld = (int)InterlockedCompareExchange( (PVOID*)(ptoken), (PVOID)1, (PVOID)0 );
	}

}

void threadReleaseToken( int* ptoken )
{
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
mex wavrecord_directx.c user32.lib dsound.lib libcmt.lib
y = wavrecord_directx( 100000, 48000, 1);

%are there spikes at start and end?
x(1:5000) = x(1:5000).*[1:5000]'/5000;
x(end+[-5000:0]) = x(end+[-5000:0]).*[5000:-1:0]'/5000;
wavplay_directx(x,48000, 1)
*/


void sample_rate_conversion_int16_mono_mono_ex( int16_t *pwInPtr, unsigned int *uiInSamples, 
										  int16_t *pwOutPtr, unsigned int *uiOutSamples, 
										  double *ty, double r, int *iReset )
{

	/*
		pwInPtr = start pointer of source data
		uiInSamples = input value: no of samples pointed by pwInPtr, output value: no of samples, which have been processed
		pwOutPtr = start pointer of destination data
		uiOutSamples = input value: no of samples pointed by pwInPtr, output value: no of samples, which have been written

		ty, r = values as defined in "sample_rate_conversion_int16_mono_mono()"
		iReset = set in order to clear initial state. iReset is set to zero after call.
	*/

	int N_src = *uiInSamples;
	int N_dst = *uiOutSamples;
	int *N_src_processed = uiInSamples;
	int *N_dst_processed = uiOutSamples;

	int16_t *src = pwInPtr;
	int16_t *dst = pwOutPtr;

	static int16_t state_buffer[2*SUBFILTER_LENGTH];
	int N_src_step1, N_src_step2, index_dst_inc;


/* 
	// for debugging only: simple copy src->dst
	int N_min;
	N_min = MIN(N_src, N_dst);
	memcpy( dst, src, N_min*sizeof(int16_t) );
	*N_src_processed = N_min;
	*N_dst_processed = N_min;
	return;
*/

	if (*iReset==1)
	{
		memset( state_buffer, 0, SUBFILTER_LENGTH*sizeof(int16_t) );	//clear first half of buffer
		*iReset=0;
	}


	N_src_step1 = MIN(SUBFILTER_LENGTH, N_src);
	N_src_step2 = N_src - N_src_step1;

	memcpy( state_buffer + SUBFILTER_LENGTH, src, N_src_step1*sizeof(int16_t) );   //fill last half of buffer

	// first step in sample rate conversion
	index_dst_inc = sample_rate_conv_int16_mono_mono( state_buffer, N_src_step1+SUBFILTER_LENGTH, dst, N_dst, ty, r );
	*N_src_processed = N_src_step1;
	*N_dst_processed = index_dst_inc;

	if (*ty<0)	// then N_dst was not enough
	{
		int ty_int = (int)floor( *ty );
		*ty = *ty - (double)ty_int;
		*N_src_processed += ty_int;
		// update state_buffer for next call
		memcpy( state_buffer, state_buffer + (*N_src_processed), SUBFILTER_LENGTH*sizeof(int16_t) );

		return;
	}

	if (N_src_step2==0)
	{
		// update state_buffer for next call
		memcpy( state_buffer, state_buffer + (*N_src_processed), SUBFILTER_LENGTH*sizeof(int16_t) );

		return;
	}

	if (N_src_step2>0)	//then, N_src is greater than SUBFILTER_LENGTH and N_src_step2=N_src-SUBFILTER_LENGTH
	{
		dst += index_dst_inc;
		N_dst = N_dst - index_dst_inc;

		// second step in sample rate conversion
		index_dst_inc = sample_rate_conv_int16_mono_mono( src, N_src_step2+SUBFILTER_LENGTH, dst, N_dst, ty, r );
		*N_src_processed += N_src_step2;
		*N_dst_processed += index_dst_inc;

		if (*ty<0)	// then N_dst was not enough
		{
			int ty_int = (int)floor( *ty );
			*ty = *ty - (double)ty_int;
			*N_src_processed += ty_int;
			// update state_buffer for next call
			memcpy( state_buffer, src + N_src_step2 + ty_int, SUBFILTER_LENGTH*sizeof(int16_t) );

			return;
		}

		// update state_buffer for next call
		memcpy( state_buffer, src + N_src_step2, SUBFILTER_LENGTH*sizeof(int16_t) );
	}
}






void sample_rate_conversion_int16_stereo_stereo_ex( int16_t *pwInPtr, unsigned int *uiInSamples, 
										  int16_t *pwOutPtr, unsigned int *uiOutSamples, 
										  double *ty, double r, int *iReset )
{

	/*
		pwInPtr = start pointer of source data
		uiInSamples = input value: no of samples pointed by pwInPtr, output value: no of samples, which have been processed
		pwOutPtr = start pointer of destination data
		uiOutSamples = input value: no of samples pointed by pwInPtr, output value: no of samples, which have been written

		ty, r = values as defined in "sample_rate_conversion_int16_mono_mono()"
		iReset = set in order to clear initial state. iReset is set to zero after call.
	*/

	int N_src = *uiInSamples;
	int N_dst = *uiOutSamples;
	int *N_src_processed = uiInSamples;
	int *N_dst_processed = uiOutSamples;

	int16_t *src = pwInPtr;
	int16_t *dst = pwOutPtr;

	static int16_t state_buffer[2*2*SUBFILTER_LENGTH];
	int N_src_step1, N_src_step2, index_dst_inc;


	if (*iReset==1)
	{
		memset( state_buffer, 0, 2*SUBFILTER_LENGTH*sizeof(int16_t) );	//clear first half of buffer
		*iReset=0;
	}


	N_src_step1 = MIN(SUBFILTER_LENGTH, N_src);
	N_src_step2 = N_src - N_src_step1;

	memcpy( state_buffer + 2*SUBFILTER_LENGTH, src, 2*N_src_step1*sizeof(int16_t) );   //fill last half of buffer

	// first step in sample rate conversion
	index_dst_inc = sample_rate_conv_int16_stereo_stereo( state_buffer, N_src_step1+SUBFILTER_LENGTH, dst, N_dst, ty, r );
	*N_src_processed = N_src_step1;
	*N_dst_processed = index_dst_inc;

	if (*ty<0)	// then N_dst was not enough
	{
		int ty_int = (int)floor( *ty );
		*ty = *ty - (double)ty_int;
		*N_src_processed += ty_int;
		// update state_buffer for next call
		memcpy( state_buffer, state_buffer + 2*(*N_src_processed), 2*SUBFILTER_LENGTH*sizeof(int16_t) );

		return;
	}

	if (N_src_step2==0)
	{
		// update state_buffer for next call
		memcpy( state_buffer, state_buffer + 2*(*N_src_processed), 2*SUBFILTER_LENGTH*sizeof(int16_t) );

		return;
	}

	if (N_src_step2>0)	//then, N_src is greater than SUBFILTER_LENGTH and N_src_step2=N_src-SUBFILTER_LENGTH
	{
		dst += 2*index_dst_inc;
		N_dst = N_dst - index_dst_inc;

		// second step in sample rate conversion
		index_dst_inc = sample_rate_conv_int16_stereo_stereo( src, N_src_step2+SUBFILTER_LENGTH, dst, N_dst, ty, r );
		*N_src_processed += N_src_step2;
		*N_dst_processed += index_dst_inc;

		if (*ty<0)	// then N_dst was not enough
		{
			int ty_int = (int)floor( *ty );
			*ty = *ty - (double)ty_int;
			*N_src_processed += ty_int;
			// update state_buffer for next call
			memcpy( state_buffer, src + 2*(N_src_step2 + ty_int), 2*SUBFILTER_LENGTH*sizeof(int16_t) );

			return;
		}

		// update state_buffer for next call
		memcpy( state_buffer, src + 2*N_src_step2, 2*SUBFILTER_LENGTH*sizeof(int16_t) );
	}
}
