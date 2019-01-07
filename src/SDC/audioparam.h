#ifndef AUDIOPARAM_H
#define AUDIOPARAM_H

#include "../util/Vector.h"

class CAudioParam
{
public:

    /* AC: Audio Coding */
    enum EAudCod { AC_NONE, AC_AAC, AC_OPUS, AC_RESERVED, AC_xHE_AAC, AC_CELP, AC_HVXC };

    /* SB: SBR */
    enum ESBRFlag { SB_NOT_USED, SB_USED };

    /* AM: Audio Mode */
    enum EAudMode { AM_MONO, AM_P_STEREO, AM_STEREO, AM_SURROUND };

    /* HR: HVXC Rate */
    enum EHVXCRate { HR_2_KBIT, HR_4_KBIT };

    /* AS: Audio Sampling rate */
    enum EAudSamRat { AS_9_6KHZ, AS_12KHZ, AS_16KHZ, AS_19_2KHZ, AS_24KHZ, AS_32KHZ, AS_38_4KHZ, AS_48KHZ };

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

    CAudioParam(): strTextMessage(), iStreamID(STREAM_ID_NOT_USED),
            eAudioCoding(AC_NONE), eSBRFlag(SB_NOT_USED), eAudioSamplRate(AS_24KHZ),
            bTextflag(FALSE), bEnhanceFlag(FALSE), eAudioMode(AM_MONO),
            iCELPIndex(0), bCELPCRC(FALSE), eHVXCRate(HR_2_KBIT), bHVXCCRC(FALSE),
            xHE_AAC_config(),
            eOPUSBandwidth(OB_FB), eOPUSSubCod(OS_SILK), eOPUSChan(OC_STEREO),
            eOPUSSignal(OG_MUSIC), eOPUSApplication(OA_AUDIO),
            bOPUSForwardErrorCorrection(FALSE), bOPUSRequestReset(FALSE),
            bParamChanged(FALSE)
    {
    }

    virtual ~CAudioParam();

    CAudioParam(const CAudioParam& ap):
            strTextMessage(ap.strTextMessage),
            iStreamID(ap.iStreamID),
            eAudioCoding(ap.eAudioCoding),
            eSBRFlag(ap.eSBRFlag),
            eAudioSamplRate(ap.eAudioSamplRate),
            bTextflag(ap.bTextflag),
            bEnhanceFlag(ap.bEnhanceFlag),
            eAudioMode(ap.eAudioMode),
            iCELPIndex(ap.iCELPIndex),
            bCELPCRC(ap.bCELPCRC),
            eHVXCRate(ap.eHVXCRate),
            bHVXCCRC(ap.bHVXCCRC),
            xHE_AAC_config(ap.xHE_AAC_config),
            eOPUSBandwidth(ap.eOPUSBandwidth),
            eOPUSSubCod(ap.eOPUSSubCod),
            eOPUSChan(ap.eOPUSChan),
            eOPUSSignal(ap.eOPUSSignal),
            eOPUSApplication(ap.eOPUSApplication),
            bOPUSForwardErrorCorrection(ap.bOPUSForwardErrorCorrection),
            bOPUSRequestReset(ap.bOPUSRequestReset),
            bParamChanged(ap.bParamChanged)
    {
    }
    CAudioParam& operator=(const CAudioParam& ap)
    {
        strTextMessage = ap.strTextMessage;
        iStreamID = ap.iStreamID;
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
        xHE_AAC_config = ap.xHE_AAC_config;
        eOPUSBandwidth = ap.eOPUSBandwidth;
        eOPUSSubCod = ap.eOPUSSubCod;
        eOPUSChan = ap.eOPUSChan;
        eOPUSSignal = ap.eOPUSSignal;
        eOPUSApplication = ap.eOPUSApplication;
        bOPUSForwardErrorCorrection = ap.bOPUSForwardErrorCorrection;
        bOPUSRequestReset = ap.bOPUSRequestReset;
        bParamChanged = ap.bParamChanged;
        return *this;
    }

    bool setFromType9Bits(CVector<_BINARY>& biData, unsigned numBytes);
    void setFromType9Bytes(const std::vector<uint8_t>& data);
    std::vector<uint8_t> getType9Bytes() const;
    void EnqueueType9(CVector<_BINARY>& biData) const;

    /* Text-message */
    string strTextMessage;	/* Max length is (8 * 16 Bytes) */

    int iStreamID;			/* Stream Id of the stream which carries the audio service */

    EAudCod eAudioCoding;	/* This field indicated the source coding system */
    ESBRFlag eSBRFlag;		/* SBR flag */
    EAudSamRat eAudioSamplRate;	/* Audio sampling rate */
    _BOOLEAN bTextflag;		/* Indicates whether a text message is present or not */
    _BOOLEAN bEnhanceFlag;	/* Enhancement flag */

    /* For AAC: Mono, LC Stereo, Stereo --------------------------------- */
    EAudMode eAudioMode;	/* Audio mode */

    /* For CELP --------------------------------------------------------- */
    int iCELPIndex;			/* This field indicates the CELP bit rate index */
    _BOOLEAN bCELPCRC;		/* This field indicates whether the CRC is used or not */

    /* For HVXC --------------------------------------------------------- */
    EHVXCRate eHVXCRate;	/* This field indicates the rate of the HVXC */
    _BOOLEAN bHVXCCRC;		/* This field indicates whether the CRC is used or not */

    /* for xHE-AAC ------------------------------------------------------ */
    vector<_BYTE> xHE_AAC_config;

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

        case AC_xHE_AAC:
            if (eAudioMode != AudioParam.eAudioMode)
                return TRUE;
            break;

        case AC_CELP:
        case AC_HVXC:
        case AC_NONE:
        case AC_OPUS:
        case AC_RESERVED:
            break;
        }
        if(xHE_AAC_config != AudioParam.xHE_AAC_config) {
            return TRUE;
        }
        return FALSE;
    }
};

#endif // AUDIOPARAM_H
