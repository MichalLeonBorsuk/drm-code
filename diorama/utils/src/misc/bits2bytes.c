/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 23.07.2004                                                 */
/*  Last change  : 23.07.2004                                                 */
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
/*  bits2bytes.c                                                              */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  Deinterleaver/Interleaver generation for DRM frames                       */
/*  Usage:                                                                    */
/*                                                                            */
/*  bytes = bits2bytes(bits);                                                 */
/*                                                                            */
/*  converts a serial double bit-stream into a uint8 byte-stream              */
/*                                                                            */
/******************************************************************************/

#include <stdio.h>
#include <math.h>
#include "mex.h"

#define NARGS_RHS_STR "1"
#define NARGS_RHS 1 
#define PROGNAME "bits2bytes"


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	double *inbits;
	unsigned char *outbytes, single_byte;
	int dims[2];
	int m,n,N;


	/* Check for proper number of arguments */
	if (nrhs != NARGS_RHS) {
		mexErrMsgTxt(PROGNAME " requires " NARGS_RHS_STR " input argument(s).\n");
	}
  
   /* Check dimensions */
#define	ARG_INDEX 0
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	
	if ((m != 1) && (n != 1)) {
		mexErrMsgTxt(PROGNAME ": \"inbits\" must be a scalar double vector\n");
	}
	N = m*n;

	if (N%8 != 0) {
		mexErrMsgTxt(PROGNAME ": The length of \"inbits\" must be a multiple of 8\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME ": \"inbits\" must be a scalar double vector\n");
	}	inbits = mxGetPr(prhs[ARG_INDEX]);


#undef ARG_INDEX

	dims[0] = 1;
	dims[1] = N/8;
	plhs[0] = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
	outbytes = mxGetData(plhs[0]);

	for (m = 0;m < N/8; m++) {
		single_byte = 0;
		for (n = 7; n >= 0;n--) {
			single_byte |= ((inbits[8*m + 7 - n] != 0) & 0x01) << n;
		}
		outbytes[m] = single_byte;
	}

	return;
}

