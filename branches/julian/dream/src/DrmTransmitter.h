/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DrmTransmitter.cpp
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

#if !defined(DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "Parameter.h"
#include "DrmEncoder.h"
#include "DrmModulator.h"
#include "DrmTransmitterInterface.h"
#include "MDI/MDIRSCI.h"
#include "MDI/MDIDecode.h"

/* Classes ********************************************************************/
class CSettings;

class CDRMTransmitter: public CDRMTransmitterInterface
{
public:
							CDRMTransmitter();
	virtual 				~CDRMTransmitter() {}
	void					LoadSettings(CSettings&); // can write to settings to set defaults
	void					SaveSettings(CSettings&) const;

	void					Start();
	void					Stop();

	void					SetOperatingMode(const ETxOpMode);
	ETxOpMode				GetOperatingMode() const;

	void					CalculateChannelCapacities();

	_REAL 					GetLevelMeter() const;

	/* Source Encoder Interface */
	void					AddTextMessage(const string& strText);
	void					ClearTextMessages();
	void					GetTextMessages(vector<string>&) const;

	void					AddPic(const string& strFileName, const string& strFormat);
	void					ClearPics();
	void					GetPics(map<string,string>&) const;
	bool				    GetTransStat(string& strCPi, _REAL& rCPe) const;
	void					GetData(map<int, map<int,CDataParam> >&) const;
	void					PutData(const map<int, map<int,CDataParam> >&);

	void					GetSoundInChoices(vector<string>&) const;
	void					SetSoundInInterface(int);
	int						GetSoundInInterface() const;
	void 					SetReadFromFile(const string& strNFN);
	string					GetReadFromFile() const;
	void					GetAudio(map<int,CAudioParam>&) const;
	void					PutAudio(const map<int,CAudioParam>&);

	void					GetSoundOutChoices(vector<string>&) const;
	void					SetCOFDMOutputs(const vector<string>& o);
	void					GetCOFDMOutputs(vector<string>& o) const;
	double					GetCarrierOffset() const { return Parameters.rCarOffset;}
	void					SetCarrierOffset(double r) {Parameters.rCarOffset=r;}
	EOutFormat				GetOutputFormat() const {return Parameters.eOutputFormat;}
	void					SetOutputFormat(EOutFormat e) {Parameters.eOutputFormat=e;}

	void					SetMDIIn(const string& s) { strMDIinAddr = s; }
	string					GetMDIIn() const { return strMDIinAddr; }

	void					SetMDIOut(const vector<string>& v) { MDIoutAddr = v; }
	void					GetMDIOut(vector<string>& v) const { v = MDIoutAddr; }

	void					GetChannel(CChannel& c) const { c = Parameters.Channel;}
	void					PutChannel(const CChannel& c);

	void					GetMSC(CMSCParameters& msc) const {msc=Parameters.MSCParameters;}
	void					PutMSC(const CMSCParameters& msc) {Parameters.MSCParameters=msc;}

	void					GetServices(vector<CService>&, int&, int&) const;
	void					PutServices(const vector<CService>&, int, int);
	int						NumBitsMSC() const { return Parameters.iNumDecodedBitsMSC; }
	int						NumBitsSDCsuperFrame() const { return Parameters.iNumSDCBitsPerSuperFrame; }

protected:

	CParameter				Parameters;
	ETxOpMode				eOpMode;
	bool                    bRunning;

	string					strMDIinAddr;
	vector<string>			MDIoutAddr;

	CDRMEncoder*			pEncoder;
	CDRMModulator*			pModulator;
	CMDIIn*					pMDIIn;
	CDecodeMDI*				pDecodeMDI;
	CMDIOut*				pMDIOut;
};


#endif // !defined(DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
