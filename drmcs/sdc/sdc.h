/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):  Julian Cable, Ollie Haffenden, Andrew Murphy
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

#ifndef _SDC_H
#define _SDC_H

#include <timestamp.h>
#include <SdcElement.h>
#include <sdcblock.h>

class SDC
{
public:

  SDC();
  ~SDC();
  void AnnounceReConfigure(const DrmMuxConfig&, uint8_t);
  void ReConfigure(
            const DrmMuxConfig& config,
            bool new_reconfiguration_version=false,
            uint8_t new_services_pattern = 0
            );
  void NextFrame(std::vector<uint8_t>&, DrmTime&);

  std::vector<uint8_t> sdci;
  bool afs_index_valid;

protected:

  static const unsigned short sdc_length[4][2][6];

  DrmMuxConfig current;
  uint8_t afs_index; // 0-15, static between reconfigurations, blocks to next repeat
  uint16_t length;
  vector<SdcBlock> block;
  bytevector elements_in_every_block;
  bool reconfiguration_version, afs_version, announcement_version;
  uint16_t current_block;
  bool send_date_and_time;
  uint16_t num_elements_in_every_block;

  void check_build_date_and_time(DrmTime& timestamp);
  void build_sdci(const DrmMuxConfig& mux);
  void SdcDataLength(unsigned short &length);
  void buildBlocks(vector<SdcElement>& element, size_t max_blocks);
  void ConfigureService(vector<SdcElement>& element, const DrmMuxConfig& mux);
  void ConfigureServiceUnique(vector<SdcElement>& element,
           const Service& service, size_t short_id);
  void ConfigureChannel(vector<SdcElement>& element, const RFChannel& chan);
  void ConfigureAnnouncement(vector<SdcElement>& element, 
            const vector<Announcement>& a,
            size_t short_id
            );
  void ConfigureAFS(vector<SdcElement>& afs_element, const AFS& afs);

};

#endif
