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

#ifndef _DRMMUX_H
#define _DRMMUX_H

#include <DrmMuxConfig.h>
#include "fac.h"
#include "sdc.h"
#include "msc.h"
#include "timestamp.h"

class DrmMux
{
public:

  uint32_t frame_count;
  FAC fac;
  SDC sdc;
  MSC msc;
  uint8_t robustness_mode;
  bytevector fac_bytes;
  crcbytevector sdc_bytes;
  vector<bytevector> msc_bytes;
  DrmMuxConfig wanted;
  enum { first_time, requested, signalled, running} reconfiguration_state;
  uint8_t reconfiguration_index;
  uint8_t service_pattern;

  DrmMux();
  virtual ~DrmMux();
  virtual void ReConfigure(const DrmMuxConfig&, unsigned int initial_reconfiguration_index=7);
  virtual void ReConfigureNow();
  virtual void NextFrame(DrmTime& timestamp);
  virtual void NextSuperFrame();
  void scheduleSdcElement(
        uint64_t requested_tx_us, 
        DrmTime& timestamp,
        const SdcElement& element);
};
#endif
