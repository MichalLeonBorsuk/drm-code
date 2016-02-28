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

#ifndef _BYTEVECTOR_H
#define _BYTEVECTOR_H

#include <string>
#include <bitset>
#include "bitvector.h"
#include "bytev.h"

class bytevector : public bytev
{
public:
    uint8_t in_progress, bits;

    bytevector():bytev(),in_progress(0),bits(0) {}
    virtual ~bytevector() {}
    virtual void clear() {
        bytev::clear();
        in_progress=0;
        bits=0;
    }
    void put(uint64_t, unsigned=8);
    void put(const bitvector&);
    void put(const string&);
    void put(const bytev&);
    void putbytes(const char *, unsigned);
    uint64_t get(unsigned=8);
    int64_t getSigned(unsigned);
    void get(bytev&, unsigned);
    uint8_t peek() const;
    bool dataAvailable() const;
protected:
    virtual void putb(uint8_t);
};

bytevector& operator<<(bytevector& b, const bytev& c);

bytevector& operator<< (bytevector& b, const string& c);

template<size_t X> bytevector& operator<< (bytevector& b, const bitset<X>& c)
{
    b.put(c.to_ulong(), static_cast<unsigned>(c.size()));
    return b;
}


#endif
