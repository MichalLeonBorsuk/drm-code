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
    eAudioCoding = EAudCod(biData.Separate(2));

    /* SBR flag */
    eSBRFlag = ESBRFlag(biData.Separate(1));

    /* Audio mode */
    eAudioMode = EAudioMode(biData.Separate(2));

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
    eSurround = ESurround(biData.Separate(3)); // no need to worry if not AAC/xHE-AAC

    /* rfa 2 bits */
    biData.Separate(2);

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
    bits.Init(int(data.size()*SIZEOF__BYTE));
    bits.ResetBitAccess();
    for(int i=0; i<int(data.size()); i++) {
        bits.Enqueue(uint32_t(data[unsigned(i)]), 8);
    }
    setFromType9Bits(bits, unsigned(data.size()));
}

vector<uint8_t> CAudioParam::getType9Bytes() const
{
    CVector<_BINARY> bits;
    bits.Init(int((2+xHE_AAC_config.size())*SIZEOF__BYTE));
    bits.ResetBitAccess();
    EnqueueType9(bits);
    for(int i=0; i<bits.Size(); i++) {
        cerr << int(bits[i]);
    }
    cerr << endl;
    bits.ResetBitAccess();
    vector<uint8_t> bytes;
    for(int i=0; i<bits.Size()/SIZEOF__BYTE; i++) {
        bytes.push_back(uint8_t(bits.Separate(SIZEOF__BYTE)));
    }
    return bytes;
}

void CAudioParam::EnqueueType9(CVector<_BINARY>& vecbiData) const
{
    ESBRFlag sbr = eSBRFlag;

    /* Audio coding */
    switch (eAudioCoding)
    {
    case CAudioParam::AC_AAC:
        vecbiData.Enqueue(0 /* 00 */, 2);
        break;
    case CAudioParam::AC_xHE_AAC:
        vecbiData.Enqueue(3 /* 11 */, 2);
        break;
    case CAudioParam::AC_OPUS:
        vecbiData.Enqueue(1 /* 01 */, 2); // non-standard
        sbr = CAudioParam::SB_NOT_USED;
        break;
    case CAudioParam::AC_NONE:
    case CAudioParam::AC_RESERVED:
        vecbiData.Enqueue(2 /* 10 */, 2);
    }

    /* SBR flag */
    switch (sbr)
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

    /* Text flag */
    vecbiData.Enqueue(bTextflag?1:0, 1);

    /* Enhancement flag */
    vecbiData.Enqueue(bEnhanceFlag?1:0, 1);

    /* Coder field */
    if ((eAudioCoding == CAudioParam::AC_AAC) || (eAudioCoding == CAudioParam::AC_xHE_AAC))
    {
        vecbiData.Enqueue(eSurround, 3);
        vecbiData.Enqueue(0, 2); // rfa
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
