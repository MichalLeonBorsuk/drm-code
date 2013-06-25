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

#ifndef _CRC16_H
#define _CRC16_H

#include "platform.h"
/* TODO (jfbc#1#): Use this class in the PFT layer */

class CCrc16
{
public:
	CCrc16();
	uint16_t	ReverseBits( uint16_t x );
	uint8_t		ReverseBits( uint8_t b );
	uint16_t	CreateTableEntry( int index );
	void		CreateTable();
	uint16_t	ComputeCrcUsingTable(  uint8_t* data, int length );
    void        reset();
    void        accumulate(uint8_t byte);
    uint16_t    result();

	uint16_t	m_Crc;
	uint16_t	m_ResetValue;
	uint16_t	m_Polynomial;
	bool		m_Reflect;
	bool		m_ReflectCrc;
	uint16_t	m_CrcXorOutput;
	static uint16_t	m_Table[256];
	static bool no_table;
};
#endif
