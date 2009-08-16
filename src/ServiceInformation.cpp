/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *  see ServiceInformation.h
 *
 *
 *******************************************************************************
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

#include "ServiceInformation.h"
#include <time.h>

// TODO - don't replicate these in Parameter.cpp

static ostream& operator<<(ostream& out, const CDumpable& d)
{
    d.dump(out);
    return out;
}

template<typename T>
void dump(ostream& out, T val)
{
    out << val;
}

template<typename T>
void dump(ostream& out, const vector<T>& vec)
{
    string sep = "";
    out << "[";
    for (size_t i = 0; i < vec.size(); i++)
    {
        out << sep; ::dump(out, vec[i]);
        sep = ", ";
    }
    out << "]";
}

void
CAltFreqSched::dump(ostream& out) const
{
    out << "{ iDayCode: " << iDayCode << "," << endl;
    out << "StartTime: " << iStartTime << "," << endl;
    out << "Duration: " << iDuration << "}" << endl;
}

void
CAltFreqRegion::dump(ostream& out) const
{
    out << "{ CIRAFZones: "; ::dump(out, veciCIRAFZones); out << endl;
    out << "Latitude: " << iLatitude << "," << endl;
    out << "LatitudeEx: " << iLatitudeEx << "," << endl;
    out << "LongitudeEx: " << iLongitudeEx << "}" << endl;
}

void
CServiceDefinition::dump(ostream& out) const
{
    out << "{ Frequencies: "; ::dump(out, veciFrequencies); out << endl;
    out << "RegionID: " << iRegionID << "," << endl;
    out << "ScheduleID: " << iScheduleID << "," << endl;
    out << "SystemID: " << iSystemID << "}" << endl;
}

void
CMultiplexDefinition::dump(ostream& out) const
{
    out << "{ Frequencies: "; ::dump(out, veciFrequencies); out << endl;
    out << "RegionID: " << iRegionID << "," << endl;
    out << "ScheduleID: " << iScheduleID << "," << endl;
    out << "SystemID: " << iSystemID << "," << endl;
    out << "{ ServiceRestriction: "; ::dump(out, veciServRestrict); out << endl;
    out << "IsSyncMultplx: " << bIsSyncMultplx << "}" << endl;
}

void
COtherService::dump(ostream& out) const
{
    out << "{ Frequencies: "; ::dump(out, veciFrequencies); out << endl;
    out << "RegionID: " << iRegionID << "," << endl;
    out << "ScheduleID: " << iScheduleID << "," << endl;
    out << "SystemID: " << iSystemID << "," << endl;
    out << "SameService: " << bSameService << "," << endl;
    out << "ServiceID: " << iServiceID << "}" << endl;
}

void
CAltFreqSign::dump(ostream& out) const
{
    out << "{ Regions: "; ::dump(out, vecRegions); out << endl;
    out << "Schedules: "; ::dump(out, vecSchedules); out << endl;
    out << "Multiplexes: "; ::dump(out, vecMultiplexes); out << endl;
    out << "OtherServices: "; ::dump(out, vecOtherServices); out << "}" << endl;
}

void
CServiceInformation::dump(ostream& out) const
{
	out << "{ id: " << id << endl << " label: [ ";
	string sep = "";
	for(set<string>::const_iterator i=label.begin(); i!=label.end(); i++)
	{
		out << sep << "'" << (*i) << "'" << endl;
		sep = ", ";
	}
	out << "]" << endl;
	AltFreqSign.dump(out);
	out << "}" << endl;
}

string COtherService::ServiceID() const
{
	stringstream ss;
	/*
	switch (iSystemID)
	{
	case 0:
	case 1:
		ss << "ID:" << hex << setw(6) << iServiceID;
		break;

	case 3:
	case 6:
		ss << "ECC+PI:" << hex << setw(6) << iServiceID;
		break;
	case 4:
	case 7:
		ss << "PI:" << hex << setw(4) << iServiceID;
		break;
	case 9:
		ss << "ECC+aud:" << hex << setw(6) << iServiceID;
		break;
	case 10:
		ss << "AUDIO:" << hex << setw(4) << iServiceID;
		break;
	case 11:
		ss << "DATA:" << hex << setw(8) << iServiceID;
		break;
		break;

	default:
		break;
	}
	*/
	return ss.str();
}

/* See ETSI ES 201 980 v2.1.1 Annex O */
bool
CAltFreqSched::IsActive(const time_t ltime) const
{
	int iScheduleStart;
	int iScheduleEnd;
	int iWeekDay;

	/* Empty schedule is always active */
	if (iDuration == 0)
		return true;

	/* Calculate time in UTC */
	struct tm *gmtCur = gmtime(&ltime);

	/* Check day
	   tm_wday: day of week (0 - 6; Sunday = 0)
	   I must normalize so Monday = 0   */

	if (gmtCur->tm_wday == 0)
		iWeekDay = 6;
	else
		iWeekDay = gmtCur->tm_wday - 1;

	/* iTimeWeek minutes since last Monday 00:00 in UTC */
	/* the value is in the range 0 <= iTimeWeek < 60 * 24 * 7)   */

	const int iTimeWeek =
		(iWeekDay * 24 * 60) + (gmtCur->tm_hour * 60) + gmtCur->tm_min;

	/* Day Code: this field indicates which days the frequency schedule
	 * (the following Start Time and Duration) applies to.
	 * The msb indicates Monday, the lsb Sunday. Between one and seven bits may be set to 1.
	 */
	for (int i = 0; i < 7; i++)
	{
		/* Check if day is active */
		if ((1 << (6 - i)) & iDayCode)
		{
			/* Tuesday -> 1 * 24 * 60 = 1440 */
			iScheduleStart = (i * 24 * 60) + iStartTime;
			iScheduleEnd = iScheduleStart + iDuration;

			/* the normal check (are we inside start and end?) */
			if ((iTimeWeek >= iScheduleStart) && (iTimeWeek <= iScheduleEnd))
				return true;

			/* the wrap-around check */
			const int iMinutesPerWeek = 7 * 24 * 60;

			if (iScheduleEnd > iMinutesPerWeek)
			{
				/* our duration wraps into next Monday (or even later) */
				if (iTimeWeek < (iScheduleEnd - iMinutesPerWeek))
					return true;
			}
		}
	}
	return false;
}
