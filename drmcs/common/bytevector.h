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

#include <vector>
#include <string>
#include <bitset>
#include <cstdint>

class bytevector 
{
public:
    uint8_t in_progress, bits;

    bytevector():in_progress(0),bits(0),_data() {}
    bytevector(const std::vector<uint8_t>& v):in_progress(0),bits(0),_data(v) {}
    virtual ~bytevector() {}
    virtual 
    void clear() {
        _data.clear();
        in_progress=0;
        bits=0;
    }
    const std::vector<uint8_t>& data() const { return _data; }
    void put(uint64_t, unsigned);
    void put(const std::string&);
    void put(const bytevector& bv) { put(bv._data); }
    void put(const std::vector<uint8_t>&);
    void putbytes(const char *, unsigned);
    void putb(uint8_t);
    uint64_t get(unsigned=8);
    int64_t getSigned(unsigned);
    void get(std::vector<uint8_t>&, unsigned);
    uint8_t peek() const;
    bool dataAvailable() const;
    void reserve(size_t n) { _data.reserve(n); }
    void resize(size_t n) { _data.resize(n); }
    size_t size() const { return _data.size(); }
    bool empty() const { return _data.empty(); }
    void push_back(const uint8_t& val) { _data.push_back(val); }
    uint8_t& operator[](int i) { return _data[i]; }
protected:
    std::vector<uint8_t> _data;
};

bytevector& operator<<(bytevector& b, const std::vector<uint8_t>& c);

bytevector& operator<< (bytevector& b, const std::string& c);

template<size_t X> bytevector& operator<< (bytevector& b, const std::bitset<X>& c)
{
    b.put(c.to_ulong(), static_cast<unsigned>(c.size()));
    return b;
}


#endif
