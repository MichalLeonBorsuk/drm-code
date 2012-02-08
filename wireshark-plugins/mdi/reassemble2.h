/******************************************************************************\
 * Copyright (c) 2007 British Broadcasting Corporation
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	See reassemble2.c
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef REASSEMBLE2_H_INCLUDED
#define REASSEMBLE2_H_INCLUDED

typedef struct _reassembler* reassembler;

reassembler reassembler_new();
void reassembler_delete(reassembler r);
int reassembler_ready(reassembler r);
void reassembler_add_segment(reassembler r, 
            const void* data, int size, int seg, int last);
int reassembler_reassembled_size(reassembler r);
int reassembler_reassemble(reassembler r, void* data_out, int max_size);

#endif
