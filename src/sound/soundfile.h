/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable, Volker Fischer
 *
 * Description:
 *	See soundfile.cpp
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

#ifndef _SOUNDFILE_H
#define _SOUNDFILE_H

#include "../soundinterface.h"
#include <sndfile.h>
#include "../util/Pacer.h"

/* Classes ********************************************************************/
class CSoundFileIn : public CSoundInInterface
{
public:
	CSoundFileIn();
	virtual ~CSoundFileIn();

	virtual void		Enumerate(vector<string>&) const;
	virtual void		SetDev(int) {}
	virtual int		GetDev() const { return -1; }
	virtual void		SetFileName(const string& strFileName);

	virtual void 		Init(int iNewBufferSize, bool bNewBlocking = true, int iChannels=2);
	virtual bool 		Read(vector<_SAMPLE>& data);
	virtual void 		Close();

protected:
	FILE*			pFile;
	string			strInFileName;
	enum {
		fmt_txt, fmt_raw_mono, fmt_raw_stereo, fmt_other
		}		eFmt;
	int			iFileSampleRate;
	int			iFileChannels;
	CPacer*			pacer;
};

class CSoundFileOut : public CSoundOutInterface
{
public:
	CSoundFileOut();
	virtual ~CSoundFileOut();

	virtual void		Enumerate(vector<string>& choices) const { choices = files; }
	virtual void		SetDev(int iNewDevice);
	virtual void		SetDev(const string& s);
	virtual int		GetDev() const { return dev; }

	virtual void		Init(int iNewBufferSize, bool bNewBlocking = true, int iChannels=2);
	virtual void		Close();
	virtual bool		Write(vector<_SAMPLE>& data);
	virtual void		SetFiles(const vector<string>& choices) { files = choices; }

protected:

	vector<string> files;
	int channels,dev;
	bool blocking,device_changed;
	SNDFILE*	pFile;
};

#endif
