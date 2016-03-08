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

#include "Crc16.h"

// This class provides functions for computing 16-bit CRC values.
// It implements both a slow bit-at-a-time method and a table-driven
// method.  The simple method was implemented as a way of verifying
// that the optimized table-method works.


// POPULAR POLYNOMIALS

//  CCITT:	x^16 + x^12 + x^5 + x^0 (set m_Polynomial to 0x1021)
//
//	CRC-16:	x^16 + x^15 + x^2 + x^0 (set m_Polynomial to 0x8005)


// REFLECTED INPUT/CRC

//	Reflected input and reflected CRC is common with serial interfaces.
//  This is because the least-significant-bit is typically transmitted
//  first with serial interfaces.

bool CCrc16::no_table=true;
uint16_t CCrc16::m_Table[256];

CCrc16::CCrc16():m_Crc(0xffff),m_ResetValue(0xffff),m_Polynomial(0x1021),
    m_Reflect(false),m_ReflectCrc(false),m_CrcXorOutput(0xffff)
{
    if(no_table) { // static initialisation
        CreateTable();
        no_table=false;
    }
}

uint16_t CCrc16::ReverseBits( uint16_t x )
{
    int i;
    uint16_t newValue = 0;

    for( i=15 ; i>=0; i-- )
    {
        newValue |= (x & 1) << i;
        x >>= 1;
    }
    return newValue;
}

uint8_t CCrc16::ReverseBits( uint8_t b )
{
    int i;
    uint8_t newValue = 0;

    for( i=7 ; i>=0; i-- )
    {
        newValue |= (b & 1) << i;
        b >>= 1;
    }
    return newValue;
}

uint16_t CCrc16::CreateTableEntry( int index )
{
    int   i;
    uint16_t r;

    uint16_t inbyte = (uint16_t) index;

    if ( m_Reflect )
        inbyte = ReverseBits( (uint8_t) inbyte );

    r = inbyte << 8;

    for (i=0; i<8; i++)
        if ( r & 0x8000 )
            r = (r << 1) ^ m_Polynomial;
        else
            r <<= 1;
    if ( m_Reflect )
        r = ReverseBits( r );

    return r;
}

void CCrc16::CreateTable()
{

    for( int i=0; i<256; i++ )
        m_Table[i] = CreateTableEntry( i );
}

uint16_t CCrc16::ComputeCrcUsingTable(  uint8_t* data, int length )
{
    int i;
    m_Crc = m_ResetValue;

    // Note: If using a table, m_ReflectCrc and m_Reflect
    // should match ( both true or both false ).  Otherwise
    // the CRC calculation will be incorrect.

    //	ASSERT( (m_ReflectCrc && m_Reflect) ||
    //				(!m_ReflectCrc && !m_Reflect));

    if( m_ReflectCrc )
    {
        for( i=0; i < length; i++ )
        {
            m_Crc = m_Table[ (m_Crc ^ *(data++)) & 0xFF ] ^ ( m_Crc >> 8 );
        }
    }
    else
    {
        for( i=0; i < length; i++ )
        {
            m_Crc = m_Table[ ((m_Crc >> 8 ) ^ *(data++)) & 0xFF ] ^ (m_Crc << 8 );
        }
    }

    m_Crc = m_CrcXorOutput ^ m_Crc;

    return m_Crc;
}

void CCrc16::reset()
{
    m_Crc = m_ResetValue;
}

void CCrc16::accumulate(uint8_t byte)
{
    m_Crc = m_Table[ ((m_Crc >> 8 ) ^ byte) & 0xFF ] ^ (m_Crc << 8 );
}

uint16_t CCrc16::result()
{
    return m_CrcXorOutput ^ m_Crc;
}

