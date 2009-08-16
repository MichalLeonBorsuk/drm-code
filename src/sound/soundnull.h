/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	dummy sound classes
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

#ifndef _SOUNDNULL_H
#define _SOUNDNULL_H

#include "../soundinterface.h"

/* Classes ********************************************************************/
class CSoundInNull : public CSoundInInterface
{
public:
	CSoundInNull():iDev(-1){}
	virtual ~CSoundInNull() {}

	virtual void	Init(int, bool, int) {}
	virtual bool	Read(vector<_SAMPLE>&) { return false; }
	virtual void	Enumerate(vector<string>&choices) const { choices.push_back("(File or Network)"); }
	virtual int	GetDev() const { return iDev; }
	virtual void	SetDev(int iNewDev) { iDev = iNewDev; }
	virtual void	Close() {}
private:
    int iDev;
};

class CSoundOutNull : public CSoundOutInterface
{
public:
	CSoundOutNull():iDev(-1){}
	virtual ~CSoundOutNull(){}

	virtual void		Init(int, bool, int) {}
	virtual bool	    Write(vector<_SAMPLE>&) { return false;}
	virtual void		Enumerate(vector<string>& choices) {choices.push_back("(None)");}
	virtual int			GetDev() { return iDev; }
	virtual void		SetDev(int iNewDev) { iDev = iNewDev; }
	virtual void		Close() {}
private:
    int iDev;
};

#endif
