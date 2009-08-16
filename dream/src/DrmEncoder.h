/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	See DrmEncoder.cpp
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

#ifndef _DRMENCODER_H
#define _DRMENCODER_H

#include "GlobalDefinitions.h"
#include "DataIO.h"
#include "util/Buffer.h"
#include "sourcedecoders/AudioSourceEncoder.h"
#include "datadecoding/DataEncoder.h"
#include "util/Utilities.h"
#include <map>
#include "util/TransmitterModul_impl.h"

/* Classes ********************************************************************/
class CSettings;
class CParameter;
class CReadData;
class SoundInInterface;

class CDRMEncoder
{
public:
							CDRMEncoder();
	virtual 				~CDRMEncoder() {}
	void					LoadSettings(CSettings&, CParameter&);
	void					SaveSettings(CSettings&, const CParameter&) const;

	void					Init(CParameter&,
								CBuffer<_BINARY>& FACBuf, CBuffer<_BINARY>& SDCBuf,
								vector<CSingleBuffer<_BINARY> >& MSCBuf);
	void					ReadData(CParameter& Parameter,
								CBuffer<_BINARY>& FACBuf, CBuffer<_BINARY>& SDCBuf,
								vector<CSingleBuffer<_BINARY> >& MSCBuf);
	void					Cleanup(CParameter&);

	_REAL					GetLevelMeter() const {return SignalLevelMeter.Level();}

	/* Source Encoder Interface */
	void					AddTextMessage(const string& strText);
	void					ClearTextMessages();
	void					GetTextMessages(vector<string>&) const;

	void					GetSoundInChoices(vector<string>&) const;
	void					SetSoundInInterface(int);
	int						GetSoundInInterface() const { return iSoundInDev; }
	void 					SetReadFromFile(const string& strNFN);
	string					GetReadFromFile() const { return strInputFileName; }

	void					AddPic(const string& strFileName, const string& strFormat);
	void					ClearPics();
	void					GetPics(map<string,string>&) const;
	bool					GetTransStat(string& strCPi, _REAL& rCPe) const;
protected:

	/* Buffers */
	CSingleBuffer<_SAMPLE>	DataBuf;

	/* Modules */
	CSoundInInterface*		pSoundInInterface;
	CAudioSourceEncoder		AudioSourceEncoder;
	CDataEncoder			DataEncoder;
	CGenerateFACData		GenerateFACData;
	CGenerateSDCData		GenerateSDCData;
	CReadData*				pReadData;
	string					strInputFileName;
	vector<string>			vecstrTexts;
	vector<string>			vecstrPics;
	vector<string>			vecstrPicTypes;
	int						iSoundInDev;
	bool				bUseUEP;

	CSignalLevelMeter		SignalLevelMeter;
};


#endif
