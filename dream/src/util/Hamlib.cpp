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

#include <cstdlib>
#include "Hamlib.h"

#include "../Parameter.h"
#include "Settings.h"
#include <sstream>
#include <iostream>
#include "../Parameter.h"

/* Implementation *************************************************************/
/*
	This code is based on patches and example code from Tomi Manninen and
	Stephane Fillod (developer of hamlib)
*/

void
CRigSettings::LoadSettings(CSettings& s, const string& secpref)
{
#if 0
	INISection sec;
	s.Get(secpref+"config", sec);
	INISection::iterator i;
	for (i=sec.begin(); i!=sec.end(); i++)
		config[i->first] = i->second;
	sec.clear();
	s.Get(section+"modes", sec);
	for (i=sec.begin(); i!=sec.end(); i++)
	    modes[i->first] = i->second;
	sec.clear();
	s.Get(section+"int-levels", sec);
	for (i=sec.begin(); i!=sec.end(); i++)
	    ilevel[i->first] = atoi(i->second);
	sec.clear();
	s.Get(section+"functions", sec);
	for (i=sec.begin(); i!=sec.end(); i++)
	    set_function(m, i->first, i->second);
	sec.clear();
	s.Get(section+"parameters", sec);
	for (i=sec.begin(); i!=sec.end(); i++)
	    set_parameter(m, i->first, i->second);
	s.Get(section+"attributes", sec);
	for (i=sec.begin(); i!=sec.end(); i++)
	    set_attribute(m, i->first, i->second);
#endif
}

void
CRigSettings::SaveSettings(CSettings& s, const string& section) const
{
	map<string,int>::const_iterator i;
	map<string,string>::const_iterator st;
	map<string,float>::const_iterator f;

	for (i=modes.begin(); i!=modes.end(); i++)
	    s.Put(section+"modes", i->first, i->second);

	for (i=ilevels.begin(); i!=ilevels.end(); i++)
	{
	    s.Put(section+"int-levels", i->first, i->second);
	}
	for (f=flevels.begin(); f!=flevels.end(); f++)
	{
	    s.Put(section+"float-levels", f->first, f->second);
	}
	for (i=iparameters.begin(); i!=iparameters.end(); i++)
	{
	    s.Put(section+"int-parameters", i->first, i->second);
	}
	for (f=fparameters.begin(); f!=fparameters.end(); f++)
	{
	    s.Put(section+"float-parameters", f->first, f->second);
	}

	for (st=functions.begin(); st!=functions.end(); st++)
	    s.Put(section+"functions", st->first, st->second);


	for (st=attributes.begin(); st!=attributes.end(); st++)
	    s.Put(section+"attributes", st->first, st->second);
}

void CRigSettings::apply(Rig& rig) const
{
    // modes
    for (map < string, int >::const_iterator i = modes.begin(); i != modes.end(); i++)
    {
	rmode_t mode = rig_parse_mode(i->first.c_str());
	if (mode != RIG_MODE_NONE)
	{
	    rig.setMode(mode, i->second);
	}
    }
    // levels
    for (map <string,int>::const_iterator i = ilevels.begin(); i != ilevels.end(); i++)
    {
	setting_t setting = rig_parse_level(i->first.c_str());
	if (setting != RIG_LEVEL_NONE)
	{
	    rig.setLevel(setting, i->second);
	}
    }
    for (map <string,float>::const_iterator i = flevels.begin(); i != flevels.end(); i++)
    {
	setting_t setting = rig_parse_level(i->first.c_str());
	if (setting != RIG_LEVEL_NONE)
	{
	    rig.setLevel(setting, i->second);
	}
    }
    // funcs
    for (map < string, string >::const_iterator i = functions.begin();
	    i != functions.end(); i++)
    {
	setting_t setting = rig_parse_func(i->first.c_str());
	if (setting != RIG_FUNC_NONE)
	{
	    rig.setFunc(setting, atoi(i->second.c_str()));
	}
    }
    // params
    for (map<string,int>::const_iterator i = iparameters.begin();
	    i != iparameters.end(); i++)
    {
	setting_t setting = rig_parse_parm(i->first.c_str());
	if (setting != RIG_PARM_NONE)
	{
	    rig.setParm(setting, i->second);
	}
    }
    for (map<string,float>::const_iterator i = fparameters.begin();
	    i != fparameters.end(); i++)
    {
	setting_t setting = rig_parse_parm(i->first.c_str());
	if (setting != RIG_PARM_NONE)
	{
	    rig.setParm(setting, i->second);
	}
    }
}

CRig::CRig(/*CParameter& p, */rig_model_t m):Rig(m),
bSMeterWanted(false), bEnableSMeter(false),iOffset(0),
mode_for_drm(RIG_MODE_AM), width_for_drm(0)
#ifdef QT_CORE_LIB
	,mutex()
#endif
	//,Parameters(p)
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
#ifdef QT_GUI_LIB
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
    //Parameters.Lock();
    //Parameters.Measurements.SigStrstat.invalidate();
    //Parameters.Unlock();
    bEnableSMeter = false;
}

void CRig::run()
{
    bEnableSMeter = true;
    while (bEnableSMeter)
    {
	int val;
	mutex.lock();
	getLevel(RIG_LEVEL_STRENGTH, val);
	mutex.unlock();
	    //Parameters.Lock();
	    // Apply any correction
	    const _REAL S9_DBuV = 34.0; // S9 in dBuV for converting HamLib S-meter readings
	    //Parameters.Measurements.SigStrstat.addSample(_REAL(val) + S9_DBuV + Parameters.rSigStrengthCorrection);
	    //Parameters.Unlock();
#ifdef QT_GUI_LIB
	    msleep(400);
#endif
    }
}

CHamlib::CHamlib():
#ifdef QT_CORE_LIB
	mutex()
#endif
{
    /* Load all available front-end remotes in hamlib library */
    rig_load_all_backends();
}

CHamlib::~CHamlib()
{
}

int
CHamlib::rig_enumerator(const rig_caps *caps, void *data)
{
    CRigMap& map = *(CRigMap *)data;
    map.rigs[caps->mfg_name][caps->model_name] = caps->rig_model;
    return 1;					/* !=0, we want them all! */
}

int
CHamlib::rig_enumerator1(const rig_caps *caps, void *data)
{
    rig_caps* c = reinterpret_cast<rig_caps*>(data);
    if(c->rig_model == caps->rig_model)
    {
    	*c = *caps;
    	return 0;
    }
    return 1;					/* !=0, we want them all! */
}

void
CHamlib::GetRigList(CRigMap& rigs) const
{

    /* Get all models which are available.
     * A call-back function is called to return the different rigs */
    rig_list_foreach(rig_enumerator, &rigs);
}

const rig_caps*
CHamlib::GetRigCaps(rig_model_t m) const
{
    rig_caps* caps = new rig_caps;
    caps->rig_model = m;
    rig_list_foreach(rig_enumerator1, caps);
    return caps;
}

void
CHamlib::LoadSettings(CSettings & s)
{

#ifdef RIG_MODEL_G303
    /* Winradio G303 */
    rigmodemap[RIG_MODEL_G303][DRM].ilevels["ATT"]=0;
    rigmodemap[RIG_MODEL_G303][DRM].ilevels["AGC"]=3;
#endif

#ifdef RIG_MODEL_G313
    /* Winradio G313 */
    rigmodemap[RIG_MODEL_G313][DRM].ilevels["ATT"]=0;
    rigmodemap[RIG_MODEL_G313][DRM].ilevels["AGC"]=3;
# ifdef __linux
    rigmodemap[RIG_MODEL_G313][DRM].config["if_path"] = "/dreamg3xxif";
    rigmodemap[RIG_MODEL_G313][AM].attributes["audiotype"] = "shm";
    rigmodemap[RIG_MODEL_G313][USB].attributes["audiotype"] = "shm";
    rigmodemap[RIG_MODEL_G313][LSB].attributes["audiotype"] = "shm";
    rigmodemap[RIG_MODEL_G313][NBFM].attributes["audiotype"] = "shm";
    rigmodemap[RIG_MODEL_G313][WBFM].attributes["audiotype"] = "shm";
# endif
#endif

#ifdef RIG_MODEL_G315
    /* Winradio G315 */
    rigmodemap[RIG_MODEL_G315][DRM].ilevels["ATT"]=0;
    rigmodemap[RIG_MODEL_G315][DRM].ilevels["AGC"]=3;
# ifdef __linux
    rigmodemap[RIG_MODEL_G315][DRM].config["if_path"] = "/dreamg3xxif";
    rigmodemap[RIG_MODEL_G315][AM].attributes["audiotype"] = "shm";
    rigmodemap[RIG_MODEL_G315][USB].attributes["audiotype"] = "shm";
    rigmodemap[RIG_MODEL_G315][LSB].attributes["audiotype"] = "shm";
    rigmodemap[RIG_MODEL_G315][NBFM].attributes["audiotype"] = "shm";
    rigmodemap[RIG_MODEL_G315][WBFM].attributes["audiotype"] = "shm";
# endif
    rigmodemap[RIG_MODEL_G315][WBFM].attributes["onboarddemod"] = "must";
#endif

#ifdef RIG_MODEL_AR7030
    /* AOR 7030 */
    rigmodemap[RIG_MODEL_AR7030][DRM].ilevels["AGC"]=5;
    rigmodemap[-RIG_MODEL_AR7030][DRM].ilevels["AGC"]=5;

    rigmodemap[-RIG_MODEL_AR7030][DRM].modes["AM"]=6;
    rigmodemap[RIG_MODEL_AR7030][DRM].modes["AM"]=3;
#endif

#ifdef RIG_MODEL_NRD535
    /* JRC NRD 535 */
    /* AGC=slow */
    rigmodemap[RIG_MODEL_NRD535][DRM].ilevels["AGC"]=3;
    rigmodemap[-RIG_MODEL_NRD535][DRM].ilevels["AGC"]=3;

    rigmodemap[RIG_MODEL_NRD535][DRM].modes["CW"]=12000;
    rigmodemap[RIG_MODEL_NRD535][DRM].ilevels["CWPITCH"]=-5000;
    rigmodemap[RIG_MODEL_NRD535][DRM].ilevels["IF"]=-2000;
    rigmodemap[RIG_MODEL_NRD535][DRM].config["offset"]="3";
#endif

#ifdef RIG_MODEL_RX320
    /* TenTec RX320D */
    rigmodemap[RIG_MODEL_RX320][DRM].ilevels["AGC"]=3;
    rigmodemap[-RIG_MODEL_RX320][DRM].ilevels["AGC"]=3;

    rigmodemap[RIG_MODEL_RX320][DRM].modes["AM"]=6000;
    rigmodemap[RIG_MODEL_RX320][DRM].ilevels["AF"]=1;

#endif

#ifdef RIG_MODEL_RX340
    /* TenTec RX340D */
    rigmodemap[RIG_MODEL_RX340][DRM].ilevels["AGC"]=3;
    rigmodemap[-RIG_MODEL_RX340][DRM].ilevels["AGC"]=3;

    rigmodemap[RIG_MODEL_RX340][DRM].modes["USB"]=16000;
    rigmodemap[RIG_MODEL_RX340][DRM].ilevels["IF"]=2000;
    rigmodemap[RIG_MODEL_RX340][DRM].ilevels["AF"]=1;
    rigmodemap[RIG_MODEL_RX340][DRM].config["offset"]=-12;

#endif

#ifdef RIG_MODEL_ELEKTOR507
    /* Elektor USB SDR 5/07 */
    rigmodemap[RIG_MODEL_ELEKTOR507][DRM].config["offset"]="-12";
#endif

    EModulationType eRigMode = EModulationType(s.Get("Hamlib", "mode", 0));

    for (map<rig_model_t,
		map<EModulationType,CRigSettings>
		>::iterator
	    r = rigmodemap.begin(); r != rigmodemap.end(); r++)
    {
	stringstream section;
	rig_model_t model = r->first;
	section << "Hamlib" << ((model<0)?"-Modified-":"-") << abs(model) << "-";
	//r->second.LoadSettings(s, section.str());
    }

    /* Initial mode/band */
    eRigMode = EModulationType(s.Get("Hamlib", "mode", int(DRM)));

    //rig_model_t model = WantedModelID[eRigMode];

    /* extract config from -C command line arg */
    string command_line_config = s.Get("command", "hamlib-config", string(""));
    if (command_line_config!="")
    {
	istringstream params(command_line_config);
	string name, value;
	while (getline(params, name, '='))
	{
	    getline(params, value, ',');
	    //CapsHamlibModels[model].set_config(name, value);
	    // TODO support levels, params, etc.
	}
    }
}

void
CHamlib::SaveSettings(CSettings & s) const
{
#if 0
    //s.Put("Hamlib", "smeter", bSMeterWanted);

    for (map<rig_model_t,CRigCaps>::const_iterator
	    r = CapsHamlibModels.begin(); r != CapsHamlibModels.end(); r++)
    {
	if (r->first != 0)
	{
	    stringstream section;
	    rig_model_t model = r->first;
	    section << "Hamlib" << ((model<0)?"-Modified-":"-") << abs(model) << "-";
	    r->second.SaveSettings(s, section.str());
	}
    }
#endif
}

bool CHamlib::GetRigSettings(CRigSettings& s,rig_model_t m, EModulationType e) const
{
	map<rig_model_t,
		map<EModulationType, CRigSettings >
		>::const_iterator i = rigmodemap.find(m);
	if(i==rigmodemap.end())
		return false;

	map<EModulationType, CRigSettings >::const_iterator j = i->second.find(e);
	if(j==i->second.end())
		return false;
	s = j->second;
	return true;
}

void CHamlib::set_attribute(rig_model_t m, EModulationType e, const string& a, const string& v)
{
	rigmodemap[m][e].attributes[a]=v;
}
