/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Andrew Murphy
 *
 * Description:
 *	passive data class for GPS data
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

#ifndef _GPS_DATA_H
#define _GPS_DATA_H

#include "GlobalDefinitions.h"

class CGPSData
{
public:
    CGPSData()
    {
        Reset();
    }
    ~CGPSData() {}

    enum EFix { MODE_NO_FIX, MODE_2D, MODE_3D };

    enum EStatus { GPS_RX_NOT_CONNECTED, GPS_RX_NO_DATA, GPS_RX_DATA_AVAILABLE };

    enum EGPSSource
    { GPS_SOURCE_INVALID, GPS_SOURCE_GPS_RECEIVER,
      GPS_SOURCE_DIFFERENTIAL_GPS_RECEIVER, GPS_SOURCE_MANUAL_ENTRY,
      GPS_SOURCE_NOT_AVAILABLE
    };

    /////////

    EGPSSource GetGPSSource() const;
    void SetGPSSource(EGPSSource eNewSource);

    /////////

    void SetSatellitesVisibleAvailable(_BOOLEAN bNew);
    _BOOLEAN GetSatellitesVisibleAvailable() const;

    void SetSatellitesVisible(uint16_t usSatellitesVisible);
    uint16_t GetSatellitesVisible() const;

    /////////

    void SetPositionAvailable(_BOOLEAN bNew);
    _BOOLEAN GetPositionAvailable() const;

    void SetLatLongDegrees(double fLatitudeDegrees, double fLongitudeDegrees);
    void GetLatLongDegrees(double& fLatitudeDegrees, double& fLongitudeDegrees) const;

    void GetLatLongDegrees(string& latitude, string& longitude) const;

    unsigned int ExtractMinutes(double dblDeg) const;
    void asDM(string& lat, string& lng) const;
    void asDM(string& pos, double d, char n, char p) const;

    /////////

    void SetSpeedAvailable(_BOOLEAN bNew);
    _BOOLEAN GetSpeedAvailable() const;

    void SetSpeedMetresPerSecond(double fSpeedMetresPerSecond);
    double GetSpeedMetresPerSecond() const;

    /////////

    void SetHeadingAvailable(_BOOLEAN bNew);
    _BOOLEAN GetHeadingAvailable() const;

    void SetHeadingDegrees(uint16_t usHeadingDegrees);
    unsigned short GetHeadingDegrees() const;

    /////////

    void SetTimeAndDateAvailable(_BOOLEAN bNew);
    _BOOLEAN GetTimeAndDateAvailable() const;

    void SetTimeSecondsSince1970(uint32_t ulTimeSecondsSince1970);
    uint32_t GetTimeSecondsSince1970() const;
    string GetTimeDate() const;
    void GetTimeDate(uint32_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute, uint8_t& second) const;

    /////////

    void SetAltitudeAvailable(_BOOLEAN bNew);
    _BOOLEAN GetAltitudeAvailable() const;

    void SetAltitudeMetres(double fAltitudeMetres);
    double GetAltitudeMetres() const;

    /////////

    void SetFix(EFix Fix);
    EFix GetFix() const;

    /////////

    void SetStatus(EStatus eStatus);
    EStatus GetStatus() const;

private:
    _BOOLEAN m_bSatellitesVisibleAvailable;
    uint16_t m_usSatellitesVisible;

    _BOOLEAN m_bPositionAvailable;
    double	m_fLatitudeDegrees;
    double	m_fLongitudeDegrees;

    _BOOLEAN m_bSpeedAvailable;
    double	m_fSpeedMetresPerSecond;

    _BOOLEAN m_bHeadingAvailable;
    uint16_t	m_usHeadingDegrees;

    _BOOLEAN m_bTimeAndDateAvailable;
    uint32_t m_ulTimeSecondsSince1970;

    _BOOLEAN m_bAltitudeAvailable;
    double m_fAltitudeMetres;

    EFix	m_eFix;
    EStatus m_eStatus;
    EGPSSource eGPSSource;

    void Reset();
};
#endif
