/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2005 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 01.11.2004                                                 */
/*  Last change  : 11.01.2005                                                 */
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
/*  src_filter.h                                                              */
/*                                                                            */
/******************************************************************************/
/*  Description:                                                              */
/*  Sample-rate conversion for fractional timing corrections                  */
/*                                                                            */
/******************************************************************************/

#ifndef SRC_OS
#define WITHOUT_TABLES
#include "src_filter_table.h"
#undef WITHOUT_TABLES
#endif

int sample_rate_conv_int16_mono_mono (int16_t *, int , int16_t *, int ,  
				      double *, double );

int sample_rate_conv_int16_stereo_stereo (int16_t *, int , int16_t *, int ,  
					  double *, double );

int sample_rate_conv_int16_mono_stereo (int16_t *, int , int16_t *, int ,  
					double *, double );

int sample_rate_conv_int16_stereo_mono (int16_t *, int , int16_t *, int ,  
					double *, double );
