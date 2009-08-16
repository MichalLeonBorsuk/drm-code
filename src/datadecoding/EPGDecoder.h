/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	ETSI DAB/DRM Electronic Programme Guide class
 *
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

#ifndef _EGPDECODER_H
#define _EGPDECODER_H

#include "DataApplication.h"
#include "DABMOT.h"
#include <string>

class EPGDecoder: public DataApplication
{
public:

	EPGDecoder(CParameter&);
	virtual ~EPGDecoder();

	void AddDataUnit(CVector<_BINARY>&);
	void Reset();

protected:

    CMOTDABDec motdecoder;
    std::string saveDir;

};

class EPGDecoderFactory: public DataApplicationFactory
{
public:

	DataApplication* create(CParameter& p) { return new EPGDecoder(p); }
};


#endif
