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

#include "bytevector.h"

using namespace std;

void bytevector::putb(uint8_t b)
{
    _data.push_back(b);
}

void bytevector::put(uint64_t n, unsigned fmt)
{
    if(8U-bits >= fmt) { // it all fits in the in_progress byte
        uint8_t mask = (1<<fmt)-1;
        uint8_t b = static_cast<uint8_t>(n & mask);
        bits += fmt;
        in_progress |= b << (8-bits);
        if(bits==8) { // a whole byte
            putb(in_progress);
            in_progress = 0;
            bits = 0;
        }
    } else { // spans more than one byte
        if(bits>0) { // grab enough bits to fill the in_progress byte
            int leading_bits = 8 - bits;
            uint8_t mask = (1<<leading_bits)-1;
            uint8_t b = static_cast<uint8_t>((n >> (fmt-leading_bits)) & mask);
            bits += leading_bits; // should be 8!
            in_progress |= b;
            fmt -= leading_bits;
            putb(in_progress);
            in_progress = 0;
            if(bits != 8) {
                fprintf(stderr, "bytevector bits should be 8 is %d\n", bits);
            }
            bits = 0;
        }
        // handle complete bytes
        int mid = fmt/8;
        for(int i=0; i<mid; i++) {
            uint8_t b = static_cast<uint8_t>((n >> (fmt-8)) & 0xff);
            putb(b);
            fmt -= 8;
        }
        // handle the last bits: fmt will now be < 8 and bits will be 0
        if(fmt>0) {
            bits = fmt;
            in_progress = static_cast<uint8_t>((n & ((1<<fmt)-1)) << (8-bits));
        }
    }
}

void bytevector::put(const string& s)
{
    if(bits==0) {
        for(unsigned i=0; i<s.length(); i++)
            putb(s[i]);
    } else {
        for(unsigned i=0; i<s.length(); i++)
            put(s[i], 8);
    }
}

void bytevector::put(const vector<uint8_t>& s)
{
    if(bits==0) {
        for(unsigned i=0; i<s.size(); i++)
            putb(s[i]);
    } else {
        for(unsigned i=0; i<s.size(); i++)
            put(s[i], 8);
    }
}

void bytevector::putbytes(const char* s, unsigned bytes)
{
    if(bits==0) {
        for(unsigned i=0; i<bytes; i++)
            putb(s[i]);
    } else {
        for(unsigned i=0; i<bytes; i++)
            put(s[i], 8);
    }
}

uint64_t bytevector::get(unsigned wanted)
{
    uint64_t result = 0;
    if(8*_data.size()+bits<wanted) {
        return 0;
    } else {
        // stuff in progress
        uint8_t b = (wanted>=bits)?bits:wanted;
        result = in_progress >> (8-b);
        in_progress <<= b;
        wanted -= b;
        bits -= b;
        // whole octets
        vector<uint8_t>::iterator i = _data.begin();
        while(wanted>=8) {
            result = (result << 8) | *i++;
            wanted -= 8;
        }
        // partial last octet
        if(wanted>0) {
            in_progress = *i++;
            bits = 8;
            result = (result << wanted) | in_progress >> (8-wanted);
            in_progress <<= wanted;
            bits -= wanted;
        }
        _data.erase(_data.begin(), i);
    }
    return result;
}

int64_t bytevector::getSigned(unsigned wanted)
{
    uint64_t n = get(wanted);
    if(n & (1ULL<<(wanted-1))) {
        uint64_t x = 1<<wanted;
        uint64_t xx = x-1;
        uint64_t xxx = ~xx;
        return int64_t(xxx|n);
    } else
        return n;
}

void bytevector::get(vector<uint8_t>& v, unsigned len)
{
    if(bits==0) {
        if(len>=_data.size()) {
            vector<uint8_t>::iterator s = _data.begin();
            v.insert(v.end(), s, s+len);
            _data.erase(s, s+len);
        }
    } else {
        v.resize(len);
        for(unsigned i=0; i<len; i++)
            v[i]=static_cast<uint8_t>(get(8));
    }
}


bytevector& operator<<(bytevector& b, const vector<uint8_t>& c)
{
    b.put(c);
    return b;
}

bytevector& operator<< (bytevector& b, const string& c)
{
    b.put(c);
    return b;
}

uint8_t bytevector::peek() const
{
    return _data[0];
}

bool bytevector::dataAvailable() const
{
    if(bits==0)
        return _data.size()>0;
    else
        return true;
}
