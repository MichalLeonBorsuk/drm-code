/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2008
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	DrmTransmitter (almost) abstract interface
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

#ifndef _DRM_TRANSMITTERINTERFACE_H
#define _DRM_TRANSMITTERINTERFACE_H

#ifdef QT_GUI_LIB
# include <QThread>
#endif

class CParameter;
class CSettings;

class CDRMTransmitterInterface
#ifdef QT_GUI_LIB
	: public QThread
#endif
{
public:
	enum ETxOpMode { T_ENC, T_MOD, T_TX };

	CDRMTransmitterInterface() {}
	virtual ~CDRMTransmitterInterface() {}

#ifdef QT_GUI_LIB
	void					run() { Start(); }
#else
	virtual void			start() {};
	virtual int				wait(int) {};
	bool					finished(){return true;}
#endif
	virtual void			Start()=0;
	virtual void			Stop()=0;

	virtual void			LoadSettings(CSettings&)=0;
	virtual void			SaveSettings(CSettings&) const = 0;

	virtual void			SetOperatingMode(const ETxOpMode)=0;
	virtual ETxOpMode		GetOperatingMode() const = 0;

	virtual void			CalculateChannelCapacities()=0;

	virtual _REAL 			GetLevelMeter() const = 0;

	/* Source Encoder Interface */
	virtual void			AddTextMessage(const string& strText)=0;
	virtual void			ClearTextMessages()=0;
	virtual void			GetTextMessages(vector<string>&) const = 0;
	virtual void			AddPic(const string& strFileName, const string& strFormat)=0;
	virtual void			ClearPics()=0;
	virtual void			GetPics(map<string,string>&) const = 0;
	virtual bool			GetTransStat(string& strCPi, _REAL& rCPe) const = 0;
	virtual void			GetData(map<int, map<int,CDataParam> >&) const = 0;
	virtual void			PutData(const map<int, map<int,CDataParam> >&) = 0;

	virtual void			GetSoundInChoices(vector<string>&) const = 0;
	virtual void			SetSoundInInterface(int)=0;
	virtual int				GetSoundInInterface() const = 0;
	virtual void 			SetReadFromFile(const string& strNFN)=0;
	virtual string			GetReadFromFile() const = 0;
	virtual void			GetAudio(map<int,CAudioParam>&) const = 0;
	virtual void			PutAudio(const map<int,CAudioParam>&)=0;

	virtual void			GetSoundOutChoices(vector<string>&) const = 0;
	virtual void			GetCOFDMOutputs(vector<string>&) const = 0;
	virtual void			SetCOFDMOutputs(const vector<string>&)=0;
	virtual double			GetCarrierOffset() const = 0;
	virtual void			SetCarrierOffset(double) = 0;
	virtual EOutFormat		GetOutputFormat() const = 0;
	virtual void			SetOutputFormat(EOutFormat) = 0;

	virtual void			SetMDIIn(const string& s)=0;
	virtual string			GetMDIIn() const = 0;

	virtual void			SetMDIOut(const vector<string>& v)=0;
	virtual void			GetMDIOut(vector<string>&) const = 0;

	virtual void			GetChannel(CChannel&) const = 0;
	virtual void			PutChannel(const CChannel&) = 0;

	virtual void			GetMSC(CMSCParameters&) const = 0;
	virtual void			PutMSC(const CMSCParameters&) = 0;

	virtual void			GetServices(vector<CService>&, int&, int&) const = 0;
	virtual void			PutServices(const vector<CService>&, int, int) = 0;
	virtual int				NumBitsMSC() const = 0;
	virtual int				NumBitsSDCsuperFrame() const = 0;

};

#endif
