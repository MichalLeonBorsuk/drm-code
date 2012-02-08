/* packet-drm-mdi-sdc.c
 * Routines for Digital Radio Mondiale Multiplex Distribution Interface Protocol
 * Service Data Channel (SDC) Elements
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
# include "config.h"
#endif

#include <gmodule.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <string.h>
#include <epan/crcdrm.h>
#include "drm-data.h"

/* forward reference */
static void dissect_sdc (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree);

static dissector_table_t sdc_dissector_table;

int proto_drm_sdc = -1;
static dissector_handle_t sdc_handle;
static dissector_handle_t sdci_handle;
static int hf_sdc_afs_index = -1;
static int hf_sdc_length = -1;
static int hf_sdc_version = -1;
static int hf_sdc_type = -1;
static int hf_sdc_crc = -1;
static int hf_sdc_crc_ok = -1;
static int hf_sdc_short_id = -1;
static int hf_sdc_stream_id = -1;
static int hf_sdc_audio_coding = -1;
static int hf_sdc_sbr_flag = -1;
static int hf_sdc_audio_mode = -1;
static int hf_sdc_audio_sample_rate = -1;
static int hf_sdc_text_flag = -1;
static int hf_sdc_enhancement_flag = -1;
static int hf_sdc_coder_field = -1;
static int hf_sdc_label = -1;
static int hf_sdc_prot_a = -1;
static int hf_sdc_prot_b = -1;
static int hf_sdc_data_len_a = -1;
static int hf_sdc_data_len_b = -1;
static int hf_sdc_packet_flag = -1;
static int hf_sdc_app_data = -1;
static int hf_sdc_data_unit_indicator = -1;
static int hf_sdc_packet_id = -1;
static int hf_sdc_data_enhancement_flag = -1;
static int hf_sdc_application_domain = -1;
static int hf_sdc_packet_length = -1;
static int hf_sdc_ca_id = -1;
static int hf_sdc_ca_data = -1;
static int hf_sdc_region_id = -1;
static int hf_sdc_latitude = -1;
static int hf_sdc_longitude = -1;
static int hf_sdc_latx = -1;
static int hf_sdc_longx = -1;
static int hf_sdc_zone = -1;
static int hf_sdc_ann_sup = -1;
static int hf_sdc_ann_sw = -1;
static int hf_sdc_mjd = -1;
static int hf_sdc_hour = -1;
static int hf_sdc_minute = -1;

/* Initialize the subtree pointers */
static gint ett_sdc = -1;
static gint ett_sdci = -1;
static gint ett_type0 = -1;
static gint ett_type2 = -1;
static gint ett_type3 = -1;
static gint ett_type5 = -1;
static gint ett_type6 = -1;
static gint ett_type7 = -1;
static gint ett_type9 = -1;
static gint ett_type10 = -1;
static gint ett_type11 = -1;

static const value_string fac_coder_vals[] = {
	{ 0,	"AAC" },
	{ 1,	"CELP" },
	{ 2,	"HVXC" },
	{ 0,	NULL }
};

static const value_string fac_sample_rate_vals[] = {
	{ 0,	"8 kHz" },
	{ 1,	"12 kHz" },
	{ 2,	"16 kHz" },
	{ 3,	"24 kHz" },
	{ 0,	NULL }
};

static const value_string fac_protection_vals[] = {
	{ 0,	"0.5" },
	{ 1,	"0.6 (0.62 if MSC uses 16-QAM)" },
	{ 2,	"0.71" },
	{ 3,	"0.78" },
	{ 0,	NULL }
};

static const value_string system_id_vals[] = {
	{ 0,	"DRM" },
	{ 1,	"AM" },
	{ 2,	"AM" },
	{ 3,	"FM RDS" },
	{ 4,	"FM RDS" },
	{ 5,	"FM" },
	{ 6,	"FM RDS" },
	{ 7,	"FM RDS" },
	{ 8,	"FM" },
	{ 9,	"DAB" },
	{10,	"DAB" },
	{11,	"DAB" },
	{ 0,	NULL }
};

static const value_string app_domain_vals[] = {
	{ 0,	"DRM" },
	{ 1,	"DAB" },
	{ 2,	"reserved for future definition" },
	{ 3,	"reserved for future definition" },
	{ 4,	"reserved for future definition" },
	{ 5,	"reserved for future definition" },
	{ 6,	"reserved for future definition" },
	{ 7,	"reserved for future definition" },
	{ 0,	NULL }
};

void MjdToDate (guint32 Mjd, guint32 *Year, guint32 *Month, guint32 *Day)
{
    guint32 J, C, Y, M;

    J = Mjd + 2400001 + 68569;
    C = 4 * J / 146097;
    J = J - (146097 * C + 3) / 4;
    Y = 4000 * (J + 1) / 1461001;
    J = J - 1461 * Y / 4 + 31;
    M = 80 * J / 2447;
    *Day = J - 2447 * M / 80;
    J = M / 11;
    *Month = M + 2 - (12 * J);
    *Year = 100 * (C - 49) + Y + J;
}

void
proto_register_drm_sdc (int proto)
{
  static hf_register_info hf[] = {
    {&hf_sdc_afs_index,
     {"AFS Index", "drm-sdc.afs_index",
      FT_UINT8, BASE_DEC, NULL, 0x0f,
      "Pointer to the next identical SDC block", HFILL}
     },
    {&hf_sdc_length,
     {"Length", "drm-sdc.length",
      FT_UINT8, BASE_DEC, NULL, 0xfe,
      "length of element", HFILL}
     },
    {&hf_sdc_version,
     {"Version", "drm-sdc.version",
      FT_UINT8, BASE_DEC, NULL, 0x01,
      "element version, changes at reconfiguration, list change, ...", HFILL}
     },
    {&hf_sdc_type,
     {"Type", "drm-sdc.type",
      FT_UINT8, BASE_DEC, NULL, 0xf0,
      "Element Type", HFILL}
     },
    {&hf_sdc_crc,
     {"CRC", "drm-sdc.crc",
      FT_UINT16, BASE_HEX, NULL, 0,
      "CRC", HFILL}
     },
    {&hf_sdc_crc_ok,
     {"CRC OK", "drm-sdc.crc_ok",
      FT_BOOLEAN, BASE_NONE, NULL, 0,
      "CRC OK", HFILL}
     },
    {&hf_sdc_short_id,
     {"short id", "drm-sdc.short_id",
      FT_UINT8, BASE_DEC, NULL, 0x0c,
      "short id", HFILL}
     },
    {&hf_sdc_stream_id,
     {"stream id", "drm-sdc.stream_id",
      FT_UINT8, BASE_DEC, NULL, 0x03,
      "stream id", HFILL}
     },
    {&hf_sdc_audio_coding,
     {"audio coding", "drm-sdc.audio_coding",
      FT_UINT8, BASE_DEC, VALS(fac_coder_vals), 0xc0,
      "audio coding", HFILL}
     },
    {&hf_sdc_sbr_flag,
     {"SBR", "drm-sdc.sbr_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x20,
      "SBR", HFILL}
     },
    {&hf_sdc_audio_mode,
     {"audio mode", "drm-sdc.audio_mode",
      FT_UINT8, BASE_DEC, NULL, 0x18,
      "audio mode", HFILL}
     },
    {&hf_sdc_audio_sample_rate,
     {"audio sample rate", "drm-sdc.audio_sample_rate",
      FT_UINT8, BASE_DEC, VALS(fac_sample_rate_vals), 0x07,
      "audio sample rate", HFILL}
     },
    {&hf_sdc_text_flag,
     {"Text Messages", "drm-sdc.text_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x80,
      "Text Messages", HFILL}
     },
    {&hf_sdc_enhancement_flag,
     {"enhancement flag", "drm-sdc.enhancement_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x40,
      "enhancement flag", HFILL}
     },
    {&hf_sdc_coder_field,
     {"coder field", "drm-sdc.coder_field",
      FT_UINT8, BASE_HEX, NULL, 0x3e,
      "coder field", HFILL}
     },
    {&hf_sdc_label,
     {"Type 1: Label", "drm-sdc.label",
      FT_STRING, BASE_NONE, NULL, 0,
      "Label", HFILL}
     },
    {&hf_sdc_prot_a,
     {"Protection (part A)", "drm-sdc.prot_a",
      FT_UINT8, BASE_DEC, VALS(fac_protection_vals), 0x0c,
      "protection level for part A", HFILL}
     },
    {&hf_sdc_prot_b,
     {"Protection (part B)", "drm-sdc.prot_b",
      FT_UINT8, BASE_DEC, VALS(fac_protection_vals), 0x03,
      "protection level for part B", HFILL}
     },
    {&hf_sdc_data_len_a,
     {"data length for part A", "drm-sdc.data_len_a",
      FT_UINT24, BASE_DEC, NULL, 0xfff000,
      "data length for part A", HFILL}
     },
    {&hf_sdc_data_len_b,
     {"data length for part B", "drm-sdc.data_len_b",
      FT_UINT24, BASE_DEC, NULL, 0xfff,
      "data length for part B", HFILL}
     },
    {&hf_sdc_packet_flag,
     {"Packet mode indicator", "drm-sdc.packet_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x80,
      "Packet mode indicator", HFILL}
     },
    {&hf_sdc_app_data,
     {"application data", "drm-sdc.app_data",
      FT_BYTES, BASE_HEX, NULL, 0,
      "application data", HFILL}
     },
    {&hf_sdc_data_unit_indicator,
     {"Data unit indicator", "drm-sdc.data_unit_indicator",
      FT_BOOLEAN, BASE_NONE, NULL, 0x40,
      "Data unit indicator", HFILL}
     },
    {&hf_sdc_packet_id,
     {"packet id", "drm-sdc.packet_id",
      FT_UINT8, BASE_DEC, NULL, 0x30,
      "packet id", HFILL}
     },
    {&hf_sdc_data_enhancement_flag,
     {"enhancement flag", "drm-sdc.data_enhancement_flag",
      FT_BOOLEAN, BASE_NONE, NULL, 0x08,
      "enhancement flag", HFILL}
     },
    {&hf_sdc_application_domain,
     {"application domain", "drm-sdc.application_domain",
      FT_UINT8, BASE_DEC, VALS(app_domain_vals), 0x07,
      "application domain", HFILL}
     },
    {&hf_sdc_packet_length,
     {"packet length", "drm-sdc.packet_length",
      FT_UINT8, BASE_DEC, NULL, 0,
      "packet length", HFILL}
     },
    {&hf_sdc_ca_data,
     {"CA data", "drm-sdc.ca_data",
      FT_BYTES, BASE_HEX, NULL, 0,
      "CA data", HFILL}
     },
    {&hf_sdc_ca_id,
     {"CA System id", "drm-sdc.ca_id",
      FT_UINT8, BASE_DEC, NULL, 0,
      "CA System id", HFILL}
     },
    {&hf_sdc_region_id,
     {"region id", "drm-sdc.region_id",
      FT_UINT8, BASE_DEC, NULL, 0x0f,
      "region id", HFILL}
     },
    {&hf_sdc_latitude,
     {"Latitude", "drm-sdc.latitude",
      FT_INT8, BASE_DEC, NULL, 0,
      "Latitude of south corner", HFILL}
     },
    {&hf_sdc_longitude,
     {"Longitude", "drm-sdc.longitude",
      FT_INT16, BASE_DEC, NULL, 0,
      "Longitude of west corner", HFILL}
     },
    {&hf_sdc_latx,
     {"Latitude extent", "drm-sdc.latx",
      FT_UINT16, BASE_DEC, NULL, 0x7f,
      "Latitude extent", HFILL}
     },
    {&hf_sdc_longx,
     {"Longitude extent", "drm-sdc.longx",
      FT_UINT8, BASE_DEC, NULL, 0,
      "Longitude extent", HFILL}
     },
    {&hf_sdc_zone,
     {"CIRAF Zone", "drm-sdc.zone",
      FT_UINT8, BASE_DEC, NULL, 0,
      "CIRAF Zone", HFILL}
     },
    {&hf_sdc_ann_sup,
     {"Possible announcements", "drm-sdc.ann_sup",
      FT_UINT16, BASE_HEX, NULL, 0xffc00,
      "Announcement support flags", HFILL}
     },
    {&hf_sdc_ann_sw,
     {"Active announcements", "drm-sdc.ann_sw",
      FT_UINT16, BASE_HEX, NULL, 0x3ff,
      "Announcement switching flags", HFILL}
     },
    {&hf_sdc_mjd,
     {"MJD", "drm-sdc.mjd",
      FT_UINT32, BASE_DEC, NULL, 0x3fffe000,
      "Modified Julian Day", HFILL}
     },
    {&hf_sdc_hour,
     {"hour", "drm-sdc.hour",
      FT_UINT32, BASE_DEC, NULL, 0x1fc0,
      "UTC hour", HFILL}
     },
    {&hf_sdc_minute,
     {"minute", "drm-sdc.minute",
      FT_UINT32, BASE_DEC, NULL, 0x3f,
      "UTC minute", HFILL}
     }
  };


/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_sdc,
    &ett_sdci,
    &ett_type0,
    &ett_type2,
    &ett_type3,
    &ett_type5,
    &ett_type7,
    &ett_type6,
    &ett_type9,
    &ett_type10,
    &ett_type11
  };

  proto_drm_sdc = proto;

  proto_register_field_array (proto_drm_sdc, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
  /* subdissector code */
  sdc_dissector_table = register_dissector_table("drm-sdc.type",
	    "SDC Element Type", FT_UINT8, BASE_DEC);
}

static void
dissect_sdc (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sdc_tree = NULL;
  tvbuff_t *next_tvb = NULL;
  proto_item *ti = NULL;
  guint offset = 0;
  guint len = tvb_length(tvb);
  const guint8 *crc_buf;
  unsigned long c;
  gint crcp = len-2;
  drm_data_t *data = (drm_data_t*)pinfo->private_data;

  if(len==0) /* in the RSCI, sdc_ can be empty when rx not synced */
  {
    if (tree) {			/* we are being asked for details */
      proto_tree_add_text(tree, tvb, 0, 0, "DRM Service Data Channel (SDC) (not synced)");
	}
    return;
  }

  if (check_col (pinfo->cinfo, COL_INFO))
    col_add_str (pinfo->cinfo, COL_INFO, "SDC");

  crc_buf = tvb_get_ptr(tvb, 0, len);
  c = crc_drm(crc_buf, len, 16, 0x11021, 1);

  if(tree) {
    ti = proto_tree_add_text(tree, tvb, 0, -1, "DRM Service Data Channel (SDC)");
    sdc_tree = proto_item_add_subtree (ti, ett_sdc);
    proto_tree_add_item (sdc_tree, hf_sdc_afs_index, tvb, offset, 1, FALSE);
  }
  offset += 1;
  while(offset<crcp) {
    gboolean dissected;
    guint bytes = tvb_get_guint8(tvb, offset);
    offset++;
    guint8 type = tvb_get_guint8(tvb, offset) >> 4;
    if(tree) {
      proto_item *ci;
      ci = proto_tree_add_item (sdc_tree, hf_sdc_type, tvb, offset, 1, FALSE);
      PROTO_ITEM_SET_HIDDEN(ci);
    }
    data->sdc_bytes = bytes;
    bytes = bytes/2;
    if(bytes == 0)
      break;
    bytes += 1; /* SDC length is weird because data field is n bytes + 4 bits */
    next_tvb = tvb_new_subset (tvb, offset, bytes, bytes);
    dissected = dissector_try_port(sdc_dissector_table, type, next_tvb, pinfo, sdc_tree);
    if(dissected == FALSE) {
      if(tree) {
        proto_tree_add_text (sdc_tree, tvb, offset, bytes, "Type %d", type);
      }
    }
    offset += bytes;
    /*g_free(next_tvb);*/
  }
  if(tree) {
    proto_item *ci;
    ci = proto_tree_add_item (sdc_tree, hf_sdc_crc, tvb, crcp, 2, FALSE);
   	proto_item_append_text(ci, " (%s)", (c==0xe2f0)?"Ok":"bad");
    ci = proto_tree_add_boolean(sdc_tree, hf_sdc_crc_ok, tvb, crcp, 2, c==0xe2f0);
    PROTO_ITEM_SET_HIDDEN(ci);
  }
}

/*
• protection level for part A 2 bits.
• protection level for part B 2 bits.
• stream description for stream 0 24 bits.
and optionally, dependent upon the number of streams in the multiplex:
• stream description for stream 1 24 bits.
• stream description for stream 2 24 bits.
• stream description for stream 3 24 bits.
The stream description for stream 0 depends on whether the MSC mode field of the FAC indicates that the hierarchical
frame is present or not.
If the hierarchical frame is not present then the stream description is as follows:
• data length for part A 12 bits.
• data length for part B 12 bits.
If the hierarchical frame is present then the stream description is as follows:
• protection level for hierarchical 2 bits.
• rfu 10 bits.
• data length for hierarchical 12 bits.
The stream descriptions for streams 1, 2 and 3, when present, are as follows:
• data length for part A 12 bits.
• data length for part B 12 bits.
*/

static void
dissect_type0 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  guint offset = 0;
  if(tree) {
    guint len = tvb_length(tvb);
    guint i, streams;
    drm_data_t *data = (drm_data_t*)pinfo->private_data;
    guint8 version = 0;
	if(len==0)
	  return;
    if(data) {
      version = data->sdc_bytes & 1;
    }
    streams = (len-1)/3;
    ti = proto_tree_add_text (tree, tvb, 0, tvb_length(tvb),
          "Type 0: multiplex description for %s configuration with %d streams",
          version?"the next":"this", streams
          );
    sub_tree = proto_item_add_subtree (ti, ett_type0);
    proto_tree_add_item (sub_tree, hf_sdc_prot_a, tvb, offset, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_prot_b, tvb, offset, 1, FALSE);
    offset += 1;
    for(i=0; i<streams; i++) {
      guint32 len = tvb_get_ntoh24(tvb, offset);
      proto_tree_add_uint_format (sub_tree, hf_sdc_data_len_a, tvb, offset, 3,
         len>>12, "part a length for stream %d: %d", i, len>>12);
      proto_tree_add_uint_format (sub_tree, hf_sdc_data_len_b, tvb, offset, 3,
         len&0xfff, "part b length for stream %d: %d", i, len&0xfff);
      offset += 3;
    }
  }
}

static void
dissect_type1 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  if(tree) {
    guint8 id = tvb_get_guint8(tvb, 0) & 0x0f;
    char *label = (char*)tvb_get_string(tvb, 1, tvb_length(tvb)-1);
    proto_tree_add_string_format(tree, hf_sdc_label, tvb, 1, -1, label,
    "Type 1: label for service %d: %s", id>>2, label);
    g_free(label);
  }
}

static void
dissect_type2 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  if(tree) {
    drm_data_t *data = (drm_data_t*)pinfo->private_data;
    guint8 version = 0;
    if(data) {
      version = data->sdc_bytes & 1;
    }
    ti = proto_tree_add_text (tree, tvb, 0, tvb_length(tvb),
          "Type 2: conditional access data for %s configuration",
          version?"the next":"this"
          );
    sub_tree = proto_item_add_subtree (ti, ett_type0);
    proto_tree_add_item (sub_tree, hf_sdc_ca_id, tvb, 0, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_ca_data, tvb, 1, -1, FALSE);
  }
}

/*
Synchronous Multiplex flag 1 bit.
• Layer flag 1 bit.
• Service Restriction flag 1 bit.
• Region/Schedule flag 1 bit.
• Service Restriction field 0 or 8 bits.
• Region/Schedule field 0 or 8 bits.
• n frequencies n × 16 bits.
*/

static void
dissect_type3 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  guint offset = 0;
  if(tree) {
    drm_data_t *data = (drm_data_t*)pinfo->private_data;
    guint8 version = 0;
    if(data) {
      version = data->sdc_bytes & 1;
    }
    guint8 id = tvb_get_guint8(tvb, offset) & 0x0f;
    offset++;
    ti = proto_tree_add_text (tree, tvb, 0, -1,
          "Type 3: %s MFN with %s layer carrying %s services",
          (id&0x8)?"synchronous":"async",
          (id&0x4)?"enhancement":"base",
          (id&0x2)?"some":"all"
          );
    sub_tree = proto_item_add_subtree (ti, ett_type3);
    if(id&0x2) {
      guint8 s = tvb_get_guint8(tvb, offset);
	  if(s&0x10)
	    proto_tree_add_text (sub_tree, tvb, offset, 1, "service 0 carried");
	  if(s&0x20)
	    proto_tree_add_text (sub_tree, tvb, offset, 1, "service 1 carried");
	  if(s&0x40)
	    proto_tree_add_text (sub_tree, tvb, offset, 1, "service 2 carried");
	  if(s&0x80)
	    proto_tree_add_text (sub_tree, tvb, offset, 1, "service 3 carried");
      offset++;
    }
    if(id&0x1) {
      guint8 s = tvb_get_guint8(tvb, offset);
      proto_tree_add_text (sub_tree, tvb, offset, 1, "using region id %d and schedule id %d",
        s >> 4, s & 0xf
       );
      offset++;
    }
    while(offset<tvb_length(tvb)) {
      proto_tree_add_text (sub_tree, tvb, offset, 2, "%d kHz", tvb_get_ntohs(tvb, offset));
      offset += 2;
    }
  }
}

/*
• Schedule Id 4 bits
• Day Code 7 bits
• Start Time 11 bits
• Duration 14 bits
*/

static void
dissect_type4 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  if(tree) {
    drm_data_t *data = (drm_data_t*)pinfo->private_data;
    guint8 version = 0;
    if(data) {
      version = data->sdc_bytes & 1;
    }
    guint8 id = tvb_get_guint8(tvb, 0) & 0x0f;
    guint8 d = tvb_get_guint8(tvb, 1) >> 1;
    guint32 n = tvb_get_ntohl(tvb, 1);
    guint16 start = (n >> 14) & 0x7ff;
    guint16 duration = n & 0x3fff;
    gint i;
    char days[8];
    g_snprintf(days, sizeof(days), "MTWTFSS");
    for(i=0; i<7; i++)
      if((d & (1<<(6-i)))==0)
        days[i]='_';
    proto_tree_add_text (tree, tvb, 0, -1,
          "Type 4: schedule %d includes %3d mins from %2d:%02d UTC on days %s",
          id, duration, start/60, start%60, days
          );
  }
}

static void
dissect_type5 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  guint offset = 0;
  guint8 n = tvb_get_guint8(tvb, 1);
  gboolean packet_mode = (n & 0x80)!=0;
  drm_data_t *data = (drm_data_t*)pinfo->private_data;
  guint8 version = 0;
  if(data==NULL)
    return;
  version = data->sdc_bytes & 1;
  if(version==0) {
    guint8 s = tvb_get_guint8(tvb, 0) & 0x03;
    data->stream_data[s].valid=TRUE;
    data->stream_data[s].audio_not_data=FALSE;
    data->stream_data[s].packet_mode_indicator = packet_mode;
    data->stream_data[s].enhancement_flag = (n & 0x80)!=0;
    if(packet_mode) {
      guint8 packet_id = (n & 0x30) >> 4;
      data->stream_data[s].data_services[packet_id].data_unit_indicator = (n & 0x40) != 0;
      data->stream_data[s].data_services[packet_id].application_domain = n & 0x07;
      n = tvb_get_guint8(tvb, 2);
      data->stream_data[s].packet_length = n;
      /*data->stream_data[s].data_services[packet_id].application_data = ;*/
    } else {
      data->stream_data[s].data_services[0].application_domain = n & 0x07;
      /*data->stream_data[s].data_services[0].application_data = ;*/
    }
  }

  if(tree) {
    ti = proto_tree_add_text (tree, tvb, 0, -1,
          "Type 5: application information for %s configuration",
          version?"the next":"this"
          );
    sub_tree = proto_item_add_subtree (ti, ett_type5);
    proto_tree_add_item (sub_tree, hf_sdc_short_id, tvb, offset, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_stream_id, tvb, offset, 1, FALSE);
    offset++;
    proto_tree_add_item (sub_tree, hf_sdc_packet_flag, tvb, offset, 1, FALSE);
    if(packet_mode) {
      proto_tree_add_item (sub_tree, hf_sdc_data_unit_indicator, tvb, offset, 1, FALSE);
      proto_tree_add_item (sub_tree, hf_sdc_packet_id, tvb, offset, 1, FALSE);
    }
    proto_tree_add_item (sub_tree, hf_sdc_data_enhancement_flag, tvb, offset, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_application_domain, tvb, offset, 1, FALSE);
    offset++;
    if(packet_mode) {
      proto_tree_add_item (sub_tree, hf_sdc_packet_length, tvb, offset, 1, FALSE);
      offset++;
    }
		if(offset<tvb_length(tvb))
      proto_tree_add_item (sub_tree, hf_sdc_app_data, tvb, offset, -1, FALSE);
  }
}

/*
• Short Id flags 4 bits.
• Same Multiplex/Other Service flag 1 bit.
• Short Id/Announcement Id 2 bits.
• rfa 1 bit.
• Announcement support flags 10 bits.
• Announcement switching flags 10 bits.
*/

static void
dissect_type6 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  if(tree) {
    guint8 s = tvb_get_guint8(tvb, 0) & 0x0f;
    guint8 n = tvb_get_guint8(tvb, 1);
    ti = proto_tree_add_text (tree, tvb, 0, -1,
      "Type 6: Announcement definition for services %c%c%c%c",
      (s&1)?'0':'_',
      (s&2)?'1':'_',
      (s&4)?'2':'_',
      (s&8)?'3':'_'
    );
    sub_tree = proto_item_add_subtree (ti, ett_type6);
    if(n&0x80)
      proto_tree_add_text (sub_tree, tvb, 1, 1,
        "announcments are carried as defined in Type 11 message with id %2d",
        (n&0x60)>>5
      );
    else
      proto_tree_add_text (sub_tree, tvb, 1, 1,
        "announcments are carried in service %2d", (n&0x60)>>5
      );
    proto_tree_add_item (sub_tree, hf_sdc_ann_sup, tvb, 1, 3, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_ann_sw, tvb, 1, 3, FALSE);
  }
}

/*
• Region Id 4 bits.
• Latitude 8 bits.
• Longitude 9 bits.
• Latitude Extent 7 bits.
• Longitude Extent 8 bits.
• n CIRAF Zones n × 8 bits.
*/

static void
dissect_type7 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  guint offset = 0;
  if(tree) {
    drm_data_t *data = (drm_data_t*)pinfo->private_data;
    guint8 version = 0;
    if(data) {
      version = data->sdc_bytes & 1;
    }
    guint8 id = tvb_get_guint8(tvb, 0) & 0x0f;
    guint16 n = tvb_get_ntohs(tvb, 2) >> 7;
    gint16 longitude;
    if(n & 0x100){
      guint16 x = 0x200 - 1;
      longitude = (~x|n);
    } else {
      longitude = n;
    }
    ti = proto_tree_add_text (tree, tvb, 0, -1,"Type 7: region %2d", id);
    sub_tree = proto_item_add_subtree (ti, ett_type7);
    proto_tree_add_item (sub_tree, hf_sdc_region_id, tvb, 0, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_latitude, tvb, 1, 1, FALSE);
    proto_tree_add_int (sub_tree, hf_sdc_longitude, tvb, 2, 2, longitude);
    proto_tree_add_item (sub_tree, hf_sdc_latx, tvb, 2, 2, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_longx, tvb, 4, 1, FALSE);
    for(offset=5; offset<tvb_length(tvb); offset++)
      proto_tree_add_item (sub_tree, hf_sdc_zone, tvb, offset, 1, FALSE);
  }
}

/*
 Modified Julian Date 17 bits.
 UTC (hours and minutes) 11 bits.
  UTC: this field specifies the current UTC time expressed in
  hours (5 bits) and
  minutes (6 bits).
*/

static void
dissect_type8 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  if(tree) {
    proto_item *ti = NULL;
    guint32 dnt = tvb_get_ntohl(tvb, 0);
    guint32 mjd = (dnt >> 11) & 0x1ffff;
    guint8 hour = (dnt >> 6) & 0x1f;
    guint8 min = dnt & 0x3f;
    guint32 d,m,y;
    MjdToDate(mjd, &y, &m, &d);
    proto_tree_add_text (tree, tvb, 0, -1, "Type 8: Date & Time %d/%d/%d (%d) %2d:%02d",
    d,m,y,
    mjd, hour, min
    );
    ti = proto_tree_add_item (tree, hf_sdc_mjd, tvb, 0, 4, FALSE);
    PROTO_ITEM_SET_HIDDEN(ti);
    ti = proto_tree_add_item (tree, hf_sdc_hour, tvb, 0, 4, FALSE);
    PROTO_ITEM_SET_HIDDEN(ti);
    ti = proto_tree_add_item (tree, hf_sdc_minute, tvb, 0, 4, FALSE);
    PROTO_ITEM_SET_HIDDEN(ti);
  }
}


/*
 • Short Id 2 bits.
 • Stream Id 2 bits.

 • audio coding 2 bits.
 • SBR flag 1 bit.
 • audio mode 2 bits.
 • audio sampling rate 3 bits.

 • text flag 1 bit.
 • enhancement flag 1 bit.
 • coder field 5 bits.
 • rfa 1 bit.
*/

static void
dissect_type9 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  drm_data_t *data = (drm_data_t*)pinfo->private_data;
  guint8 version = 0;
  if(data==NULL)
    return;
  version = data->sdc_bytes & 1;
  if(version==0) {
    guint8 s = tvb_get_guint8(tvb, 0) & 0x03;
    guint8 n = tvb_get_guint8(tvb, 1);
    data->stream_data[s].valid=TRUE;
    data->stream_data[s].audio_not_data=TRUE;
    data->stream_data[s].audio_coding = (n & 0xc0) >> 6;
    data->stream_data[s].sbr = (n & 0x20)!=0;
    data->stream_data[s].audio_mode = (n & 0x18) >> 3;
    data->stream_data[s].sampling_rate_index = (n & 0x07);
    n = tvb_get_guint8(tvb, 2);
    data->stream_data[s].text_active = (n & 0x80)!=0;
    data->stream_data[s].enhancement_flag = (n & 0x40)!=0;
    data->stream_data[s].coder_field = (n & 0x3e) >> 1;
  }
  if(tree) {
    ti = proto_tree_add_text (tree, tvb, 0, tvb_length(tvb),
          "Type 9: audio info for %s configuration",
          version?"the next":"this"
          );
    sub_tree = proto_item_add_subtree (ti, ett_type9);
    proto_tree_add_item (sub_tree, hf_sdc_short_id, tvb, 0, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_stream_id, tvb, 0, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_audio_coding, tvb, 1, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_sbr_flag, tvb, 1, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_audio_mode, tvb, 1, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_audio_sample_rate, tvb, 1, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_text_flag, tvb, 2, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_enhancement_flag, tvb, 2, 1, FALSE);
    proto_tree_add_item (sub_tree, hf_sdc_coder_field, tvb, 2, 1, FALSE);
  }
}

static void
dissect_type10 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  if(tree) {
    drm_data_t *data = (drm_data_t*)pinfo->private_data;
    guint8 version = 0;
    if(data) {
      version = data->sdc_bytes & 1;
    }
    dissector_handle_t fac_handle;
    ti = proto_tree_add_text (tree, tvb, 0, tvb_length(tvb),
          "Type 10: FAC for %s configuration",
          version?"the next":"this"
          );
    sub_tree = proto_item_add_subtree (ti, ett_type10);
    fac_handle = find_dissector("FAC");
    if(fac_handle)
      call_dissector(fac_handle, tvb, pinfo, sub_tree);
  }
}

/*
• Short Id/Announcement Id flag 1 bit.
• Short Id/Announcement Id field 2 bits.
• Region/Schedule flag 1 bit.
• Same Service flag 1 bit.
• rfa 2 bits.
• System Id 5 bits.
• Region/Schedule field 0 bits or 8 bits.
• Other Service Id 0 bits or 16 bits or 24 bits or 32 bits.
• n frequencies n × (8 or 16) bits.

DRM/AM frequency:
each 16 bit field contains the following information:
• rfu 1 bit.
• frequency value 15 bits.
rfu: this 1 bit is reserved for future use of the frequency value field and shall be set to zero until it is defined.
frequency value: this 15 bit field is coded as an unsigned integer and gives the frequency in kHz.
FM1 (87,5 MHz to 107,9 MHz) frequency:
code meaning
0 to 204: FM frequencies 87,5 MHz to 107,9 MHz (100 kHz step)
FM2 (76,0 MHz to 90,0 MHz) frequency:
code meaning
0 to 140: FM frequencies 76,0 MHz to 90,0 MHz (100 kHz step)
DAB [3] frequency:
code meaning
0 to 11: DAB channels 2A to 4D (Band I)
64 to 95: DAB channels 5A to 12D (Band III)
96 to 101: DAB channels 13A to 13F (Band III +)
128 to 140: DAB channels (L-Band, European grid)
160 to 182: DAB channels (L-Band, Canadian grid)

*/

static void
dissect_type11 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  proto_tree *sub_tree = NULL;
  proto_item *ti = NULL;
  if(tree) {
    guint8 n = tvb_get_guint8(tvb, 0) & 0x0f;
    guint8 sid = tvb_get_guint8(tvb, 1);
    guint flen;
    guint idlen=0;
	guint8 short_id = (n>>1)&0x3;
    if(n&0x8)
      ti = proto_tree_add_text (tree, tvb, 0, -1,
      "Type 11: Location of announcements with id %2d", short_id
      );
    else
      ti = proto_tree_add_text (tree, tvb, 0, -1,
      "Type 11: Alternative frequencies for service %2d", short_id
      );
    sub_tree = proto_item_add_subtree (ti, ett_type11);
    guint32 system_id = sid & 0x1f;
    guint32 id = sid & 0x1f;
    guint offset = 1;
    const gchar* sys = match_strval(system_id, system_id_vals);
    proto_tree_add_text (sub_tree, tvb, offset, 1,
          "%s network carrying %s service",
          sys,
          (sid&0x80)?"the same":"an alternative"
    );
    offset += 1;
    if(n&1) {
      guint8 rs = tvb_get_guint8(tvb, offset);
      proto_tree_add_text (sub_tree, tvb, offset, 1,
        "restricted to region %d and schedule %d",
        rs>>4, rs&0x0f
        );
      offset += 1;
    }
    switch(system_id){
    case 0:
       flen=2;
       id = tvb_get_ntoh24(tvb, offset);
       idlen = 3;
       break;
    case 1:
       flen=2;
       id = tvb_get_ntoh24(tvb, offset);
       idlen = 3;
       break;
    case 2:
       flen=2;
       break;
    case 3:
       flen=1;
       id = tvb_get_ntoh24(tvb, offset);
       idlen = 3;
       break;
    case 4:
       flen=1;
       id = tvb_get_ntohs(tvb, offset);
       idlen = 2;
       break;
    case 5:
       flen=1;
       break;
    case 6:
       flen=1;
       id = tvb_get_ntoh24(tvb, offset);
       idlen = 3;
       break;
    case 7:
       flen=1;
       id = tvb_get_ntohs(tvb, offset);
       idlen = 2;
       break;
    case 8:
       flen=1;
       break;
    case 9:
       flen=1;
       id = tvb_get_ntoh24(tvb, offset);
       idlen = 3;
       break;
    case 10:
       flen=1;
       id = tvb_get_ntohs(tvb, offset);
       idlen = 2;
       break;
    case 11:
       flen=1;
       id = tvb_get_ntohl(tvb, offset);
       idlen = 4;
       break;
    }
    if(idlen>0) {
      proto_tree_add_text (sub_tree, tvb, offset, idlen, "with id %*X", idlen, id);
      offset += idlen;
    }
    while(offset<tvb_length(tvb)){
      guint16 f;
      if(flen==1)
         f = tvb_get_guint8(tvb, offset);
      else
        f = tvb_get_ntohs(tvb, offset);
      switch(system_id) {
        case 3:
        case 4:
        case 5:
          {
            double m = 87.5+((double)f)/10.0; /* NA & Europe Grid */
            proto_tree_add_text (sub_tree, tvb, offset, flen, "%.1f MHz", m);
          }
          break;
        case 6:
        case 7:
        case 8:
          {
            double m = 76.0+((double)f)/10.0; /* Asia Grid */
            proto_tree_add_text (sub_tree, tvb, offset, flen, "%.1f MHz", m);
          }
          break;
        case 9:
        case 10:
        case 11:
          if(f<=11) {
            guint channel = (f / 4) + 2;
            guint sub_channel = (f % 4) + 'A';
            proto_tree_add_text (sub_tree, tvb, offset, flen,
               "Band I channel %d%c", channel, sub_channel);
          } else if(64<= f && f <=95) {
            guint channel = (f / 4) - 11;
            guint sub_channel = (f % 4) + 'A';
            proto_tree_add_text (sub_tree, tvb, offset, flen,
               "Band III channel %d%c", channel, sub_channel);
          } else if(96<= f && f <=101) {
            guint channel = (f / 6) - 3;
            guint sub_channel = (f % 6) + 'A';
            proto_tree_add_text (sub_tree, tvb, offset, flen,
               "Band III+ channel %d%c", channel, sub_channel);
          } else if(128<= f && f <=143) {
            guint channel = f - 128;
            double m = 1452.96+1.712*((double)channel);
            proto_tree_add_text (sub_tree, tvb, offset, flen,
                "European L-Band channel L%c, %.3f MHz", channel+'A', m);
          } else if(160<= f && f <=182) {
            guint channel = f - 159;
            double m = 1451.072+1.744*((double)channel);
            proto_tree_add_text (sub_tree, tvb, offset, flen,
                "Canadian L-Band channel %d, %.3f MHz", channel, m);
          } else {
            proto_tree_add_text (sub_tree, tvb, offset, flen, "unknown channel %d", f);
          }
          break;
        default:
          proto_tree_add_text (sub_tree, tvb, offset, flen, "%d kHz", f);
      }
      offset += flen;
    }
  }
}

/*
• Short Id 2 bits.
• rfu 2 bits.
• language code 24 bits.
• country code 16 bits.
*/

static void
dissect_type12 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  if(tree) {
    guint8 id = tvb_get_guint8(tvb, 0) & 0x0f;
    guint8 *lang = tvb_get_string(tvb, 1, 3);
    guint8 *cc = tvb_get_string(tvb, 4, 2);
    proto_tree_add_text (tree, tvb, 0, -1, "Type 12: service %d %s, %s",
    id>>2, lang, cc
    );
    g_free(lang);
    g_free(cc);
  }
}

static void
dissect_sdci (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  guint len = tvb_length(tvb);

  if(tree) {
    if(len==0) /* in the RSCI, sdc_ can be empty when rx not synced */
	{
      proto_tree_add_text(tree, tvb, 0, 0, "sdci (not synced)");
	}
	else
	{
      proto_tree *sdci_tree = NULL;
      proto_item *ti = NULL;
      ti = proto_tree_add_text (tree, tvb, 0, -1, "sdci");
      sdci_tree = proto_item_add_subtree (ti, ett_sdci);
      drm_data_t *data = (drm_data_t*)pinfo->private_data;
      pinfo->private_data = 0;
      dissect_type0(tvb, pinfo, sdci_tree);
      pinfo->private_data = data;
	}
  }
}

void
register_sdc_dissectors (int proto)
{
  proto_drm_sdc = proto;

    sdc_handle = create_dissector_handle (dissect_sdc, proto_drm_sdc);
    dissector_add_string("drm-di.tag", "sdc_", sdc_handle);

    sdci_handle = create_dissector_handle (dissect_sdci, proto_drm_sdc);
    dissector_add_string("drm-di.tag", "sdci", sdci_handle);

    dissector_add("drm-sdc.type", 0, create_dissector_handle (dissect_type0, proto_drm_sdc));
    dissector_add("drm-sdc.type", 1, create_dissector_handle (dissect_type1, proto_drm_sdc));
    dissector_add("drm-sdc.type", 2, create_dissector_handle (dissect_type2, proto_drm_sdc));
    dissector_add("drm-sdc.type", 3, create_dissector_handle (dissect_type3, proto_drm_sdc));
    dissector_add("drm-sdc.type", 4, create_dissector_handle (dissect_type4, proto_drm_sdc));
    dissector_add("drm-sdc.type", 5, create_dissector_handle (dissect_type5, proto_drm_sdc));
    dissector_add("drm-sdc.type", 6, create_dissector_handle (dissect_type6, proto_drm_sdc));
    dissector_add("drm-sdc.type", 7, create_dissector_handle (dissect_type7, proto_drm_sdc));
    dissector_add("drm-sdc.type", 8, create_dissector_handle (dissect_type8, proto_drm_sdc));
    dissector_add("drm-sdc.type", 9, create_dissector_handle (dissect_type9, proto_drm_sdc));
    dissector_add("drm-sdc.type", 10, create_dissector_handle (dissect_type10, proto_drm_sdc));
    dissector_add("drm-sdc.type", 11, create_dissector_handle (dissect_type11, proto_drm_sdc));
    dissector_add("drm-sdc.type", 12, create_dissector_handle (dissect_type12, proto_drm_sdc));
}
