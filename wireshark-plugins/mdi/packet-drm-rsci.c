/* packet-drm-rsci.c
 * Routines for Digital Radio Mondiale Multiplex Distribution Interface Protocol
 * Copyright 2007, British Broadcasting Corporation
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Ref:
 *      ETSI DRM RSCI (ETSI TS 102 349)
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gmodule.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <string.h>
#include <stdint.h>

void MjdToDate (guint32 Mjd, guint32 *Year, guint32 *Month, guint32 *Day);

/* Initialize the subtree pointers */
static gint ett_rsci = -1;

static int hf_di_rpro = -1;
static int hf_di_fmjd = -1;
static int hf_di_rdbv = -1;
static int hf_di_rsta = -1;
static int hf_di_rfre = -1;
static int hf_di_rdmo = -1;
static int hf_di_cfre = -1;
static int hf_di_cdmo = -1;
static int hf_di_time = -1;
static int hf_di_rgps = -1;
static int hf_di_rgps_source = -1;
static int hf_di_rgps_sats = -1;
static int hf_di_rgps_lat = -1;
static int hf_di_rgps_long = -1;
static int hf_di_rgps_altitude = -1;
static int hf_di_rgps_speed = -1;
static int hf_di_rgps_heading = -1;
static int hf_di_rinf = -1;
static int hf_di_ract = -1;
static int hf_di_rbw_ = -1;
static int hf_di_rser = -1;
static int hf_di_rtty = -1;
static int hf_di_rafs = -1;
static int hf_di_reas = -1;
static int hf_di_rpil = -1;
static int hf_di_rpil_sn = -1;
static int hf_di_rpil_sr = -1;
static int hf_di_rwmf = -1;
static int hf_di_rwmm = -1;
static int hf_di_rmer = -1;
static int hf_di_rdop = -1;
static int hf_di_rpsd = -1;
static int hf_di_cact = -1;
static int hf_di_cbws = -1;
static int hf_di_cbwg = -1;
static int hf_di_cser = -1;
static int hf_di_crec = -1;

static const value_string gps_source_vals[] = {
	{ 0x0,	"Invalid" },
	{ 0x1,	"GPS Receiver" },
	{ 0x2,	"Differential GPS Receiver" },
	{ 0x3,	"Manual Entry" },
	{ 0,	NULL }
};

static float
di_tvb_get_float(tvbuff_t * tvb, guint offset)
{
  union { char s; unsigned char u; } n;
  n.u = tvb_get_guint8(tvb, offset);
  guint8 m = tvb_get_guint8(tvb, offset+1);
  return ((float)n.s)+((float)m)/256.0;
}

static float
di_tvb_get_float2(tvbuff_t * tvb, guint offset)
{
  union { gint16 s; guint16 u; } n;
  n.u = tvb_get_ntohs(tvb, offset);
  guint8 m = tvb_get_guint8(tvb, offset+2);
  return ((float)n.s)+((float)m)/256.0;
}

static void
dissect_rpro (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  if(tree) {
    proto_tree_add_item (tree, hf_di_rpro, tvb, 0, 1, FALSE);
  }
}

static void
dissect_fmjd (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
      if(tree) {
	    if(bytes>0) {
          guint32 mjd = tvb_get_ntohl(tvb, offset);
		  guint32 frac_day = tvb_get_ntohl(tvb, offset+4);
	      guint32 y,m,d,h,min; /* TODO */
		  h = frac_day / 10 / 1000 / 60 / 60;
		  min = (frac_day / 10 / 1000 / 60) - 60*h;
		  MjdToDate (mjd, &y, &m, &d);
          proto_tree_add_text (tree, tvb, offset, bytes, "Date & Time (mjd) %d/%d/%d %2d:%02d", y,d,m,h,min);
		} else {
          proto_tree_add_item (tree, hf_di_fmjd, tvb, offset, bytes, FALSE);
		}
      }
}

static void
dissect_time (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
   if(tree) {
     proto_tree_add_item (tree, hf_di_time, tvb, 0, tvb_length(tvb), FALSE);
   }
}

static void
dissect_rgps (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
  proto_tree *gps_tree = NULL;
  proto_item *ti = NULL;
  guint8 source, nSats, latmin, longmin;
  guint16 val,latdeg,longdeg,latfracmin,longfracmin, speed, heading;
  float altitude;
  struct tm tm;

  if (bytes != 26)
    return;

  if(tree) {
    ti = proto_tree_add_item (tree, hf_di_rgps, tvb, offset, bytes, FALSE);
    gps_tree = proto_item_add_subtree (ti, ett_rsci);

    source = tvb_get_guint8(tvb, offset);
    if(source == 0xff)
    {
      proto_tree_add_text (gps_tree, tvb, offset, 1, "no GPS source");
    }
    else
    {
      proto_tree_add_item (gps_tree, hf_di_rgps_source, tvb, offset, 1, FALSE);
    }
    offset++;

    nSats = tvb_get_guint8(tvb, offset);
 	if(nSats == 0xff)
 	{
      proto_tree_add_text (gps_tree, tvb, offset, 1, "no satellites visible");
 	}
 	else
 	{
      proto_tree_add_item (tree, hf_di_rgps_sats, tvb, offset, 1, FALSE);
 	}
	offset++;

    val = tvb_get_ntohs(tvb, offset);
	latdeg = *(gint16*)&val;
    latmin = tvb_get_guint8(tvb, offset+2);
    latfracmin = tvb_get_ntohs(tvb, offset+3);

    val = tvb_get_ntohs(tvb, offset+5);
	longdeg = *(guint16*)&val;
    longmin = tvb_get_guint8(tvb, offset+7);
    longfracmin = tvb_get_ntohs(tvb, offset+8);

    if(latmin == 0xff)
    {
      proto_tree_add_text (gps_tree, tvb, offset, 10, "position not available");
    }
    else
    {
		double latitude, longitude;
		latitude = (double)latdeg
		 + ((double)latmin + ((double)latfracmin)/65536.0)/60.0;
		longitude = (double)longdeg
		 + ((double)longmin + ((double)longfracmin)/65536.0)/60.0;
      proto_tree_add_double_format_value(gps_tree, hf_di_rgps_lat, tvb, offset, 5, latitude, "%f", latitude);
      proto_tree_add_double_format_value(gps_tree, hf_di_rgps_long, tvb, offset, 5, longitude, "%f", longitude);
    }
	offset += 10;

    val = tvb_get_guint8(tvb, offset);
	if(val == 0xff)
	{
      proto_tree_add_text (gps_tree, tvb, offset, 3, "altitude not available");
	}
	else
	{
	  altitude = di_tvb_get_float2(tvb, offset);
      proto_tree_add_float_format_value(gps_tree, hf_di_rgps_altitude, tvb, offset, 3, altitude, "%f", altitude);
	}

    tm.tm_hour = tvb_get_guint8(tvb, offset);
    tm.tm_min = tvb_get_guint8(tvb, offset+1);
    tm.tm_sec = tvb_get_guint8(tvb, offset+2);
    tm.tm_year = tvb_get_ntohs(tvb, offset+3) - 1900;
    tm.tm_mon = tvb_get_guint8(tvb, offset+5);
    tm.tm_mday = tvb_get_guint8(tvb, offset+6);

    if(tm.tm_hour == 0xff)
    {
      proto_tree_add_text (gps_tree, tvb, offset, 7, "time not available");
    }
    else
    {
      proto_tree_add_text (gps_tree, tvb, offset, 7, "Date & Time %d/%d/%d %2d:%02d:%02d",
	  	tm.tm_year+1900, tm.tm_mon,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
	offset+=7;

    speed = tvb_get_ntohs(tvb, offset);
    if(speed == 0xffff)
    {
      proto_tree_add_text (gps_tree, tvb, offset, 2, "speed not available");
    }
    else
    {
	  float s = ((float)speed)/10.0;
      proto_tree_add_float_format_value(gps_tree, hf_di_rgps_speed, tvb, offset, 2, s, "%f", s);
    }
	offset += 2;

    heading = tvb_get_ntohs(tvb, offset);
    if(heading == 0xffff)
    {
      proto_tree_add_text (gps_tree, tvb, offset, 2, "heading not available");
    }
    else
    {
      proto_tree_add_item (tree, hf_di_rgps_heading, tvb, offset, 1, FALSE);
    }
  }
}

static void
dissect_rdmo (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
      proto_tree_add_item (tree, hf_di_rdmo, tvb, offset, bytes, FALSE);
	} else {
      proto_tree_add_text (tree, tvb, offset, 0, "rdmo");
	}
   }
}

static void
dissect_rfre (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          proto_tree_add_item(tree, hf_di_rfre, tvb, offset, bytes, FALSE);
	} else {
          proto_tree_add_text(tree, tvb, offset, 0, "rfre");
	}
   }
}

static void
dissect_rdbv (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
		  int i,n = bytes/2;
		  int off = offset;
		  for(i=0; i<n; i++)
		  {
            float db = di_tvb_get_float(tvb, off);
			off += 2;
		    proto_tree_add_float_format_value(tree, hf_di_rdbv, tvb, off, 2, db, "%f", db);
		  }
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rdbv");
	}
   }
}

static void
dissect_rinf (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    proto_tree_add_item (tree, hf_di_rinf, tvb, offset, bytes, FALSE);
   }
}

static void
dissect_ract (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          proto_tree_add_item (tree, hf_di_ract, tvb, offset, 1, FALSE);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "ract");
	}
   }
}

static void
dissect_rsta (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          guint8 sy = tvb_get_guint8(tvb, offset);
          guint8 f = tvb_get_guint8(tvb, offset+1);
          guint8 s = tvb_get_guint8(tvb, offset+2);
          guint8 a = tvb_get_guint8(tvb, offset+3);
          proto_tree_add_text (tree, tvb, offset, bytes, "status sync:%d FAC:%d SDC:%d Audio:%d", sy,f,s,a);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rsta");
	}
   }
}

static void
dissect_rbw_ (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          float bw = di_tvb_get_float(tvb, offset);
		  proto_tree_add_float_format_value(tree, hf_di_rbw_, tvb, offset, bytes, bw, "%f", bw);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rbw_");
	}
   }
}

static void
dissect_rser (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          proto_tree_add_item (tree, hf_di_rser, tvb, offset, 1, FALSE);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rser");
	}
   }
}

static void
dissect_rtty (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          guint8 s0 = tvb_get_guint8(tvb, offset);
          guint8 s1 = tvb_get_guint8(tvb, offset+1);
          guint8 s2 = tvb_get_guint8(tvb, offset+2);
          guint8 s3 = tvb_get_guint8(tvb, offset+3);
          proto_tree_add_text (tree, tvb, offset, bytes, "test type: str0:%d str1:%d str2:%d str3:%d", s0, s1, s2, s3);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rtty");
	}
   }
}

static void
dissect_rafs(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
		  int i;
          proto_item *ti = NULL;
          guint8 frames = tvb_get_guint8(tvb, offset);
          guint64 flags = tvb_get_ntohl(tvb, offset+1) << 8;
          flags |= tvb_get_guint8(tvb, offset+5);
          ti = proto_tree_add_text (tree, tvb, offset, bytes, "audio frames: ");
		  for(i=0; i<frames; i++)
		  {
            proto_item_append_text (ti, "%d", (flags&0x8000000000ULL)?1:0);
			flags <<= 1;
		  }
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rafs");
	}
   }
}

static void
dissect_reas (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
		  int i;
          proto_item *ti = NULL;
          ti = proto_tree_add_text (tree, tvb, offset, bytes, "audio frame extended status: ");
		  for(i=0; i<bytes; i++)
		  {
		    /* TODO a better decode */
            proto_item_append_text (ti, "%02x", tvb_get_guint8(tvb, offset+i));
		  }
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "reas");
	}
   }
}

static void
dissect_rpil (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
       proto_tree *pil_tree = NULL;
       proto_item *ti = NULL;
       ti = proto_tree_add_text (tree, tvb, offset, bytes, "Gain Reference Pilots");
       pil_tree = proto_item_add_subtree (ti, ett_rsci);
       proto_tree_add_item(pil_tree, hf_di_rpil_sn, tvb, offset, 1, FALSE);
       proto_tree_add_item(pil_tree, hf_di_rpil_sr, tvb, offset+1, 1, FALSE);
	   offset += 4;
	   while(offset<bytes)
	   {
	     int j;
         guint8 pe = tvb_get_guint8(tvb, offset);
         guint8 po = tvb_get_guint8(tvb, offset+1);
         guint16 be = tvb_get_guint8(tvb, offset+2);
         proto_item *pi = proto_tree_add_text (pil_tree, tvb, offset, 3+2*pe,
		   "Pilots %d offset %d block exponent %d", pe, po, be);
         proto_tree *p_tree = proto_item_add_subtree (pi, ett_rsci);
		 offset += 4;
		 for(j=0; j<pe; j++)
		 {
           guint16 i = tvb_get_ntohs(tvb, offset);
           guint16 q = tvb_get_ntohs(tvb, offset+2);
           proto_tree_add_text (p_tree, tvb, offset, 4, "I %d Q %d", i, q);
		   offset += 4;
		 }
	   }
     } else {
       proto_tree_add_text (tree, tvb, offset, bytes, "rpil");
	}
   }
}

static void
dissect_rwmf (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          float db = di_tvb_get_float(tvb, offset);
		  proto_tree_add_float_format_value(tree, hf_di_rwmf, tvb, offset, 2, db, "%f", db);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rwmf");
	}
   }
}

static void
dissect_rwmm (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          float db = di_tvb_get_float(tvb, offset);
		  proto_tree_add_float_format_value(tree, hf_di_rwmm, tvb, offset, 2, db, "%f", db);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rwmm");
	}
   }
}

static void
dissect_rmer (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          float db = di_tvb_get_float(tvb, offset);
		  proto_tree_add_float_format_value(tree, hf_di_rmer, tvb, offset, 2, db, "%f", db);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rmer");
	}
   }
}

static void
dissect_rbp(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, char stream)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          guint8 errs = tvb_get_guint8(tvb, offset);
          guint8 bits = tvb_get_guint8(tvb, offset+1);
          proto_tree_add_text (tree, tvb, offset, bytes, "str%c BER %f (%d/%d)", stream, ((float)errs)/((float)bits), errs, bits);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rbp%c", stream);
	}
   }
}

static void
dissect_rbp0 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  dissect_rbp(tvb, pinfo, tree, '0');
}

static void
dissect_rbp1 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  dissect_rbp(tvb, pinfo, tree, '1');
}

static void
dissect_rbp2 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  dissect_rbp(tvb, pinfo, tree, '2');
}

static void
dissect_rbp3 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  dissect_rbp(tvb, pinfo, tree, '3');
}

static void
dissect_rdel (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
		  int i;
          proto_item *ti = NULL;
          ti = proto_tree_add_text (tree, tvb, offset, bytes, "Delay: ");
		  for(i=0; i<bytes; i+=3)
		  {
            guint8 percent = tvb_get_guint8(tvb, offset+i);
            float dw = di_tvb_get_float(tvb, offset+i+1);
            proto_item_append_text (ti, "%d %f ", percent, dw);
		  }
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rdel");
	}
   }
}

static void
dissect_rdop(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    if(bytes>0) {
          float dp = di_tvb_get_float(tvb, offset);
		  proto_tree_add_float_format_value(tree, hf_di_rdop, tvb, offset, 2, dp, "%f", dp);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "rdop");
	}
   }
}

static void
dissect_rpsd (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    proto_item *ti = proto_tree_add_text(tree, tvb, offset, bytes, "Power Spectral Density");
    if(bytes>0) {
       proto_tree *p_tree = proto_item_add_subtree (ti, ett_rsci);
	   int i;
	   for(i=0; i<bytes; i++)
	   {
	     guint8 n = tvb_get_guint8(tvb, i);
         proto_tree_add_text(p_tree, tvb, i, 1, "%4.1f", 0.0 - ((float)n)/2.0);
	   }
    }
   }
}

static void
dissect_cact (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
    proto_tree_add_item (tree, hf_di_cact, tvb, offset, bytes, FALSE);
   }
}

static void
dissect_cfre (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
	    if(bytes>0) {
		  float f = ((float)tvb_get_ntohl(tvb, offset))/1000.0;
          proto_tree_add_float_format_value (tree, hf_di_cfre, tvb, offset, 4, f, "%9.3f kHz", f);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "cfre");
		}
   }
}

static void
dissect_cdmo (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
	    if(bytes>0) {
          proto_tree_add_item (tree, hf_di_cdmo, tvb, offset, bytes, FALSE);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "cdmo");
		}
   }
}

static void
dissect_cbws (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
	    if(bytes>0) {
          float dp = di_tvb_get_float(tvb, offset);
		  proto_tree_add_float_format_value(tree, hf_di_cbws, tvb, offset, 2, dp, "%f", dp);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "cbws");
   }
   }
}

static void
dissect_cbwg (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
	    if(bytes>0) {
          float dp = di_tvb_get_float(tvb, offset);
		  proto_tree_add_float_format_value(tree, hf_di_cbwg, tvb, offset, 2, dp, "%f", dp);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "cbwg");
   }
   }
}

static void
dissect_cser (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
	    if(bytes>0) {
          proto_tree_add_item (tree, hf_di_cser, tvb, offset, bytes, FALSE);
		} else {
   }
   }
}

static void
dissect_crec (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  guint bytes = tvb_length(tvb);
   if(tree) {
	    if(bytes>0) {
          proto_tree_add_item (tree, hf_di_crec, tvb, offset, bytes, FALSE);
		} else {
          proto_tree_add_text (tree, tvb, offset, 0, "crec");
   }
   }
}

void
register_rsci_dissectors (int proto)
{
  static int Initialized = FALSE;
  if (!Initialized) {
    dissector_add_string("drm-di.tag", "rpro", create_dissector_handle (dissect_rpro, proto));
    dissector_add_string("drm-di.tag", "fmjd", create_dissector_handle (dissect_fmjd, proto));
    dissector_add_string("drm-di.tag", "time", create_dissector_handle (dissect_time, proto));
    dissector_add_string("drm-di.tag", "rgps", create_dissector_handle (dissect_rgps, proto));
    dissector_add_string("drm-di.tag", "rdmo", create_dissector_handle (dissect_rdmo, proto));
    dissector_add_string("drm-di.tag", "rfre", create_dissector_handle (dissect_rfre, proto));
    dissector_add_string("drm-di.tag", "rdbv", create_dissector_handle (dissect_rdbv, proto));
    dissector_add_string("drm-di.tag", "rinf", create_dissector_handle (dissect_rinf, proto));
    dissector_add_string("drm-di.tag", "ract", create_dissector_handle (dissect_ract, proto));
    dissector_add_string("drm-di.tag", "rsta", create_dissector_handle (dissect_rsta, proto));
    dissector_add_string("drm-di.tag", "rbw_", create_dissector_handle (dissect_rbw_, proto));
    dissector_add_string("drm-di.tag", "rser", create_dissector_handle (dissect_rser, proto));
    dissector_add_string("drm-di.tag", "rtty", create_dissector_handle (dissect_rtty, proto));
    dissector_add_string("drm-di.tag", "rafs", create_dissector_handle (dissect_rafs, proto));
    dissector_add_string("drm-di.tag", "reas", create_dissector_handle (dissect_reas, proto));
    dissector_add_string("drm-di.tag", "rpil", create_dissector_handle (dissect_rpil, proto));
    dissector_add_string("drm-di.tag", "rwmf", create_dissector_handle (dissect_rwmf, proto));
    dissector_add_string("drm-di.tag", "rwmm", create_dissector_handle (dissect_rwmm, proto));
    dissector_add_string("drm-di.tag", "rmer", create_dissector_handle (dissect_rmer, proto));
    dissector_add_string("drm-di.tag", "rbp0", create_dissector_handle (dissect_rbp0, proto));
    dissector_add_string("drm-di.tag", "rbp1", create_dissector_handle (dissect_rbp1, proto));
    dissector_add_string("drm-di.tag", "rbp2", create_dissector_handle (dissect_rbp2, proto));
    dissector_add_string("drm-di.tag", "rbp3", create_dissector_handle (dissect_rbp3, proto));
    dissector_add_string("drm-di.tag", "rdel", create_dissector_handle (dissect_rdel, proto));
    dissector_add_string("drm-di.tag", "rdop", create_dissector_handle (dissect_rdop, proto));
    dissector_add_string("drm-di.tag", "rpsd", create_dissector_handle (dissect_rpsd, proto));
    dissector_add_string("drm-di.tag", "cact", create_dissector_handle (dissect_cact, proto));
    dissector_add_string("drm-di.tag", "cfre", create_dissector_handle (dissect_cfre, proto));
    dissector_add_string("drm-di.tag", "cdmo", create_dissector_handle (dissect_cdmo, proto));
    dissector_add_string("drm-di.tag", "cbws", create_dissector_handle (dissect_cbws, proto));
    dissector_add_string("drm-di.tag", "cbwg", create_dissector_handle (dissect_cbwg, proto));
    dissector_add_string("drm-di.tag", "cser", create_dissector_handle (dissect_cser, proto));
    dissector_add_string("drm-di.tag", "crec", create_dissector_handle (dissect_crec, proto));
  }
}

void
proto_register_drm_rsci (int proto)
{
  static hf_register_info hf[] = {
    {&hf_di_rpro,
     {"RSCI profile", "drm-di.rpro",
      FT_STRING, BASE_NONE, NULL, 0,
      "profile", HFILL}
     },
    {&hf_di_fmjd,
     {"Frac MJD", "drm-di.rpro",
      FT_BYTES, BASE_HEX, NULL, 0,
      "Fractional MJD", HFILL}
     },
    {&hf_di_time,
     {"time", "drm-di.time",
      FT_STRING, BASE_NONE, NULL, 0,
      "time", HFILL}
     },
    {&hf_di_rgps,
     {"GPS data", "drm-di.rgps",
      FT_BYTES, BASE_HEX, NULL, 0,
      "GPS Data", HFILL}
     },
    {&hf_di_rgps_source,
     {"GPS Source", "drm-di.rgps.source",
      FT_UINT8, BASE_DEC, VALS(gps_source_vals), 0,
      "GPS Source", HFILL}
     },
    {&hf_di_rgps_sats,
     {"Satellites", "drm-di.rgps.source",
      FT_UINT8, BASE_DEC, NULL, 0,
      "Satellites visible", HFILL}
     },
    {&hf_di_rgps_lat,
     {"Latitude", "drm-di.rgps.lat",
      FT_DOUBLE, BASE_DEC, NULL, 0,
      "Latitude", HFILL}
     },
    {&hf_di_rgps_long,
     {"Longitude", "drm-di.rgps.long",
      FT_DOUBLE, BASE_DEC, NULL, 0,
      "Longitude", HFILL}
     },
    {&hf_di_rgps_altitude,
     {"altitude", "drm-di.altitude",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "altitude (m)", HFILL}
     },
    {&hf_di_rgps_speed,
     {"speed", "drm-di.speed",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "speed (m/s)", HFILL}
     },
    {&hf_di_rgps_heading,
     {"heading", "drm-di.heading",
      FT_UINT16, BASE_DEC, NULL, 0,
      "heading (deg)", HFILL}
     },
    {&hf_di_rdbv,
     {"signal strength", "drm-di.rdbv",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "signal strength (dB)", HFILL}
     },
    {&hf_di_rinf,
     {"info", "drm-di.rinf",
      FT_STRING, BASE_NONE, NULL, 0,
      "Rx info", HFILL}
     },
    {&hf_di_ract,
     {"Rx Active Status", "drm-di.ract",
      FT_STRING, BASE_NONE, NULL, 0,
      "on/off status", HFILL}
     },
    {&hf_di_rsta,
     {"Rx Sync Status", "drm-di.rsta",
      FT_BYTES, BASE_HEX, NULL, 0,
      "Rx sync status", HFILL}
     },
    {&hf_di_rbw_,
     {"B/W", "drm-di.rbw_",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "Bandwidth", HFILL}
     },
    {&hf_di_rser,
     {"selected service", "drm-di.rser",
      FT_UINT8, BASE_DEC, NULL, 0,
      "selected service", HFILL}
     },
    {&hf_di_rtty,
     {"received test type", "drm-di.rtty",
      FT_BYTES, BASE_HEX, NULL, 0,
      "received test type", HFILL}
     },
    {&hf_di_rafs,
     {"audio status", "drm-di.rafs",
      FT_BYTES, BASE_HEX, NULL, 0,
      "audio status", HFILL}
     },
    {&hf_di_reas,
     {"extended audio status", "drm-di.reas",
      FT_BYTES, BASE_HEX, NULL, 0,
      "extended audio status", HFILL}
     },
    {&hf_di_rpil,
     {"pilots", "drm-di.rpil",
      FT_BYTES, BASE_HEX, NULL, 0,
      "pilots", HFILL}
     },
    {&hf_di_rpil_sn,
     {"symbols per frame", "drm-di.rpil.sn",
      FT_UINT8, BASE_DEC, NULL, 0,
      "symbols per frame", HFILL}
     },
    {&hf_di_rpil_sr,
     {"pilot pattern symbol repeat", "drm-di.rpil.sr",
      FT_UINT8, BASE_DEC, NULL, 0,
      "amount of symbols until the pilot pattern repeats", HFILL}
     },
    {&hf_di_rdmo,
     {"mode", "drm-di.rdmo",
      FT_STRING, BASE_NONE, NULL, 0,
      "current mode", HFILL}
     },
    {&hf_di_rfre,
     {"tuned frequency", "drm-di.rfre",
      FT_UINT32, BASE_DEC, NULL, 0,
      "current frequency in Hz", HFILL}
     },
    {&hf_di_rmer,
     {"MER", "drm-di.rmer",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "Modulation Error Rate", HFILL}
     },
    {&hf_di_rwmm,
     {"Weighted MER (MSC)", "drm-di.rwmm",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "Weighted MER based on MSC", HFILL}
     },
    {&hf_di_rwmf,
     {"Weighted MER (FAC)", "drm-di.rwmf",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "Weighted MER based on FAC", HFILL}
     },
    {&hf_di_rdop,
     {"doppler", "drm-di.rdop",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "Doppler", HFILL}
     },
    {&hf_di_rpsd,
     {"PSD", "drm-di.rpsd",
      FT_BYTES, BASE_HEX, NULL, 0,
      "Power Spectral Density", HFILL}
     },
    {&hf_di_cfre,
     {"set frequency to", "drm-di.cfre",
      FT_FLOAT, BASE_DEC, NULL, 0,
      "required frequency in kHz", HFILL}
     },
    {&hf_di_cdmo,
     {"mode command", "drm-di.cdmo",
      FT_STRING, BASE_NONE, NULL, 0,
      "required mode", HFILL}
     },
    {&hf_di_cact,
     {"on/off command", "drm-di.cact",
      FT_STRING, BASE_NONE, NULL, 0,
      "required on/off state", HFILL}
     },
    {&hf_di_cbws,
     {" command", "drm-di.cbws",
      FT_STRING, BASE_NONE, NULL, 0,
      "required on/off state", HFILL}
     },
    {&hf_di_cbwg,
     {"on/off command", "drm-di.cbwg",
      FT_STRING, BASE_NONE, NULL, 0,
      "required on/off state", HFILL}
     },
    {&hf_di_cser,
     {"required service", "drm-di.cser",
      FT_UINT8, BASE_DEC, NULL, 0,
      "service command", HFILL}
     },
    {&hf_di_crec,
     {"RSCI record command", "drm-di.crec",
      FT_STRING, BASE_NONE, NULL, 0,
      "required on/off state", HFILL}
     }
  };

  /* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_rsci
  };

  proto_register_field_array (proto, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
}

