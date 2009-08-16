/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	See DrmModulator.cpp
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

#ifndef _DRMMODULATOR_H
#define _DRMMODULATOR_H

#include "GlobalDefinitions.h"
#include "util/Buffer.h"
#include "mlc/MLC.h"
#include "interleaver/SymbolInterleaver.h"
#include "ofdmcellmapping/OFDMCellMapping.h"
#include "OFDM.h"
#include "DRMSignalIO.h"
#include "util/TransmitterModul_impl.h"

/* Classes ********************************************************************/
class CSettings;
class CParameter;

class CDRMModulator
{
public:
							CDRMModulator();
	virtual 				~CDRMModulator() {}
	void					LoadSettings(CSettings&, CParameter&);
	void					SaveSettings(CSettings&, const CParameter&) const;
	void					Init(CParameter& Parameter);
	void					WriteData(CParameter& Parameter,
								CBuffer<_BINARY>& FACBuf, CBuffer<_BINARY>& SDCBuf,
								vector<CSingleBuffer<_BINARY> >& MSCBuf);
	void					Cleanup(CParameter&);

	void					GetSoundOutChoices(vector<string>&) const;
	void					SetOutputs(const vector<string>& o);
	void					GetOutputs(vector<string>& o) const;

protected:

	CSingleBuffer<_COMPLEX>	MLCEncBuf;
	CSingleBuffer<_COMPLEX>	CarMapBuf;
	CSingleBuffer<_COMPLEX>	OFDMModBuf;
	CCyclicBuffer<_COMPLEX>	IntlBuf;
	CCyclicBuffer<_COMPLEX>	FACMapBuf;
	CCyclicBuffer<_COMPLEX>	SDCMapBuf;

	/* Modules */
	CTransmitData			TransmitData;
	CMLCEncoder				MSCMLCEncoder;
	CMLCEncoder				FACMLCEncoder;
	CMLCEncoder				SDCMLCEncoder;
	CSymbInterleaver		SymbInterleaver;
	COFDMCellMapping		OFDMCellMapping;
	COFDMModulation			OFDMModulation;
};

#endif
