/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Andreas Dittrich                                       */
/*                                                                            */
/*  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de)                  */
/*                 Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 27.05.2004                                                 */
/*  Last change  : 05.04.2005                                                 */
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
/*  aacdecode_c.c                                                             */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Description:                                                               */
/* just a gateway to use FAAD2                                                */
/*                                                                            */
/* Usage:                                                                     */
/* samples = aacdecode_c( data, samplerate, channels )                        */
/*                                                                            */
/******************************************************************************/


/* we need: libfaad2.lib, has to be compiled with -D DRM */
/* mex aacdecode.c libfaad2.lib */

#include <math.h>
#include <stdlib.h>
#include "mex.h"
#include <memory.h>
#include <neaacdec.h>


#define NARGS_RHS_STR "1"
#define NARGS_RHS 3 
#define NARGS_LHS_STR "2"
#define NARGS_LHS 2
#define PROGNAME "drm_aacdecode_c"

#define TRANSFORM_LENGTH 960
const int samples_per_audioframe_table[5] = { 
	TRANSFORM_LENGTH, TRANSFORM_LENGTH*2, TRANSFORM_LENGTH*2, TRANSFORM_LENGTH*2, TRANSFORM_LENGTH*2
};




typedef short int16_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;


NeAACDecHandle hDecoder;
NeAACDecFrameInfo frameInfo;
int dll_state = 0;					/* 0: first run, >0: initialized */


void print_channel_info(NeAACDecFrameInfo *frameInfo);

void MyExitFunction( void );

/******************************************************************************/
/*  Gateway-Routine                                                           */
/******************************************************************************/

void mexFunction(
                  int nlhs,       mxArray *plhs[],
                  int nrhs, const mxArray *prhs[]
           		 )
{
	uint8_t *in_ptr;
	int N_in;

	long N;
	unsigned long m,n;
	int dims[2];
	int16_t *out_ptr;
	
	long samplerate;
	char channels;

	uint32_t uiResult;
	int16_t *pcm_output;
	long lResult;
	int iResult;
   
	/* Check for proper number of arguments */
	if (nrhs != NARGS_RHS) {
		mexErrMsgTxt(PROGNAME " requires " NARGS_RHS_STR " input arguments.");
	} else if ( (nlhs > NARGS_LHS) && !((nlhs==0)&&(NARGS_LHS==1)) )
		mexErrMsgTxt(PROGNAME " requires " NARGS_LHS_STR " output argument(s).");
  
   /* Check dimensions */
    	
	/* in */
	m = mxGetM(prhs[0]);
	n = mxGetN(prhs[0]); 
	N = m*n;
	N_in = N;
	if ( !mxIsNumeric(prhs[0]) || mxIsComplex(prhs[0]) || 
		 mxIsSparse(prhs[0])  || !( mxIsDouble(prhs[0]) || mxIsUint8(prhs[0]) || mxIsEmpty(prhs[0]) )  )
		mexErrMsgTxt(PROGNAME " requires that in be a real vector.");

	/* samplerate */
	m = mxGetM(prhs[1]);
	n = mxGetN(prhs[1]); 
	if (!mxIsNumeric(prhs[1]) || mxIsComplex(prhs[1]) || 
	   mxIsSparse(prhs[1])  || !mxIsDouble(prhs[1]) ||
	   (m != 1) || (n != 1) ) {
		mexErrMsgTxt(PROGNAME " requires that samplerate be a real scalar.");
	}

   	/* channels */
	m = mxGetM(prhs[2]);
	n = mxGetN(prhs[2]); 
	if (!mxIsNumeric(prhs[2]) || mxIsComplex(prhs[2]) || 
	   mxIsSparse(prhs[2])  || !mxIsDouble(prhs[2]) ||
	   (m != 1) || (n != 1) ) {
		mexErrMsgTxt(PROGNAME " requires that channels be a real scalar.");
	}

	/* Assign pointers to the various parameters of the right hand side*/
	samplerate = (int)floor( 0.5 + *mxGetPr(prhs[1]) );
	if ( (samplerate!=12000) && (samplerate!=24000) )
		mexErrMsgTxt(PROGNAME " requires that samplerate is 12000Hz or 24000Hz");

	channels = (int)floor( 0.5 + *mxGetPr(prhs[2]) );
	if ( (channels<1) || (channels>5) )
		mexErrMsgTxt(PROGNAME " requires that channels is in [1..5]");


	if ( (dll_state==0) && (mxIsEmpty(prhs[0]))	) 
	{
		dims[0] = 0; dims[1] = 0; plhs[0] = mxCreateNumericArray(2, dims, mxINT16_CLASS, mxREAL);
		return;
	}

	if (dll_state==0)	/* at the first call, we have to do some initializing */
	{
		mexAtExit( MyExitFunction );

		hDecoder = NeAACDecOpen();
		if (hDecoder == NULL) 
		{
			mexErrMsgTxt(PROGNAME " failed to open faac-decoder");
		}
		else
		{
			
			/* Set the default object type and samplerate */
			/*
			config = NeAACDecGetCurrentConfiguration(hDecoder);
			*/
			/*memcpy( pconfig, &config_default, sizeof(NeAACDecConfiguration) ); */ 
			/*
			config->defObjectType = LC;
			config->defSampleRate = 0;
			config->outputFormat = FAAD_FMT_16BIT;
			config->downMatrix = 0;
			config->useOldADTSFormat = 0;
			config->dontUpSampleImplicitSBR = 1;
			NeAACDecSetConfiguration(hDecoder, config);
			*/

			lResult = NeAACDecInitDRM(&hDecoder, samplerate, channels);
			/*lResult = NeAACDecInit(hDecoder, aac_buffer.readptr, aac_buffer.level, &samplerate, &channels); */
/*mexPrintf("lResult: %i, samplerate: %i, channels: %i\n", lResult, samplerate, channels ); */
			if (lResult<0)
			{
				NeAACDecClose( hDecoder );
				mexErrMsgTxt("Error initializing FAAC decoder library");
			}
			else
			{
				dll_state = 1;
			}
		}
	}


	if ( mxIsEmpty(prhs[0]) )
	{
		NeAACDecClose( hDecoder );

		dims[0] = 0; dims[1] = 0; plhs[0] = mxCreateNumericArray(2, dims, mxINT16_CLASS, mxREAL);
		dll_state = 0;
		return;
	}

	if ( !mxIsUint8(prhs[0]) )
		mexErrMsgTxt("conversion double->bitstream not implemented yet");

	in_ptr = (uint8_t*)mxGetData(prhs[0]);


    pcm_output = (int16_t*)NeAACDecDecode(hDecoder, &frameInfo, in_ptr, N_in);
    
	if ((nlhs >= 2) || (frameInfo.error == 0))
    {
		if (frameInfo.samples>0)
		{
			/* Assign pointers to the various parameters of the left hand side*/
			/* Create a matrix for the return arguments */
			dims[1] = frameInfo.samples / frameInfo.channels; 
			dims[0] = frameInfo.channels;
			plhs[0] = mxCreateNumericArray(2, dims, mxINT16_CLASS, mxREAL);
			out_ptr = (int16_t*)mxGetData(plhs[0]);

			memcpy( out_ptr, pcm_output, frameInfo.samples*sizeof(int16_t) );
		}
		else 
		{
			dims[0] = 0; dims[1] = 0; plhs[0] = mxCreateNumericArray(2, dims, mxINT16_CLASS, mxREAL);
		}

	} 

	if (nlhs >= 2) {

		plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);

		*mxGetPr(plhs[1]) = (double) frameInfo.error;
			
	} else if (frameInfo.error) {

		dims[0] = 0; dims[1] = 0; plhs[0] = mxCreateNumericArray(2, dims, mxINT16_CLASS, mxREAL);	

	}




	dll_state=2;	/* 0=not initialized, 1=first run, 2=normal */

    /* print some channel info */
    /*print_channel_info(&frameInfo); */

	return;
}





void MyExitFunction( void )
{
	if (dll_state>0)
	NeAACDecClose( hDecoder );

	dll_state = 0;
}








/* MicroSoft channel definitions */
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

long aacChannelConfig2wavexChannelMask(NeAACDecFrameInfo *hInfo)
{
    if (hInfo->channels == 6 && hInfo->num_lfe_channels)
    {
        return SPEAKER_FRONT_LEFT + SPEAKER_FRONT_RIGHT +
            SPEAKER_FRONT_CENTER + SPEAKER_LOW_FREQUENCY +
            SPEAKER_BACK_LEFT + SPEAKER_BACK_RIGHT;
    } else {
        return 0;
    }
}

char *position2string(int position)
{
    switch (position)
    {
    case FRONT_CHANNEL_CENTER: return "Center front";
    case FRONT_CHANNEL_LEFT:   return "Left front";
    case FRONT_CHANNEL_RIGHT:  return "Right front";
    case SIDE_CHANNEL_LEFT:    return "Left side";
    case SIDE_CHANNEL_RIGHT:   return "Right side";
    case BACK_CHANNEL_LEFT:    return "Left back";
    case BACK_CHANNEL_RIGHT:   return "Right back";
    case BACK_CHANNEL_CENTER:  return "Center back";
    case LFE_CHANNEL:          return "LFE";
    case UNKNOWN_CHANNEL:      return "Unknown";
    default: return "";
    }

    return "";
}

void print_channel_info(NeAACDecFrameInfo *frameInfo)
{

    /* print some channel info */
    int i;
    long channelMask = aacChannelConfig2wavexChannelMask(frameInfo);

mexPrintf( "*** frameinfo ***\n");
mexPrintf( "bytesconsumed: %i\n",frameInfo->bytesconsumed );
mexPrintf( "samples: %i\n",frameInfo->samples );
mexPrintf( "channels: %i\n",frameInfo->channels );
mexPrintf( "error: %i\n",frameInfo->error );
mexPrintf( "samplerate: %i\n",frameInfo->samplerate );
mexPrintf( "sbr: %i\n",frameInfo->sbr );
mexPrintf( "object_type: %i\n",frameInfo->object_type );
mexPrintf( "header_type: %i\n",frameInfo->header_type );
mexPrintf( "\n\n");

    mexPrintf( "  ---------------------\n");
    if (frameInfo->num_lfe_channels > 0)
    {
        mexPrintf( " | Config: %2d.%d Ch     |", frameInfo->channels-frameInfo->num_lfe_channels, frameInfo->num_lfe_channels);
    } else {
        mexPrintf( " | Config: %2d Ch       |", frameInfo->channels);
    }
    if (channelMask)
        mexPrintf( " WARNING: channels are reordered according to\n");
    else
        mexPrintf( "\n");
    mexPrintf( "  ---------------------");
    if (channelMask)
        mexPrintf( "  MS defaults defined in WAVE_FORMAT_EXTENSIBLE\n");
    else
        mexPrintf( "\n");
    mexPrintf( " | Ch |    Position    |\n");
    mexPrintf( "  ---------------------\n");
    for (i = 0; i < frameInfo->channels; i++)
    {
        mexPrintf( " | %.2d | %-14s |\n", i, position2string((int)frameInfo->channel_position[i]));
    }
    mexPrintf( "  ---------------------\n");
    mexPrintf( "\n");
}
