/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *
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

#if !defined(BIT_INTERLEAVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define BIT_INTERLEAVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../interleaver/BlockInterleaver.h"
#include "../util/Vector.h"
#include "ChannelCode.h"

/* Classes ********************************************************************/
class CBitInterleaver: public CBlockInterleaver
{
public:
	CBitInterleaver():CBlockInterleaver(),
	vecInterlMemory1(),vecInterlMemory2()
	{}
	virtual ~CBitInterleaver() {}

	void Init(int iNewx_in1, int iNewx_in2, int it_0);
	void Interleave(CVector<_DECISION>& InputData);

protected:
	vector<_DECISION>	vecInterlMemory1;
	vector<_DECISION>	vecInterlMemory2;
};

class CBitDeinterleaver: public CBlockInterleaver
{
public:
	CBitDeinterleaver():CBlockInterleaver(),
	vecDeinterlMemory1(),vecDeinterlMemory2()
	{}
	virtual ~CBitDeinterleaver() {}

	void Init(int iNewx_in1, int iNewx_in2, int it_0);
	void Deinterleave(CVector<CDistance>& vecInput);

protected:
	vector<CDistance>	vecDeinterlMemory1;
	vector<CDistance>	vecDeinterlMemory2;
};


#endif // !defined(BIT_INTERLEAVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
