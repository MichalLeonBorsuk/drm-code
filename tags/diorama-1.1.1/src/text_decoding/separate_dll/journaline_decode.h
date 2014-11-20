/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2005 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 09.02.2005                                                 */
/*  Last change  : 17.02.2005                                                 */
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
/*  journaline_decode.h (part of journaline_decode)                           */
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





#define PROGNAME "journaline_decode"

struct Output_Files {
	char filename[10];
	int	update; 
	char *filebody;
	int body_length;
};

#ifdef __cplusplus
extern "C" {
#endif

int journaline_decode(unsigned char *, int, int, struct Output_Files **, int *);

void free_output(struct Output_Files *, int);
	
void journaline_decode_exit (void);

#ifdef __cplusplus
}
#endif
