/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *
 *  This defines a concrete subclass of CPacketSink that writes to a file
 *  For the moment this will be a raw file but FF could be added as a decorator
 *  The writing can be stopped and started - if it is not currently writing,
 *  any packets it receives will be silently discarded
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

#ifndef PACER_H_INCLUDED
#define PACER_H_INCLUDED

#include "../GlobalDefinitions.h"

#ifdef _WIN32
# ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
# endif
# define _WIN32_WINNT 0x0400
# include <windows.h>
# include <qnamespace.h>
#endif

class CPacer
{
public:
	CPacer(uint64_t ns);
	~CPacer();
	uint64_t nstogo();
	void wait();
	void changeInterval(uint64_t ns) { interval = ns; }
protected:
	uint64_t timekeeper;
	uint64_t interval;
#ifdef _WIN32
	Qt::HANDLE hTimer;
#endif
};
#endif
