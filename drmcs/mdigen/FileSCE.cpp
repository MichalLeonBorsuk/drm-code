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


#include "FileSCE.h"
#include <platform.h>
#include <iostream>

FileSCE::~FileSCE()
{
  if(file.is_open())
    file.close();
  if(buffer)
    delete[] buffer;
}

void FileSCE::ReConfigure(const ServiceComponent& config)
{
	string oldfile = current.source_selector;
    ServiceComponentEncoder::ReConfigure(config);
    if(oldfile != current.source_selector){
      if(file.is_open())
        file.close();
    }
    if(!file.is_open()) {
      file.open(current.source_selector.c_str(), ios::in|ios::binary);
      if(!file.is_open()) {
        throw string("FileSCE: can't open file ")+current.source_selector;
      }

      if(buffer)
        delete[] buffer;
      buffer = new char[current.bytes_per_frame];
    }
}

unsigned long FileSCE::ReadInt()
{
	unsigned long n;
	file.read((char*)&n, 4);
	return ntohl(n);
}

void FileSCE::NextFrame(bytevector& buf, size_t max, double)
{
	if (!current.misconfiguration 
         && max>=(unsigned)current.bytes_per_frame
         && file.is_open())
	{
		file.read(buffer, current.bytes_per_frame);
		buf.putbytes(buffer, current.bytes_per_frame);
        current.loop=true;
		if(file.eof()) {
            file.clear();
            if( current.loop) {
			    file.seekg(0);
			} else {
			    buf.insert(buf.end(), current.bytes_per_frame, 0);
			}
        }
	}
}
