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

#include <string>
#include <map>
#ifdef QT_CORE_LIB
# include <QThread>
# include <QMutex>
#endif

#ifdef HAVE_LIBHAMLIB
# include <hamlib/rig.h>
#else
 typedef int rig_model_t;
 typedef int rmode_t;
 typedef int pbwidth_t;
 class Rig {};
#endif

enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};

class CParameter;
class CSettings;

class CRigMap
{
public:
    std::map<std::string,std::map<std::string,rig_model_t> > rigs;
};

class CRig
#ifdef QT_CORE_LIB
	: public QThread
#endif
{
public:
	CRig(rig_model_t, CParameter* =NULL);
	virtual ~CRig();

	virtual void		run();

	bool			SetFrequency(const int iFreqkHz);
	void			SetFrequencyOffset(const int iOffkHz) { iOffset = iOffkHz; }
	int				GetFrequencyOffset() { return iOffset; }
	void 			SetEnableSMeter(const bool bStatus); // sets/clears wanted flag and starts/stops
	bool			GetEnableSMeter(); // returns wanted flag
	void 			StopSMeter(); // stops (clears run flag) but leaves wanted flag alone
	void			SetDRMMode();
	void			SetModeForDRM(rmode_t, pbwidth_t);
	void			LoadSettings(const std::string&, const CSettings&);
	void			SaveSettings(const std::string&, CSettings&) const;

	const char *		model_name;
	rig_model_t		rig_model;
	rig_status_e		status;
	rig_port_e		port_type() const { return (the_rig)?the_rig->caps->port_type:RIG_PORT_NONE; }
protected:
	bool			bSMeterWanted, bEnableSMeter;
	int			iOffset;
	rmode_t			mode_for_drm;
	pbwidth_t		width_for_drm;
	RIG*			the_rig;
	CParameter*		pParameters;
#ifdef QT_CORE_LIB
	QMutex			mutex;
#endif
};

#endif
