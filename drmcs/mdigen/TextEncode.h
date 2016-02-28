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

#ifndef AFX_TEXTENCODE_H
#define AFX_TEXTENCODE_H

#include <libxml/encoding.h>
#include <Crc16.h>
#include <bytevector.h>

class CTextEncode  
{
public:
  static const unsigned NO_SYNC_BYTES;
  static const unsigned NO_SEG_CHAR;
  static const unsigned NO_MSG_CHAR;
  static const unsigned MSG_SEGS;
  static const unsigned MSG_SYNC_WORD;
  static const unsigned BYTES_PER_FRAME;
  
	CTextEncode():m_toggle(false),m_len(0){}
    ~CTextEncode() {}
    void puttext(const char *utf8string);
    void put(uint8_t byte);
    void putheader(bool toggle, bool first, bool last,
        bool command_flag, uint8_t field1, uint8_t field2);
    void cleartext();
	void puttextsegment(int segment, bool last,
			 const char *text, size_t length);
	void getsubsegment(bytevector& buf);
    bool empty();
    void reset();
	bool m_toggle;
protected:
   	uint8_t m_buffer[192];
   	uint8_t *m_outp;
   	uint16_t m_len;
	CCrc16 m_crc;
};
#endif
