/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 15.06.2004                                                 */
/*  Last change  : 01.07.2004                                                 */
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
/*  viterbi_decode.c (part of msd_hard)                                       */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  Viterbi Decoder for Multi-Stage Decoding                                  */
/*                                                                            */
/******************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include "viterbi_decode.h"


int viterbi_decode (float *llr, int N, int N_PartA, char *puncturing1, char *puncturing2, char *puncturing3,
			char *infoout, char *cwout, int bitpos, int *Deinterleaver, int L, int N_tail, char *memory_ptr) {

	float *old_metrics, *new_metrics, inf = (float)1e20;
	char *path_reg;
	char *puncture_ptr;
	void *swap_ptr;
	int symbol_index, j, symbol_pos, symbol_pos2, symbol_pos3;
	float symbols_acc[QOFB];
	float metric_path2, metric_s1, metric_s2, metric_inc;
	char mask, codebits;
	int part, state, N_Part, path_reg_index, butterfly, info_index;

	if (!llr || !infoout || !cwout || !memory_ptr) {
		return ENOMEM;
	}

	old_metrics = (float *)(memory_ptr + 0);
	new_metrics = (float *)(memory_ptr + STATES * sizeof(float));
	path_reg = (char *)(memory_ptr + STATES * sizeof(float) + STATES * sizeof(float));

	for (j = 1; j < STATES; j++) {
		old_metrics[j] = -inf;
	}
	old_metrics[0] = 0;


	symbol_index = 0;
	path_reg_index = 0;
	puncture_ptr = puncturing1 + 1;
	N_Part = N_PartA;
	
	for (part = 0; part < 3; part++){

		while (symbol_index < N_Part) {

			symbol_pos = Deinterleaver[symbol_index];
			symbols_acc[0] = -llr[symbol_pos];
			symbols_acc[1] =  llr[symbol_pos];
			symbols_acc[2] = -llr[symbol_pos];
			symbols_acc[3] =  llr[symbol_pos];			

			switch (*puncture_ptr) {
			
			case 3:
				symbol_pos = Deinterleaver[symbol_index + 1];
				symbols_acc[0] += -llr[symbol_pos];
				symbols_acc[1] += -llr[symbol_pos];
				symbols_acc[2] +=  llr[symbol_pos];
				symbols_acc[3] +=  llr[symbol_pos];
				symbol_index += 2;
				break;
			case 5:
				symbol_pos = Deinterleaver[symbol_index + 1];
				symbols_acc[0] += -llr[symbol_pos];
				symbols_acc[1] += -llr[symbol_pos];
				symbols_acc[2] += -llr[symbol_pos];
				symbols_acc[3] += -llr[symbol_pos];
				symbol_index += 2;
				break;	
			case 7:
				symbol_pos = Deinterleaver[symbol_index + 1];
				symbol_pos2 = Deinterleaver[symbol_index + 2];
				symbols_acc[0] += -llr[symbol_pos] - llr[symbol_pos2];
				symbols_acc[1] += -llr[symbol_pos] - llr[symbol_pos2];
				symbols_acc[2] +=  llr[symbol_pos] - llr[symbol_pos2];
				symbols_acc[3] +=  llr[symbol_pos] - llr[symbol_pos2];
				symbol_index += 3;
				break;
			case 15:
				symbol_pos = Deinterleaver[symbol_index + 1];
				symbol_pos2 = Deinterleaver[symbol_index + 2];
				symbol_pos3 = Deinterleaver[symbol_index + 3];
				symbols_acc[0] += -llr[symbol_pos] - llr[symbol_pos2] - llr[symbol_pos3];
				symbols_acc[1] += -llr[symbol_pos] - llr[symbol_pos2] + llr[symbol_pos3];
				symbols_acc[2] +=  llr[symbol_pos] - llr[symbol_pos2] - llr[symbol_pos3];
				symbols_acc[3] +=  llr[symbol_pos] - llr[symbol_pos2] + llr[symbol_pos3];
				symbol_index += 4;
				break;	
			default:
				symbol_index++;			
			}
			
			puncture_ptr += *(puncture_ptr + 1);
			
			symbols_acc[7] = -symbols_acc[0];
			symbols_acc[6] = -symbols_acc[1];
			symbols_acc[5] = -symbols_acc[2];
			symbols_acc[4] = -symbols_acc[3];
			

			for (butterfly = 0; butterfly < NOOFBF; butterfly++) {
				metric_s1 = old_metrics[butterfly];
				metric_s2 = old_metrics[butterfly + NOOFBF];
				metric_inc = symbols_acc[CODER_OUTPUT[butterfly]];

				new_metrics[2 * butterfly] = metric_s1 + metric_inc;
				path_reg[path_reg_index + 2 * butterfly] = 0;
				metric_path2 = metric_s2 - metric_inc;					/* Add */
				if (metric_path2 > new_metrics[2 * butterfly]) {			/* Compare */
					new_metrics[2 * butterfly] = metric_path2;
					path_reg[path_reg_index + 2 * butterfly] = 1;			/* Select */
				}
				new_metrics[2 * butterfly + 1] = metric_s1 - metric_inc;
				path_reg[path_reg_index + 2 * butterfly + 1] = 0;
				metric_path2 = metric_s2 + metric_inc;					/* Add */
				if (metric_path2 > new_metrics[2 * butterfly + 1]) {			/* Compare */
					new_metrics[2 * butterfly + 1] = metric_path2;
					path_reg[path_reg_index + 2 * butterfly + 1] = 1;		/* Select */
				}

			}
			/* are hard coded butterflies faster? */

			path_reg_index += STATES;
			swap_ptr = old_metrics;
			old_metrics = new_metrics;
			new_metrics = swap_ptr;
		}

		if (part == 0) {
			puncture_ptr = puncturing2 + 1;
			N_Part = N - N_tail;
		} else {
			puncture_ptr = puncturing3 + 1;
			N_Part = N;
		}
	}


	/* trace back */
	state = 0;
	symbol_index = N - 1;
	info_index = L - 1;
	N_Part = N - N_tail;
	path_reg_index -= STATES;
	puncture_ptr = puncturing3 + 1 - *puncturing3;

	for (part = 2; part >= 0; part--) {
		while (symbol_index >= N_Part) {
			infoout[info_index] = state & 0x1;		/* information bit output */
			info_index--;
			mask = 0 - ((state & 0x1) ^ path_reg[path_reg_index + state]);
			state = (state >> 1) | (path_reg[path_reg_index + state] * NOOFBF);
			/* code bits ouput and interleaving: */
			codebits = CODER_OUTPUT[state & ~(NOOFBF)] ^ mask;
			switch (*puncture_ptr) {
			
			case 3:
				cwout[Deinterleaver[symbol_index]] &= ~(0x1 << bitpos);
				cwout[Deinterleaver[symbol_index]] |= ((codebits & 0x2) >> 1) << bitpos;	
				symbol_index -= 2;
				break;
			case 5:
				cwout[Deinterleaver[symbol_index]] &= ~(0x1 << bitpos);
				cwout[Deinterleaver[symbol_index]] |= ((codebits & 0x4) >> 2) << bitpos;
				symbol_index -= 2;
				break;
			case 7:
				cwout[Deinterleaver[symbol_index]] &= ~(0x1 << bitpos);
				cwout[Deinterleaver[symbol_index]] |= ((codebits & 0x4) >> 2) << bitpos;
				cwout[Deinterleaver[symbol_index - 1]] &= ~(0x1 << bitpos);
				cwout[Deinterleaver[symbol_index - 1]] |= ((codebits & 0x2) >> 1) << bitpos;
				symbol_index -= 3;
				break;
			case 15:
				cwout[Deinterleaver[symbol_index]] &= ~(0x1 << bitpos);
				cwout[Deinterleaver[symbol_index]] |= (codebits & 0x1) << bitpos;
				cwout[Deinterleaver[symbol_index - 1]] &= ~(0x1 << bitpos);
				cwout[Deinterleaver[symbol_index - 1]] |= ((codebits & 0x4) >> 2) << bitpos;
				cwout[Deinterleaver[symbol_index - 2]] &= ~(0x1 << bitpos);
				cwout[Deinterleaver[symbol_index - 2]] |= ((codebits & 0x2) >> 1) << bitpos;
				symbol_index -= 4;
				break;
			default:
				symbol_index --;
			}
			
			cwout[Deinterleaver[symbol_index + 1]] &= ~(0x1 << bitpos);
			cwout[Deinterleaver[symbol_index + 1]] |= (codebits & 0x1) << bitpos;
			
			path_reg_index -= STATES;
			puncture_ptr -= *(puncture_ptr - 1);
		}
		
		if (part == 2) {
			puncture_ptr = puncturing2 + 1 - *puncturing2;
			N_Part = N_PartA;
		} else {
			puncture_ptr = puncturing1 + 1 - *puncturing1;
			N_Part = 0;
		}

	}

	return 0;
}
	
