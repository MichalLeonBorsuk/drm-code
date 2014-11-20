/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2005 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 10.03.2005                                                 */
/*  Last change  : 10.03.2005                                                 */
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
/*  hist2D_equidist.c                                                         */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  2D histogram, e.g. for complex values (.m too slow, histc N/A @ R10)      */
/*  Usage:                                                                    */
/*                                                                            */
/*  histogram = hist2D_equidist (values, range, no_of_bins);                  */
/*                                                                            */
/*  values: 2D values in an Nx2 matrix                                        */
/*  range: 2x2 matrix with minimum values in the first row and maximum        */
/*         values in the second row for each dimension:                       */
/*         [xmin, ymin; xmax, ymax]                                           */
/*  no_of_bins: 1x2 vector with number of bins for each dimension:            */
/*              [xbins, ybins]                                                */
/*                                                                            */
/******************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include "mex.h"


#define PROGNAME "hist2D_equidist"


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	int m, n, N, dims[2], xindex, yindex, xbins, ybins;
	double *values, *range, *no_of_bins, xmin, ymin, xrange, yrange, xfactor, yfactor, *hist2D;

	/* Check for proper number of arguments */
	if (nrhs != 3) {
		mexErrMsgTxt(PROGNAME " requires 3 input arguments.\n");
	}
  
   /* Check dimensions */
#define	ARG_INDEX 0
	N = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	
	if ((n != 2)) {
		mexErrMsgTxt(PROGNAME": \"values\" must be a real Nx2 matrix\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) ||
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME": \"values\" must be a real Nx2 matrix\n");
	}
	values = mxGetPr(prhs[ARG_INDEX]);

#undef ARG_INDEX
#define	ARG_INDEX 1
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	
	if ((m != 2) && (n != 2)) {
		mexErrMsgTxt(PROGNAME": \"range\" must be a real 2x2 matrix\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME": \"range\" must be a real 2x2 matrix\n");
	}
	range = mxGetPr(prhs[ARG_INDEX]);

#undef ARG_INDEX
#define	ARG_INDEX 2
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]); 	
	if ((m != 1) && (n != 2)) {
		mexErrMsgTxt(PROGNAME": \"no_of_bins\" must be a real 1x2 vector\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME": \"no_of_bins\" must be a real 1x2 vector\n");
	}	
	no_of_bins = mxGetPr(prhs[ARG_INDEX]);

	xbins = (int)no_of_bins[0];
	ybins = (int)no_of_bins[1];

#undef ARG_INDEX

	dims[0] = ybins;
	dims[1] = xbins;
	plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
	hist2D = mxGetPr(plhs[0]);


	xmin = range[0];
	xrange = range[1] - xmin;
	ymin = range[2];
	yrange = range[3] - ymin;

	xfactor = (double)xbins / xrange;
	yfactor = (double)ybins / yrange;

	for (m = 0; m < N; m++) {
		xindex = (int)((values[m] - xmin) * xfactor);
		/*if (xindex < 0) xindex = 0; */
		/*if (xindex >= xbins) xindex = xbins - 1; */
		if ((xindex >= xbins) || (xindex < 0)) continue;

		yindex = (int)((values[m + N] - ymin) * yfactor);
		/*if (yindex < 0) yindex = 0; */
		/*if (yindex >= ybins) yindex = ybins - 1; */
		if ((yindex >= ybins) || (yindex < 0)) continue;

		hist2D[yindex + ybins * xindex] += 1.0;

	}

	return;
}
