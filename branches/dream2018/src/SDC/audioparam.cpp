#include "audioparam.h"
#include <iostream>

CAudioParam::~CAudioParam()
{
}

bool CAudioParam::setFromType9Bits(CVector<_BINARY>& biData, unsigned numBytes)
{

    /* Init error flag with "no error" */
    bool bError = false;

    /* Audio coding */
    switch (biData.Separate(2))
    {
    case 0: /* 00 */
        eAudioCoding = CAudioParam::AC_AAC;
        break;

    case 1: /* 01 */
        eAudioCoding = CAudioParam::AC_OPUS; // non-standard
        break;

    case 2: /* 10 */
        eAudioCoding = CAudioParam::AC_RESERVED;
        break;

    case 3: /* 11 */
        eAudioCoding = CAudioParam::AC_xHE_AAC;
        break;
    }
    /* SBR flag */
    switch (biData.Separate(1))
    {
    case 0: /* 0 */
        eSBRFlag = CAudioParam::SB_NOT_USED;
        break;

    case 1: /* 1 */
        eSBRFlag = CAudioParam::SB_USED;
        break;
    }

    /* Audio mode */
    switch (biData.Separate(2))
    {
    case 0: /* 00 */
        eAudioMode = CAudioParam::AM_MONO;
        break;

    case 1: /* 01 */
        eAudioMode = CAudioParam::AM_P_STEREO; // Not used in xHE-AAC
        break;

    case 2: /* 10 */
        eAudioMode = CAudioParam::AM_STEREO;
        break;

    default: /* reserved */
        eAudioMode = CAudioParam::AM_STEREO;
        bError = TRUE;
        break;
    }

    /* Audio sampling rate */

    /* Init bAudioSamplingRateValue7 flag to FALSE */
    bool bAudioSamplingRateValue7 = false;

    unsigned asr = biData.Separate(3);
    if(eAudioCoding == CAudioParam::AC_xHE_AAC) {
        switch (asr)
        {
        case 0: /* 000 */
            eAudioSamplRate = CAudioParam::AS_9_6KHZ;
            break;

        case 1: /* 001 */
            eAudioSamplRate = CAudioParam::AS_12KHZ;
            break;

        case 2: /* 010 */
            eAudioSamplRate = CAudioParam::AS_16KHZ;
            break;

        case 3: /* 011 */
            eAudioSamplRate = CAudioParam::AS_19_2KHZ;
            break;

        case 4: /* 100 */
            eAudioSamplRate = CAudioParam::AS_24KHZ;
            break;

        case 5: /* 101 */
            eAudioSamplRate = CAudioParam::AS_32KHZ;
            break;

        case 6: /* 110 */
            eAudioSamplRate = CAudioParam::AS_38_4KHZ;
            break;

        case 7: /* 111 */
            eAudioSamplRate = CAudioParam::AS_48KHZ;
        default: /* reserved */
            bError = TRUE;
            break;
        }
    }
    else {
        switch (asr)
        {
        case 1: /* 001 */
            eAudioSamplRate = CAudioParam::AS_12KHZ;
            break;
        case 3: /* 011 */
            eAudioSamplRate = CAudioParam::AS_24KHZ;
            break;
        case 5: /* 101 */
            eAudioSamplRate = CAudioParam::AS_48KHZ;
            break;
        case 7: /* 111 */
            bAudioSamplingRateValue7 = true;
        default: /* reserved */
            bError = TRUE;
            break;
        }
    }

    /* XXX EXPERIMENTAL THIS IS NOT PART OF DRM STANDARD XXX */
    if ((bAudioSamplingRateValue7 && eAudioCoding == CAudioParam::AC_AAC) || (eAudioCoding == CAudioParam::AC_OPUS) ) {
        bError = eSBRFlag != CAudioParam::SB_NOT_USED || eAudioMode != CAudioParam::AM_MONO;
        eAudioCoding = CAudioParam::AC_OPUS;
        eAudioMode = CAudioParam::AM_STEREO;
        eAudioSamplRate = CAudioParam::AS_48KHZ;
        eSBRFlag = CAudioParam::SB_NOT_USED;
    }

    /* Text flag */
    switch (biData.Separate(1))
    {
    case 0: /* 0 */
        bTextflag = FALSE;
        break;

    case 1: /* 1 */
        bTextflag = TRUE;
        break;
    }

    /* Enhancement flag */
    switch (biData.Separate(1))
    {
    case 0: /* 0 */
        bEnhanceFlag = FALSE;
        break;

    case 1: /* 1 */
        bEnhanceFlag = TRUE;
        break;
    }
    /* Coder field */
    if ((eAudioCoding == CAudioParam::AC_AAC) || (eAudioCoding == CAudioParam::AC_xHE_AAC))
    {
        int mpeg_surround_config = biData.Separate(3);
        switch(mpeg_surround_config) {
        case 0: // no MPEG Surround information available.
        case 1: // reserved.
            break;
        case 2: // MPEG Surround with 5.1 output channels.
            break;
        case 3: // MPEG Surround with 7.1 output channels.
            break;
        case 4: // reserved.
        case 5: // reserved.
        case 6: // reserved.
            break;
        case 7: // other mode (the mode can be derived from the MPEG Surround data stream)
            break;
        }
        /* rfa 2 bits */
        biData.Separate(2);
    }
    else
    {
        /* rfa 5 bit */
        iCELPIndex = biData.Separate(5);
    }

    /* rfa 1 bit */
    biData.Separate(1);

    if (eAudioCoding == CAudioParam::AC_xHE_AAC)
    {
        xHE_AAC_config.resize(numBytes-2);
        for(int i=0; i < xHE_AAC_config.size(); i++) {
            xHE_AAC_config[i] = biData.Separate(8);
        }
    }

    return bError;
}

void CAudioParam::setFromType9Bytes(const std::vector<uint8_t>& data)
{
    CVector<_BINARY> bits;
    bits.Init(data.size()*SIZEOF__BYTE);
    bits.ResetBitAccess();
    for(int i=0; i<data.size(); i++) {
        bits.Enqueue(data[i], 8);
    }
    setFromType9Bits(bits, data.size());
}

vector<uint8_t> CAudioParam::getType9Bytes() const
{
    CVector<_BINARY> bits;
    bits.Init((2+xHE_AAC_config.size())*SIZEOF__BYTE);
    bits.ResetBitAccess();
    EnqueueType9(bits);
    for(int i=0; i<bits.Size(); i++) {
        cerr << int(bits[i]);
    }
    cerr << endl;
    bits.ResetBitAccess();
    vector<uint8_t> bytes;
    for(int i=0; i<bits.Size()/SIZEOF__BYTE; i++) {
        bytes.push_back(bits.Separate(SIZEOF__BYTE));
    }
    return bytes;
}

void CAudioParam::EnqueueType9(CVector<_BINARY>& vecbiData) const
{
    /* Audio coding */
    switch (eAudioCoding)
    {
    case CAudioParam::AC_AAC:
    case CAudioParam::AC_OPUS:
        vecbiData.Enqueue(0 /* 00 */, 2);
        break;

    case CAudioParam::AC_CELP:
        vecbiData.Enqueue(1 /* 01 */, 2);
        break;

    case CAudioParam::AC_HVXC:
        vecbiData.Enqueue(2 /* 10 */, 2);
        break;

    default:
        vecbiData.Enqueue(3 /* 11 */, 2);
        break;
    }

    if (eAudioCoding == CAudioParam::AC_OPUS)
    {
        /* XXX EXPERIMENTAL THIS IS NOT PART OF DRM STANDARD XXX */
        /* SBR flag, Audio mode */
        vecbiData.Enqueue(0 /* 000 */, 3); /* set to zero, rfa for OPUS */
        /* Audio sampling rate */
        vecbiData.Enqueue(7 /* 111 */, 3); /* set to seven, reserved value for AAC */
    }
    else
    {
        /* SBR flag */
        switch (eSBRFlag)
        {
        case CAudioParam::SB_NOT_USED:
            vecbiData.Enqueue(0 /* 0 */, 1);
            break;

        case CAudioParam::SB_USED:
            vecbiData.Enqueue(1 /* 1 */, 1);
            break;
        }

        /* Audio mode */
        switch (eAudioCoding)
        {
        case CAudioParam::AC_AAC:
            /* Channel type */
            switch (eAudioMode)
            {
            case CAudioParam::AM_MONO:
                vecbiData.Enqueue(0 /* 00 */, 2);
                break;

            case CAudioParam::AM_P_STEREO:
                vecbiData.Enqueue(1 /* 01 */, 2);
                break;

            case CAudioParam::AM_STEREO:
                vecbiData.Enqueue(2 /* 10 */, 2);
                break;
            }
            break;

        case CAudioParam::AC_CELP:
            /* rfa */
            vecbiData.Enqueue((uint32_t) 0, 1);

            /* CELP_CRC */
            switch (bCELPCRC)
            {
            case FALSE:
                vecbiData.Enqueue(0 /* 0 */, 1);
                break;

            case TRUE:
                vecbiData.Enqueue(1 /* 1 */, 1);
                break;
            }
            break;

        case CAudioParam::AC_HVXC:
            /* HVXC_rate */
            switch (eHVXCRate)
            {
            case CAudioParam::HR_2_KBIT:
                vecbiData.Enqueue(0 /* 0 */, 1);
                break;

            case CAudioParam::HR_4_KBIT:
                vecbiData.Enqueue(1 /* 1 */, 1);
                break;
            }

            /* HVXC CRC */
            switch (bHVXCCRC)
            {
            case FALSE:
                vecbiData.Enqueue(0 /* 0 */, 1);
                break;

            case TRUE:
                vecbiData.Enqueue(1 /* 1 */, 1);
                break;
            }
            break;

        default:
            vecbiData.Enqueue(0 /* 00 */, 2);
            break;
        }

        // Audio sampling rate AS_9_6_KHZ, AS_12KHZ, AS_16KHZ, AS_19_2KHZ, AS_24KHZ, AS_32KHZ, AS_38_4KHZ, AS_48KHZ
        unsigned int iVal=0;
        switch (eAudioSamplRate)
        {
        case CAudioParam::AS_9_6KHZ:
            iVal = 0;
            break;
        case CAudioParam::AS_12KHZ:
            iVal = 1;
            break;
        case CAudioParam::AS_16KHZ:
            iVal = 2;
            break;
        case CAudioParam::AS_19_2KHZ:
            iVal = 3;
            break;
        case CAudioParam::AS_24KHZ:
            if(eAudioCoding==CAudioParam::AC_xHE_AAC) iVal = 4;
            if(eAudioCoding==CAudioParam::AC_AAC) iVal = 3;
            if(eAudioCoding==CAudioParam::AC_OPUS) iVal = 3;
            break;
        case CAudioParam::AS_32KHZ:
            iVal = 5;
            break;
        case CAudioParam::AS_38_4KHZ:
            iVal = 6;
            break;
        case CAudioParam::AS_48KHZ:
            iVal = (eAudioCoding==CAudioParam::AC_xHE_AAC)?7:5;
            break;
        }
        vecbiData.Enqueue(iVal, 3);
    }

    /* Text flag */
    switch (bTextflag)
    {
    case FALSE:
        vecbiData.Enqueue(0 /* 0 */, 1);
        break;

    case TRUE:
        vecbiData.Enqueue(1 /* 1 */, 1);
        break;
    }

    /* Enhancement flag */
    switch (bEnhanceFlag)
    {
    case FALSE:
        vecbiData.Enqueue(0 /* 0 */, 1);
        break;

    case TRUE:
        vecbiData.Enqueue(1 /* 1 */, 1);
        break;
    }

    /* Coder field */
    if (
            eAudioCoding == CAudioParam::AC_CELP)
    {
        /* CELP index */
        vecbiData.Enqueue(
            (uint32_t) iCELPIndex, 5);
    }
    else
    {
        /* rfa 5 bit */
        vecbiData.Enqueue((uint32_t) 0, 5);
    }

    /* rfa 1 bit */
    vecbiData.Enqueue((uint32_t) 0, 1);

    for(int i=0; i<xHE_AAC_config.size(); i++) {
        vecbiData.Enqueue(xHE_AAC_config[i], 8);
    }
}
