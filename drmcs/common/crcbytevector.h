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

#ifndef _CRCBYTEVECTOR_H
#define _CRCBYTEVECTOR_H

#include "bytevector.h"
#include "Crc16.h"

class crcbytevector : public bytevector
{
public:
    CCrc16 crc;
    crcbytevector():bytevector(),crc() {}
    virtual ~crcbytevector() {}
    virtual void clear() {
        crc.reset();
        bytevector::clear();
    }
    virtual void putb(uint8_t b) {
        push_back(b);
        crc.accumulate(b);
    }
};

#endif
