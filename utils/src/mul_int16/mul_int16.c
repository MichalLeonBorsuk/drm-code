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
/*  mul_int16.c                                                               */
/*                                                                            */
/******************************************************************************/
/* y = mul_int16( x, A_int16 )                                                */
/*                                                                            */
/*    is identical to                                                         */
/*                                                                            */
/* y = x*double(A_int16)                                                      */
/******************************************************************************/


#include <float.h>
#include "mex.h"


#define NARGS_RHS_STR "2"
#define NARGS_LHS_STR "1"
#define PROGNAME "mul_int16_c"


#ifndef _LINUX_
typedef char int8_t;
#endif
typedef unsigned char uint8_t;
#ifndef _LINUX_
typedef short int16_t;
#endif
typedef unsigned int uint32_t;



void multiply_real( double *y, double *x, int16_t *A_int16, int rows, int cols );
void multiply_complex( double *yr, double *yi, double *xr, double *xi, int16_t *A_int16, int rows, int cols );


/******************************************************************************/
/*  Gateway-Routine                                                           */
/******************************************************************************/

void mexFunction(
                  int nlhs,       mxArray *plhs[],
                  int nrhs, const mxArray *prhs[]
           		 )
{
	int nargs_rhs = atoi( NARGS_RHS_STR );
	int nargs_lhs = atoi( NARGS_LHS_STR );

	mxArray const *parg;
	unsigned long m,n;
	long N;

	int16_t *A_int16;
	double *x_real, *x_imag, *y_real, *y_imag;
	int rows, cols;

	/* Check for proper number of arguments */
	if (nrhs != nargs_rhs) {
		mexErrMsgTxt(PROGNAME " requires " NARGS_RHS_STR " input arguments.");
	} else if ( (nlhs != nargs_lhs) && !((nlhs==0)&&(nargs_lhs==1)) )
		mexErrMsgTxt(PROGNAME " requires " NARGS_LHS_STR " output argument(s).");
   
	/* Check dimensions */
	/* A_int16 */
	parg = prhs[1];
	m = mxGetM(parg);
	n = mxGetN(parg); 
	N = m*n;
	if ( !mxIsNumeric(parg) || 
		 mxIsSparse(parg)  || !mxIsInt16(parg) || (N<1) )
  		mexErrMsgTxt(PROGNAME " requires that A_int16 is a real matrix.");
	rows = m;
	cols = n;

	/* x_double */
	parg = prhs[0];
	m = mxGetM(parg);
	n = mxGetN(parg); 
	N = m*n;
	if ( !mxIsNumeric(parg) ||  
		 mxIsSparse(parg)  || !mxIsDouble(parg) || ((m!=1) && (n!=1)) || (N<1) )
		mexErrMsgTxt(PROGNAME " requires that x is a vector.");
	if (N!=rows)
		mexErrMsgTxt(PROGNAME " wrong dimension of x");


	/* assign pointer */
	A_int16 = (int16_t*)mxGetData(prhs[1]);


	if ( mxIsComplex(prhs[0]) )
	{
		x_real = (double*)mxGetPr(prhs[0]);
		x_imag = (double*)mxGetPi(prhs[0]);

		/* Create a matrix for the return arguments */
		plhs[0] = mxCreateDoubleMatrix(cols, 1, mxCOMPLEX);
		y_real = (double*)mxGetPr(plhs[0]);
		y_imag = (double*)mxGetPi(plhs[0]);

		multiply_complex( y_real, y_imag, x_real, x_imag, A_int16, rows, cols );
	}
	else
	{
		x_real = (double*)mxGetPr(prhs[0]);

		/* Create a matrix for the return arguments */
		plhs[0] = mxCreateDoubleMatrix(cols, 1, mxREAL);
		y_real = (double*)mxGetPr(plhs[0]);

		multiply_real( y_real, x_real, A_int16, rows, cols );
	}

	return;
}


/******************************************************************************/
/* function                                                                   */
/******************************************************************************/


void multiply_real( double *y, double *x, int16_t *A_int16, int rows, int cols )
{
	int i,j;
	double u;
	double scale = 1.0/(1<<15);

	for (i=0; i<cols; i++)
	{
		u = 0;
		for (j=0; j<rows; j++)
		{
			u = u + x[j]*(double)(A_int16[j]);
		}
		y[i] = u*scale;
		A_int16 += rows;
	}
}


void multiply_complex( double *yr, double *yi, double *xr, double *xi, int16_t *A_int16, int rows, int cols )
{
	int i,j;
	double ur, ui, temp;
	double scale = 1.0/(1<<15);

	for (i=0; i<cols; i++)
	{
		ur = 0;
		ui = 0;
		for (j=0; j<rows; j++)
		{
			temp = (double)(A_int16[j]);
			ur = ur + xr[j+0]*temp;
			ui = ui + xi[j+0]*temp;
		}
		yr[i] = ur*scale;
		yi[i] = ui*scale;
		A_int16 += rows;
	}
}


void multiply_complex_( double *yr, double *yi, double *xr, double *xi, int16_t *A_int16, int rows, int cols )
{
	int i,j;
	double ur, ui, temp;
	int rows1 = (rows/4)*4;

	for (i=0; i<cols; i++)
	{
		ur = 0;
		ui = 0;
		for (j=0; j<rows; j+=4)
		{
			temp = (double)(A_int16[j+0]);
			ur = ur + xr[j+0]*temp;
			ui = ui + xi[j+0]*temp;

			temp = (double)(A_int16[j+1]);
			ur = ur + xr[j+1]*temp;
			ui = ui + xi[j+1]*temp;

			temp = (double)(A_int16[j+2]);
			ur = ur + xr[j+2]*temp;
			ui = ui + xi[j+2]*temp;

			temp = (double)(A_int16[j+3]);
			ur = ur + xr[j+3]*temp;
			ui = ui + xi[j+3]*temp;
		}

		for (j=rows1; j<rows; j++)
		{
			temp = (double)(A_int16[j+0]);
			ur = ur + xr[j+0]*temp;
			ui = ui + xi[j+0]*temp;
		}

		yr[i] = ur;
		yi[i] = ui;

		A_int16 += rows;
	}

}
