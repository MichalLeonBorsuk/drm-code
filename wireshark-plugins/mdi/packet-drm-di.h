/* packet-drm-mdi.h
 * Routines for Digital Radio Mondiale Multiplex Distribution Interface Protocol 
 * Copyright 2006, British Broadcasting Corporation 
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
 
#ifndef __PACKET_DRM_DI_H
#define __PACKET_DRM_DI_H

#include <epan/packet.h>

void MjdToDate (guint32 Mjd, guint32 *Year, guint32 *Month, guint32 *Day);

void proto_register_drm_fac (int);
void proto_register_drm_sdc (int);
void proto_register_drm_msc (int);
void proto_register_drm_rsci (int);
void register_sdc_dissectors(int);
void register_msc_dissectors(int);
void register_rsci_dissectors(int);
void dissect_fac (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree);

#endif
