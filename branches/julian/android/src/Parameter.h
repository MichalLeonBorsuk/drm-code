/******************************************************************************\
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
#include "ofdmcellmapping/CellMappingTable.h"
#include "matlib/Matlib.h"
#include <time.h>
#include "ServiceInformation.h"
#include <map>
#include <iostream>
#ifdef HAVE_LIBGPS
# include <gps.h>
#else
    struct gps_fix_t {
	double time;
	double latitude;
	double longitude;
	double altitude;
	double speed;
	double track;
    };
    struct gps_data_t {
	uint32_t set;
#define TIME_SET        0x00000002u
#define LATLON_SET      0x00000008u
#define ALTITUDE_SET    0x00000010u
#define SPEED_SET       0x00000020u
#define TRACK_SET       0x00000040u
#define STATUS_SET      0x00000100u
#define SATELLITE_SET   0x00010000u
	gps_fix_t fix;
	int    status; 
	int satellites_used;
    };
#endif

class CDRMReceiver;

/* SI: Symbol Interleaver */
enum ESymIntMod { SI_LONG, SI_SHORT, SI_MODE_E };

/* CS: Coding Scheme */
enum ECodScheme { CS_1_SM, CS_2_SM, CS_3_SM, CS_3_HMSYM, CS_3_HMMIX };

/* CT: Channel Type */
enum EChanType { CT_MSC, CT_SDC, CT_FAC };

enum ETypeIntFreq
{ FLINEAR, FDFTFILTER, FWIENER };
enum ETypeIntTime
{ TLINEAR, TWIENER };
enum ETypeSNREst
{ SNR_FAC, SNR_PIL };
enum ETypeRxStatus
{ NOT_PRESENT, CRC_ERROR, DATA_ERROR, RX_OK };
/* RM: Receiver mode (analog or digital demodulation) */

enum ERecMode
{ RM_DRM, RM_AM, RM_FM, RM_NONE };

/* Acquisition state of receiver */
enum EAcqStat {AS_NO_SIGNAL, AS_WITH_SIGNAL};

/* Receiver state */
enum ERecState {RS_TRACKING, RS_ACQUISITION};

/* Classes ********************************************************************/

struct CServiceParam
{
    CServiceParam():
        iStreamID(STREAM_ID_NOT_USED),bEnhanceFlag(false),bCA(false),CAdata(),rBitrate(0.0)
    {
    }

    int iStreamID;		    // Stream Id of the stream which carries the audio service
    bool bEnhanceFlag;	    // Enhancement flag
    bool bCA;               // Conditional Access flag;
    vector<uint8_t> CAdata; // Conditional Access Data;
    _REAL rBitrate;
};

class CAudioParam: public CServiceParam
{
public:

    /* AC: Audio Coding */
    enum EAudCod { AC_NONE, AC_AAC, AC_CELP, AC_HVXC, AC_OPUS, AC_xHEAAC };

    /* SB: SBR */
    enum ESBRFlag { SB_NOT_USED, SB_USED };

    /* AM: Audio Mode */
    enum EAudMode { AM_MONO, AM_P_STEREO, AM_STEREO };

    /* HR: HVXC Rate */
    enum EHVXCRate { HR_2_KBIT, HR_4_KBIT };

    /* AS: Audio Sampling rate */
    enum EAudSamRat { AS_8_KHZ, AS_12KHZ, AS_16KHZ, AS_24KHZ, AS_48KHZ };

    /* OB: Opus Audio Bandwidth, coded in audio data stream */
    enum EOPUSBandwidth { OB_NB, OB_MB, OB_WB, OB_SWB, OB_FB };

    /* OS: Opus Audio Sub Codec, coded in audio data stream */
    enum EOPUSSubCod { OS_SILK, OS_HYBRID, OS_CELT };

    /* OC: Opus Audio Channels, coded in audio data stream */
    enum EOPUSChan { OC_MONO, OC_STEREO };

    /* OG: Opus encoder signal type, for encoder only */
    enum EOPUSSignal { OG_VOICE, OG_MUSIC };

    /* OA: Opus encoder intended application, for encoder only */
    enum EOPUSApplication { OA_VOIP, OA_AUDIO };

    CAudioParam(): CServiceParam(), strTextMessage(),
            eAudioCoding(AC_NONE), eSBRFlag(SB_NOT_USED), eAudioSamplRate(AS_24KHZ),
            bTextflag(FALSE), eAudioMode(AM_MONO),
            iCELPIndex(0), bCELPCRC(FALSE), eHVXCRate(HR_2_KBIT), bHVXCCRC(FALSE),
            eOPUSBandwidth(OB_FB), eOPUSSubCod(OS_SILK), eOPUSChan(OC_STEREO),
            eOPUSSignal(OG_MUSIC), eOPUSApplication(OA_AUDIO),
            bOPUSForwardErrorCorrection(FALSE), bOPUSRequestReset(FALSE),
            bParamChanged(FALSE),
            bCanDecode(false),bCanEncode(false)
    {
    }

    /* Text-message */
    string strTextMessage;	/* Max length is (8 * 16 Bytes) */


    EAudCod eAudioCoding;	/* This field indicated the source coding system */
    ESBRFlag eSBRFlag;		/* SBR flag */
    EAudSamRat eAudioSamplRate;	/* Audio sampling rate */
    _BOOLEAN bTextflag;		/* Indicates whether a text message is present or not */

    /* For AAC: Mono, LC Stereo, Stereo --------------------------------- */
    EAudMode eAudioMode;	/* Audio mode */

    /* For CELP --------------------------------------------------------- */
    int iCELPIndex;			/* This field indicates the CELP bit rate index */
    _BOOLEAN bCELPCRC;		/* This field indicates whether the CRC is used or not */

    /* For HVXC --------------------------------------------------------- */
    EHVXCRate eHVXCRate;	/* This field indicates the rate of the HVXC */
    _BOOLEAN bHVXCCRC;		/* This field indicates whether the CRC is used or not */

    /* For OPUS --------------------------------------------------------- */
    EOPUSBandwidth eOPUSBandwidth; /* Audio bandwidth */
    EOPUSSubCod eOPUSSubCod; /* Audio sub codec */
    EOPUSChan eOPUSChan;	/* Audio channels */
    EOPUSSignal eOPUSSignal; /* Encoder signal type */
    EOPUSApplication eOPUSApplication; /* Encoder intended application */
    _BOOLEAN bOPUSForwardErrorCorrection; /* Encoder Forward Error Correction enabled */
    _BOOLEAN bOPUSRequestReset; /* Request encoder reset */

    /* CAudioParam has changed */
    _BOOLEAN bParamChanged;

    bool bCanDecode;
    bool bCanEncode;

    /* This function is needed for detection changes in the class */
    _BOOLEAN operator!=(const CAudioParam AudioParam)
    {
        if (iStreamID != AudioParam.iStreamID)
            return TRUE;
        if (eAudioCoding != AudioParam.eAudioCoding)
            return TRUE;
        if (eSBRFlag != AudioParam.eSBRFlag)
            return TRUE;
        if (eAudioSamplRate != AudioParam.eAudioSamplRate)
            return TRUE;
        if (bTextflag != AudioParam.bTextflag)
            return TRUE;
        if (bEnhanceFlag != AudioParam.bEnhanceFlag)
            return TRUE;

        switch (AudioParam.eAudioCoding)
        {
        case AC_AAC:
            if (eAudioMode != AudioParam.eAudioMode)
                return TRUE;
            break;

        case AC_CELP:
            if (bCELPCRC != AudioParam.bCELPCRC)
                return TRUE;
            if (iCELPIndex != AudioParam.iCELPIndex)
                return TRUE;
            break;

        case AC_HVXC:
            if (eHVXCRate != AudioParam.eHVXCRate)
                return TRUE;
            if (bHVXCCRC != AudioParam.bHVXCCRC)
                return TRUE;
            break;

        case AC_NONE:
        case AC_OPUS:
        case AC_xHEAAC:
            break;
        }
        return FALSE;
    }
};

class CDataParam: public CServiceParam
{
public:

    /* PM: Packet Mode */
    enum EPackMod { PM_SYNCHRON_STR_MODE, PM_PACKET_MODE };

    /* DU: Data Unit */
    enum EDatUnit { DU_SINGLE_PACKETS, DU_DATA_UNITS };

    /* AD: Application Domain */
    enum EApplDomain { AD_DRM_SPEC_APP, AD_DAB_SPEC_APP, AD_OTHER_SPEC_APP };

    EPackMod ePacketModInd;	/* Packet mode indicator */

    /* In case of packet mode ------------------------------------------- */
    EDatUnit eDataUnitInd;	/* Data unit indicator */
    int iPacketID;			/* Packet Id (2 bits) */
    int iPacketLen;			/* Packet length */

    // "DAB specified application" not yet implemented!!!
    EApplDomain eAppDomain;	/* Application domain */
    int iUserAppIdent;		/* User application identifier, only DAB */

    CDataParam():CServiceParam(),
            ePacketModInd(PM_PACKET_MODE),
            eDataUnitInd(DU_DATA_UNITS),
            iPacketID(0),
            iPacketLen(0),
            eAppDomain(AD_DAB_SPEC_APP),
            iUserAppIdent(0)
    {
    }

    /* This function is needed to detect changes in the data service */
    _BOOLEAN operator!=(const CDataParam DataParam)
    {
        if (iStreamID != DataParam.iStreamID)
            return TRUE;
        if (bCA != DataParam.bCA)
            return TRUE;
        if (ePacketModInd != DataParam.ePacketModInd)
            return TRUE;
        if (DataParam.ePacketModInd == PM_PACKET_MODE)
        {
            if (eDataUnitInd != DataParam.eDataUnitInd)
                return TRUE;
            if (iPacketID != DataParam.iPacketID)
                return TRUE;
            if (iPacketLen != DataParam.iPacketLen)
                return TRUE;
            if (eAppDomain != DataParam.eAppDomain)
                return TRUE;
            if (DataParam.eAppDomain == AD_DAB_SPEC_APP)
                if (iUserAppIdent != DataParam.iUserAppIdent)
                    return TRUE;
        }
        return FALSE;
    }
};

class CService
{
public:

    /* SF: Service Flag */
    enum ETyOServ { SF_AUDIO, SF_DATA };

    CService():
            iServiceID(SERV_ID_NOT_USED),
            iLanguage(0), eAudDataFlag(SF_AUDIO), iServiceDescr(0),
            strCountryCode(), strLanguageCode(), strLabel(),
            AudioParam(), DataParam()
    {
    }
    CService(const CService& s):
            iServiceID(s.iServiceID),
            iLanguage(s.iLanguage), eAudDataFlag(s.eAudDataFlag),
            iServiceDescr(s.iServiceDescr), strCountryCode(s.strCountryCode),
            strLanguageCode(s.strLanguageCode), strLabel(s.strLabel),
            AudioParam(s.AudioParam), DataParam(s.DataParam)
    {
    }
    CService& operator=(const CService& s)
    {
        iServiceID = s.iServiceID;
        iLanguage = s.iLanguage;
        eAudDataFlag = s.eAudDataFlag;
        iServiceDescr = s.iServiceDescr;
        strCountryCode = s.strCountryCode;
        strLanguageCode = s.strLanguageCode;
        strLabel = s.strLabel;
        AudioParam = s.AudioParam;
        DataParam = s.DataParam;
        return *this;
    }

    _BOOLEAN IsActive() const
    {
        return iServiceID != SERV_ID_NOT_USED;
    }

    uint32_t iServiceID;
    int iLanguage;
    ETyOServ eAudDataFlag;
    int iServiceDescr;
    string strCountryCode;
    string strLanguageCode;

    /* Label of the service */
    string strLabel;

    /* Audio parameters */
    CAudioParam AudioParam;

    /* Data parameters */
    CDataParam DataParam;
};

class CStream
{
public:

    enum EStreamType { ST_UNKNOWN, ST_AUDIO, ST_DATA_STREAM, ST_DATA_PACKET };

    CStream():iLenPartA(0), iLenPartB(0), eType(ST_UNKNOWN),
        bFEC(false), packet_length(0), R(0), C(0)
    {
    }
    CStream(const CStream& s):iLenPartA(s.iLenPartA), iLenPartB(s.iLenPartB)
    {
    }
#if 0
    CStream& operator=(const CStream& Stream)
    {
        iLenPartA=Stream.iLenPartA;
        iLenPartB=Stream.iLenPartB;
        return *this;
    }

    bool operator==(const CStream Stream)
    {
        if (iLenPartA != Stream.iLenPartA)
            return false;
        if (iLenPartB != Stream.iLenPartB)
            return false;
        return true;
    }
#endif
    int iLenPartA;			/* Data length for part A */
    int iLenPartB;			/* Data length for part B */
    EStreamType eType;
    bool bFEC;
    uint8_t packet_length;
    uint8_t R;
    uint8_t C;
};

class CMSCProtLev
{
public:

    CMSCProtLev():iPartA(0),iPartB(0),iHierarch(0) {}
    CMSCProtLev(const CMSCProtLev& p):iPartA(p.iPartA),iPartB(p.iPartB),iHierarch(p.iHierarch) {}
    CMSCProtLev& operator=(const CMSCProtLev& NewMSCProtLev)
    {
        iPartA = NewMSCProtLev.iPartA;
        iPartB = NewMSCProtLev.iPartB;
        iHierarch = NewMSCProtLev.iHierarch;
        return *this;
    }

    int iPartA;				/* MSC protection level for part A */
    int iPartB;				/* MSC protection level for part B */
    int iHierarch;			/* MSC protection level for hierachical frame */
};

/* Alternative Frequency Signalling ************************************** */
/* Alternative frequency signalling Schedules informations class */
class CAltFreqSched
{
public:
    CAltFreqSched():iDayCode(0),iStartTime(0),iDuration(0)
    {
    }
    CAltFreqSched(const CAltFreqSched& nAFS):
            iDayCode(nAFS.iDayCode), iStartTime(nAFS.iStartTime),
            iDuration(nAFS.iDuration)
    {
    }

    CAltFreqSched& operator=(const CAltFreqSched& nAFS)
    {
        iDayCode = nAFS.iDayCode;
        iStartTime = nAFS.iStartTime;
        iDuration = nAFS.iDuration;

        return *this;
    }

    _BOOLEAN operator==(const CAltFreqSched& nAFS)
    {
        if (iDayCode != nAFS.iDayCode)
            return FALSE;
        if (iStartTime != nAFS.iStartTime)
            return FALSE;
        if (iDuration != nAFS.iDuration)
            return FALSE;

        return TRUE;
    }

    _BOOLEAN IsActive(const time_t ltime);

    int iDayCode;
    int iStartTime;
    int iDuration;
};

/* Alternative frequency signalling Regions informations class */
struct CAltFreqSquare
{
    CAltFreqSquare():iLatitude(0), iLongitude(0),iLatitudeEx(0), iLongitudeEx(0)
    {
    }

    bool operator==(const CAltFreqSquare& s) const
    {
        return (iLatitude == s.iLatitude) && (iLongitude == s.iLongitude)
                && (iLatitudeEx == s.iLatitudeEx) && (iLongitudeEx == s.iLongitudeEx);
    }

    int iLatitude;
    int iLongitude;
    int iLatitudeEx;
    int iLongitudeEx;
};

struct CAltFreqRegion
{
    CAltFreqRegion():veciCIRAFZones(),square()
    {
    }

    bool operator==(const CAltFreqRegion& r) const
    {
        return (veciCIRAFZones == r.veciCIRAFZones) && (square == r.square);
    }

    vector<int> veciCIRAFZones;
    CAltFreqSquare square;
};

struct CAltFreqDetailedRegion
{
    CAltFreqDetailedRegion():squares()
    {
    }

    bool operator==(const CAltFreqDetailedRegion& r) const
    {
        return squares == r.squares;
    }

    vector<CAltFreqSquare> squares;
};

class CServiceDefinition
{
public:
    CServiceDefinition():veciFrequencies(), iRegionID(0), iScheduleID(0),iSystemID(0)
    {
    }

    CServiceDefinition(const CServiceDefinition& nAF):
            veciFrequencies(nAF.veciFrequencies),
            iRegionID(nAF.iRegionID), iScheduleID(nAF.iScheduleID),
            iSystemID(nAF.iSystemID)
    {
    }

    CServiceDefinition& operator=(const CServiceDefinition& nAF)
    {
        veciFrequencies = nAF.veciFrequencies;
        iRegionID = nAF.iRegionID;
        iScheduleID = nAF.iScheduleID;
        iSystemID = nAF.iSystemID;
        return *this;
    }

    bool operator==(const CServiceDefinition& sd) const
    {
        size_t i;

        /* Vector sizes */
        if (veciFrequencies.size() != sd.veciFrequencies.size())
            return FALSE;

        /* Vector contents */
        for (i = 0; i < veciFrequencies.size(); i++)
            if (veciFrequencies[i] != sd.veciFrequencies[i])
                return FALSE;

        if (iRegionID != sd.iRegionID)
            return FALSE;

        if (iScheduleID != sd.iScheduleID)
            return FALSE;

        if (iSystemID != sd.iSystemID)
            return FALSE;

        return TRUE;
    }
    bool operator!=(const CServiceDefinition& sd) const {
        return !(*this==sd);
    }

    string Frequency(size_t) const;
    string FrequencyUnits() const;
    string System() const;

    vector<int> veciFrequencies;
    int iRegionID;
    int iScheduleID;
    int iSystemID;
};

class CMultiplexDefinition: public CServiceDefinition
{
public:
    CMultiplexDefinition():CServiceDefinition(), veciServRestrict(4), bIsSyncMultplx(FALSE)
    {
    }

    CMultiplexDefinition(const CMultiplexDefinition& nAF):CServiceDefinition(nAF),
            veciServRestrict(nAF.veciServRestrict),
            bIsSyncMultplx(nAF.bIsSyncMultplx)
    {
    }

    CMultiplexDefinition& operator=(const CMultiplexDefinition& nAF)
    {
        CServiceDefinition(*this) = nAF;
        veciServRestrict = nAF.veciServRestrict;
        bIsSyncMultplx = nAF.bIsSyncMultplx;
        return *this;
    }

    bool operator==(const CMultiplexDefinition& md) const
    {
        if (CServiceDefinition(*this) != md)
            return FALSE;

        /* Vector sizes */
        if (veciServRestrict.size() != md.veciServRestrict.size())
            return FALSE;

        /* Vector contents */
        for (size_t i = 0; i < veciServRestrict.size(); i++)
            if (veciServRestrict[i] != md.veciServRestrict[i])
                return FALSE;

        if (bIsSyncMultplx != md.bIsSyncMultplx)
            return FALSE;

        return TRUE;
    }

    vector<int> veciServRestrict;
    _BOOLEAN bIsSyncMultplx;
};

class COtherService: public CServiceDefinition
{
public:
    COtherService(): CServiceDefinition(), bSameService(TRUE),
            iShortID(0), iServiceID(SERV_ID_NOT_USED)
    {
    }

    COtherService(const COtherService& nAF):
            CServiceDefinition(nAF), bSameService(nAF.bSameService),
            iShortID(nAF.iShortID), iServiceID(nAF.iServiceID)
    {
    }

    COtherService& operator=(const COtherService& nAF)
    {
        CServiceDefinition(*this) = nAF;

        bSameService = nAF.bSameService;
        iShortID = nAF.iShortID;
        iServiceID = nAF.iServiceID;

        return *this;
    }

    bool operator==(const COtherService& nAF)
    {
        if (CServiceDefinition(*this) != nAF)
            return FALSE;

        if (bSameService != nAF.bSameService)
            return FALSE;

        if (iShortID != nAF.iShortID)
            return FALSE;

        if (iServiceID != nAF.iServiceID)
            return FALSE;

        return TRUE;
    }

    string ServiceID() const;

    _BOOLEAN bSameService;
    int iShortID;
    uint32_t iServiceID;
};

/* Alternative frequency signalling class */
class CAltFreqSign
{
public:

    CAltFreqSign():vecRegions(16),vecDetailedRegions(16),
            vecSchedules(16),vecMultiplexes(),vecOtherServices(),
            bRegionVersionFlag(FALSE),bDetailedRegionVersionFlag(FALSE),bScheduleVersionFlag(FALSE),
            bMultiplexVersionFlag(FALSE),bOtherServicesVersionFlag(FALSE)
    {
    }

    void ResetRegions(_BOOLEAN b)
    {
        vecRegions.clear();
        vecRegions.resize(16);
        bRegionVersionFlag=b;
    }

    void ResetDetailedRegions(_BOOLEAN b)
    {
        vecDetailedRegions.clear();
        vecDetailedRegions.resize(16);
        bDetailedRegionVersionFlag=b;
    }

    void ResetSchedules(_BOOLEAN b)
    {
        vecSchedules.clear();
        vecSchedules.resize(16);
        bScheduleVersionFlag=b;
    }

    void ResetMultiplexes(_BOOLEAN b)
    {
        vecMultiplexes.clear();
        bMultiplexVersionFlag=b;
    }

    void ResetOtherServices(_BOOLEAN b)
    {
        vecOtherServices.clear();
        bOtherServicesVersionFlag=b;
    }

    void Reset()
    {
        ResetRegions(FALSE);
        ResetDetailedRegions(FALSE);
        ResetSchedules(FALSE);
        ResetMultiplexes(FALSE);
        ResetOtherServices(FALSE);
    }

    vector < vector<CAltFreqRegion> > vecRegions; // outer vector indexed by regionID
    vector < vector<CAltFreqDetailedRegion> > vecDetailedRegions; // outer vector indexed by regionID
    vector < vector<CAltFreqSched> > vecSchedules; // outer vector indexed by scheduleID
    vector < CMultiplexDefinition > vecMultiplexes;
    vector < COtherService > vecOtherServices;
    _BOOLEAN bRegionVersionFlag, bDetailedRegionVersionFlag;
    _BOOLEAN bScheduleVersionFlag;
    _BOOLEAN bMultiplexVersionFlag;
    _BOOLEAN bOtherServicesVersionFlag;
};

/* Class to store information about the last service selected ------------- */

class CLastService
{
public:
    CLastService():iService(0), iServiceID(SERV_ID_NOT_USED)
    {
    }
    CLastService(const CLastService& l):iService(l.iService), iServiceID(l.iServiceID)
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
};

/* Classes to keep track of status flags for RSCI rsta tag and log file */
class CRxStatus
{
public:
    CRxStatus():status(NOT_PRESENT),iNum(0),iNumOK(0) {}
    CRxStatus(const CRxStatus& s):status(s.status),iNum(s.iNum),iNumOK(s.iNumOK) {}
    CRxStatus& operator=(const CRxStatus& s)
    {
        status = s.status;
        iNum = s.iNum;
        iNumOK = s.iNumOK;
        return *this;
    }
    void SetStatus(const ETypeRxStatus);
    ETypeRxStatus GetStatus() const {
        return status;
    }
    int GetCount() const {
        return iNum;
    }
    int GetOKCount() const {
        return iNumOK;
    }
    void ResetCounts() {
        iNum=0;
        iNumOK = 0;
    }
private:
    ETypeRxStatus status;
    int iNum, iNumOK;
};

class CReceiveStatus
{
public:
    CReceiveStatus():FSync(),TSync(),InterfaceI(),InterfaceO(),
            FAC(),SDC(),MSC(),SLAudio(),LLAudio()
    {
    }
    CReceiveStatus(const CReceiveStatus& s):FSync(s.FSync), TSync(s.TSync),
            InterfaceI(s.InterfaceI), InterfaceO(s.InterfaceO),
            FAC(s.FAC), SDC(s.SDC), MSC(s.MSC),
            SLAudio(s.SLAudio),LLAudio(s.LLAudio)
    {
    }
    CReceiveStatus& operator=(const CReceiveStatus& s)
    {
        FSync = s.FSync;
        TSync = s.TSync;
        InterfaceI = s.InterfaceI;
        InterfaceO = s.InterfaceO;
        FAC = s.FAC;
        SDC = s.SDC;
        SDC = s.MSC;
        SLAudio = s.SLAudio;
        LLAudio = s.LLAudio;
        return *this;
    }

    CRxStatus FSync;
    CRxStatus TSync;
    CRxStatus InterfaceI;
    CRxStatus InterfaceO;
    CRxStatus FAC;
    CRxStatus SDC;
    CRxStatus MSC;
    CRxStatus SLAudio;
    CRxStatus LLAudio;
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

class CFrontEndParameters
{
public:
    enum ESMeterCorrectionType {S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY, S_METER_CORRECTION_TYPE_AGC_ONLY, S_METER_CORRECTION_TYPE_AGC_RSSI};

    // Constructor
    CFrontEndParameters():
            eSMeterCorrectionType(S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY), rSMeterBandwidth(10000.0),
            rDefaultMeasurementBandwidth(10000.0), bAutoMeasurementBandwidth(TRUE), rCalFactorAM(0.0),
            rCalFactorDRM(0.0), rIFCentreFreq(12000.0)
    {}
    CFrontEndParameters(const CFrontEndParameters& p):
            eSMeterCorrectionType(p.eSMeterCorrectionType), rSMeterBandwidth(p.rSMeterBandwidth),
            rDefaultMeasurementBandwidth(p.rDefaultMeasurementBandwidth),
            bAutoMeasurementBandwidth(p.bAutoMeasurementBandwidth),
            rCalFactorAM(p.rCalFactorAM), rCalFactorDRM(p.rCalFactorDRM),
            rIFCentreFreq(p.rIFCentreFreq)
    {}
    CFrontEndParameters& operator=(const CFrontEndParameters& p)
    {
        eSMeterCorrectionType = p.eSMeterCorrectionType;
        rSMeterBandwidth = p.rSMeterBandwidth;
        rDefaultMeasurementBandwidth = p.rDefaultMeasurementBandwidth;
        bAutoMeasurementBandwidth = p.bAutoMeasurementBandwidth;
        rCalFactorAM = p.rCalFactorAM;
        rCalFactorDRM = p.rCalFactorDRM;
        rIFCentreFreq = p.rIFCentreFreq;
        return *this;
    }

    ESMeterCorrectionType eSMeterCorrectionType;
    _REAL rSMeterBandwidth; // The bandwidth the S-meter uses to do the measurement

    _REAL rDefaultMeasurementBandwidth; // Bandwidth to do measurement if not synchronised
    _BOOLEAN bAutoMeasurementBandwidth; // TRUE: use the current FAC bandwidth if locked, FALSE: use default bandwidth always
    _REAL rCalFactorAM;
    _REAL rCalFactorDRM;
    _REAL rIFCentreFreq;

};


class CMinMaxMean
{
public:
    CMinMaxMean();

    void addSample(_REAL);
    _REAL getCurrent();
    _REAL getMean();
    void getMinMax(_REAL&, _REAL&);
protected:
    _REAL rSum, rCur, rMin, rMax;
    int iNum;
};

class CParameter
{
public:
    CParameter();
    CParameter(const CParameter&);
    virtual ~CParameter();
    CParameter& operator=(const CParameter&);

    /* Enumerations --------------------------------------------------------- */
    /* AS: AFS in SDC is valid or not */
    enum EAFSVali { AS_VALID, AS_NOT_VALID };

    /* CT: Current Time */
    enum ECurTime { CT_OFF, CT_LOCAL, CT_UTC, CT_UTC_OFFSET };

    /* ST: Simulation Type */
    enum ESimType
    { ST_NONE, ST_BITERROR, ST_MSECHANEST, ST_BER_IDEALCHAN,
      ST_SYNC_PARAM, ST_SINR
    };

    /* Misc. Functions ------------------------------------------------------ */
    void SetReceiver(CDRMReceiver *pDRMReceiver);
    void GenerateRandomSerialNumber();
    void GenerateReceiverID();
    void ResetServicesStreams();
    void GetActiveServices(set<int>& actServ);
    void GetActiveStreams(set<int>& actStr);
    void InitCellMapTable(const ERobMode eNewWaveMode,
                          const ESpecOcc eNewSpecOcc);

    void SetNumDecodedBitsMSC(const int iNewNumDecodedBitsMSC);
    void SetNumDecodedBitsSDC(const int iNewNumDecodedBitsSDC);
    void SetNumBitsHieraFrTot(const int iNewNumBitsHieraFrTot);
    void SetNumAudioDecoderBits(const int iNewNumAudioDecoderBits);
    void SetNumDataDecoderBits(const int iNewNumDataDecoderBits);

    _BOOLEAN SetWaveMode(const ERobMode eNewWaveMode);
    ERobMode GetWaveMode() const {
        return eRobustnessMode;
    }

    void SetFrequency(int iNewFrequency) {
        iFrequency = iNewFrequency;
    }
    int GetFrequency() {
        return iFrequency;
    }

    void SetServiceParameters(int iShortID, const CService& newService);

    void SetCurSelAudioService(const int iNewService);
    int GetCurSelAudioService() const {
        return iCurSelAudioService;
    }
    void SetCurSelDataService(const int iNewService);
    int GetCurSelDataService() const {
        return iCurSelDataService;
    }

    void ResetCurSelAudDatServ()
    {
        iCurSelAudioService = 0;
        iCurSelDataService = 0;
    }

    /*
       Sample rate related getters/setters
    */
    int GetAudSampleRate() const
    {
        return iAudSampleRate;
    }
    int GetSigSampleRate() const
    {
        return iSigSampleRate;
    }
    int GetSigUpscaleRatio() const
    {
        return iSigUpscaleRatio;
    }
    int GetSoundCardSigSampleRate() const
    {
        return iSigSampleRate / iSigUpscaleRatio;
    }
    void SetSoundCardSigSampleRate(int sr)
    {
        iSigSampleRate = sr * iSigUpscaleRatio;
    }
    void SetNewAudSampleRate(int sr)
    {
        /* Perform range check */
        if      (sr < 8000)   sr = 8000;
        else if (sr > 192000) sr = 192000;
        /* Audio sample rate must be a multiple of 25,
           set to the nearest multiple of 25 */
        // TODO AM Demod still have issue with some sample rate
        // The buffering system is not enough flexible
        sr = (sr + 12) / 25 * 25; // <- ok for DRM mode
        iNewAudSampleRate = sr;
    }
    void SetNewSigSampleRate(int sr)
    {
        /* Set to the nearest supported sample rate */
        if      (sr < 36000)  sr = 24000;
        else if (sr < 72000)  sr = 48000;
        else if (sr < 144000) sr = 96000;
        else                  sr = 192000;
        iNewSigSampleRate = sr;
    }
    void SetNewSigUpscaleRatio(int ratio)
    {
        iNewSigUpscaleRatio = ratio < 2 ? 1 : 2;
    }
    /* New sample rate are fetched at init and restart */
    void FetchNewSampleRate()
    {
        if (iNewSigUpscaleRatio != 0)
        {
            iSigUpscaleRatio = iNewSigUpscaleRatio;
            iNewSigUpscaleRatio = 0;
        }
        if (iNewAudSampleRate != 0)
        {
            iAudSampleRate = iNewAudSampleRate;
            iNewAudSampleRate = 0;
        }
        if (iNewSigSampleRate != 0)
        {
            iSigSampleRate = iNewSigSampleRate * iSigUpscaleRatio;
            iNewSigSampleRate = 0;
        }
    }

    _REAL GetDCFrequency() const
    {
        return iSigSampleRate * (rFreqOffsetAcqui + rFreqOffsetTrack);
    }

    _REAL GetBitRateKbps(const int iShortID, const _BOOLEAN bAudData) const;
    _REAL PartABLenRatio(const int iShortID) const;

    /* Parameters controlled by FAC ----------------------------------------- */
    void SetInterleaverDepth(const ESymIntMod eNewDepth);
    ESymIntMod GetInterleaverDepth() const
    {
        return eSymbolInterlMode;
    }

    void SetMSCCodingScheme(const ECodScheme eNewScheme);
    void SetSDCCodingScheme(const ECodScheme eNewScheme);

    void SetSpectrumOccup(ESpecOcc eNewSpecOcc);
    ESpecOcc GetSpectrumOccup() const
    {
        return eSpectOccup;
    }

    void SetNumOfServices(const size_t iNNumAuSe, const size_t iNNumDaSe);
    size_t GetTotNumServices() const
    {
        return iNumAudioService + iNumDataService;
    }

    void SetAudDataFlag(const int iShortID, const CService::ETyOServ iNewADaFl);
    void SetServiceID(const int iShortID, const uint32_t iNewServiceID);

    CDRMReceiver* pDRMRec;

    /* Symbol interleaver mode (long or short interleaving) */
    ESymIntMod eSymbolInterlMode;

    ECodScheme eMSCCodingScheme;	/* MSC coding scheme */
    ECodScheme eSDCCodingScheme;	/* SDC coding scheme */

    size_t iNumAudioService;
    size_t iNumDataService;

    /* AMSS */
    int iAMSSCarrierMode;

    /* Serial number and received ID */
    string sReceiverID;
    string sSerialNumber;
protected:
    string sDataFilesDirectory;
public:

    string GetDataDirectory(const char* pcChildDirectory = NULL) const;
    void SetDataDirectory(string sNewDataFilesDirectory);

    /* Parameters controlled by SDC ----------------------------------------- */
    void SetAudioParam(const int iShortID, const CAudioParam& NewAudParam);
    CAudioParam GetAudioParam(const int iShortID) const;
    CDataParam GetDataParam(const int iShortID) const;
    void SetDataParam(const int iShortID, const CDataParam& NewDataParam);

    void SetMSCProtLev(const CMSCProtLev NewMSCPrLe, const _BOOLEAN bWithHierarch);
    void SetStreamLen(const int iStreamID, const int iNewLenPartA, const int iNewLenPartB);
    void GetStreamLen(const int iStreamID, int& iLenPartA, int& iLenPartB) const;
    int GetStreamLen(const int iStreamID) const;
    ECurTime eTransmitCurrentTime;

    /* Protection levels for MSC */
    CMSCProtLev MSCPrLe;

    vector<CStream> Stream;
    vector<CService> Service;
    vector<CRxStatus> AudioComponentStatus; // currently indexed by short id
    CRxStatus DataComponentStatus[4][4]; // index by streamid then packetid

    /* information about services gathered from SDC, EPG and web schedules */
    map<uint32_t,CServiceInformation> ServiceInformation;

    /* These values are used to set input and output block sizes of some modules */
    int iNumBitsHierarchFrameTotal;
    int iNumDecodedBitsMSC;
    int iNumSDCBitsPerSFrame;	/* Number of SDC bits per super-frame */
    int iNumAudioDecoderBits;	/* Number of input bits for audio module */
    int iNumDataDecoderBits;	/* Number of input bits for data decoder module */

    /* Date */
    int iYear;
    int iMonth;
    int iDay;

    /* UTC (hours and minutes) */
    int iUTCHour;
    int iUTCMin;
    int iUTCOff;
    int iUTCSense;
    _BOOLEAN bValidUTCOffsetAndSense;

    /* Identifies the current frame. This parameter is set by FAC */
    int iFrameIDTransm;
    int iFrameIDReceiv;

    /* Synchronization ------------------------------------------------------ */
    _REAL rFreqOffsetAcqui;
    _REAL rFreqOffsetTrack;

    _REAL rResampleOffset;

    int iTimingOffsTrack;

    ERecMode GetReceiverMode() {
        return eReceiverMode;
    }
    ERecMode eReceiverMode;
    EAcqStat GetAcquiState() {
        return eAcquiState;
    }
    EAcqStat eAcquiState;
    int iNumAudioFrames;

    CVector <_BINARY> vecbiAudioFrameStatus;
    _BOOLEAN bMeasurePSD, bMeasurePSDAlways;
    _REAL rPIRStart;
    _REAL rPIREnd;

    /* vector to hold the PSD values for the rpsd tag. */
    CVector <_REAL> vecrPSD;

    // vector to hold impulse response values for (proposed) rpir tag
    CVector <_REAL> vecrPIR;

    CMatrix <_COMPLEX> matcReceivedPilotValues;

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
    CAltFreqSign AltFreqSign;

    void Lock()
    {
        Mutex.Lock();
    }
    void Unlock()
    {
        Mutex.Unlock();
    }
    /* Channel Estimation */
    _REAL rMER;
    _REAL rWMERMSC;
    _REAL rWMERFAC;
    _REAL rSigmaEstimate;
    _REAL rMinDelay;
    _REAL rMaxDelay;

    _BOOLEAN bMeasureDelay;
    CRealVector vecrRdel;
    CRealVector vecrRdelThresholds;
    CRealVector vecrRdelIntervals;
    _BOOLEAN bMeasureDoppler;
    _REAL rRdop;
    /* interference (constellation-based measurement rnic)*/
    _BOOLEAN bMeasureInterference;
    _REAL rIntFreq, rINR, rICR;

    /* peak of PSD - for PSD-based interference measurement rnip */
    _REAL rMaxPSDwrtSig;
    _REAL rMaxPSDFreq;

    /* the signal level as measured at IF by dream */
    void SetIFSignalLevel(_REAL);
    _REAL GetIFSignalLevel() const;
    _REAL rSigStrengthCorrection;

    /* General -------------------------------------------------------------- */
    _REAL GetNominalBandwidth();
    _REAL GetSysToNomBWCorrFact();
    volatile enum { STOPPED, RUNNING, STOP_REQUESTED, RESTART } eRunState;

    CCellMappingTable CellMappingTable;

    CMinMaxMean SNRstat, SigStrstat;

    gps_data_t gps_data;
    bool use_gpsd;
    bool restart_gpsd;
    string gps_host; string gps_port;

protected:

    int iAudSampleRate;
    int iSigSampleRate;
    int iSigUpscaleRatio;
    int iNewAudSampleRate;
    int iNewSigSampleRate;
    int iNewSigUpscaleRatio;

    _REAL rSysSimSNRdB;

    int iFrequency;
    _BOOLEAN bValidSignalStrength;
    _REAL rSigStr;
    _REAL rIFSigStr;

    /* Current selected audio service for processing */
    int iCurSelAudioService;
    int iCurSelDataService;

    ERobMode eRobustnessMode;	/* E.g.: Mode A, B, C or D */
    ESpecOcc eSpectOccup;

    /* For resync to last service------------------------------------------- */
    CLastService LastAudioService;
    CLastService LastDataService;

    CMutex Mutex;
public:
    bool lenient_RSCI;
    bool bCanDecodeAAC;
    bool bCanDecodeCELP;
    bool bCanDecodeHVXC;
    bool bCanDecodeOPUS;
    bool bCanDecodexHEAAC;
    bool bCanEncodeAAC;
    bool bCanEncodeCELP;
    bool bCanEncodeHVXC;
    bool bCanEncodeOPUS;
    bool bCanEncodexHEAAC;

    bool CanDecode(CAudioParam::EAudCod eAudCod) {
        switch (eAudCod)
        {
        case CAudioParam::AC_NONE: return true;
        case CAudioParam::AC_AAC:  return bCanDecodeAAC;
        case CAudioParam::AC_CELP: return bCanDecodeCELP;
        case CAudioParam::AC_HVXC: return bCanDecodeHVXC;
        case CAudioParam::AC_OPUS: return bCanDecodeOPUS;
        case CAudioParam::AC_xHEAAC: return bCanDecodexHEAAC;
        }
        return false;
    }
    bool CanEncode(CAudioParam::EAudCod eAudCod) {
        switch (eAudCod)
        {
        case CAudioParam::AC_NONE: return true;
        case CAudioParam::AC_AAC:  return bCanEncodeAAC;
        case CAudioParam::AC_CELP: return bCanEncodeCELP;
        case CAudioParam::AC_HVXC: return bCanEncodeHVXC;
        case CAudioParam::AC_OPUS: return bCanEncodeOPUS;
        case CAudioParam::AC_xHEAAC: return bCanEncodexHEAAC;
        }
        return false;
    }

};

#endif // !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
