/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2005 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 01.11.2004                                                 */
/*  Last change  : 07.02.2005                                                 */
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
/*  src_filter.c                                                              */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  Sound recording and playback for the Open Sound System (OSS)              */
/*  Usage:                                                                    */
/*                                                                            */
/******************************************************************************/

#define MAX_SRC_1x 1.2			/* maximum SRC factor for pi-filter */
#define MAX_SRC_2x 2.5			/* maximum SRC factor for pi/2-filter */

#include <stdlib.h>
#include "src_filter_table.h"
#include "src_filter.h"


int sample_rate_conv_int16_mono_mono (int16_t *samples_ptr, int N, int16_t *buffer_ptr,  
				      int buffer_size, double *ty, double r) {

int index, phase, a, ci, in_index, offset_tail, offset_head, out_index = 0;
double Ty;
int16_t *filter_ptr = &src_filter_1x[0][0];
int os, rint = 1;

	if ((r > MAX_SRC_1x) || (1/r > MAX_SRC_1x)) { filter_ptr = &src_filter_2x[0][0]; rint = 2; }
	if ((r > MAX_SRC_2x) || (1/r > MAX_SRC_2x)) { filter_ptr = &src_filter_4x[0][0]; rint = 4; }
	os = SRC_OS * rint;
	Ty = (double)os * r;
	*ty *= (double)SRC_OS;
	

	while ( ((index = (int) *ty) < (N - SUBFILTER_LENGTH) * os) && (out_index < buffer_size) ) {
		/* sample proceeding desired time: */
		in_index = index / os;
		phase = index % os;
		a = 0;
		offset_tail = (os - 1 - phase) * FILTER_LENGTH_HALF;
		offset_head = (phase + 1) * FILTER_LENGTH_HALF - 1;
		for (ci = 0; ci < FILTER_LENGTH_HALF; ci++) {
			a += (samples_ptr[in_index + ci] * filter_ptr[offset_tail + ci]) >> 7;
			a += (samples_ptr[in_index + ci + FILTER_LENGTH_HALF] * filter_ptr[offset_head - ci]) >> 7;
		}
#ifdef SECOND_ORDER
		{
			double It;
			int b;
			if ( (It = *ty - (double)index) != 0.0 ) {
				/* sample succeeding desired time */
				in_index = (index+1) / os;
				phase = (index+1) % os;
				b = -a;
				offset_tail = (os - 1 - phase) * FILTER_LENGTH_HALF;
				offset_head = (phase + 1) * FILTER_LENGTH_HALF - 1;
				for (ci = 0; ci < FILTER_LENGTH_HALF; ci++) {
					b += (samples_ptr[in_index + ci] * filter_ptr[offset_tail + ci]) >> 7;
					b += (samples_ptr[in_index + ci + FILTER_LENGTH_HALF] * filter_ptr[offset_head - ci]) >> 7;
				}
				a += (int) (It * (double)b);
			}
		}
#endif
		a = ((a + 0x80) >> 8);
		if (a > 32767) a = 32767;
		else if (a < -32768) a = -32768;
		buffer_ptr[out_index] = (int16_t)a;
		out_index++; *ty += Ty;
	}
	*ty = (*ty - (double)((N - SUBFILTER_LENGTH) * os)) / ((double)SRC_OS);
	return (out_index);
} 

int sample_rate_conv_int16_stereo_stereo (int16_t *samples_ptr, int N, int16_t *buffer_ptr,  
					  int buffer_size, double *ty, double r) {

int index, phase, al, ar, ci, in_index, offset_tail, offset_head, out_index = 0;
double Ty;
int16_t *filter_ptr = &src_filter_1x[0][0];
int os, rint = 1;

	if ((r > MAX_SRC_1x) || (1/r > MAX_SRC_1x)) { filter_ptr = &src_filter_2x[0][0]; rint = 2; }
	if ((r > MAX_SRC_2x) || (1/r > MAX_SRC_2x)) { filter_ptr = &src_filter_4x[0][0]; rint = 4; }
	os = SRC_OS * rint;
	Ty = (double)os * r;
	*ty *= (double)SRC_OS; 
	
	while ( ((index = (int) *ty) < (N - SUBFILTER_LENGTH) * os) && (out_index < buffer_size) ) {
		/* sample proceeding desired time: */
		in_index = index / os;
		phase = index % os;
		al = 0; ar = 0; 
		offset_tail = (os - 1 - phase) * FILTER_LENGTH_HALF;
		offset_head = (phase + 1) * FILTER_LENGTH_HALF - 1;
		for (ci = 0; ci < FILTER_LENGTH_HALF; ci++) {
			al += (samples_ptr[2 * (in_index + ci)] * filter_ptr[offset_tail + ci]) >> 7;
			ar += (samples_ptr[2 * (in_index + ci) + 1] * filter_ptr[offset_tail + ci]) >> 7;
			al += (samples_ptr[2 * (in_index + ci + FILTER_LENGTH_HALF)] * filter_ptr[offset_head - ci]) >> 7;
			ar += (samples_ptr[2 * (in_index + ci + FILTER_LENGTH_HALF) + 1] * filter_ptr[offset_head - ci]) >> 7;
			
		}
#ifdef SECOND_ORDER
		{
			double It;
			int bl, br;
			if ( (It = *ty - (double)index) != 0.0 ) {
				/* sample succeeding desired time */
				in_index = (index+1) / os;
				phase = (index+1) % os;
				bl = -al; br = -ar;
				offset_tail = (os - 1 - phase) * FILTER_LENGTH_HALF;
				offset_head = (phase + 1) * FILTER_LENGTH_HALF - 1;
				for (ci = 0; ci < FILTER_LENGTH_HALF; ci++) {
					bl += (samples_ptr[2 * (in_index + ci)] * filter_ptr[offset_tail + ci]) >> 7;
					br += (samples_ptr[2 * (in_index + ci) + 1] * filter_ptr[offset_tail + ci]) >> 7;
					bl += (samples_ptr[2 * (in_index + ci + FILTER_LENGTH_HALF)] * filter_ptr[offset_head - ci]) >> 7;
					br += (samples_ptr[2 * (in_index + ci + FILTER_LENGTH_HALF) + 1] * filter_ptr[offset_head - ci]) >> 7;
				}
				al += (int) (It * (double)bl);
				ar += (int) (It * (double)br);
			}
		}
#endif
		al = ((al + 0x80) >> 8);
		ar = ((ar + 0x80) >> 8);
		if (al > 32767) al = 32767;
		else if (al < -32768) al = -32768;
		if (ar > 32767) ar = 32767;
		else if (ar < -32768) ar = -32768;
		buffer_ptr[2 * out_index] = (int16_t)al;
		buffer_ptr[2 * out_index + 1] = (int16_t)ar;
		out_index++; *ty += Ty;
	}
	*ty = (*ty - (double)((N - SUBFILTER_LENGTH) * os)) / ((double)SRC_OS);
	return (out_index);
} 

int sample_rate_conv_int16_mono_stereo (int16_t *samples_ptr, int N, int16_t *buffer_ptr,  
					int buffer_size, double *ty, double r) {

int index, phase, a, ci, in_index, offset_tail, offset_head, out_index = 0;
double Ty;
int16_t *filter_ptr = &src_filter_1x[0][0];
int os, rint = 1;

	if ((r > MAX_SRC_1x) || (1/r > MAX_SRC_1x)) { filter_ptr = &src_filter_2x[0][0]; rint = 2; }
	if ((r > MAX_SRC_2x) || (1/r > MAX_SRC_2x)) { filter_ptr = &src_filter_4x[0][0]; rint = 4; }
	os = SRC_OS * rint;
	Ty = (double)os * r;
	*ty *= (double)SRC_OS; 
	
	while ( ((index = (int) *ty) < (N - SUBFILTER_LENGTH) * os) && (out_index < buffer_size) ) {
		/* sample proceeding desired time: */
		in_index = index / os;
		phase = index % os;
		a = 0;
		offset_tail = (os - 1 - phase) * FILTER_LENGTH_HALF;
		offset_head = (phase + 1) * FILTER_LENGTH_HALF - 1;
		for (ci = 0; ci < FILTER_LENGTH_HALF; ci++) {
			a += (samples_ptr[in_index + ci] * filter_ptr[offset_tail + ci]) >> 7;
			a += (samples_ptr[in_index + ci + FILTER_LENGTH_HALF] * filter_ptr[offset_head - ci]) >> 7;
		}
#ifdef SECOND_ORDER
		{
			double It;
			int b;
			if ( (It = *ty - (double)index) != 0.0 ) {
				/* sample succeeding desired time */
				in_index = (index+1) / os;
				phase = (index+1) % os;
				b = -a;
				offset_tail = (os - 1 - phase) * FILTER_LENGTH_HALF;
				offset_head = (phase + 1) * FILTER_LENGTH_HALF - 1;
				for (ci = 0; ci < FILTER_LENGTH_HALF; ci++) {
					b += (samples_ptr[in_index + ci] * filter_ptr[offset_tail + ci]) >> 7;
					b += (samples_ptr[in_index + ci + FILTER_LENGTH_HALF] * filter_ptr[offset_head - ci]) >> 7;
				}
				a += (int) (It * (double)b);
			}
		}
#endif
		a = ((a + 0x80) >> 8);
		if (a > 32767) a = 32767;
		else if (a < -32768) a = -32768;
		buffer_ptr[2 * out_index] = (int16_t)a;
		buffer_ptr[2 * out_index + 1] = (int16_t)a;
		out_index++; *ty += Ty;
	}
	*ty = (*ty - (double)((N - SUBFILTER_LENGTH) * os)) / ((double)SRC_OS);
	return (out_index);
} 

int sample_rate_conv_int16_stereo_mono (int16_t *samples_ptr, int N, int16_t *buffer_ptr,  
					int buffer_size, double *ty, double r) {

int index, phase, a, ci, in_index, offset_tail, offset_head, out_index = 0;
double Ty;
int16_t *filter_ptr = &src_filter_1x[0][0];
int os, rint = 1;

	if ((r > MAX_SRC_1x) || (1/r > MAX_SRC_1x)) { filter_ptr = &src_filter_2x[0][0]; rint = 2; }
	if ((r > MAX_SRC_2x) || (1/r > MAX_SRC_2x)) { filter_ptr = &src_filter_4x[0][0]; rint = 4; }
	os = SRC_OS * rint;
	Ty = (double)os * r;
	*ty *= (double)SRC_OS; 
	
	while ( ((index = (int) *ty) < (N - SUBFILTER_LENGTH) * os) && (out_index < buffer_size) ) {
		/* sample proceeding desired time: */
		in_index = index / os;
		phase = index % os;
		a = 0;
		offset_tail = (os - 1 - phase) * FILTER_LENGTH_HALF;
		offset_head = (phase + 1) * FILTER_LENGTH_HALF - 1;
		for (ci = 0; ci < FILTER_LENGTH_HALF; ci++) {
			a += (((samples_ptr[2 * (in_index + ci)]>>1) + (samples_ptr[2 * (in_index + ci) + 1]>>1)) * filter_ptr[offset_tail + ci]) >> 7;
			a += (((samples_ptr[2 * (in_index + ci + FILTER_LENGTH_HALF)]>>1) + (samples_ptr[2 * (in_index + ci + FILTER_LENGTH_HALF) + 1]>>1)) * filter_ptr[offset_head - ci]) >> 7;
		}
#ifdef SECOND_ORDER
		{
			double It;
			int b;
			if ( (It = *ty - (double)index) != 0.0 ) {
				/* sample succeeding desired time */
				in_index = (index+1) / os;
				phase = (index+1) % os;
				b = -a;
				offset_tail = (os - 1 - phase) * FILTER_LENGTH_HALF;
				offset_head = (phase + 1) * FILTER_LENGTH_HALF - 1;
				for (ci = 0; ci < FILTER_LENGTH_HALF; ci++) {
					b += (((samples_ptr[2 * (in_index + ci)]>>1) + (samples_ptr[2 * (in_index + ci) + 1]>>1)) * filter_ptr[offset_tail + ci]) >> 7;
					b += (((samples_ptr[2 * (in_index + ci + FILTER_LENGTH_HALF)]>>1) + (samples_ptr[2 * (in_index + ci + FILTER_LENGTH_HALF) + 1]>>1)) * filter_ptr[offset_head - ci]) >> 7;
				}
				a += (int) (It * (double)b);
			}
		}
#endif
		a = ((a + 0x80) >> 8);
		if (a > 32767) a = 32767;
		else if (a < -32768) a = -32768;
		buffer_ptr[out_index] = (int16_t)a;
		out_index++; *ty += Ty;
	}
	*ty = (*ty - (double)((N - SUBFILTER_LENGTH) * os)) / ((double)SRC_OS);
	return (out_index);
} 
