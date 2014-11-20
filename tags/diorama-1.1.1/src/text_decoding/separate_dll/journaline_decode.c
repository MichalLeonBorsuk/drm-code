/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2005 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 09.02.2005                                                 */
/*  Last change  : 18.02.2005                                                 */
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
/*  journaline_decode.c                                                       */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  DAB data group decoding and journaline news service decoding              */
/*                                                                            */
/*  Usage:                                                                    */
/*                                                                            */
/*  news_objects = journaline_decode(input, extended_header_length);          */
/*                                                                            */
/*  The decoder is fed with an uint8 vector "input" and the length of the     */
/*  extended header. It returns a cell array "news_objects" containing        */
/*  structures with the following elements:                                   */
/*                                                                            */
/*  filename: string with proposed filename                                   */
/*  update:   update state of this news object (+1: create, -1: delete)       */
/*  filebody: uint8 array with the file data to store                         */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*  This program is based on the 'NewsService Journaline(R) Decoder'          */
/*  Copyright (c) 2003, 2004 by Fraunhofer IIS, Erlangen, Germany             */
/*                                                                            */
/*                                                                            */
/*  To use the 'NewsService Journaline(R) Decoder' software for               */
/*  COMMERCIAL purposes, please contact Fraunhofer IIS for a                  */
/*  commercial license (see below for contact information)!                   */
/*                                                                            */
/* --------------------------------------------------------------------       */
/*                                                                            */
/*  Contact:                                                                  */
/*   Fraunhofer IIS, Department 'Broadcast Applications'                      */
/*   Am Wolfsmantel 33, 91058 Erlangen, Germany                               */
/*   http://www.iis.fraunhofer.de/dab                                         */
/*   mailto:bc-info@iis.fraunhofer.de                                         */
/*                                                                            */
/******************************************************************************/


#include <stdlib.h>
#include "mex.h"
#include "journaline_decode.h"

const char *mstruct_field_name0 = "filename";
const char *mstruct_field_name1 = "update";
const char *mstruct_field_name2 = "filebody";
const char *mstruct_field_names[] = {"filename", "update", "filebody"};

static int firstcall = 1;


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	unsigned char *dab_data_group;
	int N, m, n, i, objID, debug = 0, debug_mode, dims[2];
	mxArray *mstruct_ptr, *mfield_ptr;  
	unsigned long extended_header_len = 0; // no extended header
	struct Output_Files *output;
	int count;
	
	// Check for proper number of arguments
	
	if ((nrhs != 1) && (nrhs != 2)) {
		mexErrMsgTxt("Usage: " PROGNAME "(dab_data_group [, extended_header_length]);");
	} 
  
   // Check dimensions
    	
	// in
#define	ARG_INDEX 0
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	
	if ((m != 1) && (n != 1)) {
		mexErrMsgTxt(PROGNAME": \"dab_data_group\" must be a real uint8 vector.\n");
	}
	N = m*n;
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || (!mxIsDouble(prhs[ARG_INDEX]) && !mxIsUint8(prhs[ARG_INDEX])) ) {
		mexErrMsgTxt(PROGNAME " requires \"dab_data_group\" to be a real uint8 vector.\n");
	}
	
	if (mxIsDouble(prhs[ARG_INDEX])) {
		debug = 1;
		debug_mode = (int)mxGetScalar(prhs[ARG_INDEX]);
		dab_data_group = (unsigned char *) &debug_mode;
		N = -1;
	} else {	
		dab_data_group = (unsigned char *)mxGetData(prhs[ARG_INDEX]);
	}
#undef ARG_INDEX
	if (nrhs > 1) {
#define	ARG_INDEX 1
		m = mxGetM(prhs[ARG_INDEX]);
		n = mxGetN(prhs[ARG_INDEX]);	
		if ((m != 1) || (n != 1)) {
			mexErrMsgTxt(PROGNAME": \"extended_header_length\" must be a real double scalar.\n");
		}
		if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
			mxIsSparse(prhs[ARG_INDEX]) || !mxIsDouble(prhs[ARG_INDEX]) ) {
			mexErrMsgTxt(PROGNAME " requires \"extended_header_length\" to be a real double scalar.\n");
		}
		extended_header_len = (unsigned long) mxGetScalar(prhs[ARG_INDEX]);
#undef ARG_INDEX
	}


	if (firstcall) {
		firstcall = 0;

		mexAtExit(journaline_decode_exit);
	}


	journaline_decode(dab_data_group, N, extended_header_len, &output, &count);


	/* Debug: */
/*
	m = 1;
	journaline_decode(&m, -1, 0, &output, &count);
	m = 3;
	journaline_decode(&m, -1, 0, &output, &count);
*/


	dims[0] = 1; dims[1] = count;
	plhs[0] = mxCreateCellArray(2, dims);


	for (i = 0; i < count; i++) {

		mstruct_ptr = mxCreateStructMatrix(1,1,3,mstruct_field_names);

		mfield_ptr = mxCreateString(output[i].filename);
		mxSetField(mstruct_ptr, 0, mstruct_field_name0, mfield_ptr);

		mfield_ptr = mxCreateDoubleMatrix(1,1,mxREAL);
		*mxGetPr(mfield_ptr) = output[i].update;
		mxSetField(mstruct_ptr, 0, mstruct_field_name1, mfield_ptr);

		dims[1] = output[i].body_length;
		mfield_ptr = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
		memcpy(mxGetData(mfield_ptr), output[i].filebody, output[i].body_length * sizeof(char));
		mxSetField(mstruct_ptr, 0, mstruct_field_name2, mfield_ptr);

		mxSetCell(plhs[0], i, mstruct_ptr);

	}


	free_output (output, count);

	
	return;
}

