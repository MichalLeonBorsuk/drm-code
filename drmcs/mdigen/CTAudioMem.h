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

#include <stdio.h>
#include <ServiceComponentEncoder.h>

class CCTAudioFileSCE : public CAudioSCE
{
public:
    CCTAudioFileSCE():m_file(NULL),m_buf(NULL){}
    //CCTAudioFileSCE(const CCTAudioFileSCE& s);
    ~CCTAudioFileSCE();
	virtual void ReConfigure(xmlNodePtr config);
 	virtual void NextFrame(bytevector& buf, size_t max, double stoptime=0);
protected:
    static const unsigned int DRM_FILE_HEADER_SIZE=44;
	unsigned long ReadInt();
    
    FILE *m_file;
    char *m_buf;
};
#endif
