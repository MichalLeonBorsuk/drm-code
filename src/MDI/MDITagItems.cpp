/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden, Andrew Murphy
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *  This module derives, from the CTagItemGenerator base class, tag item generators
 *  specialised to generate each of the tag items defined in MDI and RSCI.
 *  .
 *  An intermediate derived class, CTagItemGeneratorWithProfiles, is used as the
 *  base class for all these tag item generators. This takes care of the common
 *	task of checking whether a given tag is in a particular profile.
 *  The profiles for each tag are defined by the GetProfiles() member function.
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

#include "MDITagItems.h"
#include <iostream>
#include <fstream>
using namespace std;

#include "../util/LogPrint.h"

CTagItemGeneratorWithProfiles::CTagItemGeneratorWithProfiles()
{
}

bool
CTagItemGeneratorWithProfiles::IsInProfile(char cProfile)
{
	string strProfiles = GetProfiles();

	for (size_t i = 0; i < strProfiles.length(); i++)
		if (strProfiles[i] == char (toupper(cProfile)))
			return true;

	return false;
}

bool
CTagItemGenerator::IsInProfile(char)
{
	return true;
}

/* Default implementation: unless otherwise specified, tag will be in all RSCI profiles, but not MDI */
/* Make this pure virtual and remove the implementation if you want to force all tags to specify the profiles explicitly */
string
CTagItemGeneratorWithProfiles::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorProTyMDI::GenTag()
{
	/* Length: 8 bytes = 64 bits */
	PrepareTag(64);

	/* Protocol type: DMDI */
	Enqueue((uint32_t) 'D', BITS_BINARY);
	Enqueue((uint32_t) 'M', BITS_BINARY);
	Enqueue((uint32_t) 'D', BITS_BINARY);
	Enqueue((uint32_t) 'I', BITS_BINARY);

	/* Major revision */
	Enqueue((uint32_t) MDI_MAJOR_REVISION, 16);

	/* Minor revision */
	Enqueue((uint32_t) MDI_MINOR_REVISION, 16);
}

string
CTagItemGeneratorProTyMDI::GetTagName()
{
	return "*ptr";
}

string
CTagItemGeneratorProTyMDI::GetProfiles()
{
	return "ADM";
}

void
CTagItemGeneratorProTyRSCI::GenTag()
{
	/* Length: 8 bytes = 64 bits */
	PrepareTag(64);

	/* Protocol type: DMDI */
	Enqueue((uint32_t) 'R', BITS_BINARY);
	Enqueue((uint32_t) 'S', BITS_BINARY);
	Enqueue((uint32_t) 'C', BITS_BINARY);
	Enqueue((uint32_t) 'I', BITS_BINARY);

	/* Major revision */
	Enqueue((uint32_t) RSCI_MAJOR_REVISION, 16);

	/* Minor revision */
	Enqueue((uint32_t) RSCI_MINOR_REVISION, 16);
}

string
CTagItemGeneratorProTyRSCI::GetTagName()
{
	return "*ptr";
}

string
CTagItemGeneratorProTyRSCI::GetProfiles()
{
	return "ABCDGQ";
}

CTagItemGeneratorLoFrCnt::CTagItemGeneratorLoFrCnt():iLogFraCnt(0)
{
}

void
CTagItemGeneratorLoFrCnt::GenTag()
{
	/* Length: 4 bytes = 32 bits */
	PrepareTag(32);

	/* Logical frame count */
	Enqueue(iLogFraCnt, 32);

	/* Count: the value shall be incremented by one by the device generating the
	   MDI Packets for each MDI Packet sent. Wraps around at a value of
	   "(1 << 32)" since the variable type is "uint32_t" */
	iLogFraCnt++;
}

string
CTagItemGeneratorLoFrCnt::GetTagName()
{
	return "dlfc";
}

string
CTagItemGeneratorLoFrCnt::GetProfiles()
{
	return "ABCDGQM";
}

void
CTagItemGeneratorFAC::GenTag(CParameter & Parameter, CVectorEx < _BINARY > *pvecbiData)
{
	if (Parameter.ReceiveStatus.FAC.GetStatus() == false)
	{
		/* Empty tag if FAC is invalid */
		PrepareTag(0);
	}
	else
	{
		/* Length: 9 bytes = 72 bits */
		PrepareTag(NUM_FAC_BITS_PER_BLOCK);

		/* Channel parameters, service parameters, CRC */
		pvecbiData->ResetBitAccess();

		/* FAC data is always 72 bits long which is 9 bytes, copy data byte-wise */
		for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / BITS_BINARY; i++)
			Enqueue(pvecbiData->Separate(BITS_BINARY), BITS_BINARY);
	}
}

string
CTagItemGeneratorFAC::GetTagName()
{
	return "fac_";
}

string
CTagItemGeneratorFAC::GetProfiles()
{
	return "ACDGQM";
}

void
CTagItemGeneratorSDC::GenTag(CParameter & Parameter, CVectorEx < _BINARY > *pvecbiData)
{
	if (Parameter.ReceiveStatus.SDC.GetStatus() == false)
	{
		PrepareTag(0);
		return;
	}

	if (pvecbiData->Size() < Parameter.iNumSDCBitsPerSuperFrame)
	{
		PrepareTag(0);
		return;
	}

	/* Fixed by O.Haffenden, BBC R&D */
	/* The input SDC vector is 4 bits SDC index + a whole number of bytes plus padding. */
	/* The padding is not sent in the MDI */
	const int iLenSDCDataBits = BITS_BINARY * ((Parameter.iNumSDCBitsPerSuperFrame - 4) / BITS_BINARY) + 4;

	/* Length: "length SDC block" bytes. Our SDC data vector does not
	   contain the 4 bits "Rfu" */
	PrepareTag(iLenSDCDataBits + 4);
	//CVectorEx < _BINARY > *pvecbiSDCData = SDCData.Get(Parameter.iNumSDCBitsPerSuperFrame);

	/* Service Description Channel Block */
	pvecbiData->ResetBitAccess();

	Enqueue((uint32_t) 0, 4);	/* Rfu */

	/* We have to copy bits instead of bytes since the length of SDC data is
	   usually not a multiple of 8 */
	for (int i = 0; i < iLenSDCDataBits; i++)
		Enqueue(pvecbiData->Separate(1), 1);
}

string
CTagItemGeneratorSDC::GetTagName()
{
	return "sdc_";
}

string
CTagItemGeneratorSDC::GetProfiles()
{
	return "ACDGM";
}

void
CTagItemGeneratorSDCChanInf::GenTag(CParameter & Parameter)
{
	set<int> actStreams;

	/* Get active streams */
	Parameter.GetActiveStreams(actStreams);

	/* Get number of active streams */
	const size_t iNumActStreams = actStreams.size();

	/* Length: 1 + n * 3 bytes */
	PrepareTag((1 + 3 * iNumActStreams) * BITS_BINARY);

	/* Protection */
	/* Rfu */
	Enqueue((uint32_t) 0, 4);

	/* PLA */
	Enqueue((uint32_t) Parameter.MSCParameters.ProtectionLevel.iPartA, 2);

	/* PLB */
	Enqueue((uint32_t) Parameter.MSCParameters.ProtectionLevel.iPartB, 2);

	/* n + 1 stream description(s) */
	for (set<int>::iterator i = actStreams.begin(); i!=actStreams.end(); i++)
	{
		/* In case of hirachical modulation stream 0 describes the protection
		   level and length of hierarchical data */
		if ((*i == 0) &&
			((Parameter.Channel.eMSCmode == CS_3_HMSYM) ||
			 (Parameter.Channel.eMSCmode == CS_3_HMMIX)))
		{
			/* Protection level for hierarchical */
			Enqueue((uint32_t) Parameter.MSCParameters.ProtectionLevel.iHierarch, 2);

			/* rfu */
			Enqueue((uint32_t) 0, 10);

			/* Data length for hierarchical (always stream 0) */
			Enqueue((uint32_t) Parameter.MSCParameters.Stream[0].iLenPartB, 12);
		}
		else
		{
			/* Data length for part A */
			Enqueue((uint32_t) Parameter.MSCParameters.Stream[*i].iLenPartA, 12);

			/* Data length for part B */
			Enqueue((uint32_t) Parameter.MSCParameters.Stream[*i].iLenPartB, 12);
		}
	}
}

string
CTagItemGeneratorSDCChanInf::GetTagName()
{
	return "sdci";
}

string
CTagItemGeneratorSDCChanInf::GetProfiles()
{
	return "ACDQM";
}

void
CTagItemGeneratorRobMod::GenTag(ERobMode eCurRobMode)
{
	/* Length: 1 byte */
	PrepareTag(BITS_BINARY);

	/* Robustness mode */
	switch (eCurRobMode)
	{
	case RM_ROBUSTNESS_MODE_A:
		Enqueue((uint32_t) 0, 8);
		break;

	case RM_ROBUSTNESS_MODE_B:
		Enqueue((uint32_t) 1, 8);
		break;

	case RM_ROBUSTNESS_MODE_C:
		Enqueue((uint32_t) 2, 8);
		break;

	case RM_ROBUSTNESS_MODE_D:
		Enqueue((uint32_t) 3, 8);
		break;

	default:
		break;
	}
}

string
CTagItemGeneratorRobMod::GetTagName()
{
	return "robm";
}

string
CTagItemGeneratorRobMod::GetProfiles()
{
	return "ABCDGQM";
}

void
CTagItemGeneratorRINF::GenTag(string strUTF8Text)
{
	/* Data length: n * 8 bits */
	PrepareTag(16 * BITS_BINARY);

	/* UTF-8 text */
	for (int i = 0; i < 16; i++)	// truncate to 16 chars as this is the max the TAG item can have
	{
		const char cNewChar = strUTF8Text[i];

		/* Set character */
		Enqueue((uint32_t) cNewChar, BITS_BINARY);
	}
}

string
CTagItemGeneratorRINF::GetTagName()
{
	return "rinf";
}

string
CTagItemGeneratorRINF::GetProfiles()
{
	return "ABCDGQ";
}

CTagItemGeneratorStr::CTagItemGeneratorStr():iStreamNumber(0)
{
}

// Sets the stream number. Should be called just after construction. (can't have it in the constructor because
// we want a vector of them

void
CTagItemGeneratorStr::SetStreamNumber(int iStrNum)
{
	iStreamNumber = iStrNum;
}

void
CTagItemGeneratorStr::GenTag(CParameter & Parameter, CVectorEx < _BINARY > *pvecbiData)
{
	if (iStreamNumber >= MAX_NUM_STREAMS)
		return;

	volatile int iLenStrData = BITS_BINARY * Parameter.GetStreamLen(iStreamNumber);
	volatile int iLenVec = pvecbiData->Size();

	/* check we have data in the vector */
	if(iLenStrData > iLenVec || iLenStrData == 0)
	{
		/* if there is an error, or the length is zero, send an empty tag
		 * This is optimised out at the Tag Packet level but it makes sense
		 * to always generate a tag in the tag item generator */
		PrepareTag(0);
		return;
	}

	PrepareTag(iLenStrData);

	pvecbiData->ResetBitAccess();
	/* Data is always a multiple of 8 -> copy bytes */
	for (int i = 0; i < iLenStrData / BITS_BINARY; i++)
	{
		Enqueue(pvecbiData->Separate(BITS_BINARY), BITS_BINARY);
	}
}

string
CTagItemGeneratorStr::GetProfiles()
{
	return "ADM";
}

string
CTagItemGeneratorStr::GetTagName()
{
	switch (iStreamNumber)
	{
	case 0:
		return "str0";
	case 1:
		return "str1";
	case 2:
		return "str2";
	case 3:
		return "str3";
	default:
		return "str?";
	}
}

void
CTagItemGeneratorMERFormat::GenTag(bool bIsValid, _REAL rMER)
{
	/* Common routine for rmer, rwmf, rwmm tags (all have the same format) */
	/* If no MER value is available, set tag length to zero */
	if (bIsValid == false)
	{
		/* Length: 0 byte */
		PrepareTag(0);
	}
	else
	{
		/* Length: 2 bytes = 16 bits */
		PrepareTag(16);

		/* Set value: the format of this single value is (Byte1 + Byte2 / 256)
		   = (Byte1.Byte2) in [dB] with: Byte1 is an 8-bit signed integer value;
		   and Byte2 is an 8-bit unsigned integer value */
		/* Integer part */
		Enqueue((uint32_t) rMER, BITS_BINARY);

		/* Fractional part */
		const _REAL rFracPart = rMER - (int) rMER;
		Enqueue((uint32_t) (rFracPart * 256), BITS_BINARY);
	}
}

string
CTagItemGeneratorRWMF::GetTagName()
{
	return "rwmf";
}

string
CTagItemGeneratorRWMF::GetProfiles()
{
	return "ABCDGQ";
}

string
CTagItemGeneratorRWMM::GetTagName()
{
	return "rwmm";
}

string
CTagItemGeneratorRWMM::GetProfiles()
{
	return "ABCDGQ";
}

string
CTagItemGeneratorRMER::GetTagName()
{
	return "rmer";
}

string
CTagItemGeneratorRMER::GetProfiles()
{
	return "ABCDGQ";
}

string
CTagItemGeneratorRDOP::GetTagName()
{
	return "rdop";
}

string
CTagItemGeneratorRDOP::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRDEL::GenTag(bool bIsValid, const vector<CMeasurements::CRdel>& rdel)
{
	/* If no value is available, set tag length to zero */
	if (bIsValid == false)
	{
		/* Length: 0 byte */
		PrepareTag(0);
	}
	else
	{
		/* Length: 3 bytes per value = 16 bits */
		PrepareTag(24 * rdel.size());

		for (size_t i = 0; i < rdel.size(); i++)
		{
			/* percentage for this window */
			Enqueue((uint32_t) rdel[i].threshold, BITS_BINARY);
			/* Set value: the format of this single value is (Byte1 + Byte2 / 256)
			   = (Byte1.Byte2) in [dB] with: Byte1 is an 8-bit signed integer value;
			   and Byte2 is an 8-bit unsigned integer value */
			/* Integer part */
			_REAL rDelay = rdel[i].interval;
			Enqueue((uint32_t) rDelay, BITS_BINARY);

			/* Fractional part */
			const _REAL rFracPart = rDelay - (int) rDelay;
			Enqueue((uint32_t) (rFracPart * 256), BITS_BINARY);
		}
	}
}

string
CTagItemGeneratorRDEL::GetTagName()
{
	return "rdel";
}

string
CTagItemGeneratorRDEL::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRAFS::GenTag(CParameter & Parameter)
{
    vector<bool> audioFrameStatus;

	Parameter.Measurements.audioFrameStatus.get(audioFrameStatus);

	const int iNumUnits = audioFrameStatus.size();

	if (iNumUnits == 0)
	{
		/* zero length tag item */
		PrepareTag(0);
	}
	else
	{
		/* Header - length is always 48 */
		PrepareTag(48);

		/* number of units: 8 bits */
		Enqueue(iNumUnits, 8);

		/* status for each unit */
		for (int i = 0; i < iNumUnits; i++)
		{
			Enqueue(audioFrameStatus[i]?1:0, 1);
		}
		/* pad the rest with zeros */
		Enqueue(0, 40 - iNumUnits);
	}
}

string
CTagItemGeneratorRAFS::GetTagName()
{
	return "rafs";
}

string
CTagItemGeneratorRAFS::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRINT::GenTag(bool bIsValid, _REAL rIntFreq, _REAL rINR, _REAL rICR)
{

	/* Use Bint (BBC proprietary tag name) until tag is accepted by SE group */
	/* If no value is available, set tag length to zero */
	if (bIsValid == false)
	{
		/* Length: 0 byte */
		PrepareTag(0);
	}
	else
	{
		/* Length: 2 bytes per value, 3 values = 48 bits */
		PrepareTag(48);

		/* Interference frequency (Hz) : signed value */
		Enqueue((uint32_t) ((int) rIntFreq), 16);

		/* Interference-to-noise ratio */
		/* integer part */
		Enqueue((uint32_t) rINR, BITS_BINARY);
		/* Fractional part */
		Enqueue((uint32_t) ((rINR - (int) rINR) * 256), BITS_BINARY);

		/* Interference-to-carrier ratio */
		/* integer part */
		Enqueue((uint32_t) rICR, BITS_BINARY);
		/* Fractional part */
		Enqueue((uint32_t) ((rICR - (int) rICR) * 256), BITS_BINARY);

	}

}

string
CTagItemGeneratorRINT::GetTagName()
{
	return "Bint";
}

string
CTagItemGeneratorRINT::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRNIP::GenTag(bool bIsValid, _REAL rIntFreq, _REAL rISR)
{

	/* If no value is available, set tag length to zero */
	if (bIsValid == false)
	{
		/* Length: 0 byte */
		PrepareTag(0);
	}
	else
	{
		/* Length: 2 bytes per value, 2 values = 32 bits */
		PrepareTag(32);

		/* Interference frequency (Hz) : signed value */
		Enqueue((uint32_t) ((int) rIntFreq), 16);

		/* Interference-to-signal ratio */
		/* integer part */
		Enqueue((uint32_t) ((int) (rISR * 256)), 16);
	}
}

string
CTagItemGeneratorRNIP::GetTagName()
{
	return "rnip";
}

string
CTagItemGeneratorRNIP::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorSignalStrength::GenTag(bool bIsValid, _REAL rSigStrength)
{
	if (bIsValid == false)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(16);
		Enqueue((uint32_t) ((int) (rSigStrength * 256)), 16);
	}
}

string
CTagItemGeneratorSignalStrength::GetTagName()
{
	return "rdbv";
}

string
CTagItemGeneratorSignalStrength::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorReceiverStatus::GenTag(CParameter & Parameter)
{
	PrepareTag(4 * BITS_BINARY);
	Enqueue(Parameter.ReceiveStatus.TSync.GetStatus() == RX_OK ? 0 : 1, BITS_BINARY);	/* 0=ok, 1=bad */
	Enqueue(Parameter.ReceiveStatus.FAC.GetStatus() == RX_OK ? 0 : 1, BITS_BINARY);	/* 0=ok, 1=bad */
	Enqueue(Parameter.ReceiveStatus.SDC.GetStatus() == RX_OK ? 0 : 1, BITS_BINARY);	/* 0=ok, 1=bad */
	Enqueue(Parameter.ReceiveStatus.Audio.GetStatus() == RX_OK ? 0 : 1, BITS_BINARY);	/* 0=ok, 1=bad */
}

string
CTagItemGeneratorReceiverStatus::GetTagName()
{
	return "rsta";
}

string
CTagItemGeneratorReceiverStatus::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorProfile::GenTag(char cProfile)
{
	PrepareTag(8);
	Enqueue((uint32_t) cProfile, BITS_BINARY);
}

string
CTagItemGeneratorProfile::GetTagName()
{
	return "rpro";
}

string
CTagItemGeneratorProfile::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRxDemodMode::GenTag(EModulationType eMode)	// rdmo
{
	PrepareTag(4 * BITS_BINARY);
	string s;
	switch (eMode)
	{
	case DRM: s="drm_"; break;
	case AM: s="am__"; break;
	case USB: s="usb_"; break;
	case LSB: s="lsb_"; break;
	case NBFM: s="nbfm"; break;
	case WBFM: s="wbfm"; break;
	default: s="    "; break;
	}

	for (int i=0; i<4; i++)
		Enqueue((uint32_t) s[i], BITS_BINARY);
}

string
CTagItemGeneratorRxDemodMode::GetTagName()
{
	return "rdmo";
}

string
CTagItemGeneratorRxDemodMode::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRxFrequency::GenTag(bool bIsValid, int iFrequency)	// Frequency in kHz
{
	if (bIsValid == false)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(4 * BITS_BINARY);
		Enqueue((uint32_t) iFrequency * 1000, 4 * BITS_BINARY);
	}
}

string
CTagItemGeneratorRxFrequency::GetTagName()
{
	return "rfre";
}

string
CTagItemGeneratorRxFrequency::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRxActivated::GenTag(bool bActivated)
{
	PrepareTag(BITS_BINARY);
	Enqueue(bActivated == true ? '0' : '1', BITS_BINARY);
}

string
CTagItemGeneratorRxActivated::GetTagName()
{
	return "ract";
}

string
CTagItemGeneratorRxActivated::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRxBandwidth::GenTag(bool bIsValid, _REAL rBandwidth)
{
	if (bIsValid == false)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(2 * BITS_BINARY);
		Enqueue((uint32_t) ((int) (rBandwidth * 256.0)), 2 * BITS_BINARY);
	}
}

string
CTagItemGeneratorRxBandwidth::GetTagName()
{
	return "rbw_";
}

string
CTagItemGeneratorRxBandwidth::GetProfiles()
{
	return "ABCDGQ";
}

void
CTagItemGeneratorRxService::GenTag(bool bIsValid, int iService)
{
	if (bIsValid == false)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(BITS_BINARY);
		Enqueue((uint32_t) iService, BITS_BINARY);
	}
}

string
CTagItemGeneratorRxService::GetTagName()
{
	return "rser";
}

string
CTagItemGeneratorRxService::GetProfiles()
{
	return "ABCDGQ";
}

CTagItemGeneratorRBP::CTagItemGeneratorRBP()
{
}

void
CTagItemGeneratorRBP::SetStreamNumber(int iStrNum)
{
	iStreamNumber = iStrNum;
}

void
CTagItemGeneratorRBP::GenTag()	// Not yet implemented
{
}

string
CTagItemGeneratorRBP::GetTagName()
{
	switch (iStreamNumber)
	{
	case 0:
		return "rbp0";
	case 1:
		return "rbp1";
	case 2:
		return "rbp2";
	case 3:
		return "rbp3";
	default:
		return "rbp?";			// error!
	}
}

string
CTagItemGeneratorRBP::GetProfiles()
{
	return "ABCDG";
}

// Call this to write the binary data (header + payload) to the vector
void
CTagItemGenerator::PutTagItemData(CVector < _BINARY > &vecbiDestination)
{
	vecbiTagData.ResetBitAccess();
	for (int i = 0; i < vecbiTagData.Size(); i++)
		vecbiDestination.Enqueue(vecbiTagData.Separate(1), 1);
}

void
CTagItemGenerator::Reset()	// Resets bit vector to zero length (i.e. no header)
{
	vecbiTagData.Init(0);
}

void
CTagItemGenerator::GenEmptyTag()	// Generates valid tag item with zero payload length
{
	PrepareTag(0);
}

// Prepare vector and make the header
void
CTagItemGenerator::PrepareTag(int iLenDataBits)
{
	string strTagName = GetTagName();
	/* Init vector length. 4 bytes for tag name and 4 bytes for data length
	   plus the length of the actual data */
	vecbiTagData.Init(8 * BITS_BINARY + iLenDataBits);
	vecbiTagData.ResetBitAccess();

	/* Set tag name (always four bytes long) */
	for (int i = 0; i < 4; i++)
		vecbiTagData.Enqueue((uint32_t) strTagName[i], BITS_BINARY);

	/* Set tag data length */
	vecbiTagData.Enqueue((uint32_t) iLenDataBits, 32);

}

// Put the bits to the bit vector (avoids derived classes needing to access the bit vector directly
void
CTagItemGenerator::Enqueue(uint32_t iInformation, int iNumOfBits)
{
	vecbiTagData.Enqueue(iInformation, iNumOfBits);
}

/* TODO: there are still some RSCI tags left to implement */
/* e.g. rpil, rpsd, ... */

//andrewm - 2006-12-08
void
CTagItemGeneratorGPS::GenTag(bool bIsValid, CGPSData & GPSData)	// Long/Lat in degrees
{
	if (bIsValid == false)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(26 * BITS_BINARY);

		switch (GPSData.GetGPSSource())
		{
		case CGPSData::GPS_SOURCE_INVALID:
			Enqueue((uint32_t) 0x00, BITS_BINARY);
			break;
		case CGPSData::GPS_SOURCE_GPS_RECEIVER:
			Enqueue((uint32_t) 0x01, BITS_BINARY);
			break;
		case CGPSData::GPS_SOURCE_DIFFERENTIAL_GPS_RECEIVER:
			Enqueue((uint32_t) 0x02, BITS_BINARY);
			break;
		case CGPSData::GPS_SOURCE_MANUAL_ENTRY:
			Enqueue((uint32_t) 0x03, BITS_BINARY);
			break;
		case CGPSData::GPS_SOURCE_NOT_AVAILABLE:
			Enqueue((uint32_t) 0xFF, BITS_BINARY);
			break;
		default:
			Enqueue((uint32_t) 0xFF, BITS_BINARY);
			break;
		}

		if (GPSData.GetSatellitesVisibleAvailable())
		{
			Enqueue((uint32_t) GPSData.GetSatellitesVisible(), BITS_BINARY);
		}
		else
		{
			Enqueue((uint32_t) 0xff, BITS_BINARY);
		}

		if (GPSData.GetPositionAvailable())
		{
			double latitude, longitude;
			int iLatitudeDegrees;
			uint8_t uiLatitudeMinutes;
			uint16_t uiLatitudeMinuteFractions;

			int iLongitudeDegrees;
			uint8_t uiLongitudeMinutes;
			uint16_t uiLongitudeMinuteFractions;

			GPSData.GetLatLongDegrees(latitude, longitude);

			if (latitude >= 0)
				iLatitudeDegrees = (int) latitude;
			else
				iLatitudeDegrees = (int) latitude - 1;

			uiLatitudeMinutes =
				(uint8_t) (60.0 * (latitude - iLatitudeDegrees));
			uiLatitudeMinuteFractions =
				(uint16_t) (((60.0 * (latitude - iLatitudeDegrees)) -
							 uiLatitudeMinutes) * 65536.0);

			if (longitude >= 0)
				iLongitudeDegrees = (int) longitude;
			else
				iLongitudeDegrees = (int) longitude - 1;

			uiLongitudeMinutes =
				(uint8_t) (60.0 * (longitude - iLongitudeDegrees));
			uiLongitudeMinuteFractions =
				(uint16_t) (((60.0 * (longitude - iLongitudeDegrees)) -
							 uiLongitudeMinutes) * 65536.0);

			Enqueue((uint32_t) iLatitudeDegrees, 2 * BITS_BINARY);
			Enqueue((uint32_t) uiLatitudeMinutes, BITS_BINARY);
			Enqueue((uint32_t) uiLatitudeMinuteFractions, 2 * BITS_BINARY);
			Enqueue((uint32_t) iLongitudeDegrees, 2 * BITS_BINARY);
			Enqueue((uint32_t) uiLongitudeMinutes, BITS_BINARY);
			Enqueue((uint32_t) uiLongitudeMinuteFractions, 2 * BITS_BINARY);
		}
		else
		{
			Enqueue((uint32_t) 0xffff, 2 * BITS_BINARY);
			Enqueue((uint32_t) 0xffff, BITS_BINARY);
			Enqueue((uint32_t) 0xffff, 2 * BITS_BINARY);
			Enqueue((uint32_t) 0xffff, 2 * BITS_BINARY);
			Enqueue((uint32_t) 0xffff, BITS_BINARY);
			Enqueue((uint32_t) 0xffff, 2 * BITS_BINARY);
		}

		if (GPSData.GetAltitudeAvailable())
		{
			int iAltitudeMetres;
			uint8_t uiAltitudeMetreFractions;

			if (GPSData.GetAltitudeMetres() >= 0)
				iAltitudeMetres = (int) GPSData.GetAltitudeMetres();
			else
				iAltitudeMetres = (int) (GPSData.GetAltitudeMetres() - 1);

			uiAltitudeMetreFractions = (uint8_t) (256.0 * (GPSData.GetAltitudeMetres() - iAltitudeMetres));

			Enqueue((uint32_t) iAltitudeMetres, 2 * BITS_BINARY);
			Enqueue((uint32_t) uiAltitudeMetreFractions, BITS_BINARY);
		}
		else
		{
			Enqueue((uint32_t) 0xffff, 2 * BITS_BINARY);
			Enqueue((uint32_t) 0xff, BITS_BINARY);
		}

		if (GPSData.GetTimeAndDateAvailable())
		{
			uint32_t year;
			uint8_t month, day, hour, minute, second;
			GPSData.GetTimeDate(year, month, day, hour, minute, second);
			Enqueue((uint32_t) hour, BITS_BINARY);
			Enqueue((uint32_t) minute, BITS_BINARY);
			Enqueue((uint32_t) second, BITS_BINARY);
			Enqueue(year, 2*BITS_BINARY);
			Enqueue((uint32_t) month, BITS_BINARY);
			Enqueue((uint32_t) day, BITS_BINARY);
		}
		else
		{
			Enqueue((uint32_t) 0xff, BITS_BINARY);
			Enqueue((uint32_t) 0xff, BITS_BINARY);
			Enqueue((uint32_t) 0xff, BITS_BINARY);
			Enqueue((uint32_t) 0xff, BITS_BINARY);
			Enqueue((uint32_t) 0xffff, 2 * BITS_BINARY);
			Enqueue((uint32_t) 0xff, BITS_BINARY);
		}

		if (GPSData.GetSpeedAvailable())
		{
			Enqueue((uint32_t) (GPSData.GetSpeedMetresPerSecond() * 10.0), 2 * BITS_BINARY);
		}
		else
		{
			Enqueue((uint32_t) 0xffff, 2 * BITS_BINARY);
		}

		if (GPSData.GetHeadingAvailable())
		{
			Enqueue((uint32_t) GPSData.GetHeadingDegrees(), 2 * BITS_BINARY);
		}
		else
		{
			Enqueue((uint32_t) 0xffff, 2 * BITS_BINARY);
		}
	}
}

string
CTagItemGeneratorGPS::GetTagName()
{
	return "rgps";
}

string
CTagItemGeneratorGPS::GetProfiles()
{
	return "ADG";
}

void
CTagItemGeneratorPowerSpectralDensity::GenTag(CParameter & Parameter)
{
    vector<_REAL> psd;
	if (Parameter.Measurements.PSD.get(psd) == false)
	{
		GenEmptyTag();
		return;
	}

	PrepareTag(psd.size() * BITS_BINARY);

	for (size_t i = 0; i < psd.size(); i++)
	{
		uint32_t p = uint8_t(psd[i] * _REAL(-2.0));
		Enqueue(uint32_t(p), BITS_BINARY);
	}
}

string
CTagItemGeneratorPowerSpectralDensity::GetTagName()
{
	return "rpsd";
}

string
CTagItemGeneratorPowerSpectralDensity::GetProfiles()
{
	return "ADG";
}

void
CTagItemGeneratorPowerImpulseResponse::GenTag(CParameter & Parameter)
{
	const _REAL rOffset = _REAL(-60.0);
	CMeasurements::CPIR pir;
	(void)Parameter.Measurements.PIR.get(pir);
	PrepareTag(pir.data.size() * BITS_BINARY + 4 * BITS_BINARY);

    _REAL end = pir.rStart + pir.rStep * pir.data.size();
	Enqueue(uint32_t(int(pir.rStart * _REAL(256.0))), 2*BITS_BINARY);
	Enqueue(uint32_t(int(end * _REAL(256.0))), 2*BITS_BINARY);

	for (size_t i = 0; i < pir.data.size(); i++)
	{
		uint32_t p = uint8_t((pir.data[i]+rOffset) * _REAL(-2.0));
		Enqueue(p, BITS_BINARY);
	}

}

string
CTagItemGeneratorPowerImpulseResponse::GetTagName()
{
	return "rpir";
}

string
CTagItemGeneratorPowerImpulseResponse::GetProfiles()
{
	return "ADG";
}

string
CTagItemGeneratorPilots::GetTagName()
{
	return "rpil";
}

string
CTagItemGeneratorPilots::GetProfiles()
{
	return "D";
}

void
CTagItemGeneratorPilots::GenTag(CParameter & Parameter)
{
	const CCellMappingTable& Param = Parameter.CellMappingTable;
	// Get parameters from parameter struct
	int iScatPilTimeInt = Param.iScatPilTimeInt;
	int iScatPilFreqInt = Param.iScatPilFreqInt;
	int iNumCarrier = Param.iNumCarrier;
	int iNumSymPerFrame = Param.iNumSymPerFrame;
	/* do we need these ? */
	//int iNumIntpFreqPil = Param.iNumIntpFreqPil;
	//int iFFTSizeN = Param.iFFTSizeN;

	// calculate the spacing between scattered pilots in a given symbol
	int iScatPilFreqSpacing = iScatPilFreqInt * iScatPilTimeInt;

	// Calculate how long the tag will be and write the fields that apply to the whole frame

	// Total number of pilots = number of pilot bearing carriers * number of pilot pattern repeats per frame
	// NB the DC carrier in mode D is INCLUDED in this calculation (and in the tag)
	int iTotalNumPilots =
		((iNumCarrier - 1) / iScatPilFreqInt +
		 1) * iNumSymPerFrame / iScatPilTimeInt;

	int iTagLen = 4 * BITS_BINARY;	// first 4 bytes apply to the whole frame
	iTagLen += iNumSymPerFrame * 4 * BITS_BINARY;	// 4 bytes at start of each symbol (spec typo?)
	iTagLen += iTotalNumPilots * 2 * 2 * BITS_BINARY;	// 4 bytes per pilot value (2 byte re, 2 byte imag)

	//log.GetStatus("rpil gentag: pilots %d tag length %d", iTotalNumPilots, iTagLen);

	PrepareTag(iTagLen);

	// fields for the whole frame
	Enqueue((uint32_t) iNumSymPerFrame, BITS_BINARY);	// SN = number of symbols
	Enqueue((uint32_t) iScatPilTimeInt, BITS_BINARY);	// SR = symbol repetition
	Enqueue((uint32_t) 0, 2 * BITS_BINARY);	// rfu

	// Check that the matrix has the expected dimensions (in case of a mode change)
	vector<vector<_COMPLEX> > pilots;
	if(Parameter.Measurements.Pilots.get(pilots)==false)
	{
		GenEmptyTag();
        return;
	}
	if (pilots.size() != size_t(iNumSymPerFrame / iScatPilTimeInt)
		|| pilots[0].size() != size_t((iNumCarrier - 1) / iScatPilFreqInt + 1))
	{
		GenEmptyTag();
#if 0
		log.GetStatus("Wrong size: %d x %d, expected %d x %d",
				  pilots.size(),
				  pilots[0].size(),
				  iNumSymPerFrame / iScatPilTimeInt,
				  ((iNumCarrier - 1) / iScatPilFreqInt + 1));
#endif
		return;
	}

	// Now do each symbol in turn
	for (int iSymbolNumber = 0; iSymbolNumber < iNumSymPerFrame;
		 iSymbolNumber++)
	{
		// Which row of the matrix?
		int iRow = iSymbolNumber / iScatPilTimeInt;
		int i, iCarrier;

		// Find the first pilot in this symbol (this could be calculated directly,
		// but that calculation would belong in the CellMappingTable class)
		int iFirstPilotCarrier = 0;

		while (!_IsScatPil(Param.matiMapTab[iSymbolNumber][iFirstPilotCarrier]))
		{
			iFirstPilotCarrier += iScatPilFreqInt;
		}

		// Find the biggest value we need to represent for this symbol

		_REAL rMax = _REAL(0.0);
		int iNumPilots = 0;

		// Start from first pilot and step by the pilot spacing (iScatPilFreqInt*iScatPilTimeInt)
		for (i = iFirstPilotCarrier / iScatPilFreqInt, iCarrier =
			 iFirstPilotCarrier; iCarrier < iNumCarrier;
			 i += iScatPilTimeInt, iCarrier += iScatPilFreqSpacing)
		{
			iNumPilots++;
			// Is it really a pilot? This will be false only in Mode D for the DC carrier
			if (_IsScatPil(Param.matiMapTab[iSymbolNumber][iCarrier]))
			{
				_COMPLEX cPil = pilots[iRow][i];
				if (cPil.real() > rMax)
					rMax = cPil.real();
				if (-cPil.real() > rMax)
					rMax = -cPil.real();
				if (cPil.imag() > rMax)
					rMax = cPil.imag();
				if (-cPil.imag() > rMax)
					rMax = -cPil.imag();
			}
		}

		// Calculate the exponent for the block
		_REAL rExponent = ceil(log(rMax) / log(2.0));
		_REAL rScale = 32767 * pow(_REAL(2.0), -rExponent);

		// Put to the tag
		Enqueue((uint32_t) iNumPilots, BITS_BINARY);	// PN = number of pilots
		Enqueue((uint32_t) iFirstPilotCarrier, BITS_BINARY);	// PO = pilot offset
		Enqueue((uint32_t) rExponent, 2 * BITS_BINARY);

		// Step through the pilots again and write the values
		for (i = iFirstPilotCarrier / iScatPilFreqInt, iCarrier =
			 iFirstPilotCarrier; iCarrier < iNumCarrier;
			 i += iScatPilTimeInt, iCarrier += iScatPilFreqSpacing)
		{
			Enqueue(uint32_t(pilots[iRow][i].real()*rScale), 2 * BITS_BINARY);
			Enqueue(uint32_t(pilots[iRow][i].imag()*rScale), 2 * BITS_BINARY);
		}
	}	// next symbol
}

void
CTagItemGeneratorAMAudio::GenTag(CParameter & Parameter, CSingleBuffer < _BINARY > &AudioData)
{

	const int iLenStrData =
		BITS_BINARY * (Parameter.MSCParameters.Stream[0].iLenPartA + Parameter.MSCParameters.Stream[0].iLenPartB);
	// Only generate this tag if stream input data is not of zero length
	if (iLenStrData == 0)
		return;

	CVectorEx < _BINARY > *pvecbiStrData = AudioData.Get(iLenStrData);
	// check we have data in the vector
	if (iLenStrData != pvecbiStrData->Size())
		return;

	PrepareTag(iLenStrData + 16);

	// Send audio parameters

	// Audio coding
	int iVal = 0;
	switch (Parameter.AudioParam[0].eAudioCoding)
	{
	case CAudioParam::AC_AAC:	// 00
		iVal = 0;
		break;
	case CAudioParam::AC_CELP:	// 01
		iVal = 1;
		break;
	case CAudioParam::AC_HVXC:	// 10
		iVal = 2;
		break;
	default:
		iVal = 0;				// reserved
	}

	Enqueue(iVal, 2);

	// SBR flag
	Enqueue(Parameter.AudioParam[0].eSBRFlag == CAudioParam::SB_USED ? 1 : 0, 1);

	// Audio mode
	switch (Parameter.AudioParam[0].eAudioMode)
	{
	case CAudioParam::AM_MONO:
		iVal = 0;
		break;
	case CAudioParam::AM_P_STEREO:
		iVal = 1;
		break;
	case CAudioParam::AM_STEREO:
		iVal = 2;
		break;
	default:
		iVal = 0;
	}
	Enqueue(iVal, 2);

	// Audio sampling rate
	switch (Parameter.AudioParam[0].eAudioSamplRate)
	{
	case CAudioParam::AS_8_KHZ:
		iVal = 0;
		break;
	case CAudioParam::AS_12KHZ:
		iVal = 1;
		break;
	case CAudioParam::AS_16KHZ:
		iVal = 2;
		break;
	case CAudioParam::AS_24KHZ:
		iVal = 3;
		break;
	default:
		iVal = 3;
	}

	Enqueue(iVal, 3);

	// coder field and some rfus (TODO: code the coder field correctly for all cases
	Enqueue(0, 8);

	// Now send the stream data
	pvecbiStrData->ResetBitAccess();
	// Data is always a multiple of 8 -> copy bytes
	for (int i = 0; i < iLenStrData / BITS_BINARY; i++)
	{
		Enqueue(pvecbiStrData->Separate(BITS_BINARY), BITS_BINARY);
	}
}

string
CTagItemGeneratorAMAudio::GetProfiles()
{
	return "AD";
}

string
CTagItemGeneratorAMAudio::GetTagName()
{
	return "rama";
}
