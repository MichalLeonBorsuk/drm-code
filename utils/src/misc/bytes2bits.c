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
/*  bytes2bits.c                                                              */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  Deinterleaver/Interleaver generation for DRM frames                       */
/*  Usage:                                                                    */
/*                                                                            */
/*  bits = bytes2bits(bytes);                                                 */
/*                                                                            */
/*  converts a uint8 byte-stream into a serial double bit-stream              */
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
	double *outbits;
	unsigned char *inbytes, single_byte;
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
		mexErrMsgTxt(PROGNAME ": \"inbytes\" must be a scalar uint8 vector\n");
	}
	N = m*n;

	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsUint8(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME ": \"inbytes\" must be a scalar uint8 vector\n");
	}	
	
	inbytes = (unsigned char *)mxGetData(prhs[ARG_INDEX]);


#undef ARG_INDEX

	dims[0] = 1;
	dims[1] = N * 8;
	plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
	outbits = (double *)mxGetData(plhs[0]);

	for (m = 0;m < N; m++) {
		single_byte = inbytes[m];
		for (n = 7; n >= 0 ;n--) {
			outbits[8*m+7-n] = (double)((single_byte >> n) & 0x01);
		}
	}

	return;
}

