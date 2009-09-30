/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *	DRM Parameters
 *
 ******************************************************************************
 *
 * This program is free software(), you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation(), either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY(), without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program(), if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "Parameter.h"
#include "Version.h"
#include "util/SimulationModul.h"
#include <limits>
#include <sstream>
#include <iomanip>
#include <algorithm>
//#include "util/LogPrint.h"

/* Implementation *************************************************************/

CChannel::CChannel():CDumpable(),
bEnhancementLayerInUse(false),
eRobustness(RM_ROBUSTNESS_MODE_B),
eSpectrumOccupancy(SO_3),
eInterleaverDepth(SI_LONG),
eMSCmode(), eSDCmode()
{
}

CAudioParam::CAudioParam(): CDumpable(),strTextMessage(),
    eAudioCoding(AC_AAC), eSBRFlag(SB_NOT_USED), eAudioSamplRate(AS_24KHZ),
    bTextflag(false), bEnhanceFlag(false), eAudioMode(AM_MONO),
    iCELPIndex(0), bCELPCRC(false), eHVXCRate(HR_2_KBIT), bHVXCCRC(false)
{
}

CAudioParam::CAudioParam(const CAudioParam& ap):
    CDumpable(),
    strTextMessage(ap.strTextMessage),
    eAudioCoding(ap.eAudioCoding),
    eSBRFlag(ap.eSBRFlag),
    eAudioSamplRate(ap.eAudioSamplRate),
    bTextflag(ap.bTextflag),
    bEnhanceFlag(ap.bEnhanceFlag),
    eAudioMode(ap.eAudioMode),
    iCELPIndex(ap.iCELPIndex),
    bCELPCRC(ap.bCELPCRC),
    eHVXCRate(ap.eHVXCRate),
    bHVXCCRC(ap.bHVXCCRC)
{
}

CAudioParam& CAudioParam::operator=(const CAudioParam& ap)
{
    strTextMessage = ap.strTextMessage;
    eAudioCoding = ap.eAudioCoding;
    eSBRFlag = ap.eSBRFlag;
    eAudioSamplRate = ap.eAudioSamplRate;
    bTextflag =	ap.bTextflag;
    bEnhanceFlag = ap.bEnhanceFlag;
    eAudioMode = ap.eAudioMode;
    iCELPIndex = ap.iCELPIndex;
    bCELPCRC = ap.bCELPCRC;
    eHVXCRate = ap.eHVXCRate;
    bHVXCCRC = ap.bHVXCCRC;
    return *this;
}

bool CAudioParam::operator==(const CAudioParam& AudioParam) const
{
    if (eAudioCoding != AudioParam.eAudioCoding)
        return false;
    if (eSBRFlag != AudioParam.eSBRFlag)
        return false;
    if (eAudioSamplRate != AudioParam.eAudioSamplRate)
        return false;
    if (bTextflag != AudioParam.bTextflag)
        return false;
    if (bEnhanceFlag != AudioParam.bEnhanceFlag)
        return false;

    switch (AudioParam.eAudioCoding)
    {
    case AC_AAC:
        if (eAudioMode != AudioParam.eAudioMode)
            return false;
        break;

    case AC_CELP:
        if (bCELPCRC != AudioParam.bCELPCRC)
            return false;
        if (iCELPIndex != AudioParam.iCELPIndex)
            return false;
        break;

    case AC_HVXC:
        if (eHVXCRate != AudioParam.eHVXCRate)
            return false;
        if (bHVXCCRC != AudioParam.bHVXCCRC)
            return false;
        break;
    }
    return true;
}

CDataParam::CDataParam():
    CDumpable(),
    ePacketModInd(PM_PACKET_MODE),
    eDataUnitInd(DU_DATA_UNITS),
    eAppDomain(AD_DAB_SPEC_APP),
    eUserAppIdent(AT_NOT_SUP),
    applicationData(0)
{
}

CDataParam::CDataParam(const CDataParam& DataParam):
    CDumpable(),
    ePacketModInd(DataParam.ePacketModInd),
    eDataUnitInd(DataParam.eDataUnitInd),
    eAppDomain(DataParam.eAppDomain),
    eUserAppIdent(DataParam.eUserAppIdent),
    applicationData(DataParam.applicationData)
{
}

CDataParam& CDataParam::operator=(const CDataParam& DataParam)
{
    ePacketModInd = DataParam.ePacketModInd;
    eDataUnitInd = DataParam.eDataUnitInd;
    eAppDomain = DataParam.eAppDomain;
    eUserAppIdent = DataParam.eUserAppIdent;
    applicationData = DataParam.applicationData;
    return *this;
}

bool CDataParam::operator==(const CDataParam& DataParam) const
{
    if (ePacketModInd != DataParam.ePacketModInd)
        return false;
    if (DataParam.ePacketModInd == PM_PACKET_MODE)
    {
        if (eDataUnitInd != DataParam.eDataUnitInd)
            return false;
        if (eAppDomain != DataParam.eAppDomain)
            return false;
        if (DataParam.eAppDomain == AD_DAB_SPEC_APP)
        {
            if (eUserAppIdent != DataParam.eUserAppIdent)
                return false;
            if(applicationData != DataParam.applicationData)
                return false;
        }
    }
    return true;
}

CService::CService():
    CDumpable(),
    iServiceID(SERV_ID_NOT_USED), eCAIndication(CA_NOT_USED),
    iLanguage(0), eAudDataFlag(SF_AUDIO), iServiceDescr(0),
    strCountryCode(), strLanguageCode(), strLabel(),
    iAudioStream(STREAM_ID_NOT_USED), iDataStream(STREAM_ID_NOT_USED), iPacketID(0)
{
}

CService::CService(const CService& s):
    CDumpable(),
    iServiceID(s.iServiceID), eCAIndication(s.eCAIndication),
    iLanguage(s.iLanguage), eAudDataFlag(s.eAudDataFlag),
    iServiceDescr(s.iServiceDescr), strCountryCode(s.strCountryCode),
    strLanguageCode(s.strLanguageCode), strLabel(s.strLabel),
    iAudioStream(s.iAudioStream), iDataStream(s.iDataStream),
    iPacketID(s.iPacketID)
{
}

CService& CService::operator=(const CService& s)
{
    iServiceID = s.iServiceID;
    eCAIndication = s.eCAIndication;
    iLanguage = s.iLanguage;
    eAudDataFlag = s.eAudDataFlag;
    iServiceDescr = s.iServiceDescr;
    strCountryCode = s.strCountryCode;
    strLanguageCode = s.strLanguageCode;
    strLabel = s.strLabel;
    iAudioStream = s.iAudioStream;
    iDataStream = s.iDataStream;
    iPacketID = s.iPacketID;
    return *this;
}

CStream::CStream():CDumpable(),iLenPartA(0), iLenPartB(0),
eAudDataFlag(SF_AUDIO),ePacketModInd(PM_PACKET_MODE),iPacketLen(0)
{
}

bool CStream::operator==(const CStream& Stream) const

{
    if (iLenPartA != Stream.iLenPartA)
        return false;
    if (iLenPartB != Stream.iLenPartB)
        return false;
    return true;
}

CFACParameters::CFACParameters()
:CDumpable(),iFrameId(0),bAFSindexValid(false),iReconfigurationIndex(0),
iNumAudioServices(0),iNumDataServices()
{
}

bool CFACParameters::operator==(const CFACParameters& p) const
{
    if (iFrameId != p.iFrameId)
        return false;
    if (bAFSindexValid != p.bAFSindexValid)
        return false;
    if (iReconfigurationIndex != p.iReconfigurationIndex)
        return false;
    if (iNumAudioServices != p.iNumAudioServices)
        return false;
    if (iNumDataServices != p.iNumDataServices)
        return false;
    return true;
}

CMSCParameters::CMSCParameters()
:CDumpable(),ProtectionLevel(),
Stream(MAX_NUM_STREAMS)
{
}

CMSCParameters::CMSCParameters(const CMSCParameters& p)
:CDumpable(p),ProtectionLevel(p.ProtectionLevel),
Stream(p.Stream)
{
}

CMSCParameters& CMSCParameters::operator=(const CMSCParameters& p)
{
    ProtectionLevel = p.ProtectionLevel;
    Stream = p.Stream;
	return *this;
}

bool CMSCParameters::operator==(const CMSCParameters& p) const
{
    if(!(ProtectionLevel == p.ProtectionLevel))
        return false;
    if(Stream != p.Stream)
        return false;
    return true;
}

CCoreParameter::CCoreParameter():CDumpable(),Channel(),
MSCParameters(),AudioParam(),DataParam()
{
}

CCoreParameter::CCoreParameter(const CCoreParameter& p)
:CDumpable(p),Channel(p.Channel),
MSCParameters(p.MSCParameters),
AudioParam(p.AudioParam),DataParam(p.DataParam)
{
}

CCoreParameter& CCoreParameter::operator=(const CCoreParameter& p)
{
    Channel = p.Channel;
    MSCParameters = p.MSCParameters;
    AudioParam = p.AudioParam;
    DataParam = p.DataParam;
	return *this;
}

int CCoreParameter::GetStreamLen(const int iStreamID)
{
	if(iStreamID != STREAM_ID_NOT_USED)
		return MSCParameters.Stream[iStreamID].iLenPartA + MSCParameters.Stream[iStreamID].iLenPartB;
	else
		return 0;
}

CParameter::CParameter():CCoreParameter(),
 eModulation(NONE),
 iAMSSCarrierMode(0),
 sReceiverID("                "),
 sSerialNumber(),
 sDataFilesDirectory("."),
 NextConfig(),
 FACParameters(),
 Service(MAX_NUM_SERVICES),
 bMuxHasAFS(false),
 ServiceInformation(),
 iNumBitsHierarchFrameTotal(0),
 iNumDecodedBitsMSC(0),
 iNumSDCBitsPerSuperFrame(0),
 iYear(0),
 iMonth(0),
 iDay(0),
 iUTCHour(0),
 iUTCMin(0),
 rFreqOffsetAcqui(0.0),
 rFreqOffsetTrack(0.0),
 rResampleOffset(0.0),
 iTimingOffsTrack(0),
 eAcquiState(AS_NO_SIGNAL),
 iNumAudioFrames(0),
 RawSimDa(),
 eSimType(ST_NONE),
 iDRMChannelNum(0),
 iSpecChDoppler(0),
 rBitErrRate(0.0),
 rSyncTestParam(0.0),
 rSINR(0.0),
 iNumBitErrors(0),
 iChanEstDelay(0),
 iNumTaps(0),
 iPathDelay(MAX_NUM_TAPS_DRM_CHAN),
 rGainCorr(0.0),
 iOffUsfExtr(0),
 ReceiveStatus(),
 FrontEndParameters(),
 CellMappingTable(),
 GPSData(), Measurements(),
 RxEvent(None),
 rSysSimSNRdB(0.0),
 iFrequency(0),
 iCurSelAudioService(0),
 iCurSelDataService(0),
 LastAudioService(),
 LastDataService()
#ifdef QT_CORE_LIB
 ,Mutex()
#endif
{
	CellMappingTable.MakeTable(Channel.eRobustness, Channel.eSpectrumOccupancy);
}

CParameter::~CParameter()
{
}

CParameter::CParameter(const CParameter& p):
 CCoreParameter(p),  eModulation(p.eModulation),
 iAMSSCarrierMode(p.iAMSSCarrierMode),
 sReceiverID(p.sReceiverID),
 sSerialNumber(p.sSerialNumber),
 sDataFilesDirectory(p.sDataFilesDirectory),
 NextConfig(p.NextConfig),
 FACParameters(p.FACParameters),
 Service(p.Service),
 bMuxHasAFS(p.bMuxHasAFS),
 ServiceInformation(p.ServiceInformation),
 iNumBitsHierarchFrameTotal(p.iNumBitsHierarchFrameTotal),
 iNumDecodedBitsMSC(p.iNumDecodedBitsMSC),
 iNumSDCBitsPerSuperFrame(p.iNumSDCBitsPerSuperFrame),
 iYear(p.iYear), iMonth(p.iMonth), iDay(p.iDay),
 iUTCHour(p.iUTCHour), iUTCMin(p.iUTCMin),
 rFreqOffsetAcqui(p.rFreqOffsetAcqui),
 rFreqOffsetTrack(p.rFreqOffsetTrack),
 rResampleOffset(p.rResampleOffset),
 iTimingOffsTrack(p.iTimingOffsTrack),
 eAcquiState(p.eAcquiState),
 iNumAudioFrames(p.iNumAudioFrames),
 RawSimDa(p.RawSimDa),
 eSimType(p.eSimType),
 iDRMChannelNum(p.iDRMChannelNum),
 iSpecChDoppler(p.iSpecChDoppler),
 rBitErrRate(p.rBitErrRate),
 rSyncTestParam	(p.rSyncTestParam),
 rSINR(p.rSINR),
 iNumBitErrors(p.iNumBitErrors),
 iChanEstDelay(p.iChanEstDelay),
 iNumTaps(p.iNumTaps),
 iPathDelay(p.iPathDelay),
 rGainCorr(p.rGainCorr),
 iOffUsfExtr(p.iOffUsfExtr),
 ReceiveStatus(p.ReceiveStatus),
 FrontEndParameters(p.FrontEndParameters),
 CellMappingTable(), // jfbc CCellMappingTable uses a CMatrix :(
 GPSData(p.GPSData), Measurements(p.Measurements),  RxEvent(p.RxEvent),
 rSysSimSNRdB(p.rSysSimSNRdB),
 iFrequency(p.iFrequency),
 iCurSelAudioService(p.iCurSelAudioService),
 iCurSelDataService(p.iCurSelDataService),
 LastAudioService(p.LastAudioService),
 LastDataService(p.LastDataService)
 //, Mutex() // jfbc: I don't think this state should be copied
{
	CellMappingTable.MakeTable(Channel.eRobustness, Channel.eSpectrumOccupancy);
}

CParameter& CParameter::operator=(const CParameter& p)
{
    eModulation = p.eModulation;
	Channel = p.Channel;
	iAMSSCarrierMode = p.iAMSSCarrierMode;
	sReceiverID = p.sReceiverID;
	sSerialNumber = p.sSerialNumber;
	sDataFilesDirectory = p.sDataFilesDirectory;
    NextConfig = p.NextConfig;
    FACParameters = p.FACParameters;
    Service = p.Service;
    bMuxHasAFS = p.bMuxHasAFS;
    ServiceInformation = p.ServiceInformation;
	iNumBitsHierarchFrameTotal = p.iNumBitsHierarchFrameTotal;
	iNumDecodedBitsMSC = p.iNumDecodedBitsMSC;
	iNumSDCBitsPerSuperFrame = p.iNumSDCBitsPerSuperFrame;
	iYear = p.iYear;
	iMonth = p.iMonth;
	iDay = p.iDay;
	iUTCHour = p.iUTCHour;
	iUTCMin = p.iUTCMin;
	rFreqOffsetAcqui = p.rFreqOffsetAcqui;
	rFreqOffsetTrack = p.rFreqOffsetTrack;
	rResampleOffset = p.rResampleOffset;
	iTimingOffsTrack = p.iTimingOffsTrack;
	eAcquiState = p.eAcquiState;
	iNumAudioFrames = p.iNumAudioFrames;
	RawSimDa = p.RawSimDa;
	eSimType = p.eSimType;
	iDRMChannelNum = p.iDRMChannelNum;
	iSpecChDoppler = p.iSpecChDoppler;
	rBitErrRate = p.rBitErrRate;
	rSyncTestParam	 = p.rSyncTestParam;
	rSINR = p.rSINR;
	iNumBitErrors = p.iNumBitErrors;
	iChanEstDelay = p.iChanEstDelay;
	iNumTaps = p.iNumTaps;
	iPathDelay = p.iPathDelay;
	rGainCorr = p.rGainCorr;
	iOffUsfExtr = p.iOffUsfExtr;
	ReceiveStatus = p.ReceiveStatus;
	FrontEndParameters = p.FrontEndParameters;
	CellMappingTable.MakeTable(Channel.eRobustness, Channel.eSpectrumOccupancy);
	GPSData = p.GPSData;
	Measurements = p.Measurements;
	RxEvent = p.RxEvent;
	rSysSimSNRdB = p.rSysSimSNRdB;
	iFrequency = p.iFrequency;
	iCurSelAudioService = p.iCurSelAudioService;
	iCurSelDataService = p.iCurSelDataService;
	LastAudioService = p.LastAudioService;
	LastDataService = p.LastDataService;

	return *this;
}

void CParameter::ResetServicesStreams()
{
	int i;
	if(eModulation == DRM)
	{

		/* Store information about last services selected
		 * in order to reselect current service automatically after a resync */

		if (iCurSelAudioService > 0)
			LastAudioService.Save(iCurSelAudioService, Service[iCurSelAudioService].iServiceID);

		if (iCurSelDataService > 0)
			LastDataService.Save(iCurSelDataService, Service[iCurSelDataService].iServiceID);

		/* Reset everything to possible start values */
		for (i = 0; i < MAX_NUM_SERVICES; i++)
		{
			Service[i].iAudioStream = STREAM_ID_NOT_USED;
			Service[i].iDataStream = STREAM_ID_NOT_USED;
			Service[i].iPacketID = 0;

			Service[i].iServiceID = SERV_ID_NOT_USED;
			Service[i].eCAIndication = CService::CA_NOT_USED;
			Service[i].iLanguage = 0;
			Service[i].strCountryCode = "";
			Service[i].strLanguageCode = "";
			Service[i].eAudDataFlag = SF_AUDIO;
			Service[i].iServiceDescr = 0;
			Service[i].strLabel = "";
		}
		// let the SDC configure this
		AudioParam.clear();
		DataParam.clear();
		MSCParameters.Stream.clear(); // lose old information
		/* allow index by streamID */
		MSCParameters.Stream.resize(MAX_NUM_STREAMS);
		for (i = 0; i < MAX_NUM_STREAMS; i++)
		{
			MSCParameters.Stream[i].iLenPartA = 0;
			MSCParameters.Stream[i].iLenPartB = 0;
		}
	}
	else
	{

		// Set up encoded AM audio parameters
		AudioParam.clear(); // lose old data
		DataParam.clear(); // no data apps
		AudioParam[0].strTextMessage = "";
		AudioParam[0].eAudioCoding = CAudioParam::AC_AAC;
		AudioParam[0].eSBRFlag = CAudioParam::SB_NOT_USED;
		AudioParam[0].eAudioSamplRate = CAudioParam::AS_24KHZ;
		AudioParam[0].bTextflag = false;
		AudioParam[0].bEnhanceFlag = false;
		AudioParam[0].eAudioMode = CAudioParam::AM_MONO;
		AudioParam[0].iCELPIndex = 0;
		AudioParam[0].bCELPCRC = false;
		AudioParam[0].eHVXCRate = CAudioParam::HR_2_KBIT;
		AudioParam[0].bHVXCCRC = false;

		Service[0].iServiceID = SERV_ID_NOT_USED;
		Service[0].eCAIndication = CService::CA_NOT_USED;
		Service[0].iLanguage = 0;
		Service[0].strCountryCode = "";
		Service[0].strLanguageCode = "";
		Service[0].eAudDataFlag = SF_AUDIO;
		Service[0].iServiceDescr = 0;
		Service[0].strLabel = "";

		MSCParameters.Stream[0].iLenPartA = 0;
		MSCParameters.Stream[0].iLenPartB = 1044;
	}

	/* Reset if mux has alternative frequencies */
	bMuxHasAFS = false;

	/* Date, time */
	iDay = 0;
	iMonth = 0;
	iYear = 0;
	iUTCHour = 0;
	iUTCMin = 0;
}

void CParameter::GetActiveServices(set<int>& actServ)
{
	/* Init return vector */
	actServ.clear();

	/* Get active services */
	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
			/* A service is active, add ID to set */
			actServ.insert(i);
	}
}

/* using a set ensures each stream appears only once */
void CParameter::GetActiveStreams(set<int>& actStr)
{
	actStr.clear();

	/* Determine which streams are active */
	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
		{
			/* Audio stream */
			if (Service[i].iAudioStream != STREAM_ID_NOT_USED)
				actStr.insert(Service[i].iAudioStream);

			/* Data stream */
			if (Service[i].iDataStream != STREAM_ID_NOT_USED)
				actStr.insert(Service[i].iDataStream);
		}
	}
}

_REAL CParameter::GetBitRateKbps(const int iShortID, const bool bAudData)
{
	/* Init lengths to zero in case the stream is not yet assigned */
	int iLen = 0;

	/* First, check if audio or data service and get lengths */
	if (Service[iShortID].eAudDataFlag == SF_AUDIO)
	{
		/* Check if we want to get the data stream connected to an audio
		   stream */
		if (bAudData == true)
		{
			iLen = GetStreamLen( Service[iShortID].iDataStream);
		}
		else
		{
			iLen = GetStreamLen( Service[iShortID].iAudioStream);
		}
	}
	else
	{
		iLen = GetStreamLen( Service[iShortID].iDataStream);
	}

	/* We have 3 frames with time duration of 1.2 seconds. Bit rate should be
	   returned in kbps (/ 1000) */
	return (_REAL) iLen * BITS_BINARY * 3 / (_REAL) 1.2 / 1000;
}

_REAL CParameter::PartABLenRatio(const int iShortID)
{
	int iLenA = 0;
	int iLenB = 0;

	/* Get the length of protection part A and B */
	if (Service[iShortID].eAudDataFlag == SF_AUDIO)
	{
		/* Audio service */
		if (Service[iShortID].iAudioStream != STREAM_ID_NOT_USED)
		{
			iLenA = MSCParameters.Stream[Service[iShortID].iAudioStream].iLenPartA;
			iLenB = MSCParameters.Stream[Service[iShortID].iAudioStream].iLenPartB;
		}
	}
	else
	{
		/* Data service */
		if (Service[iShortID].iDataStream != STREAM_ID_NOT_USED)
		{
			iLenA = MSCParameters.Stream[Service[iShortID].iDataStream].iLenPartA;
			iLenB = MSCParameters.Stream[Service[iShortID].iDataStream].iLenPartB;
		}
	}

	const int iTotLen = iLenA + iLenB;

	if (iTotLen != 0)
		return (_REAL) iLenA / iTotLen;
	else
		return (_REAL) 0.0;
}

void CParameter::SetCurSelAudioService(int iShortID)
{
	/* Change the current selected audio service ID only if the new ID does
	   contain an audio service. If not, keep the old ID. In that case it is
	   possible to select a "data-only" service and still listen to the audio of
	   the last selected service */
	if ((iCurSelAudioService != iShortID) &&
		(Service[iShortID].iAudioStream != STREAM_ID_NOT_USED))
	{
		iCurSelAudioService = iShortID;

		LastAudioService.Reset();

		RxEvent = SelectAudioComponent;
	}
}

void CParameter::SetCurSelDataService(int iShortID)
{
	/* Change the current selected data service ID only if the new ID does
	   contain a data service. If not, keep the old ID. In that case it is
	   possible to select a "data-only" service and click back to an audio
	   service to be able to decode data service and listen to audio at the
	   same time */
	if ((iCurSelDataService != iShortID) &&
		(Service[iShortID].iDataStream != STREAM_ID_NOT_USED))
	{
		iCurSelDataService = iShortID;

		LastDataService.Reset();

		RxEvent = SelectDataComponent;
	}
}

void CParameter::SetServiceID(const int iShortID, const uint32_t iNewServiceID)
{
	if (Service[iShortID].iServiceID != iNewServiceID)
	{
		/* JFBC - what is this for? */
		if ((iShortID == 0) && (Service[0].iServiceID > 0))
			ResetServicesStreams();

		Service[iShortID].iServiceID = iNewServiceID;

		/* If the receiver has lost the sync automatically restore
			last current service selected */

		if(LastAudioService.iServiceID == iNewServiceID)
		{
		    SetCurSelAudioService(iShortID);
		}

		if (LastDataService.iServiceID == iNewServiceID)
		{
		    SetCurSelDataService(iShortID);
		}
	}
}


/* Implementaions for simulation -------------------------------------------- */
void CRawSimData::Add(uint32_t iNewSRS)
{
	/* Attention, function does not take care of overruns, data will be
	   lost if added to a filled shift register! */
	if (iCurWritePos < ciMaxDelBlocks)
		veciShRegSt[iCurWritePos++] = iNewSRS;
}

uint32_t CRawSimData::Get()
{
	/* We always use the first value of the array for reading and do a
	   shift of the other data by adding a arbitrary value (0) at the
	   end of the whole shift register */
	uint32_t iRet = veciShRegSt[0];
	veciShRegSt.AddEnd(0);
	iCurWritePos--;

	return iRet;
}

_REAL CParameter::GetSysSNRdBPilPos() const
{
/*
	Get system SNR in dB for the pilot positions. Since the average power of
	the pilots is higher than the data cells, the SNR is also higher at these
	positions compared to the total SNR of the DRM signal.
*/
	return (_REAL) 10.0 * log10(pow((_REAL) 10.0, rSysSimSNRdB / 10) /
		CellMappingTable.rAvPowPerSymbol * CellMappingTable.rAvScatPilPow * (_REAL) CellMappingTable.iNumCarrier);
}

void
CParameter::SetSNR(const _REAL iNewSNR)
{
	Measurements.SNRstat.addSample(iNewSNR);
}

_REAL
CParameter::GetSNR()
{
	return 	Measurements.SNRstat.getCurrent();
}

_REAL CParameter::GetNominalSNRdB()
{
	/* Convert SNR from system bandwidth to nominal bandwidth */
	return (_REAL) 10.0 * log10(pow((_REAL) 10.0, rSysSimSNRdB / 10) *
		GetSysToNomBWCorrFact());
}

void CParameter::SetNominalSNRdB(const _REAL rSNRdBNominal)
{
	/* Convert SNR from nominal bandwidth to system bandwidth */
	rSysSimSNRdB = (_REAL) 10.0 * log10(pow((_REAL) 10.0, rSNRdBNominal / 10) /
		GetSysToNomBWCorrFact());
}

_REAL CParameter::GetNominalBandwidth()
{
	_REAL rNomBW;

	/* Nominal bandwidth as defined in the DRM standard */
	switch (Channel.eSpectrumOccupancy)
	{
	case SO_0:
		rNomBW = (_REAL) 4500.0; /* Hz */
		break;

	case SO_1:
		rNomBW = (_REAL) 5000.0; /* Hz */
		break;

	case SO_2:
		rNomBW = (_REAL) 9000.0; /* Hz */
		break;

	case SO_3:
		rNomBW = (_REAL) 10000.0; /* Hz */
		break;

	case SO_4:
		rNomBW = (_REAL) 18000.0; /* Hz */
		break;

	case SO_5:
		rNomBW = (_REAL) 20000.0; /* Hz */
		break;

	default:
		rNomBW = (_REAL) 10000.0; /* Hz */
		break;
	}

	return rNomBW;
}

_REAL CParameter::GetSysToNomBWCorrFact()
{
	_REAL rNomBW = GetNominalBandwidth();

	/* Calculate system bandwidth (N / T_u) */
	const _REAL rSysBW = (_REAL) CellMappingTable.iNumCarrier / CellMappingTable.iFFTSizeN * SOUNDCRD_SAMPLE_RATE;

	return rSysBW / rNomBW;
}


void CParameter::SetIFSignalLevel(_REAL rNewSigStr)
{
		Measurements.IFSigStr.set(rNewSigStr);
}

_REAL CParameter::GetIFSignalLevel()
{
	_REAL val=0.0;
	(void)Measurements.IFSigStr.get(val);
	return val;
}

void CRxStatus::SetStatus(const ETypeRxStatus OK)
{
	status = OK;
	iNum++;
	if(OK==RX_OK)
		iNumOK++;
}

void CParameter::GenerateReceiverID()
{
	//Set receiver ID
	string sVer;
	unsigned int iImplementation = 0;;
	unsigned int iMajor = 0;
	unsigned int iMinor = 0;

	sReceiverID = "drea";

	sVer = dream_version;

	size_t pos;

	while((pos = sVer.find('.')) != string::npos)
		sVer.replace(pos, 1, " ");

	if ((pos = sVer.find("cvs")) != string::npos)
		sVer.replace(pos, 3, "   ");

	stringstream ssVer(sVer);
	ssVer >> iImplementation >> iMajor >> iMinor;

	stringstream ssInfoVer;
	ssInfoVer << setw(2) << setfill('0') << iImplementation << setw(2) << setfill('0') << iMajor << setw(2) << setfill('0') << iMinor;

	sReceiverID += ssInfoVer.str();

	while (sSerialNumber.length() < 6)
			sSerialNumber += "_";

	if (sSerialNumber.length() > 6)
        sSerialNumber = sSerialNumber.substr(0, 6);

	sReceiverID += sSerialNumber;
}

void CParameter::GenerateRandomSerialNumber()
{
	//seed random number generator
	srand((unsigned int)time(0));

	char randomChars[36];

	for (size_t q=0; q < 36; q++)
	{
		if (q < 26)
			randomChars[q] = char(q)+97;
		else
			randomChars[q] = (char(q)-26)+48;
	}

    sSerialNumber = "______";
	for (size_t i=0; i < 6; i++)
		sSerialNumber[i] = randomChars[(int) 35.0*rand()/RAND_MAX];
}

string CServiceDefinition::Frequency(size_t n) const
{
	if(n>=veciFrequencies.size())
		return ""; // not in the list

	stringstream ss;
	int iFrequency = veciFrequencies[n];

	switch (iSystemID)
	{
	case 0:
	case 1:
	case 2:
		/* AM or DRM */
		ss << iFrequency;
		break;

	case 3:
	case 4:
	case 5:
		/* 'FM1 frequency' - 87.5 to 107.9 MHz (100 kHz steps) */
		ss << 87.5 + 0.1 * float(iFrequency);
		break;

	case 6:
	case 7:
	case 8:
		/* 'FM2 frequency'- 76.0 to 90.0 MHz (100 kHz steps) */
		ss << 76.0 + 0.1 * float(iFrequency);
		break;

	case 9:
	case 10:
	case 11:
		if(iFrequency<=11) {
			int chan = iFrequency / 4;
			char subchan = 'A' + iFrequency % 4;
			ss << "Band I channel " << (chan+2) << subchan;
		} else if(64<= iFrequency && iFrequency <=95) {
			int chan = iFrequency / 4;
			char subchan = 'A' + iFrequency % 4;
			ss << "Band III channel " << (chan-11) << subchan;
		} else if(96<= iFrequency && iFrequency <=101) {
			int chan = iFrequency / 6;
			char subchan = 'A' + iFrequency % 6;
			ss << "Band III+ channel " << (chan-3) << subchan;
		} else if(128<= iFrequency && iFrequency <=143) {
			char chan = iFrequency - 128;
			double m = 1452.96+1.712*double(chan);
			ss << "European L-Band channel L" << ('A'+chan) << ", " << m << " MHz";
		} else if(160<= iFrequency && iFrequency <=182) {
			int chan = iFrequency - 159;
            double m = 1451.072+1.744*double(chan);
			ss << "Canadian L-Band channel " << chan << ", " << m << " MHz";
		} else {
			ss << "unknown channel " << iFrequency;
		}
		break;
	default:
		break;
	}
	return ss.str();
}

string CServiceDefinition::FrequencyUnits() const
{
	switch (iSystemID)
	{
	case 0:
	case 1:
	case 2:
		return "kHz";
		break;

	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		return "MHz";
		break;

	default:
		return "";
		break;
	}
}

string CServiceDefinition::System() const
{
	switch (iSystemID)
	{
	case 0:
		return "DRM";
		break;

	case 1:
	case 2:
		return "AM";
		break;

	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		return "FM";
		break;
	case 9:
	case 10:
	case 11:
		return "DAB";
		break;

	default:
		return "";
		break;
	}
}

CFrontEndParameters::CFrontEndParameters():CDumpable(),
eSMeterCorrectionType(S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY), rSMeterBandwidth(10000.0),
rDefaultMeasurementBandwidth(10000.0), bAutoMeasurementBandwidth(true), rCalFactor(0.0),
rIFCentreFreq(12000.0)
{
}

ostream& operator<<(ostream& out, const CDumpable& d)
{
    d.dump(out);
    return out;
}

template<typename T>
void dump(ostream& out, T val)
{
    out << val;
}

template<typename T>
void dump(ostream& out, const vector<T>& vec)
{
    string sep = "";
    out << "[";
    for (size_t i = 0; i < vec.size(); i++)
    {
        out << sep; ::dump(out, vec[i]);
        sep = ", ";
    }
    out << "]";
}

template<typename K, typename V>
void dump(ostream& out, const map<K,V>& val)
{
    string sep = "";
    out << "{";
    for(typename map<K,V>::const_iterator i = val.begin(); i !=val.end(); i++)
    {
        out << sep; ::dump(out, i->first); out << ": "; ::dump(out, i->second);
        sep = ", ";
    }
    out << "}";
}

//for_each(a.begin(); a.end(); dump);

template<typename T>
void dump(ostream& out, const CVector<T>& vec)
{
    out << "[ " << endl;
    string sep = "";
    for(int i=0; i<vec.Size(); i++)
    {
//        out << sep; ::dump(out, vec[i]); out << endl;
        out << sep << vec[i] << endl;
        sep = ", ";
    }
    out << "]" << endl;
}

void
CChannel::dump(ostream& out) const
{
    // TODO
    out << "{ EnhancementLayerInUse: " << bEnhancementLayerInUse << endl;
    out << "Robustness: " << eRobustness << endl;
    out << "SpectrumOccupancy: " << eSpectrumOccupancy << endl;
    out << "InterleaverDepth: " << eInterleaverDepth << endl;
    out << "MSCmode: " << eMSCmode << endl;
    out << "SDCmode: " << eSDCmode << endl;
}

void CFACParameters::dump(ostream& out) const
{
    out << "{ iFrameId: " << iFrameId << "," << endl;
    out << "bAFSindexValid: " << bAFSindexValid << "," << endl;
    out << "iReconfigurationIndex: " << iReconfigurationIndex << "," << endl;
    out << "NumAudioServices: " << iNumAudioServices << "," << endl;
    out << "NumDataServices: " << iNumDataServices << "}" << endl;
}

void CMSCParameters::dump(ostream& out) const
{
    out << "ProtectionLevel: "; ProtectionLevel.dump(out); out << "," << endl;
    out << "Stream: "; ::dump(out, Stream); out << "," << endl;
}

void
CAudioParam::dump(ostream& out) const
{
    out << "{ textmessage: '" << strTextMessage << "'," << endl;
    out << "audioCoding: " << int(eAudioCoding) << "," << endl;
    out << "SBRFlag: " << int(eSBRFlag) << "," << endl;
    out << "SampleRate: " << int(eAudioSamplRate) << "," << endl;
    out << "Textflag: " << int(bTextflag) << "," << endl;
    out << "EnhanceFlag: " << int(bEnhanceFlag) << "," << endl;
    out << "AudioMode: " << int(eAudioMode) << "," << endl;
    out << "CELPIndex: " << int(iCELPIndex) << "," << endl;
    out << "CELPCRC: " << int(bCELPCRC) << "," << endl;
    out << "HVXCRate: " << int(eHVXCRate) << "," << endl;
    out << "HVXCCRC: " << int(bHVXCCRC) << "}" << endl;
}

void
CDataParam::dump(ostream& out) const
{
    out << "{ DataUnits: " << int(eDataUnitInd) << "," << endl;
    out << "AppDomain: " << int(eAppDomain) << "," << endl;
    out << "UserAppIdent: " << "0x" << hex << int(eUserAppIdent) << endl;
    out << "ApplicationData: ";
    for(size_t i=0; i<applicationData.size(); i++)
        out << setfill('0') << setw(2) << int(applicationData[i]);
    out << dec << "}" << endl;
}

void
CService::dump(ostream& out) const
{
    out << "{ ServiceID: " << iServiceID << "," << endl;
    out << "CAIndication: " << int(eCAIndication) << "," << endl;
    out << "Language: " << iLanguage << "," << endl;
    out << "AudDataFlag: " << int(eAudDataFlag) << "," << endl;
    out << "ServiceDescr: " << iServiceDescr << "," << endl;
    out << "CountryCode: '" << strCountryCode << "'," << endl;
    out << "LanguageCode: '" << strLanguageCode << "'," << endl;
    out << "Label: '" << strLabel << "'," << endl;
    out << "AudioStream: " << iAudioStream << "," << endl;
    out << "DataStream: " << iDataStream << "," << endl;
    out << "PacketID: " << iPacketID << "}" << endl;
}

void
CStream::dump(ostream& out) const
{
    out << "{ iLenPartA: " << iLenPartA << "," << endl;
    out << "iLenPartB: " << iLenPartB << "," << endl;
    out << "AudDataFlag: " << int(eAudDataFlag) << "," << endl;
    out << "PacketMode: " << int(ePacketModInd) << "," << endl;
    out << "PacketLen: " << iPacketLen << "}" << endl;
}

void
CMSCProtLev::dump(ostream& out) const
{
    out << "{ iPartA: " << iPartA << "," << endl;
    out << "iPartB: " << iPartB << "," << endl;
    out << "Hierarch: " << iHierarch << "}" << endl;
}

void
CLastService::dump(ostream& out) const
{
    out << "{ Service: " << iService << ", ServiceID: " << iServiceID << "}" << endl;
}

void
CRxStatus::dump(ostream& out) const
{
    out << "{ status: " << int(status) << ", Num: " << iNum << ", NumOK: " << iNumOK << "}";
}

void
CReceiveStatus::dump(ostream& out) const
{
    out << "{ " << endl;
    out << " Interface: "; Interface.dump(out); out << ", " << endl;
    out << " FSync: "; FSync.dump(out); out << ", " << endl;
    out << " TSync: "; TSync.dump(out); out << ", " << endl;
    out << " FAC: "; FAC.dump(out); out << ", " << endl;
    out << " SDC: "; SDC.dump(out); out << ", " << endl;
    out << " Audio "; Audio.dump(out); out << ", " << endl;
    out << " LLAudio: "; LLAudio.dump(out); out << ", " << endl;
    out << " MOT: "; MOT.dump(out);
    out << "}" << endl;
}

void
CFrontEndParameters::dump(ostream& out) const
{
    out << "{ SMeterCorrectionType: " << int(eSMeterCorrectionType) << "," << endl;
    out << "SMeterBandwidth: " << rSMeterBandwidth << "," << endl;
    out << "DefaultMeasurementBandwidth: " << rDefaultMeasurementBandwidth << "," << endl;
    out << "AutoMeasurementBandwidth: " << bAutoMeasurementBandwidth << "," << endl;
    out << "CalFactor: " << rCalFactor << "," << endl;
    out << "IFCentreFreq: " << rIFCentreFreq << "}" << endl;
}

void
CCoreParameter::dump(ostream& out) const
{
    out << "{ " << endl;
    out << "Channel: "; Channel.dump(out);
     out << "MSC: "; MSCParameters.dump(out);
    out << "AudioParam: "; ::dump(out, AudioParam); out << "," << endl;
    out << "DataParam: "; ::dump(out, DataParam);
    out << "}" << endl;
}

void
CParameter::dump(ostream& out) const
{
    out << "{ " << endl;
    CCoreParameter::dump(out);
    out << "AMSSCarrierMode: " <<  iAMSSCarrierMode << "," << endl;
    out << "ReceiverID: '" <<  sReceiverID << "'," << endl;
    out << "SerialNumber '" <<  sSerialNumber << "'," << endl;
    out << "DataFilesDirectory: '" <<  sDataFilesDirectory << "'," << endl;
    out << "NextConfig: "; NextConfig.dump(out); out << ", " << endl;
    out << "FACParameters: "; FACParameters.dump(out); out << ", " << endl;
    out << "Service: "; ::dump(out, Service); out << ", " << endl;
    out << "MuxHasAFS: " << bMuxHasAFS << endl;
    out << "ServiceInformation: {" << endl;
    string sep = "";
    for(map<uint32_t,CServiceInformation>::const_iterator i=ServiceInformation.begin();
        i!= ServiceInformation.end(); i++)
    {
        out << sep << i->first << ": {"; i->second.dump(out); out << "}";
        sep = ", ";
    }
    out << "}" << endl;
    out << "NumBitsHierarchFrameTotal: " <<  iNumBitsHierarchFrameTotal << "," << endl;
    out << "NumDecodedBitsMSC: " <<  iNumDecodedBitsMSC << "," << endl;
    out << "NumSDCBitsPerSuperFrame: " <<  iNumSDCBitsPerSuperFrame << "," << endl;
    out << "Year: " <<  iYear << "," << endl;
    out << "Month: " <<  iMonth << "," << endl;
    out << "Day: " <<  iDay << "," << endl;
    out << "UTCHour: " <<  iUTCHour << "," << endl;
    out << "UTCMin: " <<  iUTCMin << "," << endl;
    out << "FreqOffsetAcqui: " <<  rFreqOffsetAcqui << "," << endl;
    out << "FreqOffsetTrack: " <<  rFreqOffsetTrack << "," << endl;
    out << "ResampleOffset: " <<  rResampleOffset << "," << endl;
    out << "TimingOffsTrack: " <<  iTimingOffsTrack << "," << endl;
    out << "AcquiState: " <<  eAcquiState << "," << endl;
    out << "NumAudioFrames: " <<  iNumAudioFrames << "," << endl;
    out << "CarOffset: " <<  rCarOffset << "," << endl;
    out << "OutputFormat: " <<  int(eOutputFormat) << "," << endl;
    out << "ReceiveStatus: "; ReceiveStatus.dump(out); out << endl;
    out << "FrontEndParameters: "; FrontEndParameters.dump(out); out << endl;
	//CCellMappingTable CellMappingTable;
	//CGPSData GPSData;
    out << "SysSimSNRdB: " <<  rSysSimSNRdB << "," << endl;
    out << "Frequency: " <<  iFrequency << "," << endl;
    out << "CurSelAudioService: " <<  iCurSelAudioService << "," << endl;
    out << "CurSelDataService: " <<  iCurSelDataService << "," << endl;
    out << "LastAudioService: "; LastAudioService.dump(out); out << endl;
    out << "LastDataService: "; LastDataService.dump(out); out << endl;
    out << "}" << endl;
}
