/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM-transmitter
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

#include "MDI/MDIRSCI.h" /* OPH: need this near the top so winsock2 is included before winsock */
#include "MDI/MDIDecode.h"
#include "DrmTransmitter.h"
#include "mlc/MLC.h"
#include <sstream>
#include <ctime>


/* Implementation *************************************************************/
CDRMTransmitter::CDRMTransmitter():
    CDRMTransmitterInterface(),
	Parameters(), eOpMode(T_TX), bRunning(false),
	strMDIinAddr(), MDIoutAddr(),
	pEncoder(NULL), pModulator(NULL),
	pMDIIn(NULL), pDecodeMDI(NULL), pMDIOut(NULL)
{
	/* Init streams */
	Parameters.ResetServicesStreams();

	/* Init frame ID counter (index) */
	Parameters.FACParameters.iFrameId = 0;

	time_t t;
	time(&t);
    tm gmt = *gmtime(&t);

	/* Date, time. */
	Parameters.iDay = gmt.tm_mday;
	Parameters.iMonth = gmt.tm_mon+1;
	Parameters.iYear = 1900+gmt.tm_year;
	Parameters.iUTCHour = gmt.tm_hour;
	Parameters.iUTCMin = gmt.tm_min;
}

void
CDRMTransmitter::CalculateChannelCapacities()
{
    CMLC mlc(CT_MSC);
    mlc.CalculateParam(Parameters, CT_FAC);
    mlc.CalculateParam(Parameters, CT_SDC);
    mlc.CalculateParam(Parameters, CT_MSC);
}

void
CDRMTransmitter::SetOperatingMode(const ETxOpMode eNewOpMode)
{
	eOpMode = eNewOpMode;
    switch(eOpMode)
    {
    case T_TX:
 		if(pMDIIn)
 		{
			delete pMDIIn;
			pMDIIn = NULL;
 		}
		if(pDecodeMDI)
		{
			delete pDecodeMDI;
			pDecodeMDI = NULL;
		}
		if(pMDIOut)
		{
			delete pMDIOut;
			pMDIIn = NULL;
		}
		if(pEncoder == NULL)
			pEncoder = new CDRMEncoder();
		if(pModulator == NULL)
			pModulator = new CDRMModulator();
        break;
    case T_ENC:
 		if(pMDIIn)
 		{
			delete pMDIIn;
			pMDIIn = NULL;
 		}
		if(pDecodeMDI)
		{
			delete pDecodeMDI;
			pDecodeMDI = NULL;
		}
		if(pModulator)
		{
			delete pModulator;
			pModulator = NULL;
		}
		if(pEncoder == NULL)
			pEncoder = new CDRMEncoder();
		if(pMDIOut == NULL)
			pMDIOut = new CMDIOut();
        break;
    case T_MOD:
		if(pEncoder)
		{
			delete pEncoder;
			pEncoder = NULL;
		}
		if(pMDIOut)
		{
			delete pMDIOut;
			pMDIOut = NULL;
		}
		if(pMDIIn == NULL)
			pMDIIn = new CMDIIn();
		if(pDecodeMDI == NULL)
			pDecodeMDI = new CDecodeMDI();
		if(pModulator == NULL)
			pModulator = new CDRMModulator();
    }
}

CDRMTransmitter::ETxOpMode
CDRMTransmitter::GetOperatingMode() const
{
	return eOpMode;
}

void
CDRMTransmitter::
GetSoundInChoices(vector<string>& v) const
{
    if(pEncoder)
        pEncoder->GetSoundInChoices(v);
    else
        v.clear();
}

void
CDRMTransmitter::
GetSoundOutChoices(vector<string>& v) const
{
    if(pModulator)
        pModulator->GetSoundOutChoices(v);
    else
        v.clear();
}

int
CDRMTransmitter::
GetSoundInInterface() const
{
    if(pEncoder)
        return pEncoder->GetSoundInInterface();
    else
        return -1;
}

void
CDRMTransmitter::
SetSoundInInterface(int i)
{
    if(pEncoder)
        pEncoder->SetSoundInInterface(i);
}


_REAL CDRMTransmitter::GetLevelMeter() const
{
    if(pEncoder)
        return pEncoder->GetLevelMeter();
    else
        return 0.0;
}

void
CDRMTransmitter::AddTextMessage(const string& strText)
{
    if(pEncoder)
        pEncoder->AddTextMessage(strText);
}

void
CDRMTransmitter::ClearTextMessages()
{
    if(pEncoder)
        pEncoder->ClearTextMessages();
}

void
CDRMTransmitter::GetTextMessages(vector<string>& v) const
{
    if(pEncoder)
        pEncoder->GetTextMessages(v);
    else
        v.clear();
}


void
CDRMTransmitter::AddPic(const string& strFileName, const string& strFormat)
{
    if(pEncoder)
        pEncoder->AddPic(strFileName, strFormat);
}

void
CDRMTransmitter::ClearPics()
{
    if(pEncoder)
        pEncoder->ClearPics();
}

void
CDRMTransmitter::GetPics(map<string,string>& m) const
{
    if(pEncoder)
        pEncoder->GetPics(m);
}

bool
CDRMTransmitter::GetTransStat(string& strCPi, _REAL& rCPe) const
{
    if(pEncoder)
        return pEncoder->GetTransStat(strCPi, rCPe);
    else
        return false;
}

void
CDRMTransmitter::GetData(map<int, map<int,CDataParam> >& d) const
{
	d = Parameters.DataParam;
}

void
CDRMTransmitter::PutData(const map<int, map<int,CDataParam> >& d)
{
	Parameters.DataParam = d;
}

void
CDRMTransmitter::SetReadFromFile(const string & strNFN)
{
    if(pEncoder)
        pEncoder->SetReadFromFile(strNFN);
}

string
CDRMTransmitter::GetReadFromFile() const
{
    if(pEncoder)
        return pEncoder->GetReadFromFile();
    else
        return "";
}

void
CDRMTransmitter::GetAudio(map<int,CAudioParam>& a) const
{
	a = Parameters.AudioParam;
}

void
CDRMTransmitter::PutAudio(const map<int,CAudioParam>& a)
{
	Parameters.AudioParam = a;
}

void
CDRMTransmitter::SetCOFDMOutputs(const vector<string>& o)
{
    if(pModulator)
        pModulator->SetOutputs(o);
}

void
CDRMTransmitter::GetCOFDMOutputs(vector<string>& o) const
{
    if(pModulator)
        pModulator->GetOutputs(o);
    else
        o.clear();
}

void
CDRMTransmitter::PutChannel(const CChannel& c)
{
	Parameters.Channel=c;
    Parameters.CellMappingTable.MakeTable(c.eRobustness, c.eSpectrumOccupancy);
}

void
CDRMTransmitter::GetServices(vector<CService>& s, int& naudio, int& ndata) const
{
	s= Parameters.Service;
	naudio = Parameters.FACParameters.iNumAudioServices;
	ndata = Parameters.FACParameters.iNumDataServices;
}

void
CDRMTransmitter::PutServices(const vector<CService>& s, int naudio, int ndata)
{
	Parameters.Service = s;
	Parameters.FACParameters.iNumAudioServices = naudio;
	Parameters.FACParameters.iNumDataServices = ndata;
}

void
CDRMTransmitter::Stop()
{
	bRunning = false;
}

void CDRMTransmitter::Start()
{

	bool bInSync = true;
	CSingleBuffer<_BINARY> MDIPacketBuf;
	CSingleBuffer<_BINARY> FACBuf, SDCBuf;
	vector<CSingleBuffer<_BINARY> > MSCBuf(MAX_NUM_STREAMS);

	try
	{
        switch(eOpMode)
        {
        case T_TX:
            Parameters.ReceiveStatus.FAC.SetStatus(RX_OK);
            Parameters.ReceiveStatus.SDC.SetStatus(RX_OK);
            pEncoder->Init(Parameters, FACBuf, SDCBuf, MSCBuf);
            pModulator->Init(Parameters);
            break;
        case T_ENC:
            if(MDIoutAddr.size()==0)
                throw CGenErr("Encoder with no outputs");
            Parameters.ReceiveStatus.FAC.SetStatus(RX_OK);
            Parameters.ReceiveStatus.SDC.SetStatus(RX_OK);
            pEncoder->Init(Parameters, FACBuf, SDCBuf, MSCBuf);
            /* set the output address */
            for(vector<string>::const_iterator s = MDIoutAddr.begin(); s!=MDIoutAddr.end(); s++)
            {
                pMDIOut->AddSubscriber(*s, "", 'M');
            }
            pMDIOut->Init(Parameters);
            break;
        case T_MOD:
            if(strMDIinAddr == "")
            {
                throw CGenErr("Modulator with no input");
            }
            pMDIIn->SetOrigin(strMDIinAddr);
            bInSync = false;
            pMDIIn->Init(Parameters, MDIPacketBuf);
            pDecodeMDI->Init(Parameters, FACBuf, SDCBuf, MSCBuf);
        }

        /* Set run flag */
        bRunning = true;
        switch(eOpMode)
        {
        case T_TX:
            cout << "Tx: starting, in: Encoder, out: COFDM" << endl;
            break;
        case T_ENC:
            cout << "Tx: starting, in: Encoder, out: MDI" << endl;
            break;
        case T_MOD:
            cout << "Tx: starting, in: MDI, out: COFDM" << endl;
        }

		while (bRunning)
		{
			if(eOpMode == T_MOD)
			{
				if(bInSync==false)
				{
					FACBuf.SetRequestFlag(true);
					SDCBuf.SetRequestFlag(true);
					for(size_t i=0; i<MAX_NUM_STREAMS; i++)
						MSCBuf[i].SetRequestFlag(true);
					MDIPacketBuf.SetRequestFlag(true);
				}
				MDIPacketBuf.Clear();
				pMDIIn->ReadData(Parameters, MDIPacketBuf);
				if(MDIPacketBuf.GetFillLevel()>0)
				{
					//DecodeMDI.Expect(MDIPacketBuf.GetFillLevel());
					pDecodeMDI->ProcessData(Parameters, MDIPacketBuf, FACBuf, SDCBuf, MSCBuf);
				}
				if(bInSync==false && FACBuf.GetFillLevel()>0)
				{
					CFACReceive FACReceive;
					bool bCRCOk = FACReceive.FACParam(FACBuf.QueryWriteBuffer(), Parameters);
					if(bCRCOk)
					{
						Parameters.Lock();
						Parameters.Channel = Parameters.NextConfig.Channel;
						Parameters.Unlock();
						pModulator->Init(Parameters);
						if(SDCBuf.GetFillLevel()>0)
						{
							bInSync = true;
						}
						else
						{
							/* consume the FAC & MSC and wait for an SDC frame */
							(void)FACBuf.Get(NUM_FAC_BITS_PER_BLOCK);
							for(size_t i=0; i<MAX_NUM_STREAMS; i++)
							{
								(void)MSCBuf[i].Get(MSCBuf[i].GetFillLevel());
                            }
						}
					}
					else
						cerr << "bad FAC CRC" << endl;
				}
			}
			else
			{
				pEncoder->ReadData(Parameters, FACBuf, SDCBuf, MSCBuf);
			}

			if(eOpMode == T_ENC)
			{
				pMDIOut->WriteData(Parameters, FACBuf, SDCBuf, MSCBuf);
			}
			else
			{
				if(bInSync)
					pModulator->WriteData(Parameters, FACBuf, SDCBuf, MSCBuf);
				/* TODO - set bInSync false on error */
			}
		}
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
	if(eOpMode != T_ENC)
		pModulator->Cleanup(Parameters);
	if(eOpMode != T_MOD)
		pEncoder->Cleanup(Parameters);
}

void
CDRMTransmitter::LoadSettings(CSettings& s)
{
	string mode = s.Get("0", "mode", string("TX"));
	if(mode == "TX")
	{
        SetOperatingMode(T_TX);
        pEncoder->LoadSettings(s, Parameters);
        pModulator->LoadSettings(s, Parameters);
	}
	if(mode == "ENC")
	{
        SetOperatingMode(T_ENC);
        pEncoder->LoadSettings(s, Parameters);
        for(size_t i=0; i<100; i++)
        {
            stringstream ss;
            ss << "MDIout" << i;
            string addr = s.Get("Transmitter", ss.str(), string("[none]"));
            if(addr == "[none]")
                break;
            MDIoutAddr.push_back(addr);
        }
	}
	if(mode == "MOD")
	{
        SetOperatingMode(T_MOD);
        pModulator->LoadSettings(s, Parameters);
        strMDIinAddr = s.Get("Transmitter", "MDIin", string(""));
	}
}

void
CDRMTransmitter::SaveSettings(CSettings& s) const
{
	switch(eOpMode)
	{
	case T_TX:
		s.Put("0", "mode", string("TX"));
        pEncoder->SaveSettings(s, Parameters);
        pModulator->SaveSettings(s, Parameters);
		break;
	case T_ENC:
		s.Put("0", "mode", string("ENC"));
        pEncoder->SaveSettings(s, Parameters);
        for(size_t i=0; i<MDIoutAddr.size(); i++)
        {
            stringstream ss;
            ss << "MDIout" << i;
            s.Put("Transmitter", ss.str(), MDIoutAddr[i]);
        }
		break;
	case T_MOD:
		s.Put("0", "mode", string("MOD"));
        pModulator->SaveSettings(s, Parameters);
        s.Put("Transmitter", "MDIin", strMDIinAddr);
		break;
	}
}
