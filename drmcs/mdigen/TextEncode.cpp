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

#include <iostream>
#include <cstring>

#include "TextEncode.h"

using namespace std;

/*
The text message (when present) shall occupy the last four bytes of the lower
protected part of each logical frame carrying an audio stream. The message is
divided into a number of segments and UTF-8 character coding is used. The
beginning of each segment of the message is indicated by setting all four bytes
to the value 0xFF. When no text message is available for insertion all four
bytes shall be set to 0x00. The text message may comprise up to 8 segments.
Each segment consists of a header, a body and a CRC. The body shall contain
16 bytes of character data unless it is the last segment in which case it may
contain less than 16 bytes. Each segment is further divided into four-byte
pieces which are placed into each successive frame. If the length of the last
segment is not a multiple of four then the incomplete frame shall be padded
with 0x00 bytes. The structure of the segment is as follows:

- Header 16 bits.
- Body n x 8 bits.
- CRC 16 bits.

The Header is made up as follows:

- toggle bit 1 bit.
- first flag 1 bit.
- last flag 1 bit.
- command flag 1 bit.
- field 1 4 bits.
- field 2 4 bits.
- rfa 4 bits.*/

const unsigned CTextEncode::NO_MSG_CHAR=128;
const unsigned CTextEncode::NO_SEG_CHAR=16;
const unsigned CTextEncode::BYTES_PER_FRAME=4;
const unsigned CTextEncode::MSG_SEGS=8;
const unsigned CTextEncode::MSG_SYNC_WORD=0xFF;
const unsigned CTextEncode::NO_SYNC_BYTES=4;


void CTextEncode::reset()
{
    m_toggle=false;
    m_len=0;
}

bool CTextEncode::empty ()
{
    return m_len==0;
}

void CTextEncode::put(uint8_t byte)
{
    m_crc.accumulate(byte);
    if(m_len<sizeof(m_buffer))
        m_buffer[m_len++] = byte;
    else {
        cerr << "text message encode error: message too long " << m_len << " <= " << sizeof(m_buffer) << endl;
    }
}

/*
- toggle bit 1 bit.
- first flag 1 bit.
- last flag 1 bit.
- command flag 1 bit.
- field 1 4 bits.
- field 2 4 bits.
*/
void CTextEncode::putheader(bool toggle, bool first, bool last,
                            bool command_flag, uint8_t field1, uint8_t field2)
{

    /*printf("toggle %d, first %d, last %d, command %d, field1 %d field2 %d, ",
           toggle, first, last, command_flag, field1, field2);*/

    // calc CRC from here
    m_crc.reset();

    uint8_t b = 0;
    if (toggle)
        b |= 0x80;
    if (first)
        b |= 0x40;
    if (last)
        b |= 0x20;
    if (command_flag)
        b |= 0x10;
    b |= field1;
    put(b);

    put((field2 << 4) & 0xf0);
}

void CTextEncode::cleartext()
{
    uint16_t crc;
    //put on the four sync bytes of 0xFF
    for (unsigned i = 0; i < NO_SYNC_BYTES; i++)
        put(MSG_SYNC_WORD);

    putheader(m_toggle, true, true, true, 1, 0xf);
    crc = m_crc.result();
    put(crc >> 8);
    put(crc & 0xff);
}

void CTextEncode::puttextsegment(int segment, bool last,
                                 const char *text, size_t length)
{
    uint16_t crc;
    uint8_t field2;
    if (segment == 0)
        field2 = 15;
    else
        field2 = segment & 0x07;
    //put on the four sync bytes of 0xFF
    for (unsigned i = 0; i < NO_SYNC_BYTES; i++)
        put(MSG_SYNC_WORD);

    // reset the CRC generator
    m_crc.reset();

    // generate the header
    putheader(m_toggle, segment == 0, last, false, static_cast<uint8_t>(length-1), field2);

    //generate body
    for (size_t i=0; i<length; i++) {
        put(text[i]);
        //printf("%c, len=%u", text[i], m_len);
        //printf("%c", text[i]);
    }

    crc = m_crc.result();
    put(crc >> 8);
    put(crc & 0xff);
    //printf(", crc=%04x\n", crc);
}

void CTextEncode::puttext(const char *utf8string)
{
    unsigned seg;
    size_t bytes = strlen(utf8string);

    if(bytes>128) {
        bytes = 128;
    }

    const char *textp = utf8string;

    for(seg=0; bytes>=NO_SEG_CHAR; seg++) {
        puttextsegment(seg, bytes==NO_SEG_CHAR, textp, NO_SEG_CHAR);
        textp += NO_SEG_CHAR;
        bytes -= NO_SEG_CHAR;
    }
    if(bytes>0) {
        puttextsegment(seg, true, textp, bytes);
    }
    m_outp=m_buffer;
    // each text message should have a different value of the toggle bit
    //m_toggle = !m_toggle; // but we will do this in the enclosing class
}

void CTextEncode::getsubsegment(bytevector& buf)
{
    unsigned bytes = 0;
    if(m_len > 0) {
        if(m_len>BYTES_PER_FRAME) {
            bytes += BYTES_PER_FRAME;
            m_len -= BYTES_PER_FRAME;
        } else {
            bytes += m_len;
            m_len = 0;
        }
        buf.putbytes((char*)m_outp, bytes);
        m_outp += bytes;
    }
    while(bytes<BYTES_PER_FRAME) {
        buf.put(0, 8);
        bytes++;
    }
}
