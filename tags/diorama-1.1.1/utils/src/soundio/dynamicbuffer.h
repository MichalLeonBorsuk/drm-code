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
/*  dynamicbuffer.h                                                           */
/*                                                                            */
/******************************************************************************/


typedef unsigned char uint8_t;
typedef short int16_t;


typedef struct {
	uint8_t *writeptr;	// actual writepointer (readpointer = writeptr - level)
	int level;
	int size;
} ringbuffer_t, *pringbuffer_t;


typedef struct chainbuffer_t *pchainbuffer_t;
typedef struct chainbuffer_t {
	uint8_t *readptr;
	int level;
	int size;
	pchainbuffer_t next;
} chainbuffer_t;


typedef struct {
	pchainbuffer_t buffer_first;
	pchainbuffer_t buffer_last;
	int level;
	int isLocked;
} dynamicbuffer_t, *pdynamicbuffer_t;


int dynamicbuffer_init( pdynamicbuffer_t pdynbuf );
int dynamicbuffer_lock( pdynamicbuffer_t pdynbuf );
int dynamicbuffer_unlock( pdynamicbuffer_t pdynbuf );
int dynamicbuffer_getbytes( pdynamicbuffer_t pdynbuf, uint8_t* dst, int N_dst );
int dynamicbuffer_putbytes( pdynamicbuffer_t pdynbuf, uint8_t* src, int N_src );
int dynamicbuffer_putdoubles( pdynamicbuffer_t pdynbuf, double* src, int no_of_values );
int dynamicbuffer_flush( pdynamicbuffer_t pdynbuf );
int dynamicbuffer_deinit( pdynamicbuffer_t pdynbuf );
int dynamicbuffer_getlevel( pdynamicbuffer_t pdynbuf );
int dynamicbuffer_newblock( pchainbuffer_t* ppchainbuf, uint8_t** psrc, int no_of_bytes );
int dynamicbuffer_addblock( pdynamicbuffer_t pdynbuf, pchainbuffer_t pchainbuf );
