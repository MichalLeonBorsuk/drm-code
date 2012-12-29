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
 *	- Hamlib interface
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

#include "../GlobalDefinitions.h"
#include "Settings.h"
#include "Vector.h"
#include "../matlib/Matlib.h"
#include <map>
#include <iostream>

#ifdef HAVE_LIBHAMLIB
# include <hamlib/rig.h>
#endif


/* Definitions ****************************************************************/
#define	METER_FLY_BACK					15

/* Classes ********************************************************************/
/* Thread safe counter ------------------------------------------------------ */
class CCounter
{
public:
	CCounter() : count(0) {}
	~CCounter() {}
	unsigned int operator++();
	unsigned int operator++(int);
	unsigned int operator--();
	unsigned int operator--(int);
	operator unsigned int();
	unsigned int operator=(unsigned int value);
private:
	CMutex mutex;
	unsigned int count;
};


/* Signal level meter ------------------------------------------------------- */
class CSignalLevelMeter
{
public:
	CSignalLevelMeter() : rCurLevel((_REAL) 0.0) {}
	virtual ~CSignalLevelMeter() {}

	void Init(_REAL rStartVal) {rCurLevel = Abs(rStartVal);}
	void Update(const _REAL rVal);
	void Update(const CVector<_REAL> vecrVal);
	void Update(const CVector<_SAMPLE> vecsVal);
	_REAL Level();

protected:
	_REAL rCurLevel;
};


/* Bandpass filter ---------------------------------------------------------- */
class CDRMBandpassFilt
{
public:
	enum EFiltType {FT_TRANSMITTER, FT_RECEIVER};

	void Init(int iSampleRate, int iNewBlockSize, _REAL rOffsetHz,
		ESpecOcc eSpecOcc, EFiltType eNFiTy);
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
	CModJulDate() : iYear(0), iDay(0), iMonth(0), iModJulDate(0) {}
	CModJulDate(const uint32_t iModJulDate) {Set(iModJulDate);}
	CModJulDate(const uint32_t iYear, const uint32_t iMonth, const uint32_t iDay)
		{Get(iYear, iMonth, iDay);}

	void Set(const uint32_t iModJulDate);
	void Get(const uint32_t iYear, const uint32_t iMonth, const uint32_t iDay);

	int GetYear() {return iYear;}
	int GetDay() {return iDay;}
	int GetMonth() {return iMonth;}
	int GetModJulDate() {return iModJulDate;}

protected:
	int iYear, iDay, iMonth, iModJulDate;
};


/* Audio Reverbration ------------------------------------------------------- */
class CAudioReverb
{
public:
	CAudioReverb() {}
	void Init(CReal rT60, int iSampleRate);
	void Clear();
	CReal ProcessSample(const CReal rLInput, const CReal rRInput);

protected:
	void setT60(const CReal rT60, int iSampleRate);
	_BOOLEAN isPrime(const int number);

	CFIFO<int>	allpassDelays_[3];
	CFIFO<int>	combDelays_[4];
	CReal		allpassCoefficient_;
	CReal		combCoefficient_[4];
};


#ifdef HAVE_LIBHAMLIB
/* Hamlib interface --------------------------------------------------------- */
class CHamlib
{
public:
	enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};

	CHamlib();
	virtual ~CHamlib();

	struct SDrRigCaps
	{
		SDrRigCaps() : strManufacturer(""), strModelName(""),
			eRigStatus(RIG_STATUS_ALPHA),bIsSpecRig(FALSE) {}
		SDrRigCaps(const string& strNMan, const string& strNModN, rig_status_e eNSt, _BOOLEAN bNsp) :
			strManufacturer(strNMan), strModelName(strNModN), eRigStatus(eNSt),
			bIsSpecRig(bNsp)
			{}
		SDrRigCaps(const SDrRigCaps& nSDRC) :
			strManufacturer(nSDRC.strManufacturer),
			strModelName(nSDRC.strModelName),
			eRigStatus(nSDRC.eRigStatus), bIsSpecRig(nSDRC.bIsSpecRig) {}

		inline SDrRigCaps& operator=(const SDrRigCaps& cNew)
		{
			strManufacturer = cNew.strManufacturer;
			strModelName = cNew.strModelName;
			eRigStatus = cNew.eRigStatus;
			bIsSpecRig = cNew.bIsSpecRig;
			return *this;
		}

		string			strManufacturer;
		string			strModelName;
		rig_status_e	eRigStatus;
		_BOOLEAN		bIsSpecRig;
	};

	_BOOLEAN		SetFrequency(const int iFreqkHz);
	ESMeterState	GetSMeter(_REAL& rCurSigStr);

	/* backend selection */
	void			GetRigList(map<rig_model_t,SDrRigCaps>&);
	void			SetHamlibModelID(const rig_model_t model);
	rig_model_t		GetHamlibModelID() const {return iHamlibModelID;}

	/* com port selection */
	void			GetPortList(map<string,string>&);
	void			SetComPort(const string&);
	string			GetComPort() const;

	void			SetEnableModRigSettings(const _BOOLEAN bNSM);
	_BOOLEAN		GetEnableModRigSettings() const {return bModRigSettings;}
	string			GetInfo() const;

	void			RigSpecialParameters(rig_model_t id, const string& sSet, int iFrOff, const string& sModSet);
	void			ConfigureRig(const string & strSet);
	void			LoadSettings(CSettings& s);
	void			SaveSettings(CSettings& s);

protected:
	class CSpecDRMRig
	{
	public:
		CSpecDRMRig() : strDRMSetMod(""), strDRMSetNoMod(""), iFreqOffs(0) {}
		CSpecDRMRig(const CSpecDRMRig& nSpec) :
			strDRMSetMod(nSpec.strDRMSetMod),
			strDRMSetNoMod(nSpec.strDRMSetNoMod), iFreqOffs(nSpec.iFreqOffs) {}
		CSpecDRMRig(string sSet, int iNFrOff, string sModSet) :
			strDRMSetMod(sModSet), strDRMSetNoMod(sSet), iFreqOffs(iNFrOff) {}

		string		strDRMSetMod; /* Special DRM settings (modified) */
		string		strDRMSetNoMod; /* Special DRM settings (not mod.) */
		int			iFreqOffs; /* Frequency offset */
	};

	map<rig_model_t,CSpecDRMRig>	SpecDRMRigs;
	map<rig_model_t,SDrRigCaps>		CapsHamlibModels;

	void EnableSMeter(const _BOOLEAN bStatus);

	static int			PrintHamlibModelList(const struct rig_caps* caps, void* data);
	void				SetRigModes();
	void				SetRigLevels();
	void				SetRigFuncs();
	void				SetRigParams();
	void				SetRigConfig();

	RIG*				pRig;
	_BOOLEAN			bSMeterIsSupported;
	_BOOLEAN			bModRigSettings;
	rig_model_t			iHamlibModelID;
	string				strHamlibConf;
	string				strSettings;
	int					iFreqOffset;
	map<string,string> modes;
	map<string,string> levels;
	map<string,string> functions;
	map<string,string> parameters;
	map<string,string> config;

};
#else
class CHamlib
{
public:
	enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};
	void SetFrequency(int) {}
};
#endif

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

#endif // !defined(UTILITIES_H__3B0BA660_CA63_4344_B3452345D31912__INCLUDED_)
