/* packet-drm-di.c
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
 *
 * Protocol info
 * Ref: ETSI DRM MDI (ETSI TS 102 820)
 *      ETSI DRM RSCI (ETSI TS 102 349)
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gmodule.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/conversation.h>
#include <string.h>
#include "packet-drm-di.h"
#include "drm-data.h"

/* Define version if we are not building wireshark statically */
#ifndef ENABLE_STATIC
G_MODULE_EXPORT const gchar version[] = "0.3";
#endif

void MjdToDate (guint32 Mjd, guint32 *Year, guint32 *Month, guint32 *Day);

void register_msc_dissectors(int);
void register_sdc_dissectors(int);
void register_rsci_dissectors(int);
void dissect_fac (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree);
/* forward reference */
static void dissect_di (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree);

static dissector_table_t di_dissector_table;

int proto_drm_di = -1;

static dissector_handle_t di_handle;
static dissector_handle_t fac_handle;

static int hf_di_tlv = -1;
static int hf_di_robm = -1;
static int hf_di_utco = -1;
static int hf_di_tist = -1;
static int hf_di_tist_s = -1;
static int hf_di_tist_ms = -1;
static int hf_di_dlfc = -1;

/* Initialize the subtree pointers */
static gint ett_di = -1;

static const value_string robm_vals[] = {
	{ 0x0,	"Mode A" },
	{ 0x1,	"Mode B" },
	{ 0x2,	"Mode C" },
	{ 0x3,	"Mode D" },
	{ 0,	NULL }
};

static void
dissect_di (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *di_tree = NULL;
  guint offset=0;
  guint8 *prot;
  proto_item *ti = NULL;

  conversation_t *conversation;
  void* old_data;
  drm_data_t *data;
  drm_data_t data2;

  /* look up the conversation */

  conversation = find_conversation(pinfo->fd->num, &pinfo->src, &pinfo->dst,
	pinfo->ptype, pinfo->srcport, pinfo->destport, 0);

  /* if conversation found get the data pointer that you stored */
  if ( conversation) {
    data = (drm_data_t*)conversation_get_proto_data(conversation,
    	    proto_drm_di);
	if(data==NULL)
		data = &data2;
  } else {

    /* new conversation create local data structure */

    data = malloc(sizeof(drm_data_t));

    /*** add your code here to setup the new data structure ***/
	memset(data, 0, sizeof(drm_data_t));

    /* create the conversation with your data pointer  */

    conversation = conversation_new(
	    pinfo->fd->num,  &pinfo->src, &pinfo->dst, pinfo->ptype,
	    pinfo->srcport, pinfo->destport, 0
    );
    conversation_add_proto_data(conversation, proto_drm_di, (void *) data);
  }

  pinfo->current_proto = "DRM-DI";
  old_data = pinfo->private_data;
  pinfo->private_data = data;

  /* Clear out stuff in the info column */
  if (check_col (pinfo->cinfo, COL_INFO)) {
    col_clear (pinfo->cinfo, COL_INFO);
  }

  if (check_col (pinfo->cinfo, COL_PROTOCOL))
    col_add_str (pinfo->cinfo, COL_PROTOCOL, "DRM-DI");

  if(data==NULL && check_col (pinfo->cinfo, COL_PROTOCOL))
    col_add_str (pinfo->cinfo, COL_PROTOCOL, " bad data pointer");

  if(tree) {
    ti = proto_tree_add_item (tree, proto_drm_di, tvb, 0, -1, FALSE);
    di_tree = proto_item_add_subtree (ti, ett_di);
  }
  // let the sub-dissectors fill in our data structure
  while(offset<tvb_length(tvb)) {
    char *tag = (char*)tvb_get_string (tvb, offset, 4); offset += 4;
    guint32 bits = tvb_get_ntohl(tvb, offset); offset += 4;
    guint32 bytes = bits / 8;
    if(bits % 8)
      bytes++;
    if(strcmp(tag, "*ptr")==0) {
      data->maj = tvb_get_ntohs(tvb, offset+4);
      data->min = tvb_get_ntohs(tvb, offset+6);
      if(ti) {
        prot = tvb_get_string (tvb, offset, 4);
        proto_item_append_text(ti, " %s rev %d.%d", prot, data->maj, data->min);
      }
    } else if(strcmp(tag, "robm")==0) {
	  if(bytes==1) {
        data->robm = tvb_get_guint8(tvb, offset);
        if(di_tree) {
          proto_tree_add_item (di_tree, hf_di_robm, tvb, offset, 1, FALSE);
        }
	  } else {
        if(di_tree) {
          proto_tree_add_text (di_tree, tvb, offset, 0, "robm (not synced)");
		}
	  }
    } else if(strcmp(tag, "tist")==0) {
      if(di_tree) {
        guint64 seconds = tvb_get_ntoh64(tvb, offset);
        guint16 ms = seconds & 0x3ff;
        char s[255];
        struct tm ts;
        seconds = (seconds >> 10) & 0xffffffffffULL;
        data->tist.secs = seconds;
        data->tist.nsecs = 1000000UL*ms;
        proto_tree_add_item (di_tree, hf_di_utco, tvb, offset, 2, FALSE);
        proto_tree_add_time (di_tree, hf_di_tist, tvb, offset, 8, &data->tist);
        time_t epoch = 946684800L + data->tist.secs;
        ts = *gmtime(&epoch);
        sprintf(s, "tist %04d-%02d-%02dT%02d:%02d:%02d.%dZ",
		ts.tm_year+1900, ts.tm_mon+1, ts.tm_mday,
		ts.tm_hour, ts.tm_min, ts.tm_sec, data->tist.nsecs);
	proto_tree_add_text(di_tree, tvb, 0, -1, s);
      }
    } else if(strcmp(tag, "dlfc")==0) {
      if(di_tree) {
        proto_tree_add_item (di_tree, hf_di_dlfc, tvb, offset, bytes, FALSE);
	  }
    } else {
	  gboolean dissected;
  	  tvbuff_t *next_tvb = NULL;
      next_tvb = tvb_new_subset (tvb, offset, bytes, bytes);
      dissected = dissector_try_string(di_dissector_table, tag, next_tvb, pinfo, di_tree);
      if(di_tree && !dissected) {
          proto_tree_add_text(di_tree, tvb, offset, bytes, "%s", tag);
      }
    }
    offset += bytes;
  }
  pinfo->private_data = old_data;
}

void
proto_reg_handoff_drm_di (void)
{
  static int Initialized = FALSE;
  if (!Initialized) {
    di_handle = create_dissector_handle (dissect_di, proto_drm_di);
    dissector_add_string("dcp-tpl.ptr", "DMDI", di_handle);
    dissector_add_string("dcp-tpl.ptr", "RSCI", di_handle);
    fac_handle = create_dissector_handle (dissect_fac, proto_drm_di);
    dissector_add_string("drm-di.tag", "fac_", fac_handle);
	register_sdc_dissectors(proto_drm_di);
	register_msc_dissectors(proto_drm_di);
	register_rsci_dissectors(proto_drm_di);
  }
}

void
proto_register_drm_di (void)
{
  static hf_register_info hf[] = {
    {&hf_di_tlv,
     {"tag", "drm-di.tlv",
      FT_BYTES, BASE_HEX, NULL, 0,
      "Tag Packet", HFILL}
     },
    {&hf_di_robm,
     {"Robustness", "drm-di.robm",
      FT_UINT8, BASE_DEC, VALS(robm_vals), 0,
      "Robustness Mode", HFILL}
     },
    {&hf_di_utco,
     {"Leap seconds", "drm-di.utco",
      FT_UINT16, BASE_DEC, NULL, 0xfffc,
      "leap seconds since DRM t0", HFILL}
     },
    {&hf_di_tist,
     {"tist", "drm-di.tist",
      FT_RELATIVE_TIME, BASE_NONE, NULL, 0,
      "time since DRM t0", HFILL}
     },
    {&hf_di_tist_s,
     {"seconds", "drm-di.tist.seconds",
      FT_UINT64, BASE_DEC, NULL, 0, /*0x0003fffffffffc00ULL, 64 bit masks not supported*/
      "seconds since DRM t0", HFILL}
     },
    {&hf_di_tist_ms,
     {"milliseconds", "drm-di.tist.ms",
      FT_UINT16, BASE_DEC, NULL, 0x3ff,
      "milliseconds", HFILL}
     },
    {&hf_di_dlfc,
     {"Frame Number", "drm-di.dlfc",
      FT_UINT32, BASE_DEC, NULL, 0,
      "logical frame number", HFILL}
     }
  };

/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_di
  };

  if (proto_drm_di == -1) {
    proto_drm_di = proto_register_protocol ("DRM Distribution Interfaces, DI",	/* name */
					 "DRM-DI",	/* short name */
					 "drm-di"	/* abbrev */
      );
  }
  proto_register_field_array (proto_drm_di, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));

  /* subdissector code */
  di_dissector_table = register_dissector_table("drm-di.tag", "DI Tag Packet", FT_STRING, BASE_NONE);

  proto_register_drm_fac (proto_drm_di);
  proto_register_drm_sdc (proto_drm_di);
  proto_register_drm_msc (proto_drm_di);
  proto_register_drm_rsci (proto_drm_di);
}

#ifndef ENABLE_STATIC
/** Register the protocol this plugin dissects. Details follow
 *  here.
 */
G_MODULE_EXPORT void
plugin_register (void)
{
/* register the new protocol, protocol fields, and subtrees */
  if (proto_drm_di == -1) {	/* execute protocol initialization only once */
    proto_register_drm_di ();
  }
}

/** Register the handoff routine. Details follow
 *  here.
 */
G_MODULE_EXPORT void
plugin_reg_handoff (void)
{
  proto_reg_handoff_drm_di ();
}
#endif

