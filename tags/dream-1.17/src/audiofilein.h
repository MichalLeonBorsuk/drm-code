/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Decription:
 * Read a file at the correct rate
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

#ifndef _AUDIOFILEIN
#define _AUDIOFILEIN

#include "soundinterface.h"
#include "util/Pacer.h"

/* Classes ********************************************************************/
class CAudioFileIn : public CSoundInInterface
{
public:
    CAudioFileIn();
    virtual ~CAudioFileIn();

    virtual void		Enumerate(vector<string>&) { }
    virtual void		SetDev(int) {}
    virtual int			GetDev() {
        return -1;
    }
    virtual void		SetFileName(const string& strFileName);

    virtual void 		Init(int iNewSampleRate, int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE);
    virtual _BOOLEAN 	Read(CVector<short>& psData);
    virtual void 		Close();

protected:
    FILE*				pFileReceiver;
    string				strInFileName;
    enum {
        fmt_txt, fmt_raw_mono, fmt_raw_stereo, fmt_other
    }				eFmt;
    int					iSampleRate;
    int					iFileSampleRate;
    int					iFileChannels;
    CPacer*				pacer;
    short *buffer;
    int iBufferSize;
};

#endif
