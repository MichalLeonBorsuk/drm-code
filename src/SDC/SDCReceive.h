/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *	See SDC.cpp
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- AMSS data entity groups (no AFS index), added eSDCType, data type 11
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

#ifndef _SDCRECEIVE_H
#define _SDCRECEIVE_H

#include "SDC.h"

enum ESDCType {SDC_DRM, SDC_AMSS};

class CSDCReceive
{
public:
	enum ERetStatus {SR_OK, SR_BAD_CRC, SR_BAD_DATA};
	CSDCReceive();
	virtual ~CSDCReceive() {}

	ERetStatus SDCParam(CVector<_BINARY>* pbiData, CParameter& Parameter);
	void SetSDCType(ESDCType sdcType) { eSDCType = sdcType; }

	bool DataEntityType0(CVector<_BINARY>* pbiData, const int iLengthOfBody,
                             CParameter& Parameter, const bool bVersion);
protected:
	bool DataEntityType1(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter);
// ...
	bool DataEntityType3(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool bVersion);
	bool DataEntityType4(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool bVersion);
	bool DataEntityType5(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool bVersion);
	bool DataEntityType6(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool bVersion);
	bool DataEntityType7(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool bVersion);
	bool DataEntityType8(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter);
	bool DataEntityType9(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool bVersion);
	bool DataEntityType10(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool bVersion);
	bool DataEntityType11(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool bVersion);
	bool DataEntityType12(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter);
	bool DataEntityType13(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool);
	bool DataEntityType14(CVector<_BINARY>* pbiData, const int iLengthOfBody,
							 CParameter& Parameter, const bool);

	CCRC		CRCObject;
	ESDCType	eSDCType;
	bool bType3ListVersion;
	bool bType4ListVersion;
	bool bType6ListVersion;
	bool bType7ListVersion;
	bool bType11ListVersion;
	bool bType13ListVersion;
	bool bChannelReconfigurationPending;
	bool bServiceReconfigurationPending;
};


#endif // !defined(SDC_H__3B0BA660_CA63SDBOJKEWROBNER89NE877A0D312__INCLUDED_)
