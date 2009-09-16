/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Implements:
 *	- Hamlib interface
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

#ifndef _HAMLIB_H
#define _HAMLIB_H

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "rigclass.h"
#include <string>
#include <map>

class CParameter;
class CSettings;

enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};

class CRigSettings
{
public:
	CRigSettings():modes(),ilevels(),flevels(),functions(),
	iparameters(),fparameters(),config(),attributes()
	{}
	CRigSettings(const CRigSettings& c):
	modes(c.modes),ilevels(c.ilevels),flevels(c.flevels),functions(c.functions),
	iparameters(c.iparameters),fparameters(c.fparameters),
	config(c.config),attributes(c.attributes)
	{}
	CRigSettings& operator=(const CRigSettings& c)
	{
		modes = c.modes;
		ilevels = c.ilevels;
		flevels = c.flevels;
		functions = c.functions;
		iparameters = c.iparameters;
		fparameters = c.fparameters;
		config = c.config;
		attributes = c.attributes;
		return *this;
	}

	void	LoadSettings(CSettings& s, const string&);
	void	SaveSettings(CSettings& s, const string&) const;
	void	apply(Rig&) const;

	map<string,int> modes;
	map<string,int> ilevels;
	map<string,float> flevels;
	map<string,string> functions;
	map<string,int> iparameters;
	map<string,float> fparameters;
	map<string,string> config;
	map<string,string> attributes;
};

class CRigMap
{
public:
    map<string,map<string,rig_model_t> > rigs;
};

/* Hamlib interface --------------------------------------------------------- */

class CRig: public Rig
#ifdef QT_GUI_LIB
	, public QThread
#endif
{
public:
	CRig(/*CParameter&, */rig_model_t);
	virtual ~CRig();

	virtual void	run();

	bool			SetFrequency(const int iFreqkHz);
	void			SetFrequencyOffset(const int iOffkHz) { iOffset = iOffkHz; }
	int			GetFrequencyOffset() { return iOffset; }
	void 			SetEnableSMeter(const bool bStatus); // sets/clears wanted flag and starts/stops
	bool			GetEnableSMeter(); // returns wanted flag
	void 			StopSMeter(); // stops (clears run flag) but leaves wanted flag alone
	void			SetComPort(const string&);
	string			GetComPort() const;
	void			SetDRMMode();
	void			SetModeForDRM(rmode_t, pbwidth_t);

protected:
	bool			bSMeterWanted, bEnableSMeter;
	int iOffset;
	rmode_t mode_for_drm;
	pbwidth_t width_for_drm;
#ifdef QT_CORE_LIB
	QMutex			mutex;
#endif
	//CParameter&		Parameters;
};

class CHamlib
{
public:
	CHamlib();
	virtual ~CHamlib();

	/* backend selection */
	void			GetRigList(CRigMap&) const;

	void			LoadSettings(CSettings& s);
	void			SaveSettings(CSettings& s) const;

	const rig_caps*		GetRigCaps(rig_model_t) const;
	CRig*			GetRig(CParameter& p, rig_model_t m) { return new CRig(/*p,*/m); }

	bool			GetRigSettings(CRigSettings&,
					rig_model_t, EModulationType) const;
	void set_attribute(rig_model_t, EModulationType, const string&, const string&);

protected:

	static int		rig_enumerator(const rig_caps* caps, void* data);
	static int		rig_enumerator1(const rig_caps* caps, void* data);

	map<rig_model_t,
		map<EModulationType, CRigSettings >
		>		rigmodemap;
#ifdef QT_CORE_LIB
	QMutex			mutex;
#endif
};

#endif
