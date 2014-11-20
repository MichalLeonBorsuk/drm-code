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
/*  dynamicbuffer.c                                                           */
/*                                                                            */
/******************************************************************************/


#include <math.h>
#include <windows.h>
#include "dynamicbuffer.h"



#define MIN(a,b) ( (a)>(b) ? b : a )





int dynamicbuffer_init( pdynamicbuffer_t pdynbuf )
{ 
	pchainbuffer_t pchainbuf;

	// create an empty buffer, so put can hook next buffers to this one
	pchainbuf = malloc( sizeof(chainbuffer_t) );	// buffer without data
	pchainbuf->readptr = NULL;	// we dont have any data in this buffer
	pchainbuf->level = 0;
	pchainbuf->size = 0;
	pchainbuf->next = NULL;

	pdynbuf->buffer_first = pchainbuf;
	pdynbuf->buffer_last = pchainbuf;
	pdynbuf->level = 0;
	pdynbuf->isLocked = 0;

	return 0;
}

#define DYNAMICBUFFER_LOCK_TIMEOUT 200
#define DYNAMICBUFFER_LOCK_SLEEPTIME 5

int dynamicbuffer_lock( pdynamicbuffer_t pdynbuf )
{
	int isLockedOld;
	int timeout;

	timeout = DYNAMICBUFFER_LOCK_TIMEOUT;
	isLockedOld = (int)InterlockedCompareExchange( (PVOID)(&(pdynbuf->isLocked)), (PVOID)1, (PVOID)0 );

	while ((timeout>0) && (isLockedOld==1))
	{
		Sleep( DYNAMICBUFFER_LOCK_SLEEPTIME );
		timeout -= DYNAMICBUFFER_LOCK_SLEEPTIME;
		isLockedOld = (int)InterlockedCompareExchange( (PVOID)(&(pdynbuf->isLocked)), (PVOID)1, (PVOID)0 );
	}

	return (isLockedOld==0);
}

int dynamicbuffer_unlock( pdynamicbuffer_t pdynbuf )
{
	int isLockedOld;
	isLockedOld = (int)InterlockedCompareExchange( (PVOID)(&(pdynbuf->isLocked)), (PVOID)0, (PVOID)1 );

	return (isLockedOld==1);
}

int dynamicbuffer_getbytes( pdynamicbuffer_t pdynbuf, uint8_t* dst, int no_of_bytes )
{
	int bytes_to_read = no_of_bytes;
	int bytes_to_copy;

	// only one instance should read and free memory!
	dynamicbuffer_lock( pdynbuf );
	{
		while( (bytes_to_read>0) && (pdynbuf->level>0) )
		{
			if( (pdynbuf->buffer_first->level<=0) && (pdynbuf->buffer_first->next!=NULL) )
			{
				pchainbuffer_t pchainbuf_temp = pdynbuf->buffer_first;
				pdynbuf->buffer_first = pdynbuf->buffer_first->next;
				free( pchainbuf_temp );
			}

			bytes_to_copy = MIN( pdynbuf->buffer_first->level, bytes_to_read );
			if (bytes_to_copy>0 )
			{
				memcpy( dst, pdynbuf->buffer_first->readptr, bytes_to_copy );
				dst += bytes_to_copy;
				pdynbuf->buffer_first->readptr += bytes_to_copy;
				pdynbuf->buffer_first->level -= bytes_to_copy;
				pdynbuf->level -= bytes_to_copy;

				bytes_to_read -= bytes_to_copy;
			}
		}
	}
	dynamicbuffer_unlock( pdynbuf );

	return (no_of_bytes - bytes_to_read);
}

int dynamicbuffer_putbytes( pdynamicbuffer_t pdynbuf, uint8_t* src, int no_of_bytes )
{ 
	pchainbuffer_t pchainbuf;
	int iResult;

	pchainbuf = (pchainbuffer_t)malloc( sizeof(chainbuffer_t) + no_of_bytes );
	pchainbuf->readptr = (uint8_t*)pchainbuf + sizeof(chainbuffer_t);
	pchainbuf->level = no_of_bytes;
	pchainbuf->size = no_of_bytes;
	pchainbuf->next = NULL;
	memcpy( pchainbuf->readptr, src, no_of_bytes );

	dynamicbuffer_lock( pdynbuf );
	{
		pdynbuf->buffer_last->next = pchainbuf;
		pdynbuf->buffer_last = pchainbuf;
		pdynbuf->level += no_of_bytes;
		iResult = pdynbuf->level;
	}
	dynamicbuffer_unlock( pdynbuf );

	return iResult;
}


int dynamicbuffer_newblock( pchainbuffer_t* ppchainbuf, uint8_t** psrc, int no_of_bytes )
{ 
	(*ppchainbuf) = (pchainbuffer_t)malloc( sizeof(chainbuffer_t) + no_of_bytes );
	(*ppchainbuf)->readptr = (uint8_t*)(*ppchainbuf) + sizeof(chainbuffer_t);
	(*ppchainbuf)->level = no_of_bytes;
	(*ppchainbuf)->size = no_of_bytes;
	(*ppchainbuf)->next = NULL;

	*psrc = (*ppchainbuf)->readptr;

	return 0;
}


int dynamicbuffer_addblock( pdynamicbuffer_t pdynbuf, pchainbuffer_t pchainbuf )
{
	int iResult;
	dynamicbuffer_lock( pdynbuf );
	{
		pdynbuf->buffer_last->next = pchainbuf;
		pdynbuf->buffer_last = pchainbuf;
		pdynbuf->level += pchainbuf->level;
		iResult = pdynbuf->level;
	}
	dynamicbuffer_unlock( pdynbuf );

	return iResult;
}



int dynamicbuffer_getlevel( pdynamicbuffer_t pdynbuf )
{
	return pdynbuf->level;
}


int dynamicbuffer_putdoubles( pdynamicbuffer_t pdynbuf, double* src, int no_of_values )
{ 
	pchainbuffer_t pchainbuf;
	int iResult;
	int no_of_bytes = no_of_values*2;

	pchainbuf = (pchainbuffer_t)malloc( sizeof(chainbuffer_t) + no_of_bytes );
	pchainbuf->readptr = (uint8_t*)pchainbuf + sizeof(chainbuffer_t);
	pchainbuf->level = no_of_bytes;
	pchainbuf->size = no_of_bytes;
	pchainbuf->next = NULL;

	{
		int16_t *dst = (int16_t*)pchainbuf->readptr;
		int k;

		for (k=0; k<no_of_values; k++)
			dst[k] = (int16_t)floor( src[k]*32767 + 0.5 );
	}

	dynamicbuffer_lock( pdynbuf );
	{
		pdynbuf->buffer_last->next = pchainbuf;
		pdynbuf->buffer_last = pchainbuf;
		pdynbuf->level += no_of_bytes;
		iResult = pdynbuf->level;
	}
	dynamicbuffer_unlock( pdynbuf );

	return iResult;
}


int dynamicbuffer_flush( pdynamicbuffer_t pdynbuf )
{
	pchainbuffer_t pchainbuf;

	if ( pdynbuf->buffer_last == NULL ) // is allready empty
		return 0;

	// empty buffer, to hook at end of chain
	pchainbuf = malloc( sizeof(chainbuffer_t) );	// buffer without data
	pchainbuf->readptr = NULL;	// we dont have any data in this buffer
	pchainbuf->level = 0;
	pchainbuf->size = 0;
	pchainbuf->next = NULL;

	dynamicbuffer_lock( pdynbuf );
	{
		pdynbuf->buffer_last->next = pchainbuf;
		pdynbuf->buffer_last = pchainbuf;

		while ( pdynbuf->buffer_first->next!=NULL )
		{
			pchainbuffer_t pchainbuf_temp = pdynbuf->buffer_first;
			pdynbuf->buffer_first = pdynbuf->buffer_first->next;
			free( pchainbuf_temp );
		}

		pdynbuf->level = 0;
	}
	dynamicbuffer_unlock( pdynbuf );

	return 0;
}

int dynamicbuffer_deinit( pdynamicbuffer_t pdynbuf )
{
	if (pdynbuf==NULL)
		return -1;

	dynamicbuffer_flush( pdynbuf );
	if ( pdynbuf->buffer_first != NULL )
	{
		free( pdynbuf->buffer_first );
		pdynbuf->buffer_first = NULL;
		pdynbuf->buffer_last = NULL;
		pdynbuf->level = 0;
	}

	return 0;
}


