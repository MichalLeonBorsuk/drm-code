/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Andreas Dittrich, Torsten Schorr                       */
/*                                                                            */
/*  Author(s)    : Andreas Dittrich (dittrich@eit.uni-kl.de),                 */
/*                 Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 10.02.2005                                                 */
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
/*  clockex.c                                                                 */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  clockex returns a vector containing the time:                             */
/*     [year month day hour minute seconds]                                   */
/*                                                                            */
/******************************************************************************/

#include "mex.h"
#include <time.h>
#ifndef _LINUX_
#include "windows.h"
#endif

#define PROGNAME "clockex"


#ifndef _LINUX_
int firstcall = 1;
LARGE_INTEGER counts_per_second;
LARGE_INTEGER performance_counts_start;
long int time_start;
#endif


/******************************************************************************/
/*  Gateway-Routine                                                           */
/******************************************************************************/

void mexFunction(
                  int nlhs,       mxArray *plhs[],
                  int nrhs, const mxArray *prhs[]
           		 )
{

#ifndef _LINUX_
	LARGE_INTEGER performance_counts;
	__int64 performance_counts_elapsed;
	__int64 seconds_elapsed;
#else
	struct timeval actual_time_high_resolution;
#endif

	long int time_actual;
	
	double seconds_elapsed_fraction;
	double *clockVector;
	struct tm *ptmTimeVector;


	/* Check for proper number of arguments */
	if ( (nlhs != 0) && (nlhs != 1) )
		mexErrMsgTxt(PROGNAME " requires 1 output argument.");

#ifndef _LINUX_
	if (firstcall==1)
	{
		QueryPerformanceFrequency( &counts_per_second );
		QueryPerformanceCounter( &performance_counts_start );
		time( &time_start );
		firstcall=0;
	}

	/* get high precision elapsed time in "counts" */
	QueryPerformanceCounter( &performance_counts );
	performance_counts_elapsed = performance_counts.QuadPart - performance_counts_start.QuadPart;

	seconds_elapsed = performance_counts_elapsed / counts_per_second.QuadPart;
	seconds_elapsed_fraction = (double)(performance_counts_elapsed - seconds_elapsed*counts_per_second.QuadPart) / (double)counts_per_second.QuadPart;

	time_actual = time_start + (int)seconds_elapsed;
#else

	gettimeofday(&actual_time_high_resolution, NULL);
	time_actual = actual_time_high_resolution.tv_sec;
	seconds_elapsed_fraction = (double)actual_time_high_resolution.tv_usec / 1000000.0;

#endif

	ptmTimeVector = localtime( &time_actual );

	/* Create a matrix for the return arguments */
	plhs[0] = mxCreateDoubleMatrix(1, 6, mxREAL);
	clockVector = mxGetPr(plhs[0]);

	clockVector[0] = 1900 + ptmTimeVector->tm_year;
	clockVector[1] = 1 + ptmTimeVector->tm_mon;
	clockVector[2] = ptmTimeVector->tm_mday;
	clockVector[3] = ptmTimeVector->tm_hour;
	clockVector[4] = ptmTimeVector->tm_min;
	clockVector[5] = (double)(ptmTimeVector->tm_sec) + seconds_elapsed_fraction;

	return;
}

