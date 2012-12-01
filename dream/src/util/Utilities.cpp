/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
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

#include "Utilities.h"
#include <sstream>
#include <cstring>
#if defined(_WIN32)
# ifdef HAVE_SETUPAPI
#  ifndef INITGUID
#   define INITGUID 1
#  endif
#  include <windows.h>
#  include <setupapi.h>
#  if defined(_MSC_VER) && (_MSC_VER < 1400) || defined(__MINGW32__)
    DEFINE_GUID(GUID_DEVINTERFACE_COMPORT, 0x86e0d1e0L, 0x8089,
    0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
#  endif
# endif
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#endif

/* Implementation *************************************************************/
/******************************************************************************\
* Thread safe counter                                                          *
\******************************************************************************/
unsigned int CCounter::operator++()
	{ unsigned int value; mutex.Lock(); value = ++count; mutex.Unlock(); return value; }
unsigned int CCounter::operator++(int)
	{ unsigned int value; mutex.Lock(); value = count++; mutex.Unlock(); return value; }
unsigned int CCounter::operator--()
	{ unsigned int value; mutex.Lock(); value = --count; mutex.Unlock(); return value; }
unsigned int CCounter::operator--(int)
	{ unsigned int value; mutex.Lock(); value = count--; mutex.Unlock(); return value; }
CCounter::operator unsigned int()
	{ unsigned int value; mutex.Lock(); value = count;   mutex.Unlock(); return value; }
unsigned int CCounter::operator=(unsigned int value)
	{ mutex.Lock(); count = value; mutex.Unlock(); return value; }

/******************************************************************************\
* Signal level meter                                                           *
\******************************************************************************/
void
CSignalLevelMeter::Update(const _REAL rVal)
{
	/* Search for maximum. Decrease this max with time */
	/* Decrease max with time */
	if (rCurLevel >= METER_FLY_BACK)
		rCurLevel -= METER_FLY_BACK;
	else
	{
		if ((rCurLevel <= METER_FLY_BACK) && (rCurLevel > 1))
			rCurLevel -= 2;
	}

	/* Search for max */
	const _REAL rCurAbsVal = Abs(rVal);
	if (rCurAbsVal > rCurLevel)
		rCurLevel = rCurAbsVal;
}

void
CSignalLevelMeter::Update(const CVector < _REAL > vecrVal)
{
	/* Do the update for entire vector */
	const int iVecSize = vecrVal.Size();
	for (int i = 0; i < iVecSize; i++)
		Update(vecrVal[i]);
}

void
CSignalLevelMeter::Update(const CVector < _SAMPLE > vecsVal)
{
	/* Do the update for entire vector, convert to real */
	const int iVecSize = vecsVal.Size();
	for (int i = 0; i < iVecSize; i++)
		Update((_REAL) vecsVal[i]);
}

_REAL CSignalLevelMeter::Level()
{
	const _REAL
		rNormMicLevel = rCurLevel / _MAXSHORT;

	/* Logarithmic measure */
	if (rNormMicLevel > 0)
		return 20.0 * log10(rNormMicLevel);
	else
		return RET_VAL_LOG_0;
}

/******************************************************************************\
* Bandpass filter                                                              *
\******************************************************************************/
void
CDRMBandpassFilt::Process(CVector < _COMPLEX > &veccData)
{
	int i;

	/* Copy CVector data in CMatlibVector */
	for (i = 0; i < iBlockSize; i++)
		cvecDataTmp[i] = veccData[i];

	/* Apply FFT filter */
	cvecDataTmp =
		CComplexVector(FftFilt
					   (cvecB, Real(cvecDataTmp), rvecZReal, FftPlanBP),
					   FftFilt(cvecB, Imag(cvecDataTmp), rvecZImag,
							   FftPlanBP));

	/* Copy CVector data in CMatlibVector */
	for (i = 0; i < iBlockSize; i++)
		veccData[i] = cvecDataTmp[i];
}

void
CDRMBandpassFilt::Init(int iSampleRate, int iNewBlockSize, _REAL rOffsetHz,
					   ESpecOcc eSpecOcc, EFiltType eNFiTy)
{
	CReal rMargin = 0.0;

	/* Set internal parameter */
	iBlockSize = iNewBlockSize;

	/* Init temporary vector */
	cvecDataTmp.Init(iBlockSize);

	/* Choose correct filter for chosen DRM bandwidth. Also, adjust offset
	   frequency for different modes. E.g., 5 kHz mode is on the right side
	   of the DC frequency */
	CReal rNormCurFreqOffset = rOffsetHz / iSampleRate;
	/* Band-pass filter bandwidth */
	CReal rBPFiltBW = ((CReal) 10000.0 + rMargin) / iSampleRate;

	/* Negative margin for receiver filter for better interferer rejection */
	if (eNFiTy == FT_TRANSMITTER)
		rMargin = (CReal) 300.0;	/* Hz */
	else
		rMargin = (CReal) - 200.0;	/* Hz */

	switch (eSpecOcc)
	{
	case SO_0:
		rBPFiltBW = ((CReal) 4500.0 + rMargin) / iSampleRate;

		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 2190.0) / iSampleRate;
		break;

	case SO_1:
		rBPFiltBW = ((CReal) 5000.0 + rMargin) / iSampleRate;

		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 2440.0) / iSampleRate;
		break;

	case SO_2:
		rBPFiltBW = ((CReal) 9000.0 + rMargin) / iSampleRate;

		/* Centered */
		rNormCurFreqOffset = rOffsetHz / iSampleRate;
		break;

	case SO_3:
		rBPFiltBW = ((CReal) 10000.0 + rMargin) / iSampleRate;

		/* Centered */
		rNormCurFreqOffset = rOffsetHz / iSampleRate;
		break;

	case SO_4:
		rBPFiltBW = ((CReal) 18000.0 + rMargin) / iSampleRate;

		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 4500.0) / iSampleRate;
		break;

	case SO_5:
		rBPFiltBW = ((CReal) 20000.0 + rMargin) / iSampleRate;

		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 5000.0) / iSampleRate;
		break;
	}

	/* FFT plan is initialized with the long length */
	FftPlanBP.Init(iBlockSize * 2);

	/* State memory (init with zeros) and data vector */
	rvecZReal.Init(iBlockSize, (CReal) 0.0);
	rvecZImag.Init(iBlockSize, (CReal) 0.0);
	rvecDataReal.Init(iBlockSize);
	rvecDataImag.Init(iBlockSize);

	/* "+ 1" because of the Nyquist frequency (filter in frequency domain) */
	cvecB.Init(iBlockSize + 1);

	/* Actual filter design */
	CRealVector vecrFilter(iBlockSize);
	vecrFilter = FirLP(rBPFiltBW, Nuttallwin(iBlockSize));

	/* Copy actual filter coefficients. It is important to initialize the
	   vectors with zeros because we also do a zero-padding */
	CRealVector rvecB(2 * iBlockSize, (CReal) 0.0);

	/* Modulate filter to shift it to the correct IF frequency */
	for (int i = 0; i < iBlockSize; i++)
	{
		rvecB[i] =
			vecrFilter[i] * Cos((CReal) 2.0 * crPi * rNormCurFreqOffset * i);
	}

	/* Transformation in frequency domain for fft filter */
	cvecB = rfft(rvecB, FftPlanBP);
}

/******************************************************************************\
* Modified Julian Date                                                         *
\******************************************************************************/
void
CModJulDate::Set(const uint32_t iModJulDate)
{
	uint32_t iZ, iA, iAlpha, iB, iC, iD, iE;
	_REAL rJulDate/*, rF*/;

	/* Definition of the Modified Julian Date */
	rJulDate = (_REAL) iModJulDate + 2400000.5;

	/* Get "real" date out of Julian Date
	   (Taken from "http://mathforum.org/library/drmath/view/51907.html") */
	// 1. Add .5 to the JD and let Z = integer part of (JD+.5) and F the
	// fractional part F = (JD+.5)-Z
	iZ = (uint32_t) (rJulDate + (_REAL) 0.5);
//	rF = (rJulDate + (_REAL) 0.5) - iZ;

	// 2. If Z < 2299161, take A = Z
	// If Z >= 2299161, calculate alpha = INT((Z-1867216.25)/36524.25)
	// and A = Z + 1 + alpha - INT(alpha/4).
	if (iZ < 2299161)
		iA = iZ;
	else
	{
		iAlpha = (int) (((_REAL) iZ - (_REAL) 1867216.25) / (_REAL) 36524.25);
		iA = iZ + 1 + iAlpha - (int) ((_REAL) iAlpha / (_REAL) 4.0);
	}

	// 3. Then calculate:
	// B = A + 1524
	// C = INT( (B-122.1)/365.25)
	// D = INT( 365.25*C )
	// E = INT( (B-D)/30.6001 )
	iB = iA + 1524;
	iC = (int) (((_REAL) iB - (_REAL) 122.1) / (_REAL) 365.25);
	iD = (int) ((_REAL) 365.25 * iC);
	iE = (int) (((_REAL) iB - iD) / (_REAL) 30.6001);

	// The day of the month dd (with decimals) is:
	// dd = B - D - INT(30.6001*E) + F
	iDay = iB - iD - (int) ((_REAL) 30.6001 * iE);	// + rF;

	// The month number mm is:
	// mm = E - 1, if E < 13.5
	// or
	// mm = E - 13, if E > 13.5
	if ((_REAL) iE < 13.5)
		iMonth = iE - 1;
	else
		iMonth = iE - 13;

	// The year yyyy is:
	// yyyy = C - 4716   if m > 2.5
	// or
	// yyyy = C - 4715   if m < 2.5
	if ((_REAL) iMonth > 2.5)
		iYear = iC - 4716;
	else
		iYear = iC - 4715;
}

void
CModJulDate::Get(const uint32_t iYear, const uint32_t iMonth, const uint32_t iDay)
{
	/* Taken from "http://en.wikipedia.org/wiki/Julian_day" */
	uint32_t a = (14 - iMonth) / 12;
	uint32_t y = iYear + 4800 - a;
	uint32_t m = iMonth + 12*a - 3;;
	uint32_t iJulDate = iDay + (153*m+2)/5 + 365*y + y/4 - y/100 + y/400 - 32045;
	iModJulDate = iJulDate - 2400001;
}

/******************************************************************************\
* Audio Reverberation                                                          *
\******************************************************************************/
/*
	The following code is based on "JCRev: John Chowning's reverberator class"
	by Perry R. Cook and Gary P. Scavone, 1995 - 2004
	which is in "The Synthesis ToolKit in C++ (STK)"
	http://ccrma.stanford.edu/software/stk

	Original description:
	This class is derived from the CLM JCRev function, which is based on the use
	of networks of simple allpass and comb delay filters. This class implements
	three series allpass units, followed by four parallel comb filters, and two
	decorrelation delay lines in parallel at the output.
*/
void CAudioReverb::Init(CReal rT60, int iSampleRate)
{
	/* Delay lengths for 44100 Hz sample rate */
	int lengths[9] = { 1777, 1847, 1993, 2137, 389, 127, 43, 211, 179 };
	const CReal scaler = CReal(iSampleRate) / 44100.0;

	int delay, i;
	if (scaler != 1.0)
	{
		for (i = 0; i < 9; i++)
		{
			delay = (int) Floor(scaler * lengths[i]);

			if ((delay & 1) == 0)
				delay++;

			while (!isPrime(delay))
				delay += 2;

			lengths[i] = delay;
		}
	}

	for (i = 0; i < 3; i++)
		allpassDelays_[i].Init(lengths[i + 4]);

	for (i = 0; i < 4; i++)
		combDelays_[i].Init(lengths[i]);

	setT60(rT60, iSampleRate);
	allpassCoefficient_ = (CReal) 0.7;
	Clear();
}

_BOOLEAN CAudioReverb::isPrime(const int number)
{
/*
	Returns true if argument value is prime. Taken from "class Effect" in
	"STK abstract effects parent class".
*/
	if (number == 2)
		return TRUE;

	if (number & 1)
	{
		for (int i = 3; i < (int) Sqrt((CReal) number) + 1; i += 2)
		{
			if ((number % i) == 0)
				return FALSE;
		}

		return TRUE;			/* prime */
	}
	else
		return FALSE;			/* even */
}

void
CAudioReverb::Clear()
{
	/* Reset and clear all internal state */
	allpassDelays_[0].Reset(0);
	allpassDelays_[1].Reset(0);
	allpassDelays_[2].Reset(0);
	combDelays_[0].Reset(0);
	combDelays_[1].Reset(0);
	combDelays_[2].Reset(0);
	combDelays_[3].Reset(0);
}

void
CAudioReverb::setT60(const CReal rT60, int iSampleRate)
{
	/* Set the reverberation T60 decay time */
	for (int i = 0; i < 4; i++)
	{
		combCoefficient_[i] = pow((CReal) 10.0, (CReal) (-3.0 * combDelays_[i].  Size() / (rT60 * iSampleRate)));
	}
}

CReal CAudioReverb::ProcessSample(const CReal rLInput, const CReal rRInput)
{
	/* Compute one output sample */
	CReal
		temp,
		temp0,
		temp1,
		temp2;

	/* Mix stereophonic input signals to mono signal (since the maximum value of
	   the input signal is 0.5 * max due to the current implementation in
	   AudioSourceDecoder.cpp, we cannot get an overrun) */
	const CReal
		input = rLInput + rRInput;

	temp = allpassDelays_[0].Get();
	temp0 = allpassCoefficient_ * temp;
	temp0 += input;
	allpassDelays_[0].Add((int) temp0);
	temp0 = -(allpassCoefficient_ * temp0) + temp;

	temp = allpassDelays_[1].Get();
	temp1 = allpassCoefficient_ * temp;
	temp1 += temp0;
	allpassDelays_[1].Add((int) temp1);
	temp1 = -(allpassCoefficient_ * temp1) + temp;

	temp = allpassDelays_[2].Get();
	temp2 = allpassCoefficient_ * temp;
	temp2 += temp1;
	allpassDelays_[2].Add((int) temp2);
	temp2 = -(allpassCoefficient_ * temp2) + temp;

	const CReal
		temp3 = temp2 + (combCoefficient_[0] * combDelays_[0].Get());
	const CReal
		temp4 = temp2 + (combCoefficient_[1] * combDelays_[1].Get());
	const CReal
		temp5 = temp2 + (combCoefficient_[2] * combDelays_[2].Get());
	const CReal
		temp6 = temp2 + (combCoefficient_[3] * combDelays_[3].Get());

	combDelays_[0].Add((int) temp3);
	combDelays_[1].Add((int) temp4);
	combDelays_[2].Add((int) temp5);
	combDelays_[3].Add((int) temp6);

	return temp3 + temp4 + temp5 + temp6;
}

#ifdef HAVE_LIBHAMLIB
/******************************************************************************\
* Hamlib interface                                                             *
\******************************************************************************/
/*
	This code is based on patches and example code from Tomi Manninen and
	Stephane Fillod (developer of hamlib)
*/
CHamlib::CHamlib():SpecDRMRigs(), CapsHamlibModels(),
pRig(NULL), bSMeterIsSupported(FALSE),
bModRigSettings(FALSE), iHamlibModelID(0),
strHamlibConf(""), strSettings(""), iFreqOffset(0),
modes(), levels(), functions(), parameters(), config()
{
#ifdef RIG_MODEL_DWT
	/* Digital World Traveller */
	RigSpecialParameters(RIG_MODEL_DWT, "", 0, "");
#endif

#ifdef RIG_MODEL_G303
	/* Winradio G303 */
	RigSpecialParameters(RIG_MODEL_G303, "l_ATT=0,l_AGC=3", 0,
						 "l_ATT=0,l_AGC=3");
#endif

#ifdef RIG_MODEL_G313
	/* Winradio G313 */
	RigSpecialParameters(RIG_MODEL_G313, "l_ATT=0,l_AGC=3", 0,
						 "l_ATT=0,l_AGC=3");
#endif

#ifdef RIG_MODEL_AR7030
	/* AOR 7030 */
//  vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_AR7030,
//      "m_CW=9500,l_IF=-4200,l_AGC=3", 5 /* kHz frequency offset */,
//      "l_AGC=3"));
	RigSpecialParameters(RIG_MODEL_AR7030, "m_AM=3,l_AGC=5",
						 0 /* kHz frequency offset */ ,
						 "m_AM=6,l_AGC=5");
#endif

#ifdef RIG_MODEL_ELEKTOR304
	/* Elektor 3/04 */
	RigSpecialParameters(RIG_MODEL_ELEKTOR304, "", 0, "");
#endif

#ifdef RIG_MODEL_ELEKTOR507
    /* Elektor 5/07 */
    RigSpecialParameters(RIG_MODEL_ELEKTOR507, "",
						-12 /* kHz frequency offset */ ,
						"");
#endif

#ifdef RIG_MODEL_NRD535
	/* JRC NRD 535 */
	RigSpecialParameters(RIG_MODEL_NRD535,
						 "l_CWPITCH=-5000,m_CW=12000,l_IF=-2000,l_AGC=3"
						 /* AGC=slow */ ,
						 3 /* kHz frequency offset */ ,
						 "l_AGC=3");
#endif

#ifdef RIG_MODEL_RX320
	/* TenTec RX320D */
	RigSpecialParameters(RIG_MODEL_RX320, "l_AF=1,l_AGC=3,m_AM=6000", 0,
						 "l_AGC=3");
#endif

#ifdef RIG_MODEL_RX340
	/* TenTec RX340D */
	RigSpecialParameters(RIG_MODEL_RX340,
						 "l_AF=1,m_USB=16000,l_AGC=3,l_IF=2000",
						 -12 /* kHz frequency offset */ ,
						 "l_AGC=3");
#endif

	/* Load all available front-end remotes in hamlib library */
	rig_load_all_backends();

	/* Get all models which are available.
	 * A call-back function is called to return the different rigs */
	rig_list_foreach(PrintHamlibModelList, this);
}

CHamlib::~CHamlib()
{
	if (pRig != NULL)
	{
		/* close everything */
		rig_close(pRig);
		rig_cleanup(pRig);
	}
}

void
CHamlib::RigSpecialParameters(rig_model_t id, const string & sSet, int iFrOff,
							  const string & sModSet)
{
	CapsHamlibModels[id].bIsSpecRig = TRUE;
	SpecDRMRigs[id] = CSpecDRMRig(sSet, iFrOff, sModSet);
}

void
CHamlib::GetRigList(map < rig_model_t, SDrRigCaps > &rigs)
{
	rigs = CapsHamlibModels;
}

void
CHamlib::GetPortList(map < string, string > &ports)
{
	ports.clear();
/* Config string for com-port selection is different in Windows and Linux */
#ifdef _WIN32
# ifdef HAVE_SETUPAPI
	GUID guid = GUID_DEVINTERFACE_COMPORT;
	HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&guid, NULL, NULL,
											   DIGCF_PRESENT |
											   DIGCF_DEVICEINTERFACE);
	if (hDevInfoSet != INVALID_HANDLE_VALUE)
	{
		SP_DEVINFO_DATA devInfo;
		devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
		for (int i = 0; SetupDiEnumDeviceInfo(hDevInfoSet, i, &devInfo); i++)
		{
			HKEY hDeviceKey =
				SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL,
									 0, DIREG_DEV, KEY_QUERY_VALUE);
			if (hDeviceKey)
			{
				char szPortName[256];
				DWORD dwSize = sizeof(szPortName);
				DWORD dwType = 0;
				if ((RegQueryValueExA
					 (hDeviceKey, "PortName", NULL, &dwType,
					  reinterpret_cast < LPBYTE > (szPortName),
					  &dwSize) == ERROR_SUCCESS) && (dwType == REG_SZ))
				{
					char szFriendlyName[256];
					DWORD dwSize = sizeof(szFriendlyName);
					DWORD dwType = 0;
					if (SetupDiGetDeviceRegistryPropertyA
						(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType,
						 reinterpret_cast < PBYTE > (szFriendlyName), dwSize,
						 &dwSize) && (dwType == REG_SZ))
						ports[string(szFriendlyName) + " " + szPortName] = szPortName;
					else
						ports[szPortName] = szPortName;
				}

				RegCloseKey(hDeviceKey);
			}
		}

		SetupDiDestroyDeviceInfoList(hDevInfoSet);
	}
# endif
	if (ports.empty())
	{
		ports["COM1"] = "COM1";
		ports["COM2"] = "COM2";
		ports["COM3"] = "COM3 ";
		ports["COM4"] = "COM4 ";
		ports["COM5"] = "COM5 ";
	}
#elif defined(__linux)
	FILE *p =
		popen("hal-find-by-capability --capability serial", "r");
	_BOOLEAN bOK = FALSE;
	while (!feof(p))
	{
		char buf[1024];
		char* r = fgets(buf, sizeof(buf), p);
		if (strlen(buf) > 0)
		{
			string s =
				string("hal-get-property --key serial.device --udi ") +
				buf;
			FILE *p2 = popen(s.c_str(), "r");
			r = fgets(buf, sizeof(buf), p2);
			size_t n = strlen(buf);
			if (n > 0)
			{
				if (buf[n - 1] == '\n')
					buf[n - 1] = 0;
				ports[buf] = buf;
				bOK = TRUE;
				buf[0] = 0;
			}
			pclose(p2);
		}
		(void)r;
	}
	pclose(p);
	if (!bOK)
	{
		ports["ttyS0"] = "/dev/ttyS0";
		ports["ttyS1"] = "/dev/ttyS1";
		ports["ttyUSB0"] = "/dev/ttyUSB0";
	}
#elif defined(__APPLE__)
	io_iterator_t serialPortIterator;
    kern_return_t			kernResult;
    CFMutableDictionaryRef	classesToMatch;

    // Serial devices are instances of class IOSerialBSDClient
    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL)
    {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    }
    else
	{
        CFDictionarySetValue(classesToMatch,
                             CFSTR(kIOSerialBSDTypeKey),
                             CFSTR(kIOSerialBSDRS232Type));

	}
    kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &serialPortIterator);
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
    }

    io_object_t		modemService;

    // Iterate across all modems found. In this example, we bail after finding the first modem.

    while ((modemService = IOIteratorNext(serialPortIterator)))
    {
        CFStringRef	bsdPathAsCFString;

		// Get the callout device's path (/dev/cu.xxxxx). The callout device should almost always be
		// used: the dialin device (/dev/tty.xxxxx) would be used when monitoring a serial port for
		// incoming calls, e.g. a fax listener.

		bsdPathAsCFString = CFStringRef(IORegistryEntryCreateCFProperty(modemService,
                                                            CFSTR(kIOCalloutDeviceKey),
                                                            kCFAllocatorDefault,
                                                            0));
        if (bsdPathAsCFString)
        {
            Boolean result;
			char bsdPath[256];

            // Convert the path from a CFString to a C (NUL-terminated) string for use
			// with the POSIX open() call.

			result = CFStringGetCString(bsdPathAsCFString,
                                        bsdPath,
                                        sizeof(bsdPath),
                                        kCFStringEncodingUTF8);
            CFRelease(bsdPathAsCFString);

            if (result)
			{
				// make the name a bit more friendly for the menu
				string s,t=bsdPath;
				size_t p = t.find('.');
				if(p<string::npos)
					s = t.substr(p+1);
				else
					s = t;
				ports[s] = bsdPath;
            }
        }

        // Release the io_service_t now that we are done with it.

		(void) IOObjectRelease(modemService);
    }
#endif
}

void
CHamlib::SetComPort(const string & port)
{
	config["rig_pathname"] = port;
	SetHamlibModelID(iHamlibModelID);
}

string CHamlib::GetComPort() const
{
	map < string, string >::const_iterator m = config.find("rig_pathname");
	if (m == config.end())
		return "";
	return m->second;
}

int
CHamlib::PrintHamlibModelList(const struct rig_caps *caps, void *data)
{
	/* Access data members of class through pointer ((CHamlib*) data) */
	CHamlib & Hamlib = *((CHamlib *) data);

	/* Store new model in class. Use only relevant information */
	_BOOLEAN bIsSpec =
		Hamlib.SpecDRMRigs.find(caps->rig_model) != Hamlib.SpecDRMRigs.end();

	Hamlib.CapsHamlibModels[caps->rig_model] =
		SDrRigCaps(caps->mfg_name, caps->model_name, caps->status, bIsSpec);

	return 1;					/* !=0, we want them all! */
}

void
CHamlib::LoadSettings(CSettings & s)
{
	rig_model_t model = s.Get("Hamlib", "hamlib-model", 0);

	if (model != 0)
	{
		/* Hamlib configuration string */
		string strHamlibConf = s.Get("Hamlib", "hamlib-config");

		if (model == RIG_MODEL_G313)
		{
			string kwd, val;
#ifdef __linux__
			kwd = "if_path";
			val = "/tmp/g313";
			s.Put("Hamlib", kwd, val);
#endif
#ifdef _WIN32
			kwd = "wodeviceid";
			val = "-2";
#endif
			if(kwd!="")
			{
				if (strHamlibConf=="")
				{
					strHamlibConf = kwd + "=" + val;
				}
				else
				{
					// don't overwrite a saved value
					if(strHamlibConf.find_first_of(kwd)==string::npos)
					{
						strHamlibConf += "," + kwd + "=" + val;
					}
				}
			}
		}
		if (strHamlibConf != "")
		{
			istringstream params(strHamlibConf);
			while (!params.eof())
			{
				string name, value;
				getline(params, name, '=');
				getline(params, value, ',');
				config[name] = value;
			}
		}

		/* Enable DRM modified receiver flag */
		bModRigSettings = s.Get("Hamlib", "enmodrig", FALSE);

		strSettings = s.Get("Hamlib", "settings");
		iFreqOffset = s.Get("Hamlib", "freqoffset", 0);

		if (strSettings != "" || iFreqOffset != 0)
		{
			if (bModRigSettings)
				RigSpecialParameters(model, "", iFreqOffset, strSettings);
			else
				RigSpecialParameters(model, strSettings, iFreqOffset, "");
		}

		/* Hamlib Model ID */
		SetHamlibModelID(model);
	}

	s.Put("Hamlib", "hamlib-model", model);
	s.Put("Hamlib", "hamlib-config", strHamlibConf);
	s.Put("Hamlib", "settings", strSettings);
	s.Put("Hamlib", "freqoffset", iFreqOffset);
}

void
CHamlib::SaveSettings(CSettings & s)
{
	/* Hamlib Model ID */
	s.Put("Hamlib", "hamlib-model", iHamlibModelID);

	/* Hamlib configuration string */
	stringstream ss;
	string sep = "";
	for (map < string, string >::iterator i = config.begin();
		 i != config.end(); i++)
	{
		ss << sep << i->first << "=" << i->second;
		sep = ",";
	}

	s.Put("Hamlib", "hamlib-config", ss.str());

	/* Enable DRM modified receiver flag */
	s.Put("Hamlib", "enmodrig", bModRigSettings);

	s.Put("Hamlib", "settings", strSettings);

	s.Put("Hamlib", "freqoffset", iFreqOffset);
}

_BOOLEAN
CHamlib::SetFrequency(const int iFreqkHz)
{
	_BOOLEAN bSucceeded = FALSE;

	/* Check if rig was opend properly */
	if (pRig != NULL)
	{
		/* Set frequency (consider frequency offset and conversion
		   from kHz to Hz by " * 1000 ") */
		if (rig_set_freq(pRig, RIG_VFO_CURR, (iFreqkHz + iFreqOffset) * 1000)
			== RIG_OK)
		{
			bSucceeded = TRUE;
		}
	}

	return bSucceeded;
}

CHamlib::ESMeterState CHamlib::GetSMeter(_REAL & rCurSigStr)
{
	ESMeterState
		eRetVal = SS_NOTVALID;
	rCurSigStr = (_REAL) 0.0;

	if ((pRig != NULL) && (bSMeterIsSupported == TRUE))
	{
		value_t
			tVal;
		const int
			iHamlibRetVal =
			rig_get_level(pRig, RIG_VFO_CURR, RIG_LEVEL_STRENGTH, &tVal);

		if (!iHamlibRetVal)
		{
			rCurSigStr = (_REAL) tVal.i;
			eRetVal = SS_VALID;
		}

		/* If a time-out happened, do not update s-meter anymore (disable it) */
		if (iHamlibRetVal == -RIG_ETIMEOUT)
		{
			bSMeterIsSupported = FALSE;
			eRetVal = SS_TIMEOUT;
		}
	}

	return eRetVal;
}

void
CHamlib::ConfigureRig(const string & strSet)
{
	/* Parse special settings */
	istringstream params(strSet);
	while (!params.eof())
	{
		string p, name, value;

		getline(params, p, '_');
		getline(params, name, '=');
		getline(params, value, ',');
		if (p == "" || p.length() != 1 || name == "" || value == "")
		{
			/* Malformatted config string */
			rig_cleanup(pRig);
			pRig = NULL;

			throw CGenErr(string("Malformatted config string: ") + strSet);
		}
		switch (p[0])
		{
		case 'm':
			modes[name] = value;
			break;
		case 'l':
			levels[name] = value;
			break;
		case 'f':
			functions[name] = value;
			break;
		case 'p':
			parameters[name] = value;
			break;
		default:
			cerr << "Rig unknown setting: " << p << "_" << name <<
				"=" << value << endl;
		}
	}
}

void
CHamlib::SetRigModes()
{
	for (map < string, string >::const_iterator i = modes.begin();
		 i != modes.end(); i++)
	{
		rmode_t mode = rig_parse_mode(i->first.c_str());
		if (mode != RIG_MODE_NONE)
		{
			int ret =
				rig_set_mode(pRig, RIG_VFO_CURR, mode,
							 atoi(i->second.c_str()));
			if (ret != RIG_OK)
				cerr << "Rig set mode failed: " << rigerror(ret) << endl;
		}
	}
}

void
CHamlib::SetRigLevels()
{
	for (map < string, string >::const_iterator i = levels.begin();
		 i != levels.end(); i++)
	{
		setting_t setting = rig_parse_level(i->first.c_str());
		if (setting != RIG_LEVEL_NONE)
		{
			value_t val;
			if (RIG_LEVEL_IS_FLOAT(setting))
				val.f = atof(i->second.c_str());
			else
				val.i = atoi(i->second.c_str());

			int ret = rig_set_level(pRig, RIG_VFO_CURR, setting, val);
			if (ret != RIG_OK)
				cerr << "Rig set level failed: " << rigerror(ret) << endl;
		}
	}
}

void
CHamlib::SetRigFuncs()
{
	for (map < string, string >::const_iterator i = functions.begin();
		 i != functions.end(); i++)
	{
		setting_t setting = rig_parse_func(i->first.c_str());
		if (setting != RIG_FUNC_NONE)
		{
			int ret =
				rig_set_func(pRig, RIG_VFO_CURR, setting,
							 atoi(i->second.c_str()));
			if (ret != RIG_OK)
				cerr << "Rig set func failed: " << rigerror(ret) << endl;
		}
	}
}

void
CHamlib::SetRigParams()
{
	for (map < string, string >::const_iterator i = parameters.begin();
		 i != parameters.end(); i++)
	{
		setting_t setting = rig_parse_parm(i->first.c_str());
		if (setting != RIG_PARM_NONE)
		{
			value_t val;
			if (RIG_PARM_IS_FLOAT(setting))
				val.f = atof(i->second.c_str());
			else
				val.i = atoi(i->second.c_str());
			int ret = rig_set_parm(pRig, setting, val);
			if (ret != RIG_OK)
				cerr << "Rig set parm failed: " << rigerror(ret) << endl;
		}
	}
}

void
CHamlib::SetRigConfig()
{
	for (map < string, string >::const_iterator i = config.begin();
		 i != config.end(); i++)
	{
	    cerr << i->first << ":" << i->second << endl;
		int ret =
			rig_set_conf(pRig, rig_token_lookup(pRig, i->first.c_str()),
						 i->second.c_str());
		if (ret != RIG_OK)
		{
			rig_cleanup(pRig);
			pRig = NULL;
			throw CGenErr("Rig set conf failed.");
		}
	}
}

void
CHamlib::SetEnableModRigSettings(const _BOOLEAN bNSM)
{
	if (bModRigSettings != bNSM)
	{
		/* Set internal parameter */
		bModRigSettings = bNSM;

		/* Hamlib must be re-initialized with new parameter */
		SetHamlibModelID(iHamlibModelID);
	}
}

void
CHamlib::SetHamlibModelID(const rig_model_t model)
{
	int ret;

	/* Set value for current selected model ID */
	iHamlibModelID = model;

	/* Init frequency offset */
	iFreqOffset = 0;

	try
	{
		/* If rig was already open, close it first */
		if (pRig != NULL)
		{
			/* Close everything */
			rig_close(pRig);
			rig_cleanup(pRig);
			pRig = NULL;
		}

		if (iHamlibModelID == 0)
			throw CGenErr("No rig model ID selected.");

		/* Check for special DRM front-end selection */
		map < rig_model_t, CSpecDRMRig >::const_iterator s =
			SpecDRMRigs.find(iHamlibModelID);
		if (s != SpecDRMRigs.end())
		{
			/* Get correct parameter string */
			if (bModRigSettings == TRUE)
				strSettings = s->second.strDRMSetMod;
			else
			{
				strSettings = s->second.strDRMSetNoMod;

				/* Additionally, set frequency offset for this special rig */
				iFreqOffset = s->second.iFreqOffs;
			}
			if (strSettings != "")
			{
				ConfigureRig(strSettings);
			}
		}

		/* Init rig */
		pRig = rig_init(iHamlibModelID);
		if (pRig == NULL)
			throw CGenErr("Initialization of hamlib failed.");

		SetRigConfig();

		/* Open rig */
		ret = rig_open(pRig);
		if (ret != RIG_OK)
		{
			/* Fail! */
			rig_cleanup(pRig);
			pRig = NULL;

			throw CGenErr("Rig open failed.");
		}

		/* Ignore result, some rigs don't have support for this */
		rig_set_powerstat(pRig, RIG_POWER_ON);

		SetRigModes();
		SetRigLevels();
		SetRigFuncs();
		SetRigParams();

		/* Check if s-meter capabilities are available */
		if (pRig != NULL)
		{
			/* Check if s-meter can be used. Disable GUI control if not */
			if (rig_has_get_level(pRig, RIG_LEVEL_STRENGTH))
				bSMeterIsSupported = TRUE;
			else
				bSMeterIsSupported = FALSE;
		}
	}

	catch(CGenErr GenErr)
	{
		/* Print error message */
		cerr << GenErr.strError << endl;

		/* Disable s-meter */
		bSMeterIsSupported = FALSE;
	}
}
#endif
