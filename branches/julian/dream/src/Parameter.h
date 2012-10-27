/*****************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2007
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Andrea Russo
 *
 * Description:
 *	See Parameter.cpp
 *
 * 10/01/2007 Andrew Murphy, BBC Research & Development, 2005
 *	- Additions to include additional RSCI related fields
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additions to include AMSS demodulation (Added class
 *    CAltFreqOtherServicesSign)
 *
 * 11/28/2005 Andrea Russo
 *	- Added classes for store alternative frequencies schedules and regions
 *
 *******************************************************************************
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

#if !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "GlobalDefinitions.h"
#include "Measurements.h"
#include "ofdmcellmapping/CellMappingTable.h"
#include <time.h>
#include "GPSData.h"
#include "ServiceInformation.h"
#include <map>
#ifdef QT_CORE_LIB
# include <QThread>
# include <QMutex>
#endif

enum EInChanSel {CS_LEFT_CHAN, CS_RIGHT_CHAN, CS_MIX_CHAN, CS_IQ_POS,
		CS_IQ_NEG, CS_IQ_POS_ZERO, CS_IQ_NEG_ZERO};

enum EType {AT_NO_AGC, AT_SLOW, AT_MEDIUM, AT_FAST};

enum ENoiRedType {NR_OFF, NR_LOW, NR_MEDIUM, NR_HIGH};

enum EAcqStat {AS_NO_SIGNAL, AS_WITH_SIGNAL}; // Acquisition state of receiver

/* CS: Coding Scheme */
enum ECodScheme { CS_1_SM, CS_2_SM, CS_3_SM, CS_3_HMSYM, CS_3_HMMIX };

/* CT: Channel Type */
enum EChanType { CT_MSC, CT_SDC, CT_FAC };

enum ETypeIntFreq { FLINEAR, FDFTFILTER, FWIENER };
enum ETypeIntTime { TLINEAR, TWIENER };
enum ETypeSNREst { SNR_FAC, SNR_PIL };
enum ETypeRxStatus { NOT_PRESENT, CRC_ERROR, DATA_ERROR, RX_OK };
enum ETypeTiSyncTrac {TSENERGY, TSFIRSTPEAK};


	/* RM: Receiver mode (analog or digital demodulation) */

enum EStreamType { SF_AUDIO, SF_DATA };

enum EPackMod { PM_SYNCHRON_STR_MODE, PM_PACKET_MODE }; /* PM: Packet Mode */

enum EModulationType { DRM, AM, USB, LSB, CW, NBFM, WBFM, NONE };

enum EOutFormat {OF_REAL_VAL /* real valued */, OF_IQ_POS,
		OF_IQ_NEG /* I / Q */, OF_EP /* envelope / phase */};

enum EAppType
{ AT_NOT_SUP = 0,
  AT_MOTSLISHOW = 2,
  AT_JOURNALINE = 0x44A,
  AT_MOTBROADCASTWEBSITE = 3,
  AT_MOTTPEG = 4,
  AT_DGPS = 5,
  AT_TMC = 6,
  AT_MOTEPG = 7,
  AT_JAVA = 8
};

/* SI: Symbol Interleaver */
enum ESymIntMod { SI_LONG, SI_SHORT };

enum ERxEvent {
    ServiceReconfiguration, ChannelReconfiguration, Tune, Reinitialise,
    ReLoadSettings, SelectAudioComponent, SelectDataComponent, None
};

/* AMSS status */
enum EAMSSBlockLockStat {
	NO_SYNC, RE_SYNC, DEF_SYNC, DEF_SYNC_BUT_DATA_CHANGED, POSSIBLE_LOSS_OF_SYNC
};

/* Classes ********************************************************************/
    class CChannel : public CDumpable
    {
    public:
	CChannel();
	virtual ~CChannel() {}
		void dump(ostream&) const;

	bool bEnhancementLayerInUse;
	ERobMode eRobustness;
	ESpecOcc eSpectrumOccupancy;
	ESymIntMod eInterleaverDepth;
	ECodScheme eMSCmode, eSDCmode;
     };

	class CMSCProtLev : public CDumpable
	{
	  public:

		CMSCProtLev():CDumpable(),iPartA(0),iPartB(0),iHierarch(0) {}
		bool operator==(const CMSCProtLev& p) const
		{
		    if(iPartA!=p.iPartA)
		return false;
		    if(iPartB!=p.iPartB)
		return false;
		    if(iHierarch!=p.iHierarch)
		return false;
	    return true;
		}
		bool operator!=(const CMSCProtLev &other) const {return !(*this == other);}

		int iPartA;				/* MSC protection level for part A */
		int iPartB;				/* MSC protection level for part B */
		int iHierarch;			/* MSC protection level for hierachical frame */
		void dump(ostream&) const;
	};

	class CStream : public CDumpable
	{
	  public:

		CStream();
		bool operator==(const CStream&) const;
		bool operator!=(const CStream &other) const {return !(*this == other);}
		void dump(ostream&) const;

		int iLenPartA;			/* Data length for part A */
		int iLenPartB;			/* Data length for part B */
		EStreamType eAudDataFlag; /* stream is audio or data */
		EPackMod ePacketModInd;	/* Packet mode indicator for data streams */
		int iPacketLen;			/* Packet length for packet streams */
	};

    class CMSCParameters : public CDumpable
    {
    public:
	CMSCParameters();
	CMSCParameters(const CMSCParameters&);
	virtual ~CMSCParameters() {}
	CMSCParameters& operator=(const CMSCParameters&);
	bool operator==(const CMSCParameters&) const;
		bool operator!=(const CMSCParameters &other) const {return !(*this == other);}
		void dump(ostream&) const;

	CMSCProtLev ProtectionLevel;
	vector<CStream> Stream;
    };

	class CAudioParam : public CDumpable
	{
	  public:

		/* AC: Audio Coding */
		enum EAudCod { AC_AAC, AC_CELP, AC_HVXC };

		/* SB: SBR */
		enum ESBRFlag { SB_NOT_USED, SB_USED };

		/* AM: Audio Mode */
		enum EAudMode { AM_MONO, AM_P_STEREO, AM_STEREO };

		/* HR: HVXC Rate */
		enum EHVXCRate { HR_2_KBIT, HR_4_KBIT };

		/* AS: Audio Sampling rate */
		enum EAudSamRat { AS_8_KHZ, AS_12KHZ, AS_16KHZ, AS_24KHZ };

	CAudioParam();
	CAudioParam(const CAudioParam&);
	CAudioParam& operator=(const CAudioParam&);
		bool operator==(const CAudioParam&) const;
		bool operator!=(const CAudioParam &other) const {return !(*this == other);}
		void dump(ostream&) const;

		/* Text-message */
		string strTextMessage;	/* Max length is (8 * 16 Bytes) */

		EAudCod eAudioCoding;	/* This field indicated the source coding system */
		ESBRFlag eSBRFlag;		/* SBR flag */
		EAudSamRat eAudioSamplRate;	/* Audio sampling rate */
		bool bTextflag;		/* Indicates whether a text message is present or not */
		bool bEnhanceFlag;	/* Enhancement flag */

		/* For AAC: Mono, LC Stereo, Stereo --------------------------------- */
		EAudMode eAudioMode;	/* Audio mode */

		/* For CELP --------------------------------------------------------- */
		int iCELPIndex;			/* This field indicates the CELP bit rate index */
		bool bCELPCRC;		/* This field indicates whether the CRC is used or not */

		/* For HVXC --------------------------------------------------------- */
		EHVXCRate eHVXCRate;	/* This field indicates the rate of the HVXC */
		bool bHVXCCRC;		/* This field indicates whether the CRC is used or not */

	};

	class CDataParam : public CDumpable
	{
	  public:

		/* DU: Data Unit */
		enum EDatUnit { DU_SINGLE_PACKETS, DU_DATA_UNITS };

		/* AD: Application Domain */
		enum EApplDomain { AD_DRM_SPEC_APP, AD_DAB_SPEC_APP, AD_OTHER_SPEC_APP };

		CDataParam();
		CDataParam(const CDataParam&);
	CDataParam& operator=(const CDataParam&);
	bool operator==(const CDataParam&) const;
		bool operator!=(const CDataParam &other) const {return !(*this == other);}
		void dump(ostream&) const;

		EPackMod ePacketModInd;	/* Packet mode indicator */

		/* In case of packet mode ------------------------------------------- */
		EDatUnit eDataUnitInd;	/* Data unit indicator */

		// "DAB specified application" not yet implemented!!!
		EApplDomain eAppDomain;	    /* Application domain */
		EAppType    eUserAppIdent;	/* User application identifier, only DAB */
		vector<uint8_t> applicationData;

	};

    class CCoreParameter : public CDumpable
    {
    public:
	CCoreParameter();
	CCoreParameter(const CCoreParameter&);
	virtual ~CCoreParameter() {}
	CCoreParameter& operator=(const CCoreParameter&);

	int GetStreamLen(const int iStreamID);

	CChannel Channel;
	CMSCParameters MSCParameters;
	map<int, CAudioParam> AudioParam; /* key with by streamID */
	map<int, map<int, CDataParam> > DataParam; /* first key streamID, second key packetID */
	void dump(ostream&) const;
    };

	class CService : public CDumpable
	{
	  public:

		/* CA: CA system */
		enum ECACond { CA_USED, CA_NOT_USED };


		CService();
		CService(const CService& s);
		virtual ~CService() {}
		CService& operator=(const CService& s);
		void dump(ostream&) const;

		bool IsActive() const
		{
			return iServiceID != SERV_ID_NOT_USED;
		}

		uint32_t iServiceID;
		ECACond eCAIndication;
		int iLanguage;
		EStreamType eAudDataFlag;
		int iServiceDescr;
		string strCountryCode;
		string strLanguageCode;

		/* Label of the service */
		string strLabel;

		/* Audio Component */
		int iAudioStream;

		/* Data Component */
		int iDataStream;
		int iPacketID;
	};

	/* Class to store information about the last service selected ------------- */

	class CLastService : public CDumpable
	{
	  public:
		CLastService():CDumpable(),iService(0), iServiceID(SERV_ID_NOT_USED)
		{
		}
		CLastService(const CLastService& l):
		CDumpable(),iService(l.iService), iServiceID(l.iServiceID)
		{
		}
		CLastService& operator=(const CLastService& l)
		{
			iService = l.iService;
			iServiceID = l.iServiceID;
			return *this;
		}

		void Reset()
		{
			iService = 0;
			iServiceID = SERV_ID_NOT_USED;
		};

		void Save(const int iCurSel, const int iCurServiceID)
		{
			if (iCurServiceID != SERV_ID_NOT_USED)
			{
				iService = iCurSel;
				iServiceID = iCurServiceID;
			}
		};

		/* store only fac parameters */
		int iService;
		uint32_t iServiceID;
		void dump(ostream&) const;
	};

	/* Classes to keep track of status flags for RSCI rsta tag and log file */
	class CRxStatus : public CDumpable
	{
	public:
		CRxStatus():CDumpable(),status(NOT_PRESENT),iNum(0),iNumOK(0) {}
		CRxStatus(const CRxStatus& s):
		CDumpable(),status(s.status),iNum(s.iNum),iNumOK(s.iNumOK) {}
		CRxStatus& operator=(const CRxStatus& s)
			{ status = s.status; iNum = s.iNum; iNumOK = s.iNumOK; return *this;}
		void SetStatus(const ETypeRxStatus);
		ETypeRxStatus GetStatus() { return status; }
		int GetCount() { return iNum; }
		int GetOKCount() { return iNumOK; }
		void ResetCounts() { iNum=0; iNumOK = 0; }
		void dump(ostream&) const;
	private:
		ETypeRxStatus status;
		int iNum, iNumOK;
	};

	class CReceiveStatus : public CDumpable
	{
	  public:
		CReceiveStatus():CDumpable(),FSync(),TSync(),Interface(),
		FAC(),SDC(),Audio(),LLAudio(),MOT()
		{
		}
		CReceiveStatus(const CReceiveStatus& s):
	    CDumpable(),FSync(s.FSync), TSync(s.TSync),
			Interface(s.Interface), FAC(s.FAC), SDC(s.SDC),
			Audio(s.Audio),LLAudio(s.LLAudio),MOT(s.MOT)
		{
		}
		CReceiveStatus& operator=(const CReceiveStatus& s)
		{
			FSync = s.FSync;
			TSync = s.TSync;
			Interface = s.Interface;
			FAC = s.FAC;
			SDC = s.SDC;
			Audio = s.Audio;
			LLAudio = s.LLAudio;
			MOT = s.MOT;
			return *this;
		}

		CRxStatus FSync;
		CRxStatus TSync;
		CRxStatus Interface;
		CRxStatus FAC;
		CRxStatus SDC;
		CRxStatus Audio;
		CRxStatus LLAudio;
		CRxStatus MOT;
		void dump(ostream&) const;
	};


	/* Simulation raw-data management. We have to implement a shift register
	   with varying size. We do that by adding a variable for storing the
	   current write position. */
	class CRawSimData
	{
		/* We have to implement a shift register with varying size. We do that
		   by adding a variable for storing the current write position. We use
		   always the first value of the array for reading and do a shift of the
		   other data by adding a arbitrary value (0) at the end of the whole
		   shift register */
	  public:
		/* Here, the maximal size of the shift register is set */
		CRawSimData():ciMaxDelBlocks(50), iCurWritePos(0)
		{
			veciShRegSt.Init(ciMaxDelBlocks);
		}

		void Add(uint32_t iNewSRS);
		uint32_t Get();

		void Reset()
		{
			iCurWritePos = 0;
		}

	  protected:
		/* Max number of delayed blocks */
		int ciMaxDelBlocks;
		CShiftRegister < uint32_t > veciShRegSt;
		int iCurWritePos;
	};

	class CFrontEndParameters : public CDumpable
	{
	public:
		enum ESMeterCorrectionType {S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY, S_METER_CORRECTION_TYPE_AGC_ONLY, S_METER_CORRECTION_TYPE_AGC_RSSI};

		// Constructor
		CFrontEndParameters();
		void dump(ostream&) const;

		ESMeterCorrectionType eSMeterCorrectionType;
		_REAL rSMeterBandwidth; // The bandwidth the S-meter uses to do the measurement
		_REAL rDefaultMeasurementBandwidth; // Bandwidth to do measurement if not synchronised
		bool bAutoMeasurementBandwidth; // true: use the current FAC bandwidth if locked, false: use default bandwidth always
		_REAL rCalFactor;
		_REAL rIFCentreFreq;

	};


    class CFACParameters : public CDumpable
    {
      public:

	CFACParameters();
	virtual ~CFACParameters() {}
		bool operator==(const CFACParameters&) const;
		void dump(ostream&) const;

	int iFrameId;
	bool bAFSindexValid;
	int iReconfigurationIndex;
	int iNumAudioServices;
	int iNumDataServices;
    };

class CParameter : public CCoreParameter
{
  public:
	CParameter();
	CParameter(const CParameter&);
	virtual ~CParameter();
	CParameter& operator=(const CParameter&);

	/* Enumerations --------------------------------------------------------- */

	/* ST: Simulation Type */
	enum ESimType
	{ ST_NONE, ST_BITERROR, ST_MSECHANEST, ST_BER_IDEALCHAN,
		ST_SYNC_PARAM, ST_SINR
	};

	/* Misc. Functions ------------------------------------------------------ */
	void GenerateRandomSerialNumber();
	void GenerateReceiverID();
	void ResetServicesStreams();
	void GetActiveServices(set<int>& actServ);
	void GetActiveStreams(set<int>& actStr);

	void SetFrequency(int iNewFrequency) { iFrequency = iNewFrequency; }
	int GetFrequency() { return iFrequency; }

	void SetCurSelAudioService(int);
	int GetCurSelAudioService() const { return iCurSelAudioService; }
	void SetCurSelDataService(int);
	int GetCurSelDataService() const { return iCurSelDataService; }

	void ResetCurSelAudDatServ()
	{
		iCurSelAudioService = 0;
		iCurSelDataService = 0;
	}

	_REAL GetDCFrequency() const
	{
		return SOUNDCRD_SAMPLE_RATE * (rFreqOffsetAcqui + rFreqOffsetTrack);
	}

	_REAL GetBitRateKbps(const int iShortID, const bool bAudData);
	_REAL PartABLenRatio(const int iShortID);

	/* Parameters controlled by FAC ----------------------------------------- */

	void SetServiceID(const int iShortID, const uint32_t iNewServiceID);

	EModulationType eModulation;

	/* AMSS */
	int iAMSSCarrierMode;

	/* Serial number and received ID */
	string sReceiverID;
	string sSerialNumber;

	/* Directory for data files */
	string sDataFilesDirectory;

	/* information about the next configuration */
	CCoreParameter NextConfig;

    CFACParameters  FACParameters;

    vector<CService> Service;

	/* information about services gathered from SDC, EPG and web schedules */
	bool bMuxHasAFS;
	map<uint32_t,CServiceInformation> ServiceInformation;

	/* These values are used to set input and output block sizes of some modules */
	int iNumBitsHierarchFrameTotal;
	int iNumDecodedBitsMSC;
	int iNumSDCBitsPerSuperFrame;	/* Number of SDC bits per super-frame */

	/* Date */
	int iYear;
	int iMonth;
	int iDay;

	/* UTC (hours and minutes) */
	int iUTCHour;
	int iUTCMin;

	/* Synchronization ------------------------------------------------------ */
	_REAL rFreqOffsetAcqui;
	_REAL rFreqOffsetTrack;

	_REAL rResampleOffset;

	int iTimingOffsTrack;

	EAcqStat GetAcquiState() { return eAcquiState; }
	EAcqStat eAcquiState;
	int iNumAudioFrames;

	/* For Transmitter */
	_REAL rCarOffset;
	enum EOutFormat eOutputFormat;

	/* Simulation ----------------------------------------------------------- */
	CRawSimData RawSimDa;
	ESimType eSimType;

	int iDRMChannelNum;
	int iSpecChDoppler;
	_REAL rBitErrRate;
	_REAL rSyncTestParam;		/* For any other simulations, used
								   with "ST_SYNC_PARAM" type */
	_REAL rSINR;
	int iNumBitErrors;
	int iChanEstDelay;

	int iNumTaps;
	vector<int> iPathDelay;
	_REAL rGainCorr;
	int iOffUsfExtr;

	void SetSNR(const _REAL);
	_REAL GetSNR();
	void SetNominalSNRdB(const _REAL rSNRdBNominal);
	_REAL GetNominalSNRdB();
	void SetSystemSNRdB(const _REAL rSNRdBSystem)
	{
		rSysSimSNRdB = rSNRdBSystem;
	}
	_REAL GetSystemSNRdB() const
	{
		return rSysSimSNRdB;
	}
	_REAL GetSysSNRdBPilPos() const;

	CReceiveStatus ReceiveStatus;
	CFrontEndParameters FrontEndParameters;

	void Lock()
	{
#ifdef QT_CORE_LIB
		Mutex.lock();
#endif
	}
	void Unlock()
	{
#ifdef QT_CORE_LIB
		Mutex.unlock();
#endif
	}

	/* the signal level as measured at IF by dream */
	void SetIFSignalLevel(_REAL);
	_REAL GetIFSignalLevel();
	_REAL rSigStrengthCorrection;

	_REAL GetNominalBandwidth();
	_REAL GetSysToNomBWCorrFact();

	CCellMappingTable CellMappingTable;

	CGPSData GPSData;

	CMeasurements Measurements;

    ERxEvent RxEvent;

    void dump(ostream&) const;

protected:

	_REAL rSysSimSNRdB;

	int iFrequency;

	/* Current selected audio service for processing */
	int iCurSelAudioService;
	int iCurSelDataService;

	/* For resync to last service------------------------------------------- */
	CLastService LastAudioService;
	CLastService LastDataService;

#ifdef QT_CORE_LIB
	QMutex Mutex;
#endif
};

#endif // !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
