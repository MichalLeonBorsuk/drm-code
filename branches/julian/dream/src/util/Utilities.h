/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Implements:
 *	- Signal level meter
 *	- Bandpass filter
 *	- Modified Julian Date
 *	- Reverberation effect
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

#if !defined(UTILITIES_H__3B0BA660_CA63_4344_B3452345D31912__INCLUDED_)
#define UTILITIES_H__3B0BA660_CA63_4344_B3452345D31912__INCLUDED_

#include "../Parameter.h"
#include "Settings.h"
#include "Vector.h"
#include "../matlib/Matlib.h"
#include <map>
#include <iostream>

/* Definitions ****************************************************************/
#define	METER_FLY_BACK					15

/* Classes ********************************************************************/

struct CIpIf	{string name; uint32_t addr;};

/* Signal level meter ------------------------------------------------------- */
class CSignalLevelMeter
{
public:
	CSignalLevelMeter() : rCurLevel((_REAL) 0.0) {}
	virtual ~CSignalLevelMeter() {}

	void Init(_REAL rStartVal) {rCurLevel = Abs(rStartVal);}
	void Update(const _REAL rVal);
	void Update(const vector<_SAMPLE> vecrVal);
	void Update(const CVector<_REAL> vecrVal);
	_REAL Level() const;

protected:
	void doUpdate(const _REAL rVal);
	_REAL rCurLevel;
};


/* Bandpass filter ---------------------------------------------------------- */
class CDRMBandpassFilt
{
public:
	enum EFiltType {FT_TRANSMITTER, FT_RECEIVER};

	void Init(const int iNewBlockSize, const _REAL rOffsetHz,
		const ESpecOcc eSpecOcc, const EFiltType eNFiTy);
	void Process(CVector<_COMPLEX>& veccData);

protected:
	int				iBlockSize;

	CComplexVector	cvecDataTmp;

	CRealVector		rvecZReal; /* State memory real part */
	CRealVector		rvecZImag; /* State memory imaginary part */
	CRealVector		rvecDataReal;
	CRealVector		rvecDataImag;
	CFftPlans		FftPlanBP;
	CComplexVector	cvecB;
};


/* Modified Julian Date ----------------------------------------------------- */
class CModJulDate
{
public:
	CModJulDate() : iYear(0), iDay(0), iMonth(0) {}
	CModJulDate(const uint32_t iModJulDate) {Set(iModJulDate);}

	void Set(const uint32_t iModJulDate);

	int GetYear() {return iYear;}
	int GetDay() {return iDay;}
	int GetMonth() {return iMonth;}

protected:
	int iYear, iDay, iMonth;
};


/* Audio Reverbration ------------------------------------------------------- */
class CAudioReverb
{
public:
	CAudioReverb(const CReal rT60 = (CReal) 1.0);

	void Clear();
	CReal ProcessSample(const CReal rLInput, const CReal rRInput);

protected:
	void setT60(const CReal rT60);
	bool isPrime(const int number);

	CFIFO<int>	allpassDelays_[3];
	CFIFO<int>	combDelays_[4];
	CReal		allpassCoefficient_;
	CReal		combCoefficient_[4];
};

inline int Complement2toInt(const unsigned int iSize, CVector<_BINARY>* pbiData)
{
	int iVal = 0;
	const int iVecSize = iSize - 1;

	_BINARY bSign = (_BINARY)(*pbiData).Separate(1);

	if (bSign == 0)
		iVal = (int) (*pbiData).Separate(iVecSize);
	else
	{
		int k;

		unsigned int iPowerOf2 = 1;

		iPowerOf2 <<= (iVecSize - 1); /* power(2,iVecSize-1) */

		for (k = iVecSize - 1; k >= 0; k--)
		{
			if ((*pbiData).Separate(1) == 0)
				iVal = iVal + iPowerOf2;

			iPowerOf2 >>= 1; /* divide for 2 */
		}

		iVal = -1 * (iVal + 1);
	}

	return iVal;
}

void GetNetworkInterfaces(vector<CIpIf>& vecIpIf);

void GetComPortList(map<string,string>&);

#endif // !defined(UTILITIES_H__3B0BA660_CA63_4344_B3452345D31912__INCLUDED_)
