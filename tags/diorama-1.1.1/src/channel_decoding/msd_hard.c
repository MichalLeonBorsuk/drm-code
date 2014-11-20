/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 15.06.2004                                                 */
/*  Last change  : 22.07.2004                                                 */
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
/*  msd_hard.c                                                                */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  Multi-Stage-Decoder for DRM QAM signals (iterations with hard decisions)  */
/*  Usage:                                                                    */
/*                                                                            */
/*  [LPhardout, HPhardout, VSPPhardout] =                                     */
/*                      msd_hard (received, H, N1, L, Lvspp,                  */
/*                      Deinterleaver,PL, maxiter, SDCorMSC);                 */
/*                                                                            */
/*  received: samples of an FAC, SDC or MSC frame                             */
/*  H: estimated channel transfer function                                    */
/*  N1: number of OFDM cells in the higher protected part (part A)            */
/*  L: number of information bits for Part A/B for each level in (2xl)-matrix */
/*  Lvspp: number of information bits in the very strongly protected part     */
/*  Deinterleaver: deinterleavers for each levels in (Nxl)-int32-matrix       */
/*  PL: Protection Levels for Part A/B for each level in (2xl)-matrix         */
/*  maxiter: maximum number of decoding iterations                            */
/*  SDCorMSC: 1 for SDC and MSC frames, 0 for FAC frames                      */
/*                                                                            */
/******************************************************************************/




#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include "viterbi_decode.h"
#include "mex.h"
#include "msd_hard.h"

#define ITER_BREAK
#define CONSIDERING_SNR

#ifdef CONSIDERING_SNR

#define ARG_INDEX_OFFSET 1
#define NARGS_RHS_STR "9"
#define NARGS_RHS 9 

#else

#define ARG_INDEX_OFFSET 0
#define NARGS_RHS_STR "8"
#define NARGS_RHS 8 

#endif

#define PROGNAME "msd_hard"

#define PRBS_INIT(reg) reg = 511;
#define PRBS_BIT(reg) ((reg ^ (reg >> 4)) & 0x1)
#define PRBS_SHIFT(reg) reg = (((reg ^ (reg >> 4)) & 0x1) << 8) | (reg >> 1)





void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	double *received_real, *received_imag, *received, *first_received, *L1, *L2, *L1_real, *L2_real, *L1_imag, *L2_imag;
	double *PL1, *PL2, *PL1_real, *PL2_real, *PL1_imag, *PL2_imag, *output_ptr, L_dummy[3] = {0, 0, 0};
	int *Deinterleaver;
	float *metric_real, *metric_imag, *metric, *first_metric, closest_one, closest_zero, sample, *llr, dist;
	double variance, weightsum;
	char *memory_ptr, *viterbi_mem, *msd_mem, *hardpoints, *hardpoints_ptr, *lastiter, *infoout[3];
	int m, n, N, N1, no_of_levels, Lvspp, iteration, maxiter, diff, SDCorMSC, memory_size;
	int sample_index, rp_real[3], rp_imag[3], *rp, level, subset_point, no_of_bits, error, msd_mem_size, viterbi_mem_size;
	int PRBS_reg;
	int HMmix = 0, HMsym = 0;


#ifdef CONSIDERING_SNR
	double *signal_to_noise_ratio;
	float SNR;
#endif

	/* Check for proper number of arguments */
	if (nrhs != NARGS_RHS) {
		mexErrMsgTxt(PROGNAME " requires " NARGS_RHS_STR " input arguments.\n");
	}
  
   /* Check dimensions */
#define	ARG_INDEX 0
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	
	if ((m != 1) && (n != 1)) {
		mexErrMsgTxt(PROGNAME": \"received\" must be a complex vector\n");
	}
	N = m*n;
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"received\" to be a complex vector.\n");
	}
	received_real = mxGetPr(prhs[ARG_INDEX]);
	if (mxIsComplex(prhs[ARG_INDEX]))
		received_imag = mxGetPi(prhs[ARG_INDEX]);
	else
		received_imag = NULL;

#undef ARG_INDEX

#ifdef CONSIDERING_SNR
#define	ARG_INDEX 1
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);	
	if ((m != 1) && (n != 1)) {
		mexErrMsgTxt(PROGNAME": \"signal_to_noise_ratio\" must be a real vector\n");
	}
	if (m * n != N) {
		mexErrMsgTxt(PROGNAME": \"signal_to_noise_ratio\" must be as long as the input vector\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"signal_to_noise_ratio\" to be a real vector.\n");
	}
	signal_to_noise_ratio = mxGetPr(prhs[ARG_INDEX]);
#undef ARG_INDEX
#endif

#define	ARG_INDEX 1+ARG_INDEX_OFFSET
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]); 	
	if (m * n != 1) {
		mexErrMsgTxt(PROGNAME": \"N1\" has to be scalar!\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"N1\" to be a real scalar.\n");
	}	
	N1 = (int) mxGetScalar(prhs[ARG_INDEX]);
#undef ARG_INDEX

#define	ARG_INDEX 2+ARG_INDEX_OFFSET
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]); 	
	no_of_levels = m;
	
	if (n > 2) {
		HMmix = 1;
	}
	
	if (n != 2 + 2 * HMmix) {
		mexErrMsgTxt(PROGNAME": \"L\" has to provide the information block-lengths of Part A and B for all levels in a (no_of_levels X 2)-Matrix");
	}

	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"L\" to be a real matrix.\n");
	}	
	L1_real = mxGetPr(prhs[ARG_INDEX]);
	L2_real = L1_real + no_of_levels;
	if (HMmix) {
		L1_imag = L2_real + no_of_levels;
		L2_imag = L1_imag + no_of_levels;
	} else {
		L1_imag = L_dummy;
		L2_imag = L_dummy;
	}
#undef ARG_INDEX

#define	ARG_INDEX 3+ARG_INDEX_OFFSET
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]); 	
	if (m * n != 1) {
		mexErrMsgTxt(PROGNAME": \"Lvspp\" has to be scalar!\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"Lvspp\" to be a real scalar.\n");
	}	
	Lvspp = (int) mxGetScalar(prhs[ARG_INDEX]);
#undef ARG_INDEX

#define	ARG_INDEX 4+ARG_INDEX_OFFSET
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]);
	if (n != no_of_levels) {
		mexErrMsgTxt(PROGNAME " requires one deinterleaver, one L and one PL for each level.\n");
	}
	if (m != (2 - HMmix)  * N) {
		mexErrMsgTxt(PROGNAME": The length of the deinterleavers has to equal twice the length of the received signal.\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsInt32(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"Deinterleaver\" to be a real matrix.\n");
	}	
	Deinterleaver = (int *)mxGetData(prhs[ARG_INDEX]);
#undef ARG_INDEX

#define	ARG_INDEX 5+ARG_INDEX_OFFSET
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]); 	
	if (m != no_of_levels) {
		mexErrMsgTxt(PROGNAME " requires one deinterleaver one L and one PL for each level.\n");
	}	
	if (n != 2 + 2 * HMmix) {
		mexErrMsgTxt(PROGNAME": \"PL\" has to provide the Protection Levels for Part A and B for all levels in a (no_of_levels X 2)-Matrix");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"PL\" to be a real matrix.\n");
	}	
	PL1_real = mxGetPr(prhs[ARG_INDEX]);
	PL2_real = PL1_real + no_of_levels;
	PL1_imag = PL2_real + no_of_levels;
	PL2_imag = PL1_imag + no_of_levels;
#undef ARG_INDEX

#define	ARG_INDEX 6+ARG_INDEX_OFFSET
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]); 	
	if (m * n != 1) {
		mexErrMsgTxt(PROGNAME": \"maxiter\" has to be scalar!\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"maxiter\" to be a real scalar.\n");
	}	
	maxiter = (int) mxGetScalar(prhs[ARG_INDEX]);
#undef ARG_INDEX

#define	ARG_INDEX 7+ARG_INDEX_OFFSET
	m = mxGetM(prhs[ARG_INDEX]);
	n = mxGetN(prhs[ARG_INDEX]); 	
	if (m * n != 1) {
		mexErrMsgTxt(PROGNAME": \"SDCorMSC\" has to be scalar!\n");
	}
	if ( !mxIsNumeric(prhs[ARG_INDEX]) || mxIsComplex(prhs[ARG_INDEX]) || 
		mxIsSparse(prhs[ARG_INDEX])  || !mxIsDouble(prhs[ARG_INDEX]) ) {
		mexErrMsgTxt(PROGNAME " requires \"SDCorMSC\" to be a real scalar.\n");
	}	
	SDCorMSC = 0 - ((int) mxGetScalar(prhs[ARG_INDEX]) != 0);
#undef ARG_INDEX

	if (N < 20) {
		mexErrMsgTxt(PROGNAME": \"N\" has to be >= 20!\n");
	}
	if ((N1 < 0) || (N1 > N - 20)) {
		mexErrMsgTxt(PROGNAME": \"N1\" has to be >= 0!\n");
	}
	if (Lvspp < 0) {
		mexErrMsgTxt(PROGNAME": \"Lvspp\" has to be >= 0!\n");
	}
	if (maxiter < 0) {
		mexErrMsgTxt(PROGNAME": \"maxiter\" must not be negativ.");
	}

	if (HMmix && (Lvspp == 0)) {
		mexErrMsgTxt(PROGNAME": HMmix requires Lvspp > 0.");
	}

	/* memory allocation and initialization: */
	no_of_bits = 0;
	for (level = 0; level < no_of_levels; level++) {
		no_of_bits += (int)L1_real[level] + (int)L2_real[level] + 6 + (int)L1_imag[level] + (int)L2_imag[level] + 6;
	}

	msd_mem_size = 2 * N * sizeof(float) + 2 * N * sizeof(char) + 2 * N * sizeof(char) + no_of_bits * sizeof(char);
	viterbi_mem_size = STATES * sizeof(float) + STATES * sizeof(float) + 2 * N * STATES * sizeof(char);
	
	if (received_imag == NULL) {
		memory_ptr = (char *)malloc(viterbi_mem_size + msd_mem_size + N * sizeof(double));
		
		received_imag = (double *) (memory_ptr + viterbi_mem_size + msd_mem_size);		
		memset (received_imag, 0, N * sizeof(double));
	} else {
		memory_ptr = (char *)malloc(viterbi_mem_size + msd_mem_size);
	}


	if (!memory_ptr) {
		mexErrMsgTxt("Failed memory request!");
	}

	viterbi_mem = memory_ptr;
	msd_mem = memory_ptr + viterbi_mem_size;

	llr = (float *) msd_mem;
	hardpoints = (char *) (msd_mem + 2 * N * sizeof(float));
	lastiter = (char *) (msd_mem + 2 * N * sizeof(float) + 2 * N * sizeof(char));
	infoout[0] = (char *) (msd_mem + 2 * N * sizeof(float) + 2 * N * sizeof(char) + 2 * N * sizeof(char));
	infoout[1] = 0;
	for (m = 1; m < no_of_levels; m++) {
		infoout[m] = infoout[m-1] + (int)L1_real[m-1] + (int)L2_real[m-1] + 6 + (int)L1_imag[m-1] + (int)L2_imag[m-1] + 6;
	}
	memset(hardpoints, 0, 2 * N * sizeof(char));



	/* choosing partitioning type: */
	if (no_of_levels == 3) {
		if ((Lvspp != 0) && HMmix) {			/* HMmix 64-QAM */
			metric_real = partitioning[1];
			metric_imag = partitioning[0];
			rp_real[0] =        (N - 12) - RY[(int)PL2_real[0]] *        ((N - 12)/RY[(int)PL2_real[0]]);
			rp_real[1] = ((N - N1) - 12) - RY[(int)PL2_real[1]] * (((N - N1) - 12)/RY[(int)PL2_real[1]]);
			rp_real[2] = ((N - N1) - 12) - RY[(int)PL2_real[2]] * (((N - N1) - 12)/RY[(int)PL2_real[2]]);
			rp_imag[0] = ((N - N1) - 12) - RY[(int)PL2_imag[0]] * (((N - N1) - 12)/RY[(int)PL2_imag[0]]);
			rp_imag[1] = ((N - N1) - 12) - RY[(int)PL2_imag[1]] * (((N - N1) - 12)/RY[(int)PL2_imag[1]]);
			rp_imag[2] = ((N - N1) - 12) - RY[(int)PL2_imag[2]] * (((N - N1) - 12)/RY[(int)PL2_imag[2]]);

		} else if (Lvspp != 0) {				/* HMsym 64-QAM */
			HMsym = 1;
			metric_real = partitioning[1];
			metric_imag = partitioning[1];
			rp_real[0] =        (2*N - 12) - RY[(int)PL2_real[0]] *        ((2*N - 12)/RY[(int)PL2_real[0]]);
			rp_real[1] = (2*(N - N1) - 12) - RY[(int)PL2_real[1]] * ((2*(N - N1) - 12)/RY[(int)PL2_real[1]]);
			rp_real[2] = (2*(N - N1) - 12) - RY[(int)PL2_real[2]] * ((2*(N - N1) - 12)/RY[(int)PL2_real[2]]);
		} else {								/* SM 64-QAM */
			metric_real = partitioning[0];
			metric_imag = partitioning[0];
			rp_real[0] = (2*(N - N1) - 12) - RY[(int)PL2_real[0]] * ((2*(N - N1) - 12)/RY[(int)PL2_real[0]]);
			rp_real[1] = (2*(N - N1) - 12) - RY[(int)PL2_real[1]] * ((2*(N - N1) - 12)/RY[(int)PL2_real[1]]);
			rp_real[2] = (2*(N - N1) - 12) - RY[(int)PL2_real[2]] * ((2*(N - N1) - 12)/RY[(int)PL2_real[2]]);
		}

	} else if (no_of_levels == 2) {				/* SM 16-QAM */
		rp_real[0] = (2*(N - N1) - 12) - RY[(int)PL2_real[0]] * ((2*(N - N1) - 12)/RY[(int)PL2_real[0]]);
		rp_real[1] = (2*(N - N1) - 12) - RY[(int)PL2_real[1]] * ((2*(N - N1) - 12)/RY[(int)PL2_real[1]]);
		metric_real = partitioning[2];
		metric_imag = partitioning[2];
	} else {									/* SM 4-QAM */
		rp_real[0] = (2*(N - N1) - 12) - RY[(int)PL2_real[0]] * ((2*(N - N1) - 12)/RY[(int)PL2_real[0]]);
		metric_real = partitioning[3];
		metric_imag = partitioning[3];
	}
	if (!SDCorMSC) {
		rp_real[0] = -12;
		rp_real[1] = -12;
		rp_real[2] = -12;
	}
	if (Lvspp != 0) {
		L1_real[0] = 0;
		L2_real[0] = (double) Lvspp;
	}


	/* Multi-Stage Decoding: */

	/* first decoding: */
	PL1 = PL1_real;
	PL2 = PL2_real;
	L1  = L1_real;
	L2  = L2_real;
	rp  = rp_real;
	first_metric = metric_real;
	first_received = received_real;
	hardpoints_ptr = hardpoints;

	for (n = 0; n <= HMmix; n++) {

		for (level = 0; level < no_of_levels; level++) {

			metric = first_metric;
			received = first_received;

			for (m = 0; m < 2 - HMmix; m++) {			/* for real and imaginary part */


				for (sample_index = m; sample_index < (2 - HMmix) * N; sample_index += 2 - HMmix) {

					sample = (float)received[sample_index >> (1 - HMmix)];		/* extract real or imaginary part respectively */
					closest_zero = fabs(sample - metric[hardpoints_ptr[sample_index]]);
					for (subset_point = (0x1 << (level + 1)); subset_point < (0x1 << no_of_levels); subset_point += (0x1 << (level + 1))) {
						dist = fabs(sample - metric[hardpoints_ptr[sample_index] + subset_point]);
						if (dist < closest_zero) { 
							closest_zero = dist;
						}
					}
					closest_one = fabs(sample - metric[hardpoints_ptr[sample_index] + (0x1 << level)]);
					for (subset_point = (0x3 << level); subset_point < (0x1 << no_of_levels); subset_point += (0x1 << (level + 1))) {
						dist = fabs(sample - metric[hardpoints_ptr[sample_index] + subset_point]);
						if (dist < closest_one) { 
							closest_one = dist;
						}
					}

				
#ifdef CONSIDERING_SNR
					SNR = (float)signal_to_noise_ratio[sample_index >> (1 - HMmix)];
					llr[sample_index] = (closest_zero - closest_one) * SNR;
					/* llr[sample_index] = (closest_zero*closest_zero - closest_one*closest_one) * SNR * SNR; */
#else
					llr[sample_index] = (closest_zero - closest_one);
					/* llr[sample_index] = (closest_zero*closest_zero - closest_one*closest_one); */
#endif

				}
			metric = metric_imag;
			received = received_imag;


			}
			

			error = viterbi_decode (llr, (2 - HMmix) * N, (level || (!HMsym && (n || !HMmix))) * (2 - HMmix) * N1, puncturing[(int)PL1[level]],
									puncturing[(int)PL2[level]], tailpuncturing[rp[level]],
									infoout[level] + n * ((int)L1_real[level] + (int)L2_real[level] + 6),
									hardpoints_ptr, level, Deinterleaver + (2 - HMmix) * N * level,
									(int)L1[level] + (int)L2[level] + 6, rp[level] + 12, viterbi_mem);


			if (error) {
				free(memory_ptr);
				mexErrMsgTxt(PROGNAME ": Error in Viterbi decoder");
				return;		
			}

		}


		PL1 = PL1_imag;
		PL2 = PL2_imag;
		L1  = L1_imag;
		L2  = L2_imag;
		rp  = rp_imag;
		first_metric = metric_imag;
		first_received = received_imag;
		hardpoints_ptr = hardpoints + N;

	}




	diff = 1;
	iteration = 0;

	/* iterations: */
	while (iteration < maxiter) {
		PL1 = PL1_real;
		PL2 = PL2_real;
		L1  = L1_real;
		L2  = L2_real;
		rp  = rp_real;
		first_metric = metric_real;
		first_received = received_real;
		hardpoints_ptr = hardpoints;

#ifdef ITER_BREAK
		memcpy(lastiter, hardpoints, 2 * N);
#endif

		for (n = 0; n <= HMmix; n++) {

			for (level = 0; level < no_of_levels; level++) {

				metric = first_metric;
				received = first_received;

				for (m = 0; m < 2 - HMmix; m++) {			/* for real and imaginary part */


					for (sample_index = m; sample_index < (2 - HMmix) * N; sample_index += 2 - HMmix) {

						sample = (float)received[sample_index >> (1 - HMmix)];		/* extract real or imaginary part respectively */
						closest_zero = fabs(sample - metric[hardpoints_ptr[sample_index] & ~(0x1 << level)]);
						closest_one  = fabs(sample - metric[hardpoints_ptr[sample_index] |  (0x1 << level)]);
								
#ifdef CONSIDERING_SNR
						SNR = (float)signal_to_noise_ratio[sample_index >> (1 - HMmix)];
						llr[sample_index] = (closest_zero - closest_one) * SNR;
						/* llr[sample_index] = (closest_zero*closest_zero - closest_one*closest_one) * SNR * SNR; */
#else
						llr[sample_index] = (closest_zero - closest_one);
						/* llr[sample_index] = (closest_zero*closest_zero - closest_one*closest_one); */
#endif


					}
				metric = metric_imag;
				received = received_imag;

				}

				error = viterbi_decode (llr, (2 - HMmix) * N, (level || (!HMsym && (n || !HMmix))) * (2 - HMmix) * N1, puncturing[(int)PL1[level]],
										puncturing[(int)PL2[level]], tailpuncturing[rp[level]],
										infoout[level] + n * ((int)L1_real[level] + (int)L2_real[level] + 6),
										hardpoints_ptr, level, Deinterleaver + (2 - HMmix) * N * level,
										(int)L1[level] + (int)L2[level] + 6, rp[level] + 12, viterbi_mem);

				if (error) {
					free(memory_ptr);
					mexErrMsgTxt(PROGNAME ": Error in Viterbi decoder");
					return;		
				}

#ifdef ITER_BREAK			
				if (level == 0) {
					diff = 0;
					for (sample_index = 0; sample_index < (2 - HMmix) * N * sizeof(char) / sizeof(int); sample_index++) {
						diff += (((int *)hardpoints)[sample_index] ^ ((int *)lastiter)[sample_index]) != 0;
					}

					/*diff = memcmp (lastiter,hardpoints,2 * N); */
					if (!diff) {
						break;
					}
				}
#endif
			} /* for (level = 0; level < no_of_levels; level++) */


			PL1 = PL1_imag;
			PL2 = PL2_imag;
			L1  = L1_imag;
			L2  = L2_imag;
			rp  = rp_imag;
			first_metric = metric_imag;
			first_received = received_imag;
			hardpoints_ptr = hardpoints + N;

		} /* for (n = 0; n <= HMmix; n++) */



#ifdef ITER_BREAK
		if (!diff) {
			break;
		}
#endif
		iteration++;
	} /* while (iteration < maxiter) */



	/* Energy Dispersal */
	
	no_of_bits = 0;
	for (level = (Lvspp != 0); level < no_of_levels; level++) {
		no_of_bits += (int)L1_real[level] + (int)L2_real[level];
	}
	for (level = 0; level < no_of_levels; level++) {
		no_of_bits += (int)L1_imag[level] + (int)L2_imag[level];
	}


	plhs[0] = mxCreateDoubleMatrix(1,no_of_bits, mxREAL);
	output_ptr = mxGetPr(plhs[0]);


	PRBS_INIT(PRBS_reg);
	n = 0;
	if (HMmix) {
		for (m = Lvspp + 6; m < Lvspp + 6 + (int)L1_imag[0]; m++) {
			output_ptr[n++] = (double)(infoout[0][m] ^ PRBS_BIT(PRBS_reg));
			PRBS_SHIFT(PRBS_reg);
		}
	}

	for (level = (Lvspp != 0); level < no_of_levels; level++) {
		for (m = 0; m < (int)L1_real[level]; m++) {
			output_ptr[n++] = (double)(infoout[level][m] ^ PRBS_BIT(PRBS_reg));
			PRBS_SHIFT(PRBS_reg);
		}
		for (m = (int)L1_real[level] + (int)L2_real[level] + 6; m < (int)L1_real[level] + (int)L2_real[level] + 6 + (int)L1_imag[level]; m++) {
			output_ptr[n++] = (double)(infoout[level][m] ^ PRBS_BIT(PRBS_reg));
			PRBS_SHIFT(PRBS_reg);
		}
	}

	if (HMmix) {
		for (m = Lvspp + 6 + (int)L1_imag[0]; m < Lvspp + 6 + (int)L1_imag[0] + (int)L2_imag[0]; m++) {
			output_ptr[n++] = (double)(infoout[0][m] ^ PRBS_BIT(PRBS_reg));
			PRBS_SHIFT(PRBS_reg);
		}
	}

	for (level = (Lvspp != 0); level < no_of_levels; level++) {
		for (m = (int)L1_real[level]; m < (int)L1_real[level] + (int)L2_real[level]; m++) {
			output_ptr[n++] = (double)(infoout[level][m] ^ PRBS_BIT(PRBS_reg));
			PRBS_SHIFT(PRBS_reg);
		}
		for (m = (int)L1_real[level] + (int)L2_real[level] + 6 + (int)L1_imag[level]; m < (int)L1_real[level] + (int)L2_real[level] + 6 + (int)L1_imag[level] + (int)L2_imag[level]; m++) {
			output_ptr[n++] = (double)(infoout[level][m] ^ PRBS_BIT(PRBS_reg));
			PRBS_SHIFT(PRBS_reg);
		}
	}


	PRBS_INIT(PRBS_reg);
	if ((Lvspp != 0) && (nlhs < 2)) {
		mexWarnMsgTxt(PROGNAME ": There is a very strongly protected part, but no variable to put it into!");
	}

	if (nlhs > 1) {
		no_of_bits = Lvspp;
		plhs[1] = mxCreateDoubleMatrix(1,no_of_bits, mxREAL);
		output_ptr = mxGetPr(plhs[1]);
		for (m = 0; m < Lvspp; m++) {
			output_ptr[m] = (double)(infoout[0][m] ^ PRBS_BIT(PRBS_reg));
			PRBS_SHIFT(PRBS_reg);
		}
	}

	if (nlhs > 2) {
		plhs[2] = mxCreateDoubleMatrix(1,1, mxREAL);
		output_ptr = mxGetPr(plhs[2]);
		output_ptr[0] = (double)iteration;
	}

	if (nlhs > 3) {		/* variance calculation */
		plhs[3] = mxCreateDoubleMatrix(1,1, mxREAL);
		output_ptr = mxGetPr(plhs[3]);
		variance = 0.0;
		for (sample_index = 0; sample_index < N; sample_index++) {
			sample = (float)received_real[sample_index];		/* extract real part respectively */
			dist = (sample - metric_real[hardpoints[(2 - HMmix) * sample_index]]);
			variance += (double)dist * (double)dist;
			sample = (float)received_imag[sample_index];		/* extract imaginary part respectively */
			dist = (sample - metric_imag[hardpoints[HMmix * (N - 1) + (2 - HMmix) * sample_index + 1]]);
			variance += (double)dist * (double)dist;
		}

		output_ptr[0] = variance/((double)N);
	}


	if (nlhs > 4) {		/* output error signal */
		plhs[4] = mxCreateDoubleMatrix(1,N, mxCOMPLEX);

		output_ptr = mxGetPr(plhs[4]);
		for (sample_index = 0; sample_index < N; sample_index++) {
			sample = (float)received_real[sample_index];		/* extract real part */
			output_ptr[sample_index] = (sample - metric_real[hardpoints[(2 - HMmix) * sample_index]]);
		}
		output_ptr = mxGetPi(plhs[4]);
		for (sample_index = 0; sample_index < N; sample_index++) {
			sample = (float)received_imag[sample_index];		/* extract imaginary part */
			output_ptr[sample_index] = (sample - metric_imag[hardpoints[HMmix * (N - 1) + (2 - HMmix) * sample_index + 1]]);
		}
	}

	free(memory_ptr);
	return;
}
