/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2005 Andreas Dittrich, Torsten Schorr                       */
/*                                                                            */
/*  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de),                 */
/*                 Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 19.04.2004                                                 */
/*  Last change  : 25.01.2005                                                 */
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
/*  sleep_c.c                                                                 */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  Put the process to sleep for a given time in milli-seconds                */
/*  Usage:                                                                    */
/*                                                                            */
/*  sleep_c(time_in_ms);                                                      */
/*                                                                            */
/*                                                                            */
/******************************************************************************/


#ifndef _LINUX_
#include <windows.h>
#else
#include <time.h>
#endif

#include <stdio.h>

#include <math.h>
#include <stdlib.h>
#include "mex.h"

#define Nargs_rhs_str "1"
#define Nargs_rhs 1
#define Nargs_lhs_str "0"
#define Nargs_lhs 0
#define PROGNAME "sleep_c"



/******************************************************************************/
/*  Gateway-Routine                                                           */
/******************************************************************************/

void mexFunction(
                  int nlhs,       mxArray *plhs[],
                  int nrhs, const mxArray *prhs[]
           		 )
{
	double *in;
	unsigned long m,n;
#ifdef _LINUX_
	struct timespec sleep_time;
	struct timespec remaining;
#endif
  
	/* Check for proper number of arguments */
	if (nrhs != Nargs_rhs) {
		mexErrMsgTxt(PROGNAME " requires " Nargs_rhs_str " input arguments.");
	} else if (nlhs != Nargs_lhs)
		mexErrMsgTxt(PROGNAME " requires " Nargs_lhs_str " output argument.");
  
   /* Check dimensions */
	/* in */
	m = mxGetM(prhs[0]);
	n = mxGetN(prhs[0]); 
	if (!mxIsNumeric(prhs[0]) || mxIsComplex(prhs[0]) || 
		mxIsSparse(prhs[0])  || !mxIsDouble(prhs[0]) || (m!=1) || (n!=1) )
		mexErrMsgTxt(PROGNAME " requires that in be a real value.");

	/* Assign pointers to the various parameters of the right hand side*/
	in = mxGetPr(prhs[0]);
#ifndef _LINUX_
	Sleep( (int)(floor( *in + 0.5 )) );
#else
	sleep_time.tv_sec = ((int)floor( *in + 0.5 ))/1000;
	sleep_time.tv_nsec = ((int)floor( *in + 0.5 ) - 1000 * sleep_time.tv_sec) * 1000000;
	nanosleep (&sleep_time, &remaining);
#endif

	return;
}


