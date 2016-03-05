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

#ifndef _CTAUDIOFILESCE_H
#define _CTAUDIOFILESCE_H

#include <fstream>
#include <ServiceComponentEncoder.h>

class CCTAudioFileSCE : public ServiceComponentEncoder
{
public:
  CCTAudioFileSCE():ServiceComponentEncoder(),file(),buffer(NULL){}
  CCTAudioFileSCE(const CCTAudioFileSCE& s):ServiceComponentEncoder(s),file(),buffer(s.buffer){}
  CCTAudioFileSCE& operator=(const CCTAudioFileSCE& s)
  {
    *reinterpret_cast<ServiceComponentEncoder*>(this) = s;
    buffer = s.buffer;
    return *this;
  }
  ~CCTAudioFileSCE();
  virtual void ReConfigure(const ServiceComponent&);
  virtual void NextFrame(std::vector<uint8_t>& buffer, size_t max, double stoptime=0);
 	
protected:
  static const unsigned int DRM_FILE_HEADER_SIZE=44;
  unsigned long ReadInt(); 
  std::ifstream file;
  char *buffer;
};
#endif
