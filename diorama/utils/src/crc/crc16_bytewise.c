/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Andreas Dittrich, Torsten Schorr                       */
/*                                                                            */
/*  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de),                 */
/*                 Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 27.07.2004                                                 */
/*  Last change  : 27.07.2004                                                 */
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
/*  crc16_bytewise.c                                                          */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  CRC-16 checksum calculation of a byte stream                              */
/*  Usage:                                                                    */
/*                                                                            */
/*  checksum = crc16_bytewise(bytes);                                         */
/*                                                                            */
/*  calculates double checksum of uint8 bytes                                 */
/*                                                                            */
/******************************************************************************/

#include <math.h>
#include <stdlib.h>
#include "mex.h"

#define Nargs_rhs_str "1"
#define Nargs_rhs 1
#define Nargs_lhs_str "1"
#define Nargs_lhs 1
#define PROGNAME "crc16_bytewise"


void crc16_c( double out[],
			    unsigned char in[], long N );



/******************************************************************************/
/*  Gateway-Routine                                                           */
/******************************************************************************/

void mexFunction(
                  int nlhs,       mxArray *plhs[],
                  int nrhs, const mxArray *prhs[]
           		 )
{
	double *checksum;
	unsigned char *in;

	long N;
	unsigned long m,n;
  
	/* Check for proper number of arguments */
	if (nrhs != Nargs_rhs) {
		mexErrMsgTxt(PROGNAME " requires " Nargs_rhs_str " input arguments.");
	} 
  
   /* Check dimensions */
    	
	/* in */
	m = mxGetM(prhs[0]);
	n = mxGetN(prhs[0]); 
	N = m*n;
	if (!mxIsNumeric(prhs[0]) || mxIsComplex(prhs[0]) || 
		mxIsSparse(prhs[0])  || !mxIsUint8(prhs[0]) || ((m!=1)&&(n!=1)) )
		mexErrMsgTxt(PROGNAME " requires that in be a real vector.");


	/* Create a matrix for the return arguments */
	plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);

	/* Assign pointers to the various parameters of the left hand side*/
	checksum = mxGetPr(plhs[0]);

	/* Assign pointers to the various parameters of the right hand side*/
	in = mxGetData(prhs[0]);

	/* Do the actual computations in a subroutine */
	crc16_c( checksum,in, N );

	return;
}



/******************************************************************************/
/* function                                                                   */
/******************************************************************************/

void crc16_c( double checksum[],
			    unsigned char in[], long N )
{
	int i, j;

	unsigned int b = 0xFFFF;
	unsigned int x = 0x1021;	/* (1) 0001000000100001 */
	unsigned int y;

	for (i=0; i<N-2; i++)
	{
		for (j = 7; j >= 0; j--)	{

			y = ((b>>15) + (in[i]>>j)&0x01)&0x01;

			if (y==1)
				b = ( (b<<1) ^ x );
			else
				b = ( b<<1 );
		}
	}
	for (i=N-2; i<N; i++)
	{
		for (j = 7; j >= 0; j--)	{

			y = ((b>>15) + ((in[i]>>j)&0x01)^0x01)&0x01;

			if (y==1)
				b = ( (b<<1) ^ x );
			else
				b = ( b<<1 );
		}
	}
	*checksum = (double)(b&0xFFFF);
}




