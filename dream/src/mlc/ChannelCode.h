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

#if !defined(CHANNEL_CODE_H__3B0BA660_CA63345347A0D31912__INCLUDED_)
#define CHANNEL_CODE_H__3B0BA660_CA63345347A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../tables/TableMLC.h"
#include "../util/Vector.h"
#include "../Parameter.h"

/* MAP ---------------------------------------------------------------------- */
#ifdef USE_MAX_LOG_MAP
typedef _REAL							_DECISION;
# define ML_SOFT_INF_MAX_VALUE			((_DECISION) 1e10)
inline _BINARY ExtractBit(_DECISION dD) {return dD > 0 ? 1 : 0;}
inline _DECISION BitToSoft(_BINARY biB) {return biB == 0 ? -1.0 : 1.0;}
#else
typedef _BINARY							_DECISION;
#define ExtractBit(a)					(a)
#define BitToSoft(a)					(a)
#endif

/* Classes ********************************************************************/
/* For metric */
class CDistance
{
public:
	/* Distance towards 0 or towards 1 */
	_REAL rTow0;
	_REAL rTow1;
};

class CChannelCode
{
public:
	CChannelCode();
	virtual ~CChannelCode() {}

	inline _BINARY Convolution(const _BYTE byNewStateShiftReg,
							   const int iGenPolyn) const
	{
		/* Mask bits with generator polynomial and get convolution result from
		   pre-calculated table (speed optimization). Since we have a AND
		   operation on the "byGeneratorMatrix", the index of the convolution
		   table cannot exceed the size of the table (although the value in
		   "byNewStateShiftReg" can be larger) */
		return vecbiParity[byNewStateShiftReg & byGeneratorMatrix[iGenPolyn]];
	}

	CVector<int> GenPuncPatTable(ECodScheme eNewCodingScheme,
								 EChanType eNewChannelType,
								 int iN1, int iN2,
								 int iNewNumOutBitsPartA,
								 int iNewNumOutBitsPartB,
								 int iPunctPatPartA, int iPunctPatPartB,
								 int iLevel);


private:
	_BINARY vecbiParity[1 << BITS_BINARY];
};


#endif // !defined(CHANNEL_CODE_H__3B0BA660_CA63345347A0D31912__INCLUDED_)
