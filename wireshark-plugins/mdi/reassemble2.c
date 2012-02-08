/******************************************************************************\
 * Copyright (c) 2007 British Broadcasting Corporation
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	General Purpose Packet Reassemblers for data packet mode, MOT and PFT
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

#include "reassemble2.h"
#include <gmodule.h>
#include <epan/emem.h>
#include <string.h>

typedef const void* cvoidp;

struct _segment_tracker
{
	cvoidp* seg_data;
	int segments;
	int max_segments;
};

typedef struct _segment_tracker* segment_tracker;

void segment_tracker_reset(segment_tracker s)
{
  if(s==NULL)
    return;
  if(s->seg_data) {
	s->seg_data=NULL;
  }
  s->segments = s->max_segments = 0;
}

void segment_tracker_resize(segment_tracker s, int new_size)
{
  if(s==NULL)
    return;
  if(s->max_segments<new_size) {
    if(new_size<16)
	  s->max_segments = 16;
	else
	  s->max_segments = 2*new_size;
    cvoidp* n = (cvoidp*)se_alloc0(s->max_segments*sizeof(void*));
    if(s->segments>0) {
	   int i;
	   for(i=0; i<s->segments; i++)
	     n[i] = s->seg_data[i];
    }
	s->seg_data = n;
  }
}

int segment_tracker_size(segment_tracker s)
{
  if(s==NULL)
    return 0;
  return s->segments;
}

int segment_tracker_ready(segment_tracker s)
{
  size_t i;
  if(s==NULL)
    return 0;
  if (s->segments == 0)
		return 0;
  for ( i = 0; i < s->segments; i++)
  {
	if (s->seg_data[i] == 0)
	{
		return 0;
	}
  }
  return 1;
}

void segment_tracker_add_segment(segment_tracker s, int seg, const void* data)
{
  if(s==NULL)
    return;
  if(seg >= s->max_segments)
    segment_tracker_resize(s, seg + 1);
  if(seg >= s->segments)
    s->segments = seg+1;
  s->seg_data[seg] = data;
}

int segment_tracker_have_segment (segment_tracker s, int seg)
{
  if(s==NULL)
    return 0;
  if (seg <  s->segments)
    return s->seg_data[seg]?1:0;
  return 0;
}

struct _reassembler
{
	int last_segment_num;
	int last_segment_size;
	size_t segment_size;
	struct _segment_tracker tracker;
	int ready;
};

reassembler reassembler_new()
{
  reassembler r = (reassembler)se_alloc0(sizeof(struct _reassembler));
  r->last_segment_num=-1;
  r->last_segment_size = -1;
  r->segment_size=-1;
  r->tracker.seg_data=NULL;
  segment_tracker_reset(&r->tracker);
  r->ready = 0;
  return r;
}

void reassembler_delete(reassembler r)
{
 /* nothing to to? (using se_alloc routines) */
}

int reassembler_ready(reassembler r)
{
  if(r==NULL)
    return 0;
  return r->ready;
}

void reassembler_add_segment(
  reassembler r, 
  const void* data, int size,
  int seg, int last)
{
  void *dest;
  if(r==NULL)
    return;
  dest = se_alloc(size);
  memcpy(dest, data, size);
  segment_tracker_add_segment(&r->tracker, seg, dest);
  if (last) {
    if (r->last_segment_num == -1) {
      r->last_segment_num = seg;
	  r->last_segment_size = size;
	  /* three cases:
			   1: single segment - easy! (actually degenerate with case 3)
			   2: multi-segment and the last segment came first.
			   3: normal - some segment, not the last, came first, 
			   we know the segment size
       */
      if (seg == 0) {	/* case 1,3 */
        r->segment_size = size;
      } else if (r->segment_size == 0) {/* case 2 */
        r->last_segment_size = size;
      }
    } /* otherwise do nothing as we already have the last segment */
  } else {
    r->segment_size = size;
  }

  if ((r->last_segment_size != -1)	/* we have the last segment */
		&& (r->ready == 0)	/* we haven't already completed reassembly */
		&& segment_tracker_ready (&r->tracker)		/* there are no gaps */
		)
	{
		r->ready = 1;
	}
}

int reassembler_reassembled_size(reassembler r)
{
  if(r==NULL)
    return -1;
  return r->segment_size*r->last_segment_num+r->last_segment_size;
}

int reassembler_reassemble(reassembler r, void* data_out, int max_size)
{
  int i, size;
  char *dest = (char*)data_out;
  if(r==NULL)
    return -1;
  if(!r->ready)
    return -1;
  size = reassembler_reassembled_size(r);
  if(max_size<size)
    return -1;
  for(i=0; i<r->last_segment_num; i++) {
    memcpy(dest, r->tracker.seg_data[i], r->segment_size);
    dest += r->segment_size;
  }
  memcpy(dest, r->tracker.seg_data[r->last_segment_num], r->last_segment_size);
  return size;
}
