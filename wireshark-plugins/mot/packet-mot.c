/* packet-mot.c
 * Routines for ETSI DAB/DRM MOT Protocol
 * Copyright 2006, British Broadcasting Corporation 
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
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
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gmodule.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/reassemble.h>
#include <string.h>

/* forward reference */
void proto_register_mot ();
void proto_reg_handoff_mot ();
static gboolean dissect_mot (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree);
static gboolean dissect_data_group (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree);
//static gboolean dissect_data_group_type_6 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree);

/* Define version if we are not building ethereal statically */
#ifndef ENABLE_STATIC
G_MODULE_EXPORT const gchar version[] = "0.2";
#endif

static int proto_mot = -1;
static int hf_mot_fragments = -1;
static int hf_mot_fragment = -1;
static int hf_mot_fragment_overlap = -1;
static int hf_mot_fragment_overlap_conflicts = -1;
static int hf_mot_fragment_multiple_tails = -1;
static int hf_mot_fragment_too_long_fragment = -1;
static int hf_mot_fragment_error = -1;
static int hf_mot_reassembled_in = -1;

/* Initialize the subtree pointers */
static gint ett_mot = -1;
static gint ett_mot_fragment = -1;
static gint ett_mot_fragments = -1;
static gint ett_mot_payload = -1;

static GHashTable *mot_fragment_table = NULL;
static GHashTable *mot_reassembled_table = NULL;

#ifndef ENABLE_STATIC
/** Register the protocol this plugin dissects. Details follow
 *  here.
 */
G_MODULE_EXPORT void
plugin_register (void)
{
/* register the new protocol, protocol fields, and subtrees */
  if (proto_mot == -1) {	/* execute protocol initialization only once */
    proto_register_mot ();
  }
}

/** Register the handoff routine. Details follow
 *  here.
 */
G_MODULE_EXPORT void
plugin_reg_handoff (void)
{
  proto_reg_handoff_mot ();
}
#endif

static const fragment_items mot_frag_items = {
/* Fragment subtrees */
  &ett_mot_fragment,
  &ett_mot_fragments,
/* Fragment fields */
  &hf_mot_fragments,
  &hf_mot_fragment,
  &hf_mot_fragment_overlap,
  &hf_mot_fragment_overlap_conflicts,
  &hf_mot_fragment_multiple_tails,
  &hf_mot_fragment_too_long_fragment,
  &hf_mot_fragment_error,
/* Reassembled in field */
  &hf_mot_reassembled_in,
/* Tag */
  "Message fragments"
};

/** initialise the MOT protocol. Details follow
 *  here.
 */
static void
mot_init_protocol(void)
{
  fragment_table_init (&mot_fragment_table);
  reassembled_table_init (&mot_reassembled_table);
}

static int hf_ext_flag = -1;
static int hf_crc_flag = -1;
static int hf_seg_flag = -1;
static int hf_ua_flag = -1;
static int hf_data_group_type = -1;
static int hf_ci = -1;
static int hf_ri = -1;
static int hf_ext_field = -1;
static int hf_seglast = -1;
static int hf_segnum = -1;
static int hf_dg_data = -1;
static int hf_dg_crc = -1;
static int hf_uafrfa = -1;
static int hf_uaftid_flag = -1;
static int hf_uafli = -1;
static int hf_uaftid = -1;
static int hf_uafadr = -1;
static int hf_dir_cf = -1;
static int hf_dir_size = -1;
static int hf_dir_no_objects = -1;
static int hf_dir_period = -1;
static int hf_dir_segsize = -1;
static int hf_dir_ext_len = -1;
static int hf_seg_rep = -1;
static int hf_seg_size = -1;
  
void
proto_register_mot (void)
{
  module_t *mot_module;
  static hf_register_info hf_mot[] = {
    {&hf_ext_flag,
     {"Extension Flag", "mot.dg.extflag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x80,
      "Extension field presence", HFILL}
     },
    {&hf_crc_flag,
     {"CRC Flag", "mot.dg.crcflag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x40,
      "CRC presence", HFILL}
     },
    {&hf_seg_flag,
     {"Segment Flag", "mot.dg.segflag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x20,
      "Segmented", HFILL}
     },
    {&hf_ua_flag,
     {"User access flag", "mot.dg.uaflag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x10,
      "user access field presence", HFILL}
     },
    {&hf_data_group_type,
     {"data group type", "mot.dg.type",
      FT_UINT8, BASE_DEC, NULL, 0x0f,
      "type of data group", HFILL}
     },
    {&hf_ci,
     {"continuity index", "mot.dg.ci",
      FT_UINT8, BASE_DEC, NULL, 0xf0,
      "sequence no", HFILL}
     },
    {&hf_ri,
     {"repetition index", "mot.dg.ri",
      FT_UINT8, BASE_DEC, NULL, 0x0f,
      "repeat count", HFILL}
     },
    {&hf_ext_field,
     {"extension field", "mot.dg.ext",
      FT_UINT16, BASE_HEX, NULL, 0x00,
      "ext field", HFILL}
     },
    {&hf_segnum,
     {"Segment", "mot.dg.session.segment",
      FT_UINT8, BASE_DEC, NULL, 0x7fff,
      "segment number", HFILL}
     },
    {&hf_seglast,
     {"last", "mot.dg.session.last_segment",
      FT_BOOLEAN, BASE_NONE, NULL, 0x8000,
      "last segment", HFILL}
     },
    {&hf_uafrfa,
     {"rfa", "mot.dg.uaf.rfa",
      FT_UINT8, BASE_HEX, NULL, 0xe0,
      "reserved", HFILL}
     },
    {&hf_uaftid_flag,
     {"tid flag", "mot.dg.uaf.tid_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x10,
      "transport id present flag", HFILL}
     },
    {&hf_uafli,
     {"length", "mot.dg.uaf.li",
      FT_UINT8, BASE_DEC, NULL, 0x0f,
      "uaf length", HFILL}
     },
    {&hf_uaftid,
     {"tid", "mot.dg.uaf.tid",
      FT_UINT16, BASE_HEX, NULL, 0,
      "transport id", HFILL}
     },
    {&hf_uafadr,
     {"address", "mot.dg.uaf.address",
      FT_BYTES, BASE_HEX, NULL, 0,
      "end user address field", HFILL}
     },
    {&hf_dg_data,
     {"data", "mot.dg.data",
      FT_BYTES, BASE_NONE, NULL, 0x8000,
      "data field", HFILL}
     },
    {&hf_dg_crc,
     {"CRC", "mot.dg.crc",
      FT_UINT16, BASE_HEX, NULL, 0x00,
      "crc field", HFILL}
     },
    {&hf_dir_cf,
     {"compression flag", "mot.dir.cf",
      FT_BOOLEAN, BASE_NONE, NULL, 0x80,
      "compression flag", HFILL}
     },
    {&hf_dir_size,
     {"size", "mot.dir.size",
      FT_UINT32, BASE_DEC, NULL, 0x3FFFFFFF,
      "directory size", HFILL}
     },
    {&hf_dir_no_objects,
     {"no of objects", "mot.dir.count",
      FT_UINT16, BASE_DEC, NULL, 0,
      "no of objects", HFILL}
     },
    {&hf_dir_period,
     {"period", "mot.dir.period",
      FT_UINT24, BASE_DEC, NULL, 0,
      "carousel period", HFILL}
     },
    {&hf_dir_segsize,
     {"segment size", "mot.dir.segsize",
      FT_UINT16, BASE_DEC, NULL, 0x1FFF,
      "segment size", HFILL}
     },
    {&hf_dir_ext_len,
     {"extension length", "mot.dir.extlen",
      FT_UINT16, BASE_DEC, NULL, 0,
      "extension length", HFILL}
     },
    {&hf_seg_rep,
     {"repetition count", "mot.segmentation.rep",
      FT_UINT16, BASE_DEC, NULL, 0xE000,
      "repetition count", HFILL}
     },
    {&hf_seg_size,
     {"segment size", "mot.segmentation.segsize",
      FT_UINT16, BASE_DEC, NULL, 0x1FFF,
      "segment size", HFILL}
     }
    };

/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_mot,
    &ett_mot_fragment,
    &ett_mot_fragments,
    &ett_mot_payload
  };

  if (proto_mot == -1) {
    proto_mot = proto_register_protocol ("Multimedia Object Transfer",	/* name */
	   "MOT",	/* short name */
	   "mot"	/* abbrev */
      );
  }
  mot_module = prefs_register_protocol (proto_mot, proto_reg_handoff_mot);
  proto_register_field_array (proto_mot, hf_mot, array_length (hf_mot));
  proto_register_subtree_array (ett, array_length (ett));

  /* subdissector code */


  register_init_routine(mot_init_protocol);

}

void
proto_reg_handoff_mot (void)
{
  static int Initialized = FALSE;
  if (!Initialized) {
	heur_dissector_add("drm-packet-mode", dissect_mot, proto_mot);
	Initialized = TRUE;
   }
}

static int
dissect_extension_header (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree,
guint16 start_offset)
{
  guint offset = start_offset;
  guint8 header = tvb_get_guint8 (tvb, offset++);
  guint8 paramid = header & 0x3f;
  guint16 datalen;
  switch(header>>6) {
  case 0:
  	   datalen = 0;
  	   break;
  case 1:
  	   datalen = 1;
  	   break;
  case 2:
  	   datalen = 4;
  	   break;
  case 3:
       datalen = tvb_get_guint8 (tvb, offset);
       if(datalen&0x80) {
         datalen = tvb_get_ntohs (tvb, offset) & 0x7fff;
 	     offset+=2;
	   } else {
 	     offset++;
       }
  }
  offset += datalen;
  if (tree) {			/* we are being asked for details */
    proto_tree_add_text(tree, tvb, start_offset, offset-start_offset, "parameter 0x%x with %d byte value", paramid, datalen);    
  }
  return offset;
}

static void
dissect_dg3 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
}

static void
dissect_dg4 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
}

static void
dissect_dg5 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
}

static void
dissect_dg6 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  if (tree) {			/* we are being asked for details */
    guint16 ext_len;
    proto_tree *dir_tree = NULL;
    proto_item *dir_ti = proto_tree_add_text (tree, tvb, 0, -1, "MOT Directory");
    dir_tree = proto_item_add_subtree (dir_ti, ett_mot);
    offset=0;
    proto_tree_add_item(dir_tree, hf_dir_cf, tvb, offset, 1, FALSE);
    proto_tree_add_item(dir_tree, hf_dir_size, tvb, offset, 4, FALSE);
    offset += 4;
    proto_tree_add_item(dir_tree, hf_dir_no_objects, tvb, offset, 2, FALSE);
    offset += 2;
    proto_tree_add_item(dir_tree, hf_dir_period, tvb, offset, 3, FALSE);
    offset += 3;
    proto_tree_add_item(dir_tree, hf_dir_segsize, tvb, offset, 2, FALSE);
    offset += 2;
    proto_tree_add_item(dir_tree, hf_dir_ext_len, tvb, offset, 2, FALSE);
    ext_len = tvb_get_ntohs (tvb, offset);
    offset += 2;
    if(ext_len>0) {
      guint16 n=offset;
      offset += ext_len;
      while(n<offset)
        n = dissect_extension_header(tvb, pinfo, dir_tree, n);
    }
  }
}

static gboolean
dissect_data_group (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint offset = 0;
  /* data group header EN 300 401 para 5.3.3.1 */
  guint8 header0 = tvb_get_guint8 (tvb, offset++);
  gboolean extflag = (header0>>7)&1;
  gboolean crcflag = (header0>>6)&1;
  gboolean segflag = (header0>>5)&1;
  gboolean uaflag = (header0>>4)&1;
  guint8 dgtype = header0&0xf;
  guint8 header1 = tvb_get_guint8 (tvb, offset++);
  guint8 ci = (header1>>4)&0xf;
  guint8 ri = header1&0xf;
  guint16 extension=0;
  tvbuff_t * new_tvb;
  if(extflag) {
    extension = tvb_get_ntohs (tvb, offset);
    offset += 2;
  }
  if (tree) {			/* we are being asked for details */
    proto_tree *dg_tree = NULL;
    proto_item *dg_ti = proto_tree_add_text (tree, tvb, 0, -1, "Data Group type %d", dgtype);
    dg_tree = proto_item_add_subtree (dg_ti, ett_mot);
    offset=0;
    proto_tree_add_item(dg_tree, hf_ext_flag, tvb, offset, 1, FALSE);
    proto_tree_add_item(dg_tree, hf_crc_flag, tvb, offset, 1, FALSE);
    proto_tree_add_item(dg_tree, hf_seg_flag, tvb, offset, 1, FALSE);
    proto_tree_add_item(dg_tree, hf_ua_flag, tvb, offset, 1, FALSE);
    proto_tree_add_item(dg_tree, hf_data_group_type, tvb, offset, 1, FALSE);
    offset++;
    proto_tree_add_item(dg_tree, hf_ci, tvb, offset, 1, FALSE);
    proto_tree_add_item(dg_tree, hf_ri, tvb, offset, 1, FALSE);
    offset++;
    if(extflag) {
      proto_tree_add_item(dg_tree, hf_ext_field, tvb, offset, 2, FALSE);
      offset += 2;
    }
    //proto_item_append_text(ti, " ext %d crc %d seg %d ua %d ci %d ri %d", extflag, crcflag, segflag, uaflag, ci, ri);
    if(segflag || uaflag) {
      proto_item *ti;
      proto_tree *sess_tree;
      guint8 hdr=0;
      guint16 li=0,len=0;
      if(segflag) {
        len=2;
      }
      if(uaflag) {
	    hdr = tvb_get_guint8 (tvb, offset+len);
	    li = hdr & 0x0f;
	    len = len+1+li;
      }
      ti = proto_tree_add_text (dg_tree, tvb, offset, len, "Session Header");
      sess_tree = proto_item_add_subtree (ti, ett_mot);
      if(segflag) {
        guint16 segnum = tvb_get_ntohs (tvb, offset);
        gboolean lastseg = (segnum&0x8000)?TRUE:FALSE;
        segnum = segnum & 0x7fff;
        proto_tree_add_item(sess_tree, hf_seglast, tvb, offset, 2, FALSE);
        proto_tree_add_item(sess_tree, hf_segnum, tvb, offset, 2, FALSE);
        offset += 2;
        proto_item_append_text(dg_ti, " seg %d", segnum);
      }
      if(uaflag) {
        proto_tree_add_item(sess_tree, hf_uafrfa, tvb, offset, 1, FALSE);
        proto_tree_add_item(sess_tree, hf_uaftid_flag, tvb, offset, 1, FALSE);
        proto_tree_add_item(sess_tree, hf_uafli, tvb, offset, 1, FALSE);
        offset++;
        if(hdr&0x10) {
          guint16 tid = tvb_get_ntohs (tvb, offset);
          proto_tree_add_item(sess_tree, hf_uaftid, tvb, offset, 2, FALSE);
          offset += 2;
          li -= 2;
          proto_item_append_text(dg_ti, " transport id %d", tid);
 	    }
 	    if(li>0)
          proto_tree_add_item(sess_tree, hf_uafadr, tvb, offset, li, FALSE);
        offset += li;
      }
    }
    proto_tree_add_item(dg_tree, hf_seg_rep, tvb, offset, 2, FALSE);
    proto_tree_add_item(dg_tree, hf_seg_size, tvb, offset, 2, FALSE);
    offset += 2;
  }
  if(crcflag) {
    guint payload_len = tvb_length(tvb) - offset - 2;	
    if(tree)
      proto_tree_add_item(tree, hf_dg_data, tvb, offset, payload_len, FALSE);
    new_tvb = tvb_new_subset (tvb, offset, payload_len, payload_len);
    offset += payload_len;
    if(tree)
      proto_tree_add_item(tree, hf_dg_crc, tvb, offset, 2, FALSE);
  } else {
    new_tvb = tvb_new_subset (tvb, offset, -1, -1);
    if(tree)
      proto_tree_add_item(tree, hf_dg_data, tvb, offset, -1, FALSE);
  }
  switch(dgtype) {
	 case 6:
	 	  dissect_dg6 (new_tvb, pinfo, tree);
	 	  break;
     default:
	 		 ;
  }
  return TRUE;
}

static gboolean
dissect_mot (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *mot_tree = NULL;
  if (tree) {			/* we are being asked for details */
    proto_item *ti = proto_tree_add_text (tree, tvb, 0, -1, "Multimedia Object Transfer (MOT)");
    mot_tree = proto_item_add_subtree (ti, ett_mot);
  }
  dissect_data_group (tvb, pinfo, mot_tree);
  return TRUE;
}
