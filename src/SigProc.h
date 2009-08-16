/******************************************************************************\
 * Copyright (c) 2009 British Broadcasting Corporation
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	some classes
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

#ifndef _SIGPROC_H_
#define _SIGPROC_H_

class CEquSig
{
public:
	CEquSig() : cSig(_COMPLEX((_REAL) 0.0, (_REAL) 0.0)), rChan((_REAL) 0.0) {}
	CEquSig(const _COMPLEX cNS, const _REAL rNC) : cSig(cNS), rChan(rNC) {}

	_COMPLEX	cSig; /* Actual signal */
	_REAL		rChan; /* Channel power at this cell */
};

#endif
