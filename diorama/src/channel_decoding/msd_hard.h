/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2004 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 15.06.2004                                                 */
/*  Last change  : 30.06.2004                                                 */
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
/*  msd_hard.h (part of msd_hard)                                             */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  constants and tables for msd_hard.c                                       */
/*                                                                            */
/******************************************************************************/

char puncturing[13][17] = {	{0,15, 0},
				{ -4,15, 2, 7, 2, 7, -4},
				{  0, 7, 0},
				{ -6, 7, 2, 7, 2, 7, 2, 3, -6},
				{  0, 3, 0},
				{ -6, 3, 2, 5, 2, 3, 2, 1, -6},
				{ -4, 3, 2, 1, 2, 3, -4},
				{ -2, 3, 2, 1, -2},
				{-14, 3, 2, 1, 2, 1, 2, 3, 2, 1, 2, 1, 2, 3, 2, 1, -14},
				{ -4, 3, 2, 1, 2, 1, -4},
				{ -6, 3, 2, 1, 2, 1, 2, 1, -6},
				{-12, 3, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, -12},
				{-14, 3, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, -14}};

int RX[13] = {1, 3, 1, 4, 1, 4, 3, 2, 8, 3, 4, 7, 8};
int RY[13] = {4,10, 3,11, 2, 7, 5, 3,11, 4, 5, 8, 9};

char tailpuncturing[12][13] = {	{-10, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, -10},
				{-10, 7, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, -10},
				{-10, 7, 2, 3, 2, 3, 2, 7, 2, 3, 2, 3, -10},
				{-10, 7, 2, 7, 2, 3, 2, 7, 2, 3, 2, 3, -10},
				{-10, 7, 2, 7, 2, 3, 2, 7, 2, 7, 2, 3, -10},
				{-10, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 3, -10},
				{-10, 7, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, -10},
				{-10,15, 2, 7, 2, 7, 2, 7, 2, 7, 2, 7, -10},
				{-10,15, 2, 7, 2, 7, 2,15, 2, 7, 2, 7, -10},
				{-10,15, 2,15, 2, 7, 2,15, 2, 7, 2, 7, -10},
				{-10,15, 2,15, 2, 7, 2,15, 2, 7, 2,15, -10},				
				{-10,15, 2,15, 2,15, 2,15, 2, 7, 2,15, -10}};


#define SQRT2	1.41421356237310F
#define SQRT10	3.16227766016838F
#define SQRT42	6.48074069840786F

float partitioning[4][8] = {	{7/SQRT42, 5/SQRT42, 3/SQRT42, 1/SQRT42, -1/SQRT42, -3/SQRT42, -5/SQRT42, -7/SQRT42},	/* 64-QAM Ungerb�k Set Partitioning */
								{7/SQRT42, -1/SQRT42, 5/SQRT42, -3/SQRT42, 3/SQRT42, -5/SQRT42, 1/SQRT42, -7/SQRT42},	/* 64-QAM Block Partitioning */
								{3/SQRT10, 1/SQRT10, -1/SQRT10, -3/SQRT10},												/* 16-QAM Ungerb�k Set Partitioning */
								{1/SQRT2, -1/SQRT2}	};																	/*  4-QAM Ungerb�k Set Partitioning */

#undef SQRT2
#undef SQRT10
#undef SQRT42
