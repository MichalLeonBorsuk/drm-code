/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden, Andrew Murphy
 *
 * Description:
  *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *  All modules that generate MDI information are given (normally at construction) a pointer to an MDI object.
 *  They call methods in this interface when they have MDI/RSCI data to impart.
 *
 *	Note that this previously needed QT, but now the QT socket is wrapped in an abstract class so
 *  this class can be compiled without QT. (A null socket is instantiated instead in this case, so
 *  nothing will actually happen.) This could be developed further by using a factory class to make
 *  the socket, in which case this class would only need to know about the abstract interface
 *  CPacketSocket.
 *
 *  This class is now almost a facade for all of the DCP and TAG classes, designed to have the
 *  same interface as the old CMDI class had. It could be improved further by moving the
 *  MDI generation into a separate class.
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

#include "MDIRSCI.h"
#ifdef QT_NETWORK_LIB
# include "PacketSocketQT.h"
# include <QHostAddress>
#else
# include "PacketSocketNull.h"
#endif
#include "PacketSourceFile.h"
#include <sstream>
#include <iostream>
#include <iomanip>

/* Implementation *************************************************************/

CDownstreamDI::CDownstreamDI() : iLogFraCnt(0), pDrmReceiver(NULL),
	bMDIOutEnabled(false), bMDIInEnabled(false),bIsRecording(false),
	iFrequency(0), strRecordType(),
	vecTagItemGeneratorStr(MAX_NUM_STREAMS), vecTagItemGeneratorRBP(MAX_NUM_STREAMS),
	RSISubscribers(),pRSISubscriberFile(new CRSISubscriberFile)
{
	/* Initialise all the generators for strx and rbpx tags */
	for (int i=0; i<MAX_NUM_STREAMS; i++)
	{
		vecTagItemGeneratorStr[i].SetStreamNumber(i);
		vecTagItemGeneratorRBP[i].SetStreamNumber(i);
	}

	/* Reset all tags for initialization */
	ResetTags();

	/* Init constant tag */
	TagItemGeneratorProTyMDI.GenTag();
	TagItemGeneratorProTyRSCI.GenTag();

	/* Add the file subscriber to the list of subscribers */
	RSISubscribers.push_back(pRSISubscriberFile);

}

CDownstreamDI::~CDownstreamDI()
{
	for(vector<CRSISubscriber*>::iterator i = RSISubscribers.begin();
			i!=RSISubscribers.end(); i++)
	{
		delete *i;
	}
}

/******************************************************************************\
* DI send status, receive control                                             *
\******************************************************************************/
/* Access functions ***********************************************************/
void CDownstreamDI::SendLockedFrame(CParameter& Parameter,
						CSingleBuffer<_BINARY>& FACData,
						CSingleBuffer<_BINARY>& SDCData,
						vector<CSingleBuffer<_BINARY> >& vecMSCData
)
{
	TagItemGeneratorFAC.GenTag(Parameter, FACData.Get(NUM_FAC_BITS_PER_BLOCK));
	int iBits = Parameter.iNumSDCBitsPerSuperFrame;
	if(iBits>SDCData.GetFillLevel())
		iBits = 0;
	TagItemGeneratorSDC.GenTag(Parameter, SDCData.Get(iBits));
	size_t n = MAX_NUM_STREAMS;
	if(n > vecMSCData.size())
		n = vecMSCData.size();

	for (size_t i = 0; i < n; i++)
	{
		const int iLenStrData = BITS_BINARY * Parameter.GetStreamLen(i);
		/* Only generate this tag if stream input data is not of zero length */
		if (iLenStrData > 0)
			vecTagItemGeneratorStr[i]
				.GenTag(Parameter, vecMSCData[i].Get(iLenStrData));
	}
	SendLockedFrame(Parameter);
}

void CDownstreamDI::SendLockedFrame(CParameter& Parameter,
						CVectorEx<_BINARY>* pFACData,
						CVectorEx<_BINARY>* pSDCData,
						vector<CVectorEx<_BINARY>*>& vecMSCData
)
{
	TagItemGeneratorFAC.GenTag(Parameter, pFACData);
	TagItemGeneratorSDC.GenTag(Parameter, pSDCData);
	size_t n = MAX_NUM_STREAMS;
	if(n > vecMSCData.size())
		n = vecMSCData.size();
	for (size_t i = 0; i < n; i++)
	{
		/* Only generate this tag if stream input data is not of zero length */
		if (vecMSCData[i] && vecMSCData[i]->Size() > 0)
			vecTagItemGeneratorStr[i].GenTag(Parameter, vecMSCData[i]);
	}
	SendLockedFrame(Parameter);
}

void CDownstreamDI::SendLockedFrame(CParameter& Parameter)
{

	TagItemGeneratorRobMod.GenTag(Parameter.Channel.eRobustness);
	TagItemGeneratorRxDemodMode.GenTag(Parameter.eModulation);

	/* SDC channel information tag must be created here because it must be sent
	   with each AF packet */
	TagItemGeneratorSDCChanInf.GenTag(Parameter);

    if(Parameter.eModulation != NONE)
    {
        bool bValid;
        _REAL rVal;

        TagItemGeneratorRINF.GenTag(Parameter.sReceiverID);	/* rinf */

        /* RSCI tags ------------------------------------------------------------ */
        TagItemGeneratorRAFS.GenTag(Parameter);
        bValid = Parameter.Measurements.WMERFAC.get(rVal);
        TagItemGeneratorRWMF.GenTag(bValid, rVal); /* WMER for FAC */
        bValid = Parameter.Measurements.WMERMSC.get(rVal);
        TagItemGeneratorRWMM.GenTag(bValid, rVal); /* WMER for MSC */
        bValid = Parameter.Measurements.MER.get(rVal);
        TagItemGeneratorRMER.GenTag(bValid, rVal); /* MER for MSC */
        vector<CMeasurements::CRdel> rdel;
        bValid = Parameter.Measurements.Rdel.get(rdel);
        TagItemGeneratorRDEL.GenTag(bValid, rdel);
        bValid = Parameter.Measurements.Rdop.get(rVal);
        TagItemGeneratorRDOP.GenTag(bValid, rVal);
        CMeasurements::CInterferer intf;
        bValid = Parameter.Measurements.interference.get(intf);
        TagItemGeneratorRINT.GenTag(bValid, intf.rIntFreq, intf.rINR, intf.rICR);
        TagItemGeneratorRNIP.GenTag(true,
            Parameter.Measurements.rMaxPSDFreq, Parameter.Measurements.rMaxPSDwrtSig
        );
        TagItemGeneratorRxService.GenTag(true, Parameter.GetCurSelAudioService());
        TagItemGeneratorReceiverStatus.GenTag(Parameter);
        TagItemGeneratorRxFrequency.GenTag(true, Parameter.GetFrequency()); /* rfre */
        TagItemGeneratorRxActivated.GenTag(true); /* ract */
        TagItemGeneratorPowerSpectralDensity.GenTag(Parameter);
        TagItemGeneratorPowerImpulseResponse.GenTag(Parameter);
        TagItemGeneratorPilots.GenTag(Parameter);

        /* Generate some other tags */
        bValid = Parameter.Measurements.SigStr.get(rVal);
        TagItemGeneratorSignalStrength.GenTag(bValid, rVal);

        TagItemGeneratorGPS.GenTag(true, Parameter.GPSData);	// rgps
    }

	GenDIPacket();
}

void CDownstreamDI::SendUnlockedFrame(CParameter& Parameter)
{
	/* This is called once per frame if the receiver is unlocked */

	/* In the MDI profile, we used to ignore this altogether since "I assume there's no point */
	/* in generating empty packets with no reception monitoring information" */
	/* But now there could be multiple profiles at the same time. TODO: decide what to do! */
/*	if (cProfile == 'M')
		return;*/

	/* Send empty tags for most tag items */
	ResetTags();

	TagItemGeneratorFAC.GenEmptyTag();
	TagItemGeneratorSDC.GenEmptyTag();

	/* mode is unknown - make empty robm tag */
	TagItemGeneratorRobMod.GenEmptyTag();

    TagItemGeneratorRxDemodMode.GenTag(Parameter.eModulation);

    TagItemGeneratorSDCChanInf.GenEmptyTag();

    TagItemGeneratorReceiverStatus.GenTag(Parameter);

    TagItemGeneratorPowerSpectralDensity.GenTag(Parameter);

    TagItemGeneratorPowerImpulseResponse.GenEmptyTag();

    TagItemGeneratorPilots.GenEmptyTag();

    TagItemGeneratorRNIP.GenTag(true,
        Parameter.Measurements.rMaxPSDFreq,
        Parameter.Measurements.rMaxPSDwrtSig
    );

    /* Generate some other tags */
    TagItemGeneratorRINF.GenTag(Parameter.sReceiverID);	/* rinf */
    TagItemGeneratorRxFrequency.GenTag(true, Parameter.GetFrequency()); /* rfre */
    TagItemGeneratorRxActivated.GenTag(true); /* ract */
    _REAL rSigStr;
    bool bValid = Parameter.Measurements.SigStrstat.getCurrent(rSigStr);
    TagItemGeneratorSignalStrength.GenTag(bValid, rSigStr);

    TagItemGeneratorGPS.GenTag(true, Parameter.GPSData);	/* rgps */

	GenDIPacket();
}

void CDownstreamDI::SendAMFrame(CParameter& Parameter, CSingleBuffer<_BINARY>& CodedAudioData)
{
		/* This is called once per 400ms if the receiver is in AM mode */

	/* In the MDI profile, ignore this altogether since there's no DRM information */
	/*if (cProfile == 'M')
		return;*/

	/* Send empty tags for most tag items */
	ResetTags();

	TagItemGeneratorFAC.GenEmptyTag();
	TagItemGeneratorSDC.GenEmptyTag();
	/* mode is unknown - make empty robm tag */
	TagItemGeneratorRobMod.GenEmptyTag();

	/* demod mode */
	TagItemGeneratorRxDemodMode.GenTag(Parameter.eModulation);

	TagItemGeneratorSDCChanInf.GenEmptyTag();

	/* These will be set appropriately when the rx is put into AM mode */
	/* We need to decide what "appropriate" settings are */
	TagItemGeneratorReceiverStatus.GenTag(Parameter);

	TagItemGeneratorPowerSpectralDensity.GenTag(Parameter);

	TagItemGeneratorPowerImpulseResponse.GenEmptyTag();

	TagItemGeneratorPilots.GenEmptyTag();

    Parameter.Lock();

	TagItemGeneratorRNIP.GenTag(true,
        Parameter.Measurements.rMaxPSDFreq,
        Parameter.Measurements.rMaxPSDwrtSig
    );

	// Generate a rama tag with the encoded audio data
	TagItemGeneratorAMAudio.GenTag(Parameter, CodedAudioData);

	/* Generate some other tags */
	TagItemGeneratorRINF.GenTag(Parameter.sReceiverID);	/* rinf */
	TagItemGeneratorRxFrequency.GenTag(true, Parameter.GetFrequency()); /* rfre */
	TagItemGeneratorRxActivated.GenTag(true); /* ract */

	_REAL rSigStr;
	bool bValid = Parameter.Measurements.SigStrstat.getCurrent(rSigStr);

    Parameter.Lock();

	TagItemGeneratorSignalStrength.GenTag(bValid, rSigStr);

	TagItemGeneratorGPS.GenTag(true, Parameter.GPSData);	/* rgps */

	GenDIPacket();
}

void CDownstreamDI::SetReceiver(ReceiverInterface *pReceiver)
{
	pDrmReceiver = pReceiver;
	for(vector<CRSISubscriber*>::iterator i = RSISubscribers.begin();
			i!=RSISubscribers.end(); i++)
			(*i)->SetReceiver(pReceiver);
}

/* Actual DRM DI protocol implementation *****************************************/
void CDownstreamDI::GenDIPacket()
{
	/* Reset the tag packet generator */
	TagPacketGenerator.Reset();

	/* Increment MDI packet counter and generate counter tag */
	TagItemGeneratorLoFrCnt.GenTag();

	/* dlfc tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorLoFrCnt);

	/* *ptr tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyMDI);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyRSCI);

	/* rinf taf */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRINF);

	/* rgps tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorGPS);

	/* rpro tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProfile);

	/* rdmo - note that this is currently empty */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRxDemodMode);

	/* ract */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRxActivated);

	/* rfre tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRxFrequency);

	/* fac_ tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorFAC);

	/* sdc_ tag */
	//if (cProfile != 'M' || TagItemGeneratorSDC.GetTotalLength()>0)
	TagPacketGenerator.AddTagItem(&TagItemGeneratorSDC);

	/* sdci tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorSDCChanInf);

	/* robm tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRobMod);

	/* strx tag */
	size_t i;
	for (i = 0; i < MAX_NUM_STREAMS; i++)
	{
		TagPacketGenerator.AddTagItem(&vecTagItemGeneratorStr[i]);
	}
	TagPacketGenerator.AddTagItem(&TagItemGeneratorAMAudio);

	TagPacketGenerator.AddTagItem(&TagItemGeneratorSignalStrength);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRAFS);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRMER);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRWMM);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRWMF);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRDEL);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRDOP);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRINT);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRNIP);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorReceiverStatus);

	TagPacketGenerator.AddTagItem(&TagItemGeneratorPowerSpectralDensity);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorPowerImpulseResponse);

	TagPacketGenerator.AddTagItem(&TagItemGeneratorPilots);


	for (i = 0; i < MAX_NUM_STREAMS; i++)
	{
		TagPacketGenerator.AddTagItem(&vecTagItemGeneratorRBP[i]);
	}

	/*return TagPacketGenerator.GenAFPacket(bUseAFCRC);*/

	/* transmit a packet to each subscriber */
	for(vector<CRSISubscriber*>::iterator s = RSISubscribers.begin();
			s!=RSISubscribers.end(); s++)
	{
		// re-generate the profile tag for each subscriber
		TagItemGeneratorProfile.GenTag((*s)->GetProfile());
		(*s)->TransmitPacket(TagPacketGenerator);
	}
}

void CDownstreamDI::ResetTags()
{
	/* Do not reset "*ptr" tag because this one is static */
	/* This group of tags are generated each time so don't need empty tags generated */
	TagItemGeneratorLoFrCnt.Reset(); /* dlfc tag */
	TagItemGeneratorFAC.Reset(); /* fac_ tag */
	TagItemGeneratorSDCChanInf.Reset(); /* sdci tag */
	TagItemGeneratorRobMod.Reset(); /* robm tag */
	TagItemGeneratorRINF.Reset(); /* info tag */
	TagItemGeneratorReceiverStatus.Reset(); /* rsta */

	TagItemGeneratorProfile.Reset(); /* rpro */
	TagItemGeneratorGPS.Reset();	/* rgps */

	TagItemGeneratorPowerSpectralDensity.Reset();
	TagItemGeneratorPowerImpulseResponse.Reset();

	TagItemGeneratorPilots.Reset();

	/* This group of tags might not be generated, so make an empty version in case */

	TagItemGeneratorSignalStrength.GenEmptyTag(); /* rdbv tag */
	TagItemGeneratorRWMF.GenEmptyTag(); /* rwmf tag */
	TagItemGeneratorRWMM.GenEmptyTag(); /* rwmm tag */
	TagItemGeneratorRMER.GenEmptyTag(); /* rmer tag */
	TagItemGeneratorRDEL.GenEmptyTag(); /* rdel tag */
	TagItemGeneratorRDOP.GenEmptyTag(); /* rdop tag */
	TagItemGeneratorRINT.GenEmptyTag(); /* rint tag */
	TagItemGeneratorRNIP.GenEmptyTag(); /* rnip tag */
	TagItemGeneratorRAFS.GenEmptyTag(); /* rafs tag */

	/* Tags that are not fully implemented yet */
	TagItemGeneratorRxBandwidth.GenEmptyTag(); /* rbw_ */
	TagItemGeneratorRxService.GenEmptyTag(); /* rser */

	for (int i = 0; i < MAX_NUM_STREAMS; i++)
	{
		vecTagItemGeneratorStr[i].Reset(); // strx tag
		vecTagItemGeneratorRBP[i].GenEmptyTag(); // make empty version of mandatory tag that isn't implemented
	}

	TagItemGeneratorAMAudio.Reset(); // don't make the tag in DRM mode

	TagItemGeneratorSDC.GenEmptyTag();
}

void CDownstreamDI::GetNextPacket(CSingleBuffer<_BINARY>&)
{
	// TODO
}

/* allow multiple destinations, allow destinations to send cpro instructions back */
bool
CDownstreamDI::AddSubscriber(const string& dest, const string& origin, const char profile, const int iSubsamplingFactor)
{
	// check PFT prefix on destination
    string d=dest;
    bool wantPft = false;
	if(dest[0]=='P' || dest[0]=='p')
	{
	    wantPft = true;
        d.erase(0, 1);
	}
    bool bOK = true;
	CRSISubscriber* subs;

	// Delegate to socket
	CRSISubscriberSocket* s = new CRSISubscriberSocket(NULL);
	if(s->SetDestination(d))
	{
	    cout << "set socket dest " << d << " ok" << endl;
        if (origin != "")
            bOK = s->SetOrigin(origin);
        if(wantPft)
            s->SetPFTFragmentSize(800);
        subs = s;
	}
	else /* try a file */
	{
        delete s;
        CRSISubscriberFile* f = new CRSISubscriberFile();
		bOK = f->SetDestination(d);
		if(bOK)
            cout << "set file dest " << d << " ok" << endl;
		f->StartRecording();
		subs = f;
	}
    if (bOK)
    {
        subs->SetProfile(profile);
        subs->SetSubsamplingFactor(iSubsamplingFactor);
        subs->SetReceiver(pDrmReceiver);
        bMDIOutEnabled = true;
        RSISubscribers.push_back(subs);
        return true;
    }
    else
    {
        delete subs;
    }
	return false;
}

void CDownstreamDI::DefineRSIPreset(const int iPresetNum, const int cPro, const int iFactor)
{
	// Pass on to each of the subscribers. Different subscribers could have different presets.
	for(vector<CRSISubscriber*>::iterator s = RSISubscribers.begin();
			s!=RSISubscribers.end(); s++)
	{
		(*s)->DefinePreset(iPresetNum, cPro, iFactor);
	}
}


bool CDownstreamDI::SetOrigin(const string&)
{
	return false;
}

bool CDownstreamDI::SetDestination(const string&)
{
	return false;
}

bool CDownstreamDI::GetDestination(string&)
{
	return false; // makes no sense
}

void CDownstreamDI::SetAFPktCRC(const bool bNAFPktCRC)
{
	for(vector<CRSISubscriber*>::iterator i = RSISubscribers.begin();
			i!=RSISubscribers.end(); i++)
			(*i)->SetAFPktCRC(bNAFPktCRC);
}

string CDownstreamDI::GetRSIfilename(CParameter& Parameter, const char cProfile)
{
	/* Get current UTC time */
	time_t ltime;
	time(&ltime);
	struct tm* gmtCur = gmtime(&ltime);

	iFrequency = Parameter.GetFrequency(); // remember this for later

	stringstream filename;
	filename << Parameter.sDataFilesDirectory << '/';
	filename << Parameter.sReceiverID << "_";
	filename << setw(4) << setfill('0') << gmtCur->tm_year + 1900 << "-" << setw(2) << setfill('0')<< gmtCur->tm_mon + 1;
	filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_mday << "_";
	filename << setw(2) << setfill('0') << gmtCur->tm_hour << "-" << setw(2) << setfill('0')<< gmtCur->tm_min;
	filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_sec << "_";
	filename << setw(8) << setfill('0') << (iFrequency*1000) << ".rs" << char(toupper(cProfile));
	return filename.str();
}

void CDownstreamDI::SetRSIRecording(CParameter& Parameter, const bool bOn, char cPro, const string& type)
{
	strRecordType = type;
	if (bOn)
	{
		pRSISubscriberFile->SetProfile(cPro);
		string fn = GetRSIfilename(Parameter, cPro);
		if(strRecordType != "" && strRecordType != "raw")
			fn += "." + strRecordType;

		pRSISubscriberFile->SetDestination(fn);
		pRSISubscriberFile->StartRecording();
		bMDIOutEnabled = true;
		bIsRecording = true;
	}
	else
	{
		pRSISubscriberFile->StopRecording();
		bIsRecording = false;
	}
}

 /* needs to be called in case a new RSCI file needs to be started */
void CDownstreamDI::NewFrequency(CParameter& Parameter)
{
	/* Has it really changed? */
	int iNewFrequency = Parameter.GetFrequency();

	if (iNewFrequency != iFrequency)
	{
		if (bIsRecording)
		{
			pRSISubscriberFile->StopRecording();
			string fn = GetRSIfilename(Parameter, pRSISubscriberFile->GetProfile());
			if(strRecordType != "" && strRecordType != "raw")
				fn += "." + strRecordType;
			pRSISubscriberFile->SetDestination(fn);
			pRSISubscriberFile->StartRecording();
		}
	}
}

void CDownstreamDI::SendPacket(const vector<_BYTE>&, uint32_t, uint16_t)
{
	cerr << "this shouldn't get called CDownstreamDI::SendPacket" << endl;
}

/******************************************************************************\
* DI receive
\******************************************************************************/
CDIIn::CDIIn() : CPacketSink(), source(NULL), bDIInEnabled(false)
{
}

CDIIn::~CDIIn()
{
	if(source)
	{
		delete source;
	}
}

bool CDIIn::SetOrigin(const string& str)
{
	/* only allow one listening address */
	if(bDIInEnabled == true)
		return false;

	if(source)
		return false;

	strOrigin = str;

	// try a socket
#ifdef QT_NETWORK_LIB
	source = new CPacketSocketQT;
#else
	source = new CPacketSocketNull;
#endif

	// Delegate to socket
	bool bOK = source->SetOrigin(str);

	if(!bOK)
	{
		// try a file
		delete source;
		source = NULL;
		source = new CPacketSourceFile;
		bOK = source->SetOrigin(str);
	}
	if (bOK)
	{
		source->SetPacketSink(this);
		bDIInEnabled = true;
		return true;
	}
	return false;
}

/* we only support one upstream DI source, so ignore the source address */
void CDIIn::SendPacket(const vector<_BYTE>& vecbydata, uint32_t, uint16_t)
{
	if(vecbydata[0]=='P')
	{
		vector<_BYTE> vecOut;
		if(Pft.DecodePFTPacket(vecbydata, vecOut))
		{
			queue.Put(vecOut);
		}
	}
	else
		queue.Put(vecbydata);
}

void CDIIn::ProcessData(CParameter&, CVectorEx<_BINARY>& vecOutputData, int& iOutputBlockSize)
{
	if(source)
        source->Poll(); // nothing for QT sockets, vital for Pacer driven files
	vector<_BYTE> vecbydata;
	queue.Get(vecbydata);
	iOutputBlockSize = vecbydata.size()*BITS_BINARY;
	vecOutputData.Init(iOutputBlockSize);
	vecOutputData.ResetBitAccess();
	for(size_t i=0; i<vecbydata.size(); i++)
		vecOutputData.Enqueue(vecbydata[i], BITS_BINARY);
}

/******************************************************************************\
* DI receive status, send control                                             *
\******************************************************************************/
CUpstreamDI::CUpstreamDI() : /*CDIIn(),*/ sink(), bUseAFCRC(true), bMDIOutEnabled(false)
{
	/* Init constant tag */
	TagItemGeneratorProTyRSCI.GenTag();
}

CUpstreamDI::~CUpstreamDI()
{
}

bool CUpstreamDI::SetDestination(const string& str)
{
	bMDIOutEnabled = sink.SetDestination(str);
	return bMDIOutEnabled;
}

bool CUpstreamDI::GetDestination(string& str)
{
	return sink.GetDestination(str);
}

void CUpstreamDI::SetFrequency(int iNewFreqkHz)
{
	if(bMDIOutEnabled==false)
		return;
	TagPacketGenerator.Reset();
	TagItemGeneratorCfre.GenTag(iNewFreqkHz);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyRSCI);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorCfre);
	sink.TransmitPacket(TagPacketGenerator);
}

void CUpstreamDI::SetReceiverMode(EModulationType eNewMode)
{
	if(bMDIOutEnabled==false)
		return;
	TagPacketGenerator.Reset();
	TagItemGeneratorCdmo.GenTag(eNewMode);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyRSCI);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorCdmo);
	sink.TransmitPacket(TagPacketGenerator);
}

void CUpstreamDI::SetService(int iServiceID)
{
	if(bMDIOutEnabled==false)
		return;
	TagPacketGenerator.Reset();
	TagItemGeneratorCser.GenTag(iServiceID);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyRSCI);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorCser);
	sink.TransmitPacket(TagPacketGenerator);
}

void
CUpstreamDI::InitInternal(CParameter&)
{
	iInputBlockSize = 1; /* anything is enough but not zero */
	iMaxOutputBlockSize = 2048*BITS_BINARY; /* bigger than an ethernet packet */
}

void
CUpstreamDI::ProcessDataInternal(CParameter& Parameters)
{
	CDIIn::ProcessData(Parameters, *pvecOutputData, iOutputBlockSize);
}

void
CMDIIn::InitInternal(CParameter&)
{
	//outputs[0].iBlockSize = 1; /* packet */
	iMaxOutputBlockSize = 1500*BITS_BINARY;
	iOutputBlockSize = 0;
}

void
CMDIIn::ProcessDataInternal(CParameter& Parameters)
{
    CDIIn::ProcessData(Parameters, *pvecOutputData, iOutputBlockSize);
}

void
CMDIOut::InitInternal(CParameter& Parameters)
{
	iFrameCount = 0;
	iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;
	iInputBlockSize2 = Parameters.iNumSDCBitsPerSuperFrame;
	veciInputBlockSize.resize(MAX_NUM_STREAMS);
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
		veciInputBlockSize[i] = Parameters.GetStreamLen(i);
	vecpvecInputData.resize(MAX_NUM_STREAMS);

}

void
CMDIOut::ProcessDataInternal(CParameter& Parameters)
{
	SendLockedFrame(Parameters, pvecInputData, pvecInputData2, vecpvecInputData);
	switch(iFrameCount)
	{
	case 0:
		iInputBlockSize = Parameters.iNumSDCBitsPerSuperFrame;
		iFrameCount = 1;
		break;
	case 1:
		iInputBlockSize = 0;
		iFrameCount = 2;
		break;
	case 2:
		iInputBlockSize = 0;
		iFrameCount = 0;
	}
}
