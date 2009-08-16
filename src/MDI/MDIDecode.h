/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden, Julian Cable
 *
 * Description:
 *	see MDIInBuffer.cpp
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

#ifndef MDIDECODE_H_INCLUDED
#define MDIDECODE_H_INCLUDED

#include "../GlobalDefinitions.h"
#include "../util/ReceiverModul.h"
#include "../util/TransmitterModul.h"
#include "MDIDefinitions.h"
#include "TagPacketDecoderMDI.h"

class CDecodeRSIMDI
{
public:
	CDecodeRSIMDI():TagPacketDecoder() {}
	virtual ~CDecodeRSIMDI() {}
	virtual void Init(CParameter& Parameters);
	virtual void ProcessData(CParameter& Parameters,
				CVectorEx<_BINARY>* vecInputData,
				CVectorEx<_BINARY>* vecfac_Data,
				CVectorEx<_BINARY>* vecsdc_Data,
				vector<CVectorEx<_BINARY>* >& vecpvecData,
				int& iOutputBlockSize,
				int& iOutputBlockSize2,
				vector<int>& veciOutputBlockSize
				);

protected:
	CTagPacketDecoderMDI TagPacketDecoder;
	int iFramesSinceSDC;
};

class CDecodeRSI : public CReceiverModul<_BINARY, _BINARY>
{
public:
	CDecodeRSI():Decoder() {}
	virtual ~CDecodeRSI() {}
protected:
	virtual void InitInternal(CParameter& Parameters);
	virtual void ProcessDataInternal(CParameter& Parameters);

	CDecodeRSIMDI Decoder;
};

//class CDecodeMDI : public CTransmitterModul<_BINARY, _BINARY, 1, 2+MAX_NUM_STREAMS>
class CDecodeMDI : public CTransmitterModul<_BINARY, _BINARY>
{
public:
	CDecodeMDI():Decoder() {}
	virtual ~CDecodeMDI() {}
	void Expect(int);
protected:
	virtual void InitInternal(CParameter& Parameters);
	virtual void ProcessDataInternal(CParameter& Parameters);

	CDecodeRSIMDI Decoder;
};

#endif
