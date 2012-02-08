/* packet-drm-msc-data.c
 * Routines for Digital Radio Mondiale
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
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gmodule.h>
#include <epan/emem.h>
#include <epan/prefs.h>
#include <epan/crcdrm.h>
#include <string.h>
#include "packet-drm-di.h"
#include "drm-data.h"
#include "reassemble2.h"

int proto_drm_packet_mode = -1;

gboolean
dissect_msc_packet_data (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree);

struct packet_data_t {
	int seq_no;
};

struct packet_flow_data_t {
  guint seq_no[256];
  guint8* du;
  guint du_size;
};

struct packet_stream_data_t {
  gboolean visited;
  struct packet_flow_data_t flow[4];
};

struct packet_frame_data_t {
  struct packet_stream_data_t stream[4];
};


static reassembler reassembler_object[4][4];
static guint8 packet_seq[4][4];

/* Initialize the subtree pointers */
static gint ett_msc_data = -1;
static gint ett_msc_data_payload = -1;

static heur_dissector_list_t msc_packet_heur_list = NULL;

static int hf_packet_first = -1;
static int hf_packet_last = -1;
static int hf_packet_id = -1;
static int hf_packet_ppi = -1;
static int hf_packet_ci = -1;
static int hf_packet_len = -1;
static int hf_packet_crc = -1;
static int hf_packet_data = -1;
static int hf_packet_padding = -1;

void
proto_reg_handoff_drm_packet_mode (void)
{
  static int Initialized = FALSE;
  if (!Initialized) {
	heur_dissector_add("drm-msc.str0.data", dissect_msc_packet_data, proto_drm_packet_mode);
	heur_dissector_add("drm-msc.str1.data", dissect_msc_packet_data, proto_drm_packet_mode);
	heur_dissector_add("drm-msc.str2.data", dissect_msc_packet_data, proto_drm_packet_mode);
	heur_dissector_add("drm-msc.str3.data", dissect_msc_packet_data, proto_drm_packet_mode);
	Initialized = TRUE;
  }
}

void
proto_register_drm_packet_mode (void)
{
  static hf_register_info hf[] = {
    {&hf_packet_first,
     {"first packet flag", "drm-packet.first_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x80,
      "packet is first in a sequence", HFILL}
     },
    {&hf_packet_last,
     {"last packet flag", "drm-packet.last_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x40,
      "packet is last in a sequence", HFILL}
     },
    {&hf_packet_id,
     {"id", "drm-packet.id",
      FT_UINT8, BASE_DEC, NULL, 0x30,
      "packet id", HFILL}
     },
    {&hf_packet_ppi,
     {"padded packet indicator", "drm-packet.ppi_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x08,
      "packet is shorted than max", HFILL}
     },
    {&hf_packet_ci,
     {"continuity indicator", "drm-packet.ci",
      FT_UINT8, BASE_DEC, NULL, 0x7,
      "Packet Sequence Number", HFILL}
     },
    {&hf_packet_len,
     {"length", "drm-packet.len",
      FT_UINT8, BASE_DEC, NULL, 0,
      "length in bytes of the payload", HFILL}
     },
    {&hf_packet_data,
      {"data", "drm-packet.data",
       FT_BYTES, BASE_HEX, NULL, 0,
       "data", HFILL}
     },
    {&hf_packet_padding,
      {"padding", "drm-packet.padding",
       FT_BYTES, BASE_HEX, NULL, 0,
       "padding", HFILL}
     },
    {&hf_packet_crc,
     {"crc", "drm-packet.crc",
      FT_UINT16, BASE_HEX, NULL, 0,
      "crc", HFILL}
     }
    };

/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_msc_data,
    &ett_msc_data_payload
  };

  if (proto_drm_packet_mode == -1) {
    proto_drm_packet_mode = proto_register_protocol ("DRM Packet Mode",	/* name */
					 "DRM-Packet-Mode",	/* short name */
					 "drm-packet-mode"	/* abbrev */
      );
  }
  proto_register_field_array (proto_drm_packet_mode, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));

  /* subdissector code */

  register_heur_dissector_list("drm-packet-mode", &msc_packet_heur_list);

}

/*
6.6.1 Packet structure
The packet is made up as follows:
- header 8 bits.
- data field n bytes.
- CRC 16 bits.
The header contains information to describe the packet.
The data field contains the data intended for a particular service. The length of the data field is indicated by use of data
entity 5, see clause 6.4.3.6.
Cyclic Redundancy Check (CRC): this 16-bit CRC shall be calculated on the header and the data field. It shall use the
generator polynomial G16 (x) = x16 + x12 + x5 + 1 (see annex D).
6.6.1.1 Header
The header consists of the following fields:
- first flag 1 bit.
- last flag 1 bit.
- packet Id 2 bits.
- Padded Packet Indicator (PPI) 1 bit.
- Continuity Index (CI) 3 bits.
The following definitions apply:
First flag, Last flag: these flags are used to identify particular packets which form a succession of packets. The flags
are assigned as follows:
First
flag
Last
flag
The packet is:
0 0 : an intermediate packet;
0 1 : the last packet of a data unit;
1 0 : the first packet of a data unit;
1 1 : the one and only packet of a data unit.
Packet Id: this 2-bit field indicates the Packet Id of this packet.
Padded Packet Indicator: this 1-bit flag indicates whether the data field carries padding or not, as follows:
0: no padding is present: all data bytes in the data field are useful;
1: padding is present: the first byte gives the number of useful data bytes in the data field.
Continuity index: this 3-bit field shall increment by one modulo-8 for each packet with this packet Id.
*/


static void dissect_one_packet (tvbuff_t * tvb, packet_info * pinfo,
 struct packet_stream_data_t *pd, guint packet_no,
 proto_tree * tree)
{
  drm_data_t *data = (drm_data_t*)pinfo->private_data;
  gint s = data->stream;
  stream_t *stream_data = &data->stream_data[s];
  guint8 header = tvb_get_guint8 (tvb, 0);
  gboolean first = (header & 0x80)?1:0;
  gboolean last = (header & 0x40)?1:0;
  guint8 id = (header & 0x30) >> 4;
  gboolean ppi = (header & 0x08)?1:0;
  guint pad_bytes = 0;
  guint8 ci = header & 0x07;
  guint offset=0;
  guint du_size=0;
  guint payload_start, payload_len;
  proto_tree *msc_payload_tree = NULL;
  proto_item *ti;
  proto_item *ri;
  tvbuff_t *new_tvb = NULL;
  const gint len = tvb_length(tvb);
  const guint8 *crc_buf = tvb_get_ptr(tvb, 0, len);
  guint16 crc = crc_drm(crc_buf, len, 16, 0x11021, 1);

  if(ppi) {
    payload_start = 2;
    payload_len = tvb_get_guint8 (tvb, 1);
    pad_bytes = stream_data->packet_length - payload_len - 1; /* -1 for the length byte! */
  } else {
    payload_start = 1;
    payload_len = stream_data->packet_length;
  }
  if (tree) {
    ti = proto_tree_add_text (tree, tvb, 0, -1,
            "packet with id=%d, first=%d last=%d ci=%d length=%d",
            id, first, last, ci, payload_len);
    msc_payload_tree = proto_item_add_subtree (ti, ett_msc_data_payload);
    proto_tree_add_item (msc_payload_tree, hf_packet_first, tvb, offset, 1, FALSE);
    proto_tree_add_item (msc_payload_tree, hf_packet_last, tvb, offset, 1, FALSE);
    proto_tree_add_item (msc_payload_tree, hf_packet_id, tvb, offset, 1, FALSE);
    proto_tree_add_item (msc_payload_tree, hf_packet_ppi, tvb, offset, 1, FALSE);
    proto_tree_add_item (msc_payload_tree, hf_packet_ci, tvb, offset, 1, FALSE);
    offset++;
    if(ppi) {
      proto_tree_add_item (msc_payload_tree, hf_packet_len, tvb, offset, 1, FALSE);
      offset++;
    } else {
      proto_tree_add_uint (msc_payload_tree, hf_packet_len, tvb, 0, 1, payload_len);
    }
    proto_tree_add_item (msc_payload_tree, hf_packet_data, tvb, offset, payload_len, FALSE);
    offset += payload_len;
    if(pad_bytes>0) {
      proto_tree_add_item (msc_payload_tree, hf_packet_padding, tvb, offset, pad_bytes, FALSE);
      offset += pad_bytes;
    }
    ri = proto_tree_add_item (msc_payload_tree, hf_packet_crc, tvb, offset, 2, FALSE);
    proto_item_append_text(ri, " (%s)", (crc==0xe2f0)?"Ok":"bad");
  }
  if(payload_len>0) {
    if(stream_data->data_services[id].data_unit_indicator == 0) {
      new_tvb = tvb_new_subset (tvb, payload_start, payload_len, payload_len);
      du_size = payload_len;
    } else {
	  guint seq_no;
	  if(pd->visited) {
		seq_no = pd->flow[id].seq_no[packet_no];
  	  } else {
	    if(first) {
		  packet_seq[s][id]=0;
	    } else {
		  packet_seq[s][id]++;
	    }
		seq_no = packet_seq[s][id];
		pd->flow[id].seq_no[packet_no] = seq_no;
	  }
      if(first && last) {
        new_tvb = tvb_new_subset (tvb, payload_start, payload_len, payload_len);
        du_size = payload_len;
      } else {
		guint8* du = NULL;
        if(pd->visited) {
		  if(last) {
		    du = pd->flow[id].du;
		    du_size = pd->flow[id].du_size;
 		  }
		} else {
	      reassembler r=NULL;
	      /* reassemble packets into data units */
		  if(first) {
			reassembler_object[s][id] = reassembler_new();
		  }
		  r = reassembler_object[s][id];
		  if(r) {
		    reassembler_add_segment(r, tvb_get_ptr(tvb, payload_start, payload_len), payload_len, seq_no, last);
		    if(last) {
			  if(reassembler_ready(r)) {
			    du_size = reassembler_reassembled_size(r);
			    du = se_alloc(du_size);
			    (void)reassembler_reassemble(r, du, du_size);
			    reassembler_delete(r);
			    reassembler_object[s][id]=NULL;
		        pd->flow[id].du = du;
		        pd->flow[id].du_size = du_size;
		      }
		    }
		  } else {
            if (check_col (pinfo->cinfo, COL_INFO))
              col_append_fstr (pinfo->cinfo, COL_INFO, " no reassembler object");
 		  }
		}
        if(du) {
		  new_tvb = tvb_new_real_data(du, du_size, du_size);
		  tvb_set_child_real_data_tvbuff(tvb, new_tvb);
    	  add_new_data_source(pinfo, new_tvb, "Reassembled Data Unit");
		}
      }
    }
  }
  fflush(stdout);

  if(new_tvb) {
    gboolean dissected;
    proto_tree *ptree;
    if(tree)
      ptree = tree->parent->parent;
    else
      ptree = NULL;
	pinfo->circuit_id = s << 2 & id;
    dissected = dissector_try_heuristic(msc_packet_heur_list, new_tvb, pinfo, ptree);
	if(!dissected)
      proto_tree_add_text (ptree, new_tvb, 0, -1,
	 		 "new DU len %d stream %d packet_id %d",
	  		tvb_length(new_tvb), s, id);
  }
}

gboolean
dissect_msc_packet_data (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  int packet_len;
  guint offset = 0;
  guint packet_no = 0;
  proto_item *ti = NULL;
  proto_tree *packet_tree = NULL;
  drm_data_t *data = (drm_data_t*)pinfo->private_data;
  gint s = data->stream;
  if(s<0 || s>3 || data->stream_data[s].valid==FALSE || data->stream_data[s].audio_not_data==TRUE)
    return FALSE;
  if(!data->stream_data[s].packet_mode_indicator)
    return FALSE;

  if (tree) {			/* we are being asked for details */
      ti = proto_tree_add_text(tree, tvb, 0, -1, "DRM data in stream %d", s);
      packet_tree = proto_item_add_subtree (ti, ett_msc_data);
  }
  struct packet_frame_data_t *pd = p_get_proto_data(pinfo->fd, proto_drm_packet_mode);
  if(pd==NULL) {
    pd = (struct packet_frame_data_t*)se_alloc0(sizeof(struct packet_frame_data_t));
  }
  packet_len = data->stream_data[s].packet_length+3;
  if (tree)
    proto_item_append_text(ti, " %d byte packets", packet_len);
  while((offset+packet_len)<=tvb_length(tvb)) {
    tvbuff_t *new_tvb = tvb_new_subset (tvb, offset, packet_len, packet_len);
    dissect_one_packet(new_tvb, pinfo, &pd->stream[s], packet_no, packet_tree);
    offset += packet_len;
    packet_no++;
  }
  pd->stream[s].visited = TRUE;
  p_add_proto_data(pinfo->fd, proto_drm_packet_mode, pd);
  return TRUE;
}
