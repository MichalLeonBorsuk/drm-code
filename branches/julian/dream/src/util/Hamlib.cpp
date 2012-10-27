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
#include "Settings.h"

CRig::CRig(rig_model_t m, CParameter* p):
model_name(""), rig_model(m), status(RIG_STATUS_ALPHA),
bSMeterWanted(false), bEnableSMeter(false),iOffset(0),
mode_for_drm(RIG_MODE_AM), width_for_drm(0), the_rig(NULL), pParameters(p)
#ifdef QT_CORE_LIB
  ,mutex()
#endif
{
    the_rig = rig_init(rig_model);
    model_name = the_rig->caps->model_name;
    status = the_rig->caps->status;
    cerr << "rig " << m << " " << model_name << " " << rig_get_info(the_rig) << endl;
}

CRig::~CRig()
{
	rig_close(the_rig);
	rig_cleanup(the_rig);
}

bool
CRig::SetFrequency(const int iFreqkHz)
{
    return RIG_OK == rig_set_freq(the_rig, RIG_VFO_CURR, iFreqkHz+iOffset);
}

void
CRig::SetDRMMode()
{
    rig_set_mode(the_rig, RIG_VFO_CURR, mode_for_drm, width_for_drm);
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
	value_t val;
	mutex.lock();
	rig_get_level(the_rig, RIG_VFO_CURR, RIG_LEVEL_STRENGTH, &val);
	mutex.unlock();
	if(pParameters)
	{
	    pParameters->Lock();
	    pParameters->Measurements.SigStrstat.addSample(_REAL(val.i) + S9_DBuV + pParameters->rSigStrengthCorrection);
	    pParameters->Unlock();
	}
#ifdef QT_CORE_LIB
	    msleep(400);
#endif
    }
}

void CRig::LoadSettings(const std::string& sectitle, const CSettings& settings)
{
    rmode_t mode_for_drm = rmode_t(settings.Get(sectitle, "mode_for_drm", int(RIG_MODE_NONE)));
    pbwidth_t width_for_drm = pbwidth_t(settings.Get(sectitle, "width", int(0)));
    if(mode_for_drm!=RIG_MODE_NONE)
    {
	SetModeForDRM(mode_for_drm, width_for_drm);
    }
    int offset = settings.Get(sectitle, "offset", int(0));
    if(offset != 0)
    {
	SetFrequencyOffset(offset);
    }
    INISection sec;
    settings.Get(sectitle+"-conf", sec);
    for(INISection::const_iterator j=sec.begin(); j!=sec.end(); j++)
    {
	rig_set_conf(the_rig, rig_token_lookup(the_rig, j->first.c_str()), j->second.c_str());
    }
    settings.Get(sectitle+"-levels", sec);
    for(INISection::const_iterator j=sec.begin(); j!=sec.end(); j++)
    {
    	value_t val;
    	val.i = atoi(j->second.c_str());
	rig_set_level(the_rig, RIG_VFO_CURR, rig_token_lookup(the_rig, j->first.c_str()), val);
    }
}

void CRig::SaveSettings(const std::string& sec, CSettings& settings) const
{
    RIG* rig = const_cast<RIG*>(the_rig);
    vector<string> keys;
    keys.push_back("rig_pathname"); // TODO
    for(size_t j=0; j<keys.size(); j++)
    {
	try {
	    char val[200];
	    rig_get_conf(rig, rig_token_lookup(the_rig, keys[j].c_str()), val);
	    if(strlen(val)>0)
	    {
		settings.Put(sec+"-conf", keys[j], string(val));
	    }
	} catch(...)
	{
	    stringstream err;
	    cerr << "error for rig " << " config " << j << endl;
	}
    }
    keys.clear();
    keys.push_back("ATT");
    keys.push_back("AGC");
    keys.push_back("IF");
    keys.push_back("CWPITCH");
    for(size_t j=0; j<keys.size(); j++)
    {
	value_t val;
	rig_get_level(rig, RIG_VFO_CURR, rig_parse_mode(keys[j].c_str()), &val);
	settings.Put(sec+"-levels", keys[j], val.i);
    }
	pbwidth_t width;
	rmode_t mode = RIG_MODE_NONE;
	rig_get_mode(rig, RIG_VFO_CURR, &mode, &width);
	if(mode!=RIG_MODE_NONE)
	{
	    settings.Put(sec, "mode_for_drm", int(mode));
	    settings.Put(sec, "width", int(width));
	}
    if(iOffset!=0)
	settings.Put(sec, "offset", iOffset);
}
