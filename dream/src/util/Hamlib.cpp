/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Ollie Haffenden
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

#include "Hamlib.h"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include "../Parameter.h"

CRig::CRig(rig_model_t m, CParameter* p):Rig(m),
bSMeterWanted(false), bEnableSMeter(false),iOffset(0),
mode_for_drm(RIG_MODE_AM), width_for_drm(0), pParameters(p)
#ifdef QT_CORE_LIB
  ,mutex()
#endif
{
}

CRig::~CRig()
{
}

void
CRig::SetComPort(const string & port)
{
    close();
    setConf("rig_pathname", port.c_str());
}

string CRig::GetComPort() const
{
    char r[1000];
    const_cast<CRig*>(this)->getConf("rig_pathname", r);
    return r;
}


bool
CRig::SetFrequency(const int iFreqkHz)
{
    setFreq(iFreqkHz+iOffset);
    return true; // TODO
}

void
CRig::SetDRMMode()
{
    if(width_for_drm==0)
	setMode(mode_for_drm);
    else
	setMode(mode_for_drm, width_for_drm);
}

void
CRig::SetModeForDRM(rmode_t m, pbwidth_t w)
{
    mode_for_drm = m;
    width_for_drm = w;
}

void CRig::SetEnableSMeter(const bool bStatus)
{
    bSMeterWanted = bStatus;
    if (bStatus)
    {
#ifdef QT_CORE_LIB
	if (bEnableSMeter==false)
	{
	    start(); // don't do this except in GUI thread - see CReceiverSettings
	}
#endif
    }
    else
    {
	StopSMeter();
    }
}

bool CRig::GetEnableSMeter()
{
    return bEnableSMeter;
}

void CRig::StopSMeter()
{
    if(pParameters)
    {
	pParameters->Lock();
	pParameters->Measurements.SigStrstat.invalidate();
	pParameters->Unlock();
    }
    bEnableSMeter = false;
}

void CRig::run()
{
    const _REAL S9_DBuV = 34.0; // S9 in dBuV for converting HamLib S-meter readings
    bEnableSMeter = true;
    while (bEnableSMeter)
    {
	int val;
	mutex.lock();
	getLevel(RIG_LEVEL_STRENGTH, val);
	mutex.unlock();
	if(pParameters)
	{
	    pParameters->Lock();
	    pParameters->Measurements.SigStrstat.addSample(_REAL(val) + S9_DBuV + pParameters->rSigStrengthCorrection);
	    pParameters->Unlock();
	}
#ifdef QT_CORE_LIB
	    msleep(400);
#endif
    }
}
