/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *  Information about services gathered from SDC, EPG and web schedules.
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

#ifndef _SERVICE_INFORMATION_H
#define _SERVICE_INFORMATION_H

#include "GlobalDefinitions.h"
#include <set>
#include <vector>


/* Alternative Frequency Signalling ************************************** */
/* Alternative frequency signalling Schedules informations class */
class CAltFreqSched : public CDumpable
{
  public:
	CAltFreqSched():CDumpable(),iDayCode(0),iStartTime(0),iDuration(0)
	{
	}
	CAltFreqSched(const CAltFreqSched& nAFS):
		CDumpable(),
		iDayCode(nAFS.iDayCode), iStartTime(nAFS.iStartTime),
		iDuration(nAFS.iDuration)
	{
	}

	CAltFreqSched& operator=(const CAltFreqSched& nAFS)
	{
		iDayCode = nAFS.iDayCode;
		iStartTime = nAFS.iStartTime;
		iDuration = nAFS.iDuration;

		return *this;
	}

	bool operator==(const CAltFreqSched& nAFS) const
	{
		if (iDayCode != nAFS.iDayCode)
			return false;
		if (iStartTime != nAFS.iStartTime)
			return false;
		if (iDuration != nAFS.iDuration)
			return false;

		return true;
	}

	bool IsActive(const time_t ltime) const;

	int iDayCode;
	int iStartTime;
	int iDuration;
	void dump(ostream&) const;
};

/* Alternative frequency signalling Regions informations class */
class CAltFreqRegion : public CDumpable
{
  public:
	CAltFreqRegion():CDumpable(),veciCIRAFZones(),
		iLatitude(0), iLongitude(0),
		iLatitudeEx(0), iLongitudeEx(0)
	{
	}
	CAltFreqRegion(const CAltFreqRegion& nAFR):
		CDumpable(),
		veciCIRAFZones(nAFR.veciCIRAFZones),
		iLatitude(nAFR.iLatitude),
		iLongitude(nAFR.iLongitude),
		iLatitudeEx(nAFR.iLatitudeEx), iLongitudeEx(nAFR.iLongitudeEx)
	{
	}

	CAltFreqRegion& operator=(const CAltFreqRegion& nAFR)
	{
		iLatitude = nAFR.iLatitude;
		iLongitude = nAFR.iLongitude;
		iLatitudeEx = nAFR.iLatitudeEx;
		iLongitudeEx = nAFR.iLongitudeEx;

		veciCIRAFZones = nAFR.veciCIRAFZones;

		return *this;
	}

	bool operator==(const CAltFreqRegion& nAFR) const
	{
		if (iLatitude != nAFR.iLatitude)
			return false;
		if (iLongitude != nAFR.iLongitude)
			return false;
		if (iLatitudeEx != nAFR.iLatitudeEx)
			return false;
		if (iLongitudeEx != nAFR.iLongitudeEx)
			return false;

		/* Vector sizes */
		if (veciCIRAFZones.size() != nAFR.veciCIRAFZones.size())
			return false;

		/* Vector contents */
		for (size_t i = 0; i < veciCIRAFZones.size(); i++)
			if (veciCIRAFZones[i] != nAFR.veciCIRAFZones[i])
				return false;

		return true;
	}

	vector<int> veciCIRAFZones;
	int iLatitude;
	int iLongitude;
	int iLatitudeEx;
	int iLongitudeEx;
	void dump(ostream&) const;
};

class CServiceDefinition : public CDumpable
{
public:
	CServiceDefinition():
	CDumpable(),veciFrequencies(), iRegionID(0), iScheduleID(0),iSystemID(0)
	{
	}

	CServiceDefinition(const CServiceDefinition& nAF):
		CDumpable(),veciFrequencies(nAF.veciFrequencies),
		iRegionID(nAF.iRegionID), iScheduleID(nAF.iScheduleID),
		iSystemID(nAF.iSystemID)
	{
	}

	CServiceDefinition& operator=(const CServiceDefinition& nAF)
	{
		veciFrequencies = nAF.veciFrequencies;
		iRegionID = nAF.iRegionID;
		iScheduleID = nAF.iScheduleID;
		iSystemID = nAF.iSystemID;
		return *this;
	}

	bool operator==(const CServiceDefinition& sd) const
	{
		size_t i;

		/* Vector sizes */
		if (veciFrequencies.size() != sd.veciFrequencies.size())
			return false;

		/* Vector contents */
		for (i = 0; i < veciFrequencies.size(); i++)
			if (veciFrequencies[i] != sd.veciFrequencies[i])
				return false;

		if (iRegionID != sd.iRegionID)
			return false;

		if (iScheduleID != sd.iScheduleID)
			return false;

		if (iSystemID != sd.iSystemID)
			return false;

		return true;
	}
	bool operator!=(const CServiceDefinition& sd) const { return !(*this==sd); }

	string Frequency(size_t) const;
	string FrequencyUnits() const;
	string System() const;

	vector<int> veciFrequencies;
	int iRegionID;
	int iScheduleID;
	int iSystemID;
	void dump(ostream&) const;
};

class CMultiplexDefinition: public CServiceDefinition
{
public:
	CMultiplexDefinition():CServiceDefinition(), veciServRestrict(4), bIsSyncMultplx(false)
	{
	}

	CMultiplexDefinition(const CMultiplexDefinition& nAF):CServiceDefinition(nAF),
		veciServRestrict(nAF.veciServRestrict),
		bIsSyncMultplx(nAF.bIsSyncMultplx)
	{
	}

	CMultiplexDefinition& operator=(const CMultiplexDefinition& nAF)
	{
		CServiceDefinition(*this) = nAF;
		veciServRestrict = nAF.veciServRestrict;
		bIsSyncMultplx = nAF.bIsSyncMultplx;
		return *this;
	}

	bool operator==(const CMultiplexDefinition& md) const
	{
		if (CServiceDefinition(*this) != md)
			return false;

		/* Vector sizes */
		if (veciServRestrict.size() != md.veciServRestrict.size())
			return false;

		/* Vector contents */
		for (size_t i = 0; i < veciServRestrict.size(); i++)
			if (veciServRestrict[i] != md.veciServRestrict[i])
				return false;

		if (bIsSyncMultplx != md.bIsSyncMultplx)
			return false;

		return true;
	}

	vector<int> veciServRestrict;
	bool bIsSyncMultplx;
	void dump(ostream&) const;
};

class COtherService: public CServiceDefinition
{
public:
	COtherService(): CServiceDefinition(), bSameService(true),
		iServiceID(SERV_ID_NOT_USED)
	{
	}

	COtherService(const COtherService& nAF):
		CServiceDefinition(nAF), bSameService(nAF.bSameService),
		iServiceID(nAF.iServiceID)
	{
	}

	COtherService& operator=(const COtherService& nAF)
	{
		CServiceDefinition(*this) = nAF;

		bSameService = nAF.bSameService;
		iServiceID = nAF.iServiceID;

		return *this;
	}

	bool operator==(const COtherService& nAF) const
	{
		if (CServiceDefinition(*this) != nAF)
			return false;

		if (bSameService != nAF.bSameService)
			return false;

		if (iServiceID != nAF.iServiceID)
			return false;

		return true;
	}

	string ServiceID() const;

	bool bSameService;
	uint32_t iServiceID;
	void dump(ostream&) const;
};

/* Alternative frequency signalling class */
class CAltFreqSign : public CDumpable
{
  public:

	CAltFreqSign():
		CDumpable(),
		vecRegions(16),vecSchedules(16),vecMultiplexes(),vecOtherServices()
	{
	}

	CAltFreqSign(const CAltFreqSign& a):
		CDumpable(),vecRegions(a.vecRegions),vecSchedules(a.vecSchedules),
		vecMultiplexes(a.vecMultiplexes),vecOtherServices(a.vecOtherServices)
	{
	}

	CAltFreqSign& operator=(const CAltFreqSign& a)
	{
		vecRegions = a.vecRegions;
		vecSchedules = a.vecSchedules;
		vecMultiplexes = a.vecMultiplexes;
		vecOtherServices = a.vecOtherServices;
		return *this;
	}
	vector < vector<CAltFreqRegion> > vecRegions; // outer vector indexed by regionID
	vector < vector<CAltFreqSched> > vecSchedules; // outer vector indexed by scheduleID
	vector < CMultiplexDefinition > vecMultiplexes;
	vector < COtherService > vecOtherServices;
	void dump(ostream&) const;
};

class CServiceInformation : public CDumpable
{
public:
	CServiceInformation():CDumpable(),id(0),label(),AltFreqSign()
	{
	}
	CServiceInformation(const CServiceInformation& a):
	CDumpable(a),
	id(a.id),label(a.label),AltFreqSign(a.AltFreqSign)
	{
	}
	CServiceInformation& operator=(const CServiceInformation& a)
	{
		id = a.id;
		label = a.label;
		AltFreqSign = a.AltFreqSign;
		return *this;
	}
	virtual ~CServiceInformation() {}

	uint32_t		id;    /* this is the primary key but we keep it inside too for completeness */
	set<string>		label; /* gathered from the SDC. Normally the label is static and is the station name, but
							  it is officially dynamic so we collect all that we see. */
	CAltFreqSign AltFreqSign;
	void dump(ostream&) const;
};
#endif
