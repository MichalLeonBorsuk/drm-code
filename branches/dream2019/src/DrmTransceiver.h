/******************************************************************************\
 *
 * Copyright (c) 2012
 *
 * Author(s):
 *	David Flamand
 *
 * Description:
 *	DRM receiver-transmitter base class
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

#ifndef DRM_TRANSCEIVER_H
#define DRM_TRANSCEIVER_H

#include "Parameter.h"
#include "util/Settings.h"
#include "sound/soundinterface.h"

class CDRMTransceiver
{
public:
    CDRMTransceiver(CSettings* pSettings, bool bTransmitter=false);
    virtual ~CDRMTransceiver();

    virtual void LoadSettings() = 0;
    virtual void SaveSettings() = 0;
    virtual void Start() = 0;
    virtual void SetInputDevice(const QString&) = 0;
    virtual void SetOutputDevice(const QString&) = 0;
    virtual QString GetInputDevice() = 0;
    virtual QString GetOutputDevice() = 0;
    virtual void EnumerateInputs(std::vector<std::string>& names, std::vector<std::string>& descriptions)=0;
    virtual void EnumerateOutputs(std::vector<std::string>& names, std::vector<std::string>& descriptions)=0;

    virtual void Restart()
    {
        if (Parameters.eRunState == CParameter::RUNNING)
            Parameters.eRunState = CParameter::RESTART;
    }
    virtual void Stop()
    {
        Parameters.eRunState = CParameter::STOP_REQUESTED;
    }
    virtual CSettings*				GetSettings() {
        return pSettings;
    }
    virtual void					SetSettings(CSettings* pNewSettings) {
        pSettings = pNewSettings;
    }
    virtual CParameter*				GetParameters() {
        return &Parameters;
    }
	virtual _BOOLEAN				IsReceiver() const {
		return !bTransmitter;
	}
	virtual _BOOLEAN				IsTransmitter() const {
		return bTransmitter;
	}

protected:
	CSettings*				pSettings;
    CParameter				Parameters;

private:
	const _BOOLEAN bTransmitter;
};

#endif
