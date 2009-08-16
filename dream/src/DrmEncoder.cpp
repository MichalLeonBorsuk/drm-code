/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	DRM-Encoder
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

#include <iostream>
#include <sstream>
#include "util/Settings.h"
#include "DrmEncoder.h"
#include "datadecoding/MOTSlideShow.h"

#include "sound.h"
#include "sound/soundfile.h"

/* Implementation *************************************************************/
CDRMEncoder::CDRMEncoder():
	DataBuf(), pSoundInInterface(NULL),
	AudioSourceEncoder(), DataEncoder(), GenerateFACData(), GenerateSDCData(),
	pReadData(NULL), strInputFileName(),
	vecstrTexts(), vecstrPics(), vecstrPicTypes(),
	iSoundInDev(-1), bUseUEP(false)
{
    cerr << "Encoder constructed" << endl;
}

void
CDRMEncoder::GetSoundInChoices(vector<string>& v) const
{
    CSoundIn s;
	s.Enumerate(v);
}

void
CDRMEncoder::SetSoundInInterface(int i)
{
	iSoundInDev = i;
}

void
CDRMEncoder::AddTextMessage(const string& strText)
{
	vecstrTexts.push_back(strText);
}

void
CDRMEncoder::ClearTextMessages()
{
	vecstrTexts.clear();
}

void
CDRMEncoder::GetTextMessages(vector<string>& v) const
{
    v = vecstrTexts;
}

void
CDRMEncoder::AddPic(const string& strFileName, const string& strFormat)
{
	vecstrPics.push_back(strFileName);
	vecstrPicTypes.push_back(strFormat);
}

void
CDRMEncoder::ClearPics()
{
	vecstrPics.clear();
	vecstrPicTypes.clear();
}

void
CDRMEncoder::GetPics(map<string,string>& m) const
{
    m.clear();
	for(size_t i=0; i<vecstrPics.size(); i++)
		m[vecstrPics[i]] = vecstrPicTypes[i];
}

bool
CDRMEncoder::GetTransStat(string& strCPi, _REAL& rCPe) const
{
	return DataEncoder.GetTransStat(strCPi, rCPe);
}

void
CDRMEncoder::SetReadFromFile(const string & strNFN)
{
	strInputFileName = strNFN;
}

void
CDRMEncoder::Init(CParameter& Parameters,
			CBuffer<_BINARY>& FACBuf, CBuffer<_BINARY>& SDCBuf,
			vector<CSingleBuffer<_BINARY> >& MSCBuf)
{
	GenerateFACData.Init(Parameters, FACBuf);
	GenerateSDCData.Init(Parameters, SDCBuf);

	if(strInputFileName=="")
	{
		pSoundInInterface = new CSoundIn;
		pSoundInInterface->SetDev(iSoundInDev);
	}
	else
	{
		CSoundFileIn *pf = new CSoundFileIn;
		pf->SetFileName(strInputFileName);
		pSoundInInterface = pf;
	}
	pReadData = new CReadData(pSoundInInterface);
	pReadData->Init(Parameters, DataBuf);

	MSCBuf.clear(); MSCBuf.resize(1);

	AudioSourceEncoder.ClearTextMessage();
	size_t i;
	for(i=0; i<vecstrTexts.size(); i++)
		AudioSourceEncoder.SetTextMessage(vecstrTexts[i]);
	AudioSourceEncoder.Init(Parameters, MSCBuf[0]);

	DataEncoder.ClearPics();
	for(i=0; i<vecstrPics.size(); i++)
		DataEncoder.AddPic(vecstrPics[i], vecstrPicTypes[i]);
	DataEncoder.Init(Parameters);
	SignalLevelMeter.Init(0);
}

// TODO Mutex on SignalLevelMeter and Transmission Status (Slide show pic)
void
CDRMEncoder::ReadData(CParameter& Parameters,
			CBuffer<_BINARY>& FACBuf, CBuffer<_BINARY>& SDCBuf,
			vector<CSingleBuffer<_BINARY> >& MSCBuf)
{
	/* MSC *********************************************************** */
	/* Read the source signal */
	pReadData->ReadData(Parameters, DataBuf);
	SignalLevelMeter.Update(*DataBuf.QueryWriteBuffer());

	/* Audio source encoder */
	AudioSourceEncoder.ProcessData(Parameters, DataBuf, MSCBuf[0]);

	/* FAC *********************************************************** */
	GenerateFACData.ReadData(Parameters, FACBuf);

	/* SDC *********************************************************** */
	GenerateSDCData.ReadData(Parameters, SDCBuf);

}

void
CDRMEncoder::Cleanup(CParameter&)
{
	delete pReadData;
	if(pSoundInInterface)
		delete pSoundInInterface;
}

void
CDRMEncoder::LoadSettings(CSettings& s, CParameter& Parameters)
{
	/* Either one audio or one data service can be chosen */

	Parameters.Lock();

	Parameters.Service.resize(1);

	bool bIsAudio = s.Get("Encoder", "audioservice", 1);

	/* In the current version only one service and one stream is supported. The
	   stream IDs must be 0 in both cases */
	if (bIsAudio == true)
	{
		/* Audio */

		Parameters.FACParameters.iNumAudioServices = 1;
		Parameters.FACParameters.iNumDataServices = 0;

		Parameters.Service[0].eAudDataFlag = SF_AUDIO;
		Parameters.Service[0].iAudioStream = 0;

		/* Text message */
		bool bTextflag = s.Get("Encoder", "textmessages", 1);
		Parameters.AudioParam[0].bTextflag = bTextflag;
		if(bTextflag)
		{
            vecstrTexts.clear();
            for(size_t i=0; i<100; i++)
            {
                stringstream ss;
                ss << "textmessage" << i;
                string msg = s.Get("Encoder", ss.str(), string("(end)"));
                if(msg == "(end)")
                {
                    break;
                }
                vecstrTexts.push_back(msg);
            }
		}

		/* Programme Type code (see TableFAC.h, "strTableProgTypCod[]") */
		Parameters.Service[0].iServiceDescr = s.Get("Encoder", "genre", 15); /* 15 -> other music */
	}
	else
	{
		/* Data */
		Parameters.FACParameters.iNumAudioServices = 0;
		Parameters.FACParameters.iNumDataServices = 1;

		Parameters.Service[0].eAudDataFlag = SF_DATA;
		Parameters.Service[0].iDataStream = 0;
		Parameters.Service[0].iPacketID = 0;

		/* Init SlideShow application */
		Parameters.DataParam[0][0].eDataUnitInd = CDataParam::DU_DATA_UNITS;
		Parameters.DataParam[0][0].eAppDomain = CDataParam::AD_DAB_SPEC_APP;

		/* The value 0 indicates that the application details are provided
		   solely by SDC data entity type 5 */
		Parameters.Service[0].iServiceDescr = 0;
	}

	/* Init service parameters, 24 bit unsigned integer number */
	Parameters.Service[0].iServiceID = s.Get("Encoder", "sid", 0);

	/* Service label data. Up to 16 bytes defining the label using UTF-8 coding */
	Parameters.Service[0].strLabel = s.Get("Encoder", "label", string("Dream Test"));

	/* FAC Language (see TableFAC.h, "strTableLanguageCode[]") */
	Parameters.Service[0].iLanguage = s.Get("Encoder", "language", 5);	/* 5 -> english */

	/* SDC Language and Country */
	Parameters.Service[0].strLanguageCode = s.Get("Encoder", "ISOlanguage", string("eng"));
	Parameters.Service[0].strCountryCode = s.Get("Encoder", "ISOCountry", string("gb"));

	/**************************************************************************/
	/* Robustness mode and spectrum occupancy. Available transmission modes:
	   RM_ROBUSTNESS_MODE_A: Gaussian channels, with minor fading,
	   RM_ROBUSTNESS_MODE_B: Time and frequency selective channels, with longer
	   delay spread,
	   RM_ROBUSTNESS_MODE_C: As robustness mode B, but with higher Doppler
	   spread,
	   RM_ROBUSTNESS_MODE_D: As robustness mode B, but with severe delay and
	   Doppler spread.
	   Available bandwidths:
	   SO_0: 4.5 kHz, SO_1: 5 kHz, SO_2: 9 kHz, SO_3: 10 kHz, SO_4: 18 kHz,
	   SO_5: 20 kHz */
	Parameters.Channel.eRobustness = ERobMode(s.Get("Encoder", "robm", RM_ROBUSTNESS_MODE_B));
	Parameters.Channel.eSpectrumOccupancy = ESpecOcc(s.Get("Encoder", "spectrum_occupancy", SO_3));

	/* Protection levels for MSC. Depend on the modulation scheme. Look at
	   TableMLC.h, iCodRateCombMSC16SM, iCodRateCombMSC64SM,
	   iCodRateCombMSC64HMsym, iCodRateCombMSC64HMmix for available numbers */
	Parameters.MSCParameters.ProtectionLevel.iPartA = s.Get("Encoder", "PartAProt", 0);
	Parameters.MSCParameters.ProtectionLevel.iPartB = s.Get("Encoder", "PartBProt", 1);
	Parameters.MSCParameters.ProtectionLevel.iHierarch = s.Get("Encoder", "HierarchicalProt", 0);

	/* Interleaver mode of MSC service. Long interleaving (2 s): SI_LONG,
	   short interleaving (400 ms): SI_SHORT */
	Parameters.Channel.eInterleaverDepth
		= ESymIntMod(s.Get("Encoder", "interleaving", SI_LONG));

	/* MSC modulation scheme. Available modes:
	   16-QAM standard mapping (SM): CS_2_SM,
	   64-QAM standard mapping (SM): CS_3_SM,
	   64-QAM symmetrical hierarchical mapping (HMsym): CS_3_HMSYM,
	   64-QAM mixture of the previous two mappings (HMmix): CS_3_HMMIX */
	Parameters.Channel.eMSCmode = ECodScheme(s.Get("Encoder", "mscmod", CS_3_SM));

	/* SDC modulation scheme. Available modes:
	   4-QAM standard mapping (SM): CS_1_SM,
	   16-QAM standard mapping (SM): CS_2_SM */
	Parameters.Channel.eSDCmode = ECodScheme(s.Get("Encoder", "sdcmod", CS_2_SM));

	iSoundInDev = s.Get("Encoder", "snddevin", -1);
	strInputFileName = s.Get("Encoder", "inputfile", string(""));

	/* streams - do when know MSC Capacity */
	Parameters.MSCParameters.Stream.clear();
    for(size_t i=0; i<4; i++)
    {
        stringstream ss;
        CStream stream;
        ss << "s" << i << "PartALen";
        stream.iLenPartA = s.Get("Encoder", ss.str(), -1);
        ss.str("");
        ss << "s" << i << "PartBLen";
        stream.iLenPartB = s.Get("Encoder", ss.str(), -1);

        if (bIsAudio == true)
        {
            stream.eAudDataFlag = SF_AUDIO;
        }
        else
        {
            stream.eAudDataFlag = SF_DATA;
            stream.ePacketModInd = PM_PACKET_MODE;
            stream.iPacketLen = 45;	/* TEST */
        }
        if(stream.iLenPartA != -1)
            Parameters.MSCParameters.Stream.push_back(stream);
    }
	Parameters.Unlock();
}

void
CDRMEncoder::SaveSettings(CSettings& s, const CParameter& Parameters) const
{
	s.Put("Encoder", "audioservice", (Parameters.Service[0].eAudDataFlag == SF_AUDIO)?1:0);
	s.Put("Encoder", "textmessages", Parameters.AudioParam.find(0)->second.bTextflag);
	size_t i;
	for(i=0; i<vecstrTexts.size(); i++)
	{
        stringstream ss;
        ss << "textmessage" << i;
        s.Put("Encoder", ss.str(), vecstrTexts[i]);
	}
	s.Put("Encoder", "genre", Parameters.Service[0].iServiceDescr);
	s.Put("Encoder", "sid", int(Parameters.Service[0].iServiceID));
	s.Put("Encoder", "label", Parameters.Service[0].strLabel);
	s.Put("Encoder", "language", Parameters.Service[0].iLanguage);
	s.Put("Encoder", "ISOlanguage", Parameters.Service[0].strLanguageCode);
	s.Put("Encoder", "ISOCountry", Parameters.Service[0].strCountryCode);

	s.Put("Encoder", "robm", Parameters.Channel.eRobustness);
	s.Put("Encoder", "spectrum_occupancy", Parameters.Channel.eSpectrumOccupancy);
	s.Put("Encoder", "PartAProt", Parameters.MSCParameters.ProtectionLevel.iPartA);
	s.Put("Encoder", "PartBProt", Parameters.MSCParameters.ProtectionLevel.iPartB);
	s.Put("Encoder", "HierarchicalProt", Parameters.MSCParameters.ProtectionLevel.iHierarch);
	s.Put("Encoder", "interleaving", Parameters.Channel.eInterleaverDepth);
	s.Put("Encoder", "mscmod", Parameters.Channel.eMSCmode);
	s.Put("Encoder", "sdcmod", Parameters.Channel.eSDCmode);
	s.Put("Encoder", "snddevin", iSoundInDev);
	s.Put("Encoder", "inputfile", strInputFileName);
	for(i=0; i<Parameters.MSCParameters.Stream.size(); i++)
	{
        stringstream ss;
        ss << "s" << i << "PartALen";
        s.Put("Encoder", ss.str(), Parameters.MSCParameters.Stream[i].iLenPartA);
        ss.str("");
        ss << "s" << i << "PartBLen";
        s.Put("Encoder", ss.str(), Parameters.MSCParameters.Stream[0].iLenPartB);
	}
}
