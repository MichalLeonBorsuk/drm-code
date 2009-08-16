/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *  This module derives, from the CTagItemDecoder base class, tag item decoders specialised to decode each of the tag
 *  items defined in the control part of RSCI.
 *  Decoded commands are generally sent straight to the CDRMReceiver object which
 *	they hold a pointer to.
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


#include "RSCITagItemDecoders.h"
#include "RSISubscriber.h"
#include <ctime>
#include <cstdlib>
#include <iostream>

/* RX_STAT Items */


_REAL CTagItemDecoderRSI::decodeDb(CVector<_BINARY>& vecbiTag)
{
 	  int8_t n = (int8_t)vecbiTag.Separate(8);
 	  uint8_t m = (uint8_t)vecbiTag.Separate(8);
 	  return _REAL(n)+_REAL(m)/256.0;
}

void CTagItemDecoderRdbv::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen < 16)
		return;
	_REAL rSigStr = decodeDb(vecbiTag);
 	 pParameter->Measurements.SigStrstat.addSample(rSigStr);
	 /* this is the only signal strength we have so update the IF level too.
	  * TODO scaling factor ? */
 	 pParameter->SetIFSignalLevel(rSigStr);
}

void CTagItemDecoderRsta::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;
 	uint8_t sync = (uint8_t)vecbiTag.Separate(8);
 	uint8_t fac = (uint8_t)vecbiTag.Separate(8);
 	uint8_t sdc = (uint8_t)vecbiTag.Separate(8);
 	uint8_t audio = (uint8_t)vecbiTag.Separate(8);
 	if(sync==0)
		pParameter->ReceiveStatus.TSync.SetStatus(RX_OK);
	else
		pParameter->ReceiveStatus.TSync.SetStatus(CRC_ERROR);
 	if(fac==0)
		pParameter->ReceiveStatus.FAC.SetStatus(RX_OK);
	else
		pParameter->ReceiveStatus.FAC.SetStatus(CRC_ERROR);
 	if(sdc==0)
		pParameter->ReceiveStatus.SDC.SetStatus(RX_OK);
	else
		pParameter->ReceiveStatus.SDC.SetStatus(CRC_ERROR);
 	if(audio==0)
		pParameter->ReceiveStatus.Audio.SetStatus(RX_OK);
	else
		pParameter->ReceiveStatus.Audio.SetStatus(CRC_ERROR);
}

void CTagItemDecoderRwmf::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->Measurements.WMERFAC.set(decodeDb(vecbiTag));
}

void CTagItemDecoderRwmm::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->Measurements.WMERMSC.set(decodeDb(vecbiTag));
}

void CTagItemDecoderRmer::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
    // TODO do we need both ?
    _REAL rMER = decodeDb(vecbiTag);
    pParameter->Measurements.MER.set(rMER);
	pParameter->Measurements.SNRHist.set(rMER);
}

void CTagItemDecoderRdop::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
    _REAL rDop = decodeDb(vecbiTag);
    // TODO do we need both ?
    pParameter->Measurements.Rdop.set(rDop);
    pParameter->Measurements.Doppler.set(rDop);
}

void CTagItemDecoderRdel::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	int iNumEntries = iLen/(3*BITS_BINARY);
	vector<CMeasurements::CRdel> rdel(iNumEntries);

	for (int i=0; i<iNumEntries; i++)
	{
 		rdel[i].threshold = vecbiTag.Separate(BITS_BINARY);
		rdel[i].interval = decodeDb(vecbiTag);
	}
	pParameter->Measurements.Rdel.set(rdel);
	pParameter->Measurements.Delay.set(rdel[0].interval);
}

void CTagItemDecoderRpsd::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 680 && iLen !=1112)
		return;

	int iVectorLen = iLen/BITS_BINARY;

	vector<_REAL> psd(iVectorLen);
	for (int i = 0; i < iVectorLen; i++)
	{
		psd[i] = -(_REAL(vecbiTag.Separate(BITS_BINARY))/_REAL(2.0));
	}
    pParameter->Measurements.PSD.set(psd);
}

void CTagItemDecoderRpir::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	const _REAL rOffset = _REAL(-60.0);

	if (iLen == 0)
	{
		pParameter->Measurements.PIR.invalidate();
		return;
	}

    CMeasurements::CPIR pir;

	int iVectorLen = iLen/BITS_BINARY - 4; // 4 bytes for the scale start and end

    // TODO
	pir.rStart = _REAL(int16_t(vecbiTag.Separate(2 * BITS_BINARY))) / _REAL(256.0);
	_REAL rPIREnd = _REAL(int16_t(vecbiTag.Separate(2 * BITS_BINARY))) / _REAL(256.0);
	pir.rStep = (rPIREnd-pir.rStart)/iVectorLen;

	pir.data.resize(iVectorLen);

	for (int i = 0; i < iVectorLen; i++)
	{
		pir.data[i] = -(_REAL(vecbiTag.Separate(BITS_BINARY))/_REAL(2.0)) - rOffset;
	}

	pParameter->Measurements.PIR.set(pir);
}


void CTagItemDecoderRgps::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 26 * BITS_BINARY)
		return;

    CGPSData& GPSData = pParameter->GPSData;

 	uint16_t source = (uint16_t)vecbiTag.Separate(BITS_BINARY);
 	switch(source)
 	{
 	    case 0:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_INVALID);
            break;
 	    case 1:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_GPS_RECEIVER);
            break;
 	    case 2:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_DIFFERENTIAL_GPS_RECEIVER);
            break;
 	    case 3:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_MANUAL_ENTRY);
            break;
 	    case 0xff:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_NOT_AVAILABLE);
            break;
 	    default:
            cerr << "error decoding rgps" << endl;
 	}

 	uint8_t nSats = (uint8_t)vecbiTag.Separate(BITS_BINARY);
 	if(nSats == 0xff)
 	{
 	    GPSData.SetSatellitesVisibleAvailable(false);
 	}
 	else
 	{
 	    GPSData.SetSatellitesVisible(nSats);
 	    GPSData.SetSatellitesVisibleAvailable(true);
 	}

    uint16_t val;
    val = uint16_t(vecbiTag.Separate(2 * BITS_BINARY));
	int16_t iLatitudeDegrees = *(int16_t*)&val;
    uint8_t uiLatitudeMinutes = (uint8_t)vecbiTag.Separate(BITS_BINARY);
	uint16_t uiLatitudeMinuteFractions = (uint16_t)vecbiTag.Separate(2 * BITS_BINARY);
    val = uint16_t(vecbiTag.Separate(2 * BITS_BINARY));
	int16_t iLongitudeDegrees = *(int16_t*)&val;
    uint8_t uiLongitudeMinutes = (uint8_t)vecbiTag.Separate(BITS_BINARY);
	uint16_t uiLongitudeMinuteFractions = (uint16_t)vecbiTag.Separate(2 * BITS_BINARY);
    if(uiLatitudeMinutes == 0xff)
    {
        GPSData.SetPositionAvailable(false);
    }
    else
    {
		double latitude, longitude;
		latitude = double(iLatitudeDegrees)
		 + (double(uiLatitudeMinutes) + double(uiLatitudeMinuteFractions)/65536.0)/60.0;
		longitude = double(iLongitudeDegrees)
		 + (double(uiLongitudeMinutes) + double(uiLongitudeMinuteFractions)/65536.0)/60.0;
        GPSData.SetLatLongDegrees(latitude, longitude);
        GPSData.SetPositionAvailable(true);
    }

    val = uint16_t(vecbiTag.Separate(2 * BITS_BINARY));
    uint8_t uiAltitudeMetreFractions = (uint8_t)vecbiTag.Separate(BITS_BINARY);
    if(val == 0xffff)
    {
        GPSData.SetAltitudeAvailable(false);
    }
    else
    {
        uint16_t iAltitudeMetres = *(int16_t*)&val;
        GPSData.SetAltitudeMetres(iAltitudeMetres+uiAltitudeMetreFractions/256.0);
        GPSData.SetAltitudeAvailable(true);
    }

    struct tm tm;
    tm.tm_hour = uint8_t(vecbiTag.Separate(BITS_BINARY));
    tm.tm_min = uint8_t(vecbiTag.Separate(BITS_BINARY));
    tm.tm_sec = uint8_t(vecbiTag.Separate(BITS_BINARY));
    uint16_t year = uint16_t(vecbiTag.Separate(2*BITS_BINARY));
    tm.tm_year = year - 1900;
    tm.tm_mon = uint8_t(vecbiTag.Separate(BITS_BINARY))-1;
    tm.tm_mday = uint8_t(vecbiTag.Separate(BITS_BINARY));

    if(tm.tm_hour == 0xff)
    {
        GPSData.SetTimeAndDateAvailable(false);
    }
    else
    {
		string se;
		char *e = getenv("TZ");
		if(e)
			se = e;
#ifdef _WIN32
        _putenv("TZ=UTC");
        _tzset();
        time_t t = mktime(&tm);
		stringstream ss("TZ=");
		ss << se;
        _putenv(ss.str().c_str());
#else
        putenv(const_cast<char*>("TZ=UTC"));
        tzset();
        time_t t = mktime(&tm);
		if(e)
			setenv("TZ", se.c_str(), 1);
		else
			unsetenv("TZ");
#endif
        GPSData.SetTimeSecondsSince1970(t);
        GPSData.SetTimeAndDateAvailable(true);
    }

    uint16_t speed = (uint16_t)vecbiTag.Separate(2*BITS_BINARY);
    if(speed == 0xffff)
    {
        GPSData.SetSpeedAvailable(false);
    }
    else
    {
        GPSData.SetSpeedMetresPerSecond(double(speed)/10.0);
        GPSData.SetSpeedAvailable(true);
    }

    uint16_t heading = (uint16_t)vecbiTag.Separate(2*BITS_BINARY);
    if(heading == 0xffff)
    {
        GPSData.SetHeadingAvailable(false);
    }
    else
    {
        GPSData.SetHeadingDegrees(heading);
        GPSData.SetHeadingAvailable(true);
    }
}

/*
void CTagItemDecoderPilots::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{

	int iNumSymbols = vecbiTag.Separate(8);
	int iSymbolRepetition = vecbiTag.Separate(8);
	vecbiTag.Separate(16);

	for (int iSymbolNumber = 0; iSymbolNumber<iNumSymbols; iSymbolNumber++)
	{
		int iNumPilots = vecbiTag.Separate(8);
		int iPilotOffset = vecbiTag.Separate(8);
		int iBlockExponent = (int16_t) vecbiTag.Separate(16);

		//write to pParameter->matcReceivedPilotValues - but don't know what size to make it. Arghhh
	}


}*/

/* RX_CTRL Items */

void CTagItemDecoderCact::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 8)
		return;

	const int iNewState = vecbiTag.Separate(8) - '0';

	if (pDRMReceiver == NULL)
		return;

	// TODO pDRMReceiver->SetState(iNewState);
	(void)iNewState;

}

void CTagItemDecoderCfre::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;

	if (pDRMReceiver == NULL)
		return;

	const int iNewFrequency = vecbiTag.Separate(32);

	pDRMReceiver->SetFrequency(iNewFrequency/1000);

}

void CTagItemDecoderCdmo::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;

	string s = "";
	for (int i = 0; i < iLen / BITS_BINARY; i++)
		s += (_BYTE) vecbiTag.Separate(BITS_BINARY);

	if (pDRMReceiver == NULL)
		return;

    CParameter& Parameters = *(pDRMReceiver->GetParameters());
    Parameters.Lock();
    EModulationType eNew = DRM;
	if(s == "drm_")
        eNew = DRM;
	if(s == "am__")
        eNew = AM;
	else if (s == "lsb_")
        eNew = LSB;
	else if (s == "usb_")
        eNew = USB;
	else if (s == "wbfm")
        eNew = WBFM;
	else if (s == "nbfm")
        eNew = NBFM;
		// synchronous AM?
    if(Parameters.eModulation != eNew)
    {
        Parameters.RxEvent = ChannelReconfiguration;
        cerr << "RSCI old: " << Parameters.eModulation << " new: " << eNew << endl;
        Parameters.eModulation = eNew;
    }
    Parameters.Unlock();

}

void CTagItemDecoderCrec::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;

	string s = "";
	for (int i = 0; i < 2; i++)
		s += (_BYTE) vecbiTag.Separate(BITS_BINARY);
	char c3 = (char) vecbiTag.Separate(BITS_BINARY);
	char c4 = (char) vecbiTag.Separate(BITS_BINARY);

	if (pDRMReceiver == NULL)
		return;

	if(s == "st")
		pDRMReceiver->SetRSIRecording(c4=='1', c3);
	if(s == "iq")
		pDRMReceiver->SetIQRecording(c4=='1');
}

void CTagItemDecoderCser::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 8)
		return;

	int iNewServiceID = int(vecbiTag.Separate(BITS_BINARY));

    CParameter& Parameters = *pDRMReceiver->GetParameters();

    Parameters.Lock();

    Parameters.SetCurSelAudioService(iNewServiceID);
    Parameters.SetCurSelDataService(iNewServiceID);

    Parameters.Unlock();
}

void CTagItemDecoderCpro::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 8)
		return;

	char c = char(vecbiTag.Separate(BITS_BINARY));
	if (pRSISubscriber != NULL)
		pRSISubscriber->SetProfile(c);
}
/* TODO: other control tag items */

