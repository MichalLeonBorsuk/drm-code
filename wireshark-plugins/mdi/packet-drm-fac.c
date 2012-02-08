/* packet-drm-di-fac.c
 * Routines for Digital Radio Mondiale Multiplex Distribution Interface Protocol
 * Fast Access Channel (FAC) elements
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
#include <epan/crcdrm.h>
#include <string.h>
#include "drm-data.h"

extern int proto_drm_di;
static int hf_fac_layer = -1;
static int hf_fac_identity = -1;
static int hf_fac_spectrum_occupancy = -1;
static int hf_fac_depth = -1;
static int hf_fac_msc_mode = -1;
static int hf_fac_sdc_mode = -1;
static int hf_fac_nservices = -1;
static int hf_fac_recon_index = -1;
static int hf_fac_service_identifier = -1;
static int hf_fac_short_id = -1;
static int hf_fac_audio_ca = -1;
static int hf_fac_language = -1;
static int hf_fac_audio_data = -1;
static int hf_fac_prog_type = -1;
static int hf_fac_app_id = -1;
static int hf_fac_data_ca = -1;
static int hf_fac_frame_number = -1;
static int hf_fac_afs_index_valid = -1;
static int hf_fac_crc = -1;
static int hf_fac_crc_ok = -1;


/* Initialize the subtree pointers */
static gint ett_fac = -1;

static const value_string so_vals[] = {
	{ 0,	"4.5 kHz" },
	{ 1,	"5 kHz" },
	{ 2,	"9 kHz" },
	{ 3,	"10 kHz" },
	{ 4,	"18 kHz" },
	{ 5,	"20 kHz" },
	{ 0,	NULL }
};

static const value_string depth_vals[] = {
	{ 0,	"long" },
	{ 1,	"short" },
	{ 0,	NULL }
};

static const value_string msc_mode_vals[] = {
	{ 0,	"64-QAM, no hierarchical" },
	{ 1,	"64-QAM, hierarchical on I" },
	{ 2,	"64-QAM, hierarchical on I&Q" },
	{ 3,	"16-QAM" },
	{ 0,	NULL }
};

static const value_string sdc_mode_vals[] = {
	{ 0,	"16-QAM" },
	{ 1,	"4-QAM" },
	{ 0,	NULL }
};

static const value_string layer_vals[] = {
	{ 0,	"base" },
	{ 1,	"enhancement" },
	{ 0,	NULL }
};

static const value_string nservices_vals[] = {
	{ 0, "4 audio services" },
	{ 1, "1 data service" },
	{ 2, "2 data services" },
	{ 3, "3 data services" },
	{ 4, "1 audio service" },
	{ 5, "1 audio service and 1 data service" },
	{ 6, "1 audio service and 2 data services" },
	{ 7, "1 audio service and 3 data services" },
	{ 8, "2 audio services" },
	{ 9, "2 audio services and 1 data service" },
	{10, "2 audio services and 2 data services" },
	{11, "reserved" },
	{12, "3 audio services" },
	{13, "3 audio services and 1 data service" },
	{14, "reserved" },
	{15, "4 data services" },
	{ 0,	NULL }
};

static const value_string language_vals[] = {
        { 0, "No language specified" },
        { 1, "Arabic" },
        { 2, "Bengali" },
        { 3, "Chinese (Mandarin)" },
        { 4, "Dutch" },
        { 5, "English" },
        { 8, "Hindi" },
        { 6, "French" },
        { 9, "Japanese" },
        { 7, "German" },
        { 10, "Javanese" },
        { 11, "Korean" },
        { 12, "Portuguese" },
        { 13, "Russian" },
        { 14, "Spanish" },
        { 15, "Other language" },
	{ 0,	NULL }
};

static const value_string prog_type_vals[] = {
        { 0, "No programme type" },
        { 1, "News" },
        { 2, "Current Affairs" },
        { 3, "Information" },
        { 4, "Sport" },
        { 5, "Education" },
        { 6, "Drama" },
        { 7, "Culture" },
        { 8, "Science" },
        { 9, "Varied" },
        { 10, "Pop Music" },
        { 11, "Rock Music" },
        { 12, "Easy Listening Music" },
        { 13, "Light Classical" },
        { 14, "Serious Classical" },
        { 15, "Other Music" },
        { 16, "Weather/meteorology" },
        { 17, "Finance/Business" },
        { 18, "Children's programmes" },
        { 19, "Social Affairs" },
        { 20, "Religion" },
        { 21, "Phone In" },
        { 22, "Travel" },
        { 23, "Leisure" },
        { 24, "Jazz Music" },
        { 25, "Country Music" },
        { 26, "National Music" },
        { 27, "Oldies Music" },
        { 28, "Folk Music" },
        { 29, "Documentary" },
        { 30, "Not used" },
        { 31, "Not used - skip indicator" },
	{ 0,	NULL }
};

static const value_string app_id_vals[] = {
        { 0, "Application Signalled in SDC" },
        { 1, "reserved for future definition" },
        { 2, "reserved for future definition" },
        { 3, "reserved for future definition" },
        { 4, "reserved for future definition" },
        { 5, "reserved for future definition" },
        { 6, "reserved for future definition" },
        { 7, "reserved for future definition" },
        { 8, "reserved for future definition" },
        { 9, "reserved for future definition" },
        { 10, "reserved for future definition" },
        { 11, "reserved for future definition" },
        { 12, "reserved for future definition" },
        { 13, "reserved for future definition" },
        { 14, "reserved for future definition" },
        { 15, "reserved for future definition" },
        { 16, "reserved for future definition" },
        { 17, "reserved for future definition" },
        { 18, "reserved for future definition" },
        { 19, "reserved for future definition" },
        { 20, "reserved for future definition" },
        { 21, "reserved for future definition" },
        { 22, "reserved for future definition" },
        { 23, "reserved for future definition" },
        { 24, "reserved for future definition" },
        { 25, "reserved for future definition" },
        { 26, "reserved for future definition" },
        { 27, "reserved for future definition" },
        { 28, "reserved for future definition" },
        { 29, "reserved for future definition" },
        { 30, "reserved for future definition" },
        { 31, "Not used - skip indicator" },
	{ 0,	NULL }
};

/*
6.3.5 CRC
The 8-bit Cyclic Redundancy Check shall be calculated on the channel and service parameters. It shall use the generator
polynomial G8(x) = x8 + x4 + x3 + x2 + 1. See annex D.
6.3.6 FAC repetition
The FAC channel parameters shall be sent in each FAC block. The FAC service parameters for one service shall be sent
in each block. When there is more than one service in the multiplex the repetition pattern is significant to the receiver
scan time. When all services are of the same type (e.g. all audio or all data) then the services shall be signalled
sequentially. When a mixture of audio and data services is present then the patterns shown in table 53 shall be signalled.
*/

/*
   channel
   • Base/Enhancement flag 1 bit
   • Identity 2 bits
   • Spectrum occupancy 4 bits
   • Interleaver depth flag 1 bit
   • MSC mode 2 bits
   • SDC mode 1 bit
   • Number of services 4 bits
   • Reconfiguration index 3 bits
   service
   • Service identifier 24 bits
   • Short identifier 2 bits
   • Audio CA indication 1 bit
   • Language 4 bits
   • Audio/Data flag 1 bit
   • Service descriptor 5 bits
   • Data CA indication 1 bit
   • Rfa 6 bits
*/
void
proto_register_drm_fac (int proto)
{
  static hf_register_info hf[] = {
    {&hf_fac_layer,
     {"Layer", "drm-fac.layer",
      FT_UINT8, BASE_DEC, VALS(layer_vals), 0x80,
      "Base or Enhancement Layer", HFILL}
     },
    {&hf_fac_identity,
     {"Identity", "drm-fac.identity",
      FT_UINT8, BASE_DEC, NULL, 0x60,
      "FAC frame identity", HFILL}
     },
    {&hf_fac_spectrum_occupancy,
     {"Spectrum Occupancy", "drm-fac.spectrum_occupancy",
      FT_UINT8, BASE_DEC, VALS(so_vals), 0x1e,
      "Spectrum Occupancy", HFILL}
     },
    {&hf_fac_depth,
     {"Interleaver Depth", "drm-fac.interleaver_depth",
      FT_UINT8, BASE_DEC, VALS(depth_vals), 0x1,
      "Interleaver Depth", HFILL}
     },
    {&hf_fac_msc_mode,
     {"MSC Mode", "drm-fac.msc_mode",
      FT_UINT16, BASE_DEC, VALS(msc_mode_vals), 0xc000,
      "MSC Mode", HFILL}
     },
    {&hf_fac_sdc_mode,
     {"SDC Mode", "drm-fac.sdc_mode",
      FT_UINT16, BASE_DEC, VALS(sdc_mode_vals), 0x2000,
      "SDC Mode", HFILL}
     },
    {&hf_fac_nservices,
     {"Number of Services", "drm-fac.nservices",
      FT_UINT16, BASE_DEC, VALS(nservices_vals), 0x1e00,
      "Number of Services", HFILL}
     },
    {&hf_fac_recon_index,
     {"Reconfiguration index", "drm-fac.recon_index",
      FT_UINT16, BASE_DEC, NULL, 0x01c0,
      "Reconfiguration index", HFILL}
     },
    {&hf_fac_service_identifier,
     {"Service identifier", "drm-fac.service_identifier",
      FT_UINT32, BASE_HEX, NULL, 0x0ffffff0,
      "Service identifier", HFILL}
     },
    {&hf_fac_short_id,
     {"short identifier", "drm-fac.short_id",
      FT_UINT32, BASE_DEC, NULL, 0x0c,
      "Service identifier", HFILL}
     },
    {&hf_fac_audio_ca,
     {"Audio CA", "drm-fac.audio_ca",
      FT_BOOLEAN, BASE_NONE, NULL, 0x02,
      "Audio CA", HFILL}
     },
    {&hf_fac_language,
     {"Language", "drm-fac.language",
      FT_UINT16, BASE_DEC, VALS(language_vals), 0x01e0,
      "Language", HFILL}
     },
    {&hf_fac_audio_data,
     {"Data only", "drm-fac.is_data",
      FT_BOOLEAN, BASE_NONE, NULL, 0x10,
      "Audio or Data", HFILL}
     },
    {&hf_fac_prog_type,
     {"Service Descriptor", "drm-fac.service_descriptor",
      FT_UINT16, BASE_DEC, VALS(prog_type_vals), 0x0f80,
      "Service Descriptor", HFILL}
     },
    {&hf_fac_app_id,
     {"Service Descriptor", "drm-fac.service_descriptor",
      FT_UINT16, BASE_DEC, VALS(app_id_vals), 0x0f80,
      "Service Descriptor", HFILL}
     },
    {&hf_fac_data_ca,
     {"Data CA", "drm-fac.data_ca",
      FT_BOOLEAN, BASE_NONE, NULL, 0x60,
      "Data CA", HFILL}
     },
     {&hf_fac_frame_number,
     {"Frame Number", "drm-fac.frame_number",
      FT_UINT8, BASE_DEC, NULL, 0,
      "FAC Frame Number (SDC only in Frame 0)", HFILL}
     },
    {&hf_fac_afs_index_valid,
     {"AFS Index Valid", "drm-fac.afs_index_valid",
      FT_BOOLEAN, BASE_NONE, NULL, 0,
      "Whether the AFS Index in the SDC in this frame can be used", HFILL}
     },
    {&hf_fac_crc,
     {"CRC", "drm-fac.crc",
      FT_UINT8, BASE_HEX, NULL, 0,
      "CRC", HFILL}
     },
    {&hf_fac_crc_ok,
     {"CRC OK", "drm-fac.crc_ok",
      FT_BOOLEAN, BASE_NONE, NULL, 0,
      "CRC OK", HFILL}
     }
  };

/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_fac
  };

  proto_register_field_array (proto, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));

}
  /* The channel parameters are as follows:
   • Base/Enhancement flag 1 bit
   • Identity 2 bits
   • Spectrum occupancy 4 bits
   • Interleaver depth flag 1 bit
   • MSC mode 2 bits
   • SDC mode 1 bit
   • Number of services 4 bits
   • Reconfiguration index 3 bits
   • Rfu 2 bits
*/
void
dissect_fac (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{

  proto_item *ti = NULL;
  proto_item *ci = NULL;
  proto_tree *fac_tree = NULL;
  guint16 n;
  guint len = tvb_length(tvb);
  const guint8 *crc_buf;
  unsigned long c;
  drm_data_t *data;

  if(len==0) /* in the RSCI, fac_ can be empty when rx not synced */
  {
    if (tree) {			/* we are being asked for details */
      proto_tree_add_text(tree, tvb, 0, 0, "DRM Fast Access Channel (FAC) (not synced)");
	}
    return;
  }

  data = (drm_data_t*)pinfo->private_data;
  crc_buf = tvb_get_ptr(tvb, 0, len);
  n = tvb_get_guint8(tvb, 0);
  data->spectral_occupancy = (n>>1)&0x0f;
  data->interleaver_depth = n&1;
  n = tvb_get_guint8(tvb, 1);
  data->msc_mode = n>>6;
  data->sdc_mode = (n>>5)&1;
  c = crc_drm(crc_buf, len, 8, 0x11d, 1);

  if (tree) {			/* we are being asked for details */
    guint8 b;
    guint32 frame_number;
    guint16 identity;
    gboolean afs_index_valid;
    ti = proto_tree_add_text(tree, tvb, 0, -1, "DRM Fast Access Channel (FAC)");
    fac_tree = proto_item_add_subtree (ti, ett_fac);
    proto_tree_add_item (fac_tree, hf_fac_layer, tvb, 0, 1, FALSE);
    proto_tree_add_item(fac_tree, hf_fac_identity, tvb, 0, 1, FALSE);
    b = tvb_get_guint8(tvb, 0);
    identity = (b >> 5) & 0x3;
    if(identity==3){
      frame_number = 0;
      proto_tree_add_uint (fac_tree, hf_fac_frame_number, tvb, 0, 1, frame_number);
      afs_index_valid = FALSE;
    } else {
      frame_number = identity;
      afs_index_valid = (identity==0);
    }
    if(frame_number==0)
      proto_tree_add_boolean (fac_tree, hf_fac_afs_index_valid, tvb, 0, 1, afs_index_valid);
    proto_tree_add_item (fac_tree, hf_fac_spectrum_occupancy, tvb, 0, 1, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_depth, tvb, 0, 1, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_msc_mode, tvb, 1, 2, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_sdc_mode, tvb, 1, 2, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_nservices, tvb, 1, 2, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_recon_index, tvb, 1, 2, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_service_identifier, tvb, 2, 4, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_short_id, tvb, 5, 1, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_audio_ca, tvb, 5, 1, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_language, tvb, 5, 2, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_audio_data, tvb, 6, 1, FALSE);
    if(tvb_get_guint8(tvb,6)&0x10)
      proto_tree_add_item (fac_tree, hf_fac_app_id, tvb, 6, 2, FALSE);
    else
      proto_tree_add_item (fac_tree, hf_fac_prog_type, tvb, 6, 2, FALSE);
    proto_tree_add_item (fac_tree, hf_fac_data_ca, tvb, 7, 1, FALSE);
    ci = proto_tree_add_item (fac_tree, hf_fac_crc, tvb, 8, 1, FALSE);
   	proto_item_append_text(ci, (c==0x3b)?" (Ok)":" (bad)");
    proto_tree_add_boolean(fac_tree, hf_fac_crc_ok, tvb, 8, 1, c==0x3b);
  }
}
