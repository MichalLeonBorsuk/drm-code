/* packet-drm-mdi-msc.c
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
 */
 
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gmodule.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <string.h>
#include "packet-drm-di.h"
#include "drm-data.h"

int proto_drm_msc = -1;

static heur_dissector_list_t msc_heur_list = NULL;
static heur_dissector_list_t msc_audio_heur_list = NULL;
static heur_dissector_list_t msc_text_heur_list = NULL;
static heur_dissector_list_t msc_data_heur_list = NULL;

static int hf_audio[4] = {-1,-1,-1,-1};
static int hf_text[4] = {-1,-1,-1,-1};
static int hf_data[4] = {-1,-1,-1,-1};
  
void
proto_register_drm_msc (int proto)
{
  static hf_register_info hf[] = {
    {&hf_audio[0],
     {"audio in stream 0", "drm-msc.str0.audio",
      FT_BYTES, BASE_HEX, NULL, 0,
      "audio in stream 0", HFILL}
     },
    {&hf_audio[1],
     {"audio in stream 1", "drm-msc.str1.audio",
      FT_BYTES, BASE_HEX, NULL, 0,
      "audio in stream 1", HFILL}
     },
    {&hf_audio[2],
     {"audio in stream 2", "drm-msc.str2.audio",
      FT_BYTES, BASE_HEX, NULL, 0,
      "audio in stream 2", HFILL}
     },
    {&hf_audio[3],
     {"audio in stream 3", "drm-msc.str3.audio",
      FT_BYTES, BASE_HEX, NULL, 0,
      "audio in stream 3", HFILL}
     },
    {&hf_text[0],
     {"text in stream 0", "drm-msc.str0.text",
      FT_BYTES, BASE_HEX, NULL, 0,
      "text in stream 0", HFILL}
     },
    {&hf_text[1],
     {"text in stream 1", "drm-msc.str1.text",
      FT_BYTES, BASE_HEX, NULL, 0,
      "text in stream 1", HFILL}
     },
    {&hf_text[2],
     {"text in stream 2", "drm-msc.str2.text",
      FT_BYTES, BASE_HEX, NULL, 0,
      "text in stream 2", HFILL}
     },
    {&hf_text[3],
     {"text in stream 3", "drm-msc.str3.text",
      FT_BYTES, BASE_HEX, NULL, 0,
      "text in stream 3", HFILL}
     },
    {&hf_data[0],
     {"data in stream 0", "drm-msc.str0.data",
      FT_BYTES, BASE_HEX, NULL, 0,
      "data in stream 0", HFILL}
     },
    {&hf_data[1],
     {"data in stream 1", "drm-msc.str1.data",
      FT_BYTES, BASE_HEX, NULL, 0,
      "data in stream 1", HFILL}
     },
    {&hf_data[2],
     {"data in stream 2", "drm-msc.str2.data",
      FT_BYTES, BASE_HEX, NULL, 0,
      "data in stream 2", HFILL}
     },
    {&hf_data[3],
     {"data in stream 3", "drm-msc.str3.data",
      FT_BYTES, BASE_HEX, NULL, 0,
      "data in stream 3", HFILL}
     }
  };

  proto_register_field_array (proto, hf, array_length (hf));

  /* subdissector code */
  register_heur_dissector_list("drm-msc", &msc_heur_list);
  register_heur_dissector_list("drm-msc.str0.audio", &msc_audio_heur_list);
  register_heur_dissector_list("drm-msc.str1.audio", &msc_audio_heur_list);
  register_heur_dissector_list("drm-msc.str2.audio", &msc_audio_heur_list);
  register_heur_dissector_list("drm-msc.str3.audio", &msc_audio_heur_list);
  register_heur_dissector_list("drm-msc.str0.text", &msc_text_heur_list);
  register_heur_dissector_list("drm-msc.str1.text", &msc_text_heur_list);
  register_heur_dissector_list("drm-msc.str2.text", &msc_text_heur_list);
  register_heur_dissector_list("drm-msc.str3.text", &msc_text_heur_list);
  register_heur_dissector_list("drm-msc.str0.data", &msc_data_heur_list);
  register_heur_dissector_list("drm-msc.str1.data", &msc_data_heur_list);
  register_heur_dissector_list("drm-msc.str2.data", &msc_data_heur_list);
  register_heur_dissector_list("drm-msc.str3.data", &msc_data_heur_list);
}

gboolean
dissect_msc_audio (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  drm_data_t *data = (drm_data_t*)pinfo->private_data;
  gint s = data->stream;
  if(s<0 || s>3 || data->stream_data[s].valid==FALSE || data->stream_data[s].audio_not_data==FALSE)
    return FALSE;
  if (tree) {			/* we are being asked for details */
    proto_tree_add_item (tree, hf_audio[s], tvb, 0, -1, FALSE);
  }
  return TRUE;
}

gboolean
dissect_msc_text (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  if (tree) {			/* we are being asked for details */
    drm_data_t *data = (drm_data_t*)pinfo->private_data;
    gint s = data->stream;
    stream_t *stream_data = &data->stream_data[s];
    if(0<=s && s<=3) {
      if(stream_data->audio_not_data==FALSE) 
        return FALSE;
      if(stream_data->text_active==FALSE) 
        return FALSE;
      proto_tree_add_item (tree, hf_text[s], tvb, 0, -1, FALSE);
    } else {
      return FALSE;
    }
  }
  return TRUE;
}

gboolean
dissect_msc_data (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
  drm_data_t *data = (drm_data_t*)pinfo->private_data;
  gboolean done;

  gint s = data->stream;
  if(s<0 || s>3 || data->stream_data[s].valid==FALSE || data->stream_data[s].audio_not_data==TRUE)
    return FALSE;

  done = dissector_try_heuristic(msc_data_heur_list, tvb, pinfo, tree);
  if(done)
    return TRUE;

  if (tree) {			/* we are being asked for details */
      proto_tree_add_item (tree, hf_data[s], tvb, 0, -1, FALSE);
  }
  return TRUE;
}

static void
dissect_msc (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, int s)
{
  /* this allows external sub-dissectors for the msc */
  gboolean done = dissector_try_heuristic(msc_heur_list, tvb, pinfo, tree);
  if(done)
    return;
  drm_data_t *data = (drm_data_t*)pinfo->private_data;
  if(data==NULL)
    return;
  data->stream = s;
  if(data->stream_data[s].audio_not_data) {
    if(data->stream_data[s].text_active) {
      guint bytes = tvb_length(tvb);
      if(bytes<4) {
        proto_tree_add_text(tree, tvb, 0, -1, "MSC audio+text but stream %d too small", s);
      } else {
        tvbuff_t *audio_tvb = tvb_new_subset (tvb, 0, bytes-4, bytes-4);
        tvbuff_t *text_tvb = tvb_new_subset (tvb, bytes-4, 4, 4);
        dissect_msc_audio(audio_tvb, pinfo, tree);
        dissect_msc_text(text_tvb, pinfo, tree);
      }
    } else {
      dissect_msc_audio(tvb, pinfo, tree);
    }
  } else {
    dissect_msc_data(tvb, pinfo, tree);
  }
}

static void
dissect_msc0 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
    dissect_msc(tvb, pinfo, tree, 0);
}

static void
dissect_msc1 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
    dissect_msc(tvb, pinfo, tree, 1);
}

static void
dissect_msc2 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
    dissect_msc(tvb, pinfo, tree, 2);
}

static void
dissect_msc3 (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree)
{
    dissect_msc(tvb, pinfo, tree, 3);
}

void
register_msc_dissectors (int proto)
{
  static int Initialized = FALSE;
  if (!Initialized) {
    dissector_add_string("drm-di.tag", "str0", create_dissector_handle (dissect_msc0, proto));
    dissector_add_string("drm-di.tag", "str1", create_dissector_handle (dissect_msc1, proto));
    dissector_add_string("drm-di.tag", "str2", create_dissector_handle (dissect_msc2, proto));
    dissector_add_string("drm-di.tag", "str3", create_dissector_handle (dissect_msc3, proto));
  }
}
