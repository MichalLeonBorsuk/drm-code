/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *	DRM Parameters
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

#include "GPSData.h"
#include "time.h"
#include <sstream>
#include <iomanip>

CGPSData::CGPSData()
{
	Reset();
}

CGPSData::~CGPSData()
{
}

CGPSData::EGPSSource CGPSData::GetGPSSource() const
{
	return eGPSSource;
}

void
CGPSData::SetGPSSource(EGPSSource eNewSource)
{
	eGPSSource = eNewSource;
}

void
CGPSData::SetSatellitesVisibleAvailable(bool bNew)
{
	m_bSatellitesVisibleAvailable = bNew;
}

bool CGPSData::GetSatellitesVisibleAvailable() const
{
	return m_bSatellitesVisibleAvailable;
}

void
CGPSData::SetSatellitesVisible(uint16_t usSatellitesVisible)
{
	m_usSatellitesVisible = usSatellitesVisible;
}

uint16_t CGPSData::GetSatellitesVisible() const
{
	return m_usSatellitesVisible;
}

void
CGPSData::SetPositionAvailable(bool bNew)
{
	m_bPositionAvailable = bNew;
}

bool CGPSData::GetPositionAvailable() const
{
	return m_bPositionAvailable;
}

void
CGPSData::SetLatLongDegrees(double fLatitudeDegrees, double fLongitudeDegrees)
{
	m_fLatitudeDegrees = fLatitudeDegrees;
	m_fLongitudeDegrees = fLongitudeDegrees;
}

void
CGPSData::GetLatLongDegrees(double &fLatitudeDegrees, double &fLongitudeDegrees) const
{
	fLatitudeDegrees = m_fLatitudeDegrees;
	fLongitudeDegrees = m_fLongitudeDegrees;
}

void
CGPSData::GetLatLongDegrees(string& latitude, string& longitude) const
{
	stringstream s;
	s << m_fLatitudeDegrees;
	latitude = s.str();
	s.str("");
	s << m_fLongitudeDegrees;
	longitude = s.str();
}

unsigned int CGPSData::ExtractMinutes(double dblDeg) const
{
	unsigned int Degrees;
	/* Extract degrees */
	Degrees = (unsigned int) dblDeg;
	return (unsigned int) (((floor((dblDeg - Degrees) * 1000000) / 1000000) + 0.00005) * 60.0);
}

void
CGPSData::asDM(string& lat, string& lng) const
{
	asDM(lat, m_fLatitudeDegrees, 'S', 'N');
	asDM(lng, m_fLongitudeDegrees, 'W', 'E');
}

void
CGPSData::asDM(string& pos, double d, char n, char p) const
{
	stringstream s;
	/* don't use abs, its seems to be int only on MSVC6 */
	double pd;
	if(d<0)
		pd = 0.0 - d;
	else
		pd = d;
	s << int(pd) << '\xb0' << setfill('0') << setw(2) << ExtractMinutes(pd) << "'" << ((d < 0.0)?n:p);
	pos = s.str();
}

void
CGPSData::SetSpeedAvailable(bool bNew)
{
	m_bSpeedAvailable = bNew;
}

bool CGPSData::GetSpeedAvailable() const
{
	return m_bSpeedAvailable;
}

void
CGPSData::SetSpeedMetresPerSecond(double fSpeedMetresPerSecond)
{
	m_fSpeedMetresPerSecond = fSpeedMetresPerSecond;
}

double
CGPSData::GetSpeedMetresPerSecond() const
{
	return m_fSpeedMetresPerSecond;
}

void
CGPSData::SetHeadingAvailable(bool bNew)
{
	m_bHeadingAvailable = bNew;
}

bool CGPSData::GetHeadingAvailable() const
{
	return m_bHeadingAvailable;
}

void
CGPSData::SetHeadingDegrees(uint16_t usHeadingDegrees)
{
	m_usHeadingDegrees = usHeadingDegrees;
}

unsigned short
CGPSData::GetHeadingDegrees() const
{
	return m_usHeadingDegrees;
}

void
CGPSData::SetTimeAndDateAvailable(bool bNew)
{
	m_bTimeAndDateAvailable = bNew;
}

bool CGPSData::GetTimeAndDateAvailable() const
{
	return m_bTimeAndDateAvailable;
}

void
CGPSData::SetTimeSecondsSince1970(uint32_t ulTimeSecondsSince1970)
{
	m_ulTimeSecondsSince1970 = ulTimeSecondsSince1970;
}

uint32_t CGPSData::GetTimeSecondsSince1970() const
{
	return m_ulTimeSecondsSince1970;
}

void
CGPSData::SetAltitudeAvailable(bool bNew)
{
	m_bAltitudeAvailable = bNew;
}

bool CGPSData::GetAltitudeAvailable() const
{
	return m_bAltitudeAvailable;
}

void
CGPSData::SetAltitudeMetres(double fAltitudeMetres)
{
	m_fAltitudeMetres = fAltitudeMetres;
}

double
CGPSData::GetAltitudeMetres() const
{
	return m_fAltitudeMetres;
}

void
CGPSData::SetFix(CGPSData::EFix Fix)
{
	m_eFix = Fix;
}

CGPSData::EFix CGPSData::GetFix() const
{
	return m_eFix;
}

void
CGPSData::SetStatus(EStatus eStatus)
{
	m_eStatus = eStatus;
	if (m_eStatus != GPS_RX_DATA_AVAILABLE)
		Reset();
}

CGPSData::EStatus CGPSData::GetStatus() const
{
	return m_eStatus;
}

void
CGPSData::Reset()
{
	m_bPositionAvailable = false;
	m_fLatitudeDegrees = 0;
	m_fLongitudeDegrees = 0;
	m_bSpeedAvailable = false;
	m_fSpeedMetresPerSecond = 0;
	m_bHeadingAvailable = false;
	m_usHeadingDegrees = 0;
	m_bTimeAndDateAvailable = false;
	m_ulTimeSecondsSince1970 = 0;
	m_bAltitudeAvailable = false;
	m_fAltitudeMetres = 0;
	m_eStatus = GPS_RX_NOT_CONNECTED;
	m_eFix = MODE_NO_FIX;
	m_bSatellitesVisibleAvailable = false;
	m_usSatellitesVisible = 0;
}

void
CGPSData::GetTimeDate(uint32_t & year, uint8_t & month, uint8_t & day,
					  uint8_t & hour, uint8_t & minute, uint8_t & second) const
{
	struct tm *p_ts;
	time_t tt;
	{
		tt = time_t(m_ulTimeSecondsSince1970);
	}
	p_ts = gmtime(&tt);
	year = 1900+p_ts->tm_year;
	month = p_ts->tm_mon+1;
	day = p_ts->tm_mday;
	hour = p_ts->tm_hour;
	minute = p_ts->tm_min;
	second = p_ts->tm_sec;
}

string CGPSData::GetTimeDate() const
{
	struct tm * p_ts;
	time_t tt;
	{
		tt = time_t(m_ulTimeSecondsSince1970);
	}
	p_ts = gmtime(&tt);
	stringstream ss;
	ss.width(2);
	ss.fill('0');
	ss << 1900 + p_ts->tm_year << "/"
	<< setw(2) << 1 + p_ts->tm_mon << "/"
	<< setw(2) << p_ts->tm_mday
	<< " "
	<< setw(2) << p_ts->tm_hour << ":"
	<< setw(2) << p_ts->tm_min << ":"
	<< setw(2) << p_ts->tm_sec;
	return ss.str();
}
