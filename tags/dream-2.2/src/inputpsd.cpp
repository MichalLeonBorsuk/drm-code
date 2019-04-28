#include "inputpsd.h"
#include "creceivedata.h"
#include "Parameter.h"
#include "matlib/MatlibSigProToolbox.h"

/* power gain of the Hamming window */
#define PSD_WINDOW_GAIN 0.39638
#define RNIP_SEARCH_RANGE_NARROW 5100.0
#define RNIP_SEARCH_RANGE_WIDE 15100.0
#define RNIP_EXCLUDE_BINS 2 // either side of the peak

InputPSD::InputPSD():iSampleRate(48000),bNegativeFreq(false),bOffsetFreq(false),vecrAvSqMagSpect()
{

}

void InputPSD::CalculateLinearPSD(const CShiftRegister<_REAL>& vecrInpData,
                                 int iLenPSDAvEachBlock, int iNumAvBlocksPSD,
                                int iPSDOverlap)
{
    /* Length of spectrum vector including Nyquist frequency */
    const int iLenSpecWithNyFreq = iLenPSDAvEachBlock / 2 + 1;

    /* Init intermediate vectors */
    CRealVector vecrFFTInput(iLenPSDAvEachBlock);

    /* Init Hamming window */
    CRealVector vecrHammWin(Hamming(iLenPSDAvEachBlock));

    /* Init instance vector */
    vecrAvSqMagSpect.Init(iLenSpecWithNyFreq, 0.0);

    /* Calculate FFT of each small block and average results (estimation of PSD of input signal) */
    CFftPlans FftPlans;
    for (int i = 0; i < iNumAvBlocksPSD; i++)
    {
        /* Copy data from shift register in Matlib vector */
        for (int j = 0; j < iLenPSDAvEachBlock; j++) {
            vecrFFTInput[j] = vecrInpData[j + i * (iLenPSDAvEachBlock - iPSDOverlap)];
        }

        /* Apply Hamming window */
        vecrFFTInput *= vecrHammWin;

        /* Calculate squared magnitude of spectrum and average results */
        vecrAvSqMagSpect += SqMag(rfft(vecrFFTInput, FftPlans));
    }
}

void InputPSD::PSD2LogPSD(int iLenPSDAvEachBlock, int iNumAvBlocksPSD,
                                 CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
    const int iLenSpec = vecrAvSqMagSpect.GetSize();
    /* Init output vectors */
    vecrData.Init(iLenSpec, 0.0);
    vecrScale.Init(iLenSpec, 0.0);

    const int iOffsetScale = bNegativeFreq ? (iLenSpec / 2) : (bOffsetFreq ? (iLenSpec * int(VIRTUAL_INTERMED_FREQ) / (iSampleRate / 2)) : 0);

    const _REAL rFactorScale = _REAL(iSampleRate / iLenSpec / 2000);

    const _REAL rNormData = _REAL( _MAXSHORT * _MAXSHORT *
                            iLenPSDAvEachBlock * iLenPSDAvEachBlock *
                            iNumAvBlocksPSD * _REAL(PSD_WINDOW_GAIN));

    /* Log power spectrum data */
    for (int i = 0; i <iLenSpec; i++)
    {
        const _REAL rNormSqMag = vecrAvSqMagSpect[i] / rNormData;

        if (rNormSqMag > 0)
            vecrData[i] = 10.0 * log10(rNormSqMag);
        else
            vecrData[i] = RET_VAL_LOG_0;

        vecrScale[i] = _REAL(i - iOffsetScale) * rFactorScale;
    }
}


/*
 * This function is called in a context where the Parameters structure is Locked.
 */
void InputPSD::CalculateSigStrengthCorrection(CParameter &Parameters, CVector<_REAL> &vecrPSD)
{

    _REAL rCorrection = _REAL(0.0);

    /* Calculate signal power in measurement bandwidth */

    _REAL rFreqKmin, rFreqKmax;

    _REAL rIFCentreFrequency = Parameters.FrontEndParameters.rIFCentreFreq;

    if (Parameters.GetAcquiState() == AS_WITH_SIGNAL &&
            Parameters.FrontEndParameters.bAutoMeasurementBandwidth)
    {
        // Receiver is locked, so measure in the current DRM signal bandwidth Kmin to Kmax
        _REAL rDCFrequency = Parameters.GetDCFrequency();
        rFreqKmin = rDCFrequency + _REAL(Parameters.CellMappingTable.iCarrierKmin)/Parameters.CellMappingTable.iFFTSizeN * iSampleRate;
        rFreqKmax = rDCFrequency + _REAL(Parameters.CellMappingTable.iCarrierKmax)/Parameters.CellMappingTable.iFFTSizeN * iSampleRate;
    }
    else
    {
        // Receiver unlocked, or measurement is requested in fixed bandwidth
        _REAL rMeasBandwidth = Parameters.FrontEndParameters.rDefaultMeasurementBandwidth;
        rFreqKmin = rIFCentreFrequency - rMeasBandwidth/_REAL(2.0);
        rFreqKmax = rIFCentreFrequency + rMeasBandwidth/_REAL(2.0);
    }

    _REAL rSigPower = CalcTotalPower(vecrPSD, FreqToBin(rFreqKmin), FreqToBin(rFreqKmax));

    if (Parameters.FrontEndParameters.eSMeterCorrectionType == CFrontEndParameters::S_METER_CORRECTION_TYPE_AGC_ONLY)
    {
        /* Write it to the receiver params to help with calculating the signal strength */
        rCorrection += _REAL(10.0) * log10(rSigPower);
    }
    else if (Parameters.FrontEndParameters.eSMeterCorrectionType == CFrontEndParameters::S_METER_CORRECTION_TYPE_AGC_RSSI)
    {
        _REAL rSMeterBandwidth = Parameters.FrontEndParameters.rSMeterBandwidth;

        _REAL rFreqSMeterMin = _REAL(rIFCentreFrequency - rSMeterBandwidth / _REAL(2.0));
        _REAL rFreqSMeterMax = _REAL(rIFCentreFrequency + rSMeterBandwidth / _REAL(2.0));

        _REAL rPowerInSMeterBW = CalcTotalPower(vecrPSD, FreqToBin(rFreqSMeterMin), FreqToBin(rFreqSMeterMax));

        /* Write it to the receiver params to help with calculating the signal strength */

        rCorrection += _REAL(10.0) * log10(rSigPower/rPowerInSMeterBW);
    }

    /* Add on the calibration factor for the current mode */
    if (Parameters.GetReceiverMode() == RM_DRM)
        rCorrection += Parameters.FrontEndParameters.rCalFactorDRM;
    else if (Parameters.GetReceiverMode() == RM_AM)
        rCorrection += Parameters.FrontEndParameters.rCalFactorAM;

    Parameters.rSigStrengthCorrection = rCorrection;

    return;
}

/*
 * This function is called in a context where the Parameters structure is Locked.
 */
void InputPSD::CalculatePSDInterferenceTag(CParameter &Parameters, CVector<_REAL> &vecrPSD)
{

    /* Interference tag (rnip) */

    // Calculate search range: defined as +/-5.1kHz except if locked and in 20k
    _REAL rIFCentreFrequency = Parameters.FrontEndParameters.rIFCentreFreq;

    _REAL rFreqSearchMin = rIFCentreFrequency - _REAL(RNIP_SEARCH_RANGE_NARROW);
    _REAL rFreqSearchMax = rIFCentreFrequency + _REAL(RNIP_SEARCH_RANGE_NARROW);

    ESpecOcc eSpecOcc = Parameters.GetSpectrumOccup();

    if (Parameters.GetAcquiState() == AS_WITH_SIGNAL &&
            (eSpecOcc == SO_4 || eSpecOcc == SO_5) )
    {
        rFreqSearchMax = rIFCentreFrequency + _REAL(RNIP_SEARCH_RANGE_WIDE);
    }
    int iSearchStartBin = FreqToBin(rFreqSearchMin);
    int iSearchEndBin = FreqToBin(rFreqSearchMax);

    if (iSearchStartBin < 0) iSearchStartBin = 0;
    if (iSearchEndBin > LEN_PSD_AV_EACH_BLOCK_RSI/2)
        iSearchEndBin = LEN_PSD_AV_EACH_BLOCK_RSI/2;

    _REAL rMaxPSD = _REAL(-1000.0);
    int iMaxPSDBin = 0;

    for (int i=iSearchStartBin; i<=iSearchEndBin; i++)
    {
        _REAL rPSD = _REAL(2.0) * pow(_REAL(10.0), vecrPSD[i]/_REAL(10.0));
        if (rPSD > rMaxPSD)
        {
            rMaxPSD = rPSD;
            iMaxPSDBin = i;
        }
    }

    // For total signal power, exclude the biggest one and e.g. 2 either side
    int iExcludeStartBin = iMaxPSDBin - RNIP_EXCLUDE_BINS;
    int iExcludeEndBin = iMaxPSDBin + RNIP_EXCLUDE_BINS;

    // Calculate power. TotalPower() function will deal with start>end correctly
    _REAL rSigPowerExcludingInterferer = CalcTotalPower(vecrPSD, iSearchStartBin, iExcludeStartBin-1) +
                                         CalcTotalPower(vecrPSD, iExcludeEndBin+1, iSearchEndBin);

    /* interferer level wrt signal power */
    Parameters.rMaxPSDwrtSig = _REAL(10.0) * log10(rMaxPSD / rSigPowerExcludingInterferer);

    /* interferer frequency */
    Parameters.rMaxPSDFreq = _REAL(iMaxPSDBin) * _REAL(iSampleRate) / _REAL(LEN_PSD_AV_EACH_BLOCK_RSI) - rIFCentreFrequency;

}

/* Calculate PSD and put it into the CParameter class.
 * The data will be used by the rsi output.
 * This function is called in a context where the Parameters structure is Locked.
 */
void InputPSD::PutPSD(CParameter &Parameters)
{

    CVector<_REAL>		vecrData;
    CVector<_REAL>		vecrScale;

    PSD2LogPSD(LEN_PSD_AV_EACH_BLOCK_RSI, NUM_AV_BLOCKS_PSD_RSI, vecrData, vecrScale);

    /* Data required for rpsd tag */
    /* extract the values from -8kHz to +8kHz/18kHz relative to 12kHz, i.e. 4kHz to 20kHz */
    /*const int startBin = 4000.0 * LEN_PSD_AV_EACH_BLOCK_RSI /iSampleRate;
    const int endBin = 20000.0 * LEN_PSD_AV_EACH_BLOCK_RSI /iSampleRate;*/
    /* The above calculation doesn't round in the way FhG expect. Probably better to specify directly */

    /* For 20k mode, we need -8/+18, which is more than the Nyquist rate of 24kHz. */
    /* Assume nominal freq = 7kHz (i.e. 2k to 22k) and pad with zeroes (roughly 1kHz each side) */

    int iStartBin = 22;
    int iEndBin = 106;
    int iVecSize = iEndBin - iStartBin + 1; //85

    //_REAL rIFCentreFrequency = Parameters.FrontEndParameters.rIFCentreFreq;

    ESpecOcc eSpecOcc = Parameters.GetSpectrumOccup();
    if (eSpecOcc == SO_4 || eSpecOcc == SO_5)
    {
        iStartBin = 0;
        iEndBin = 127;
        iVecSize = 139;
    }
    /* Line up the the middle of the vector with the quarter-Nyquist bin of FFT */
    int iStartIndex = iStartBin - (LEN_PSD_AV_EACH_BLOCK_RSI/4) + (iVecSize-1)/2;

    /* Fill with zeros to start with */
    Parameters.vecrPSD.Init(iVecSize, 0.0);

    for (int i=iStartIndex, j=iStartBin; j<=iEndBin; i++,j++)
        Parameters.vecrPSD[i] = vecrData[j];

    CalculateSigStrengthCorrection(Parameters, vecrData);

    CalculatePSDInterferenceTag(Parameters, vecrData);

}

int InputPSD::FreqToBin(_REAL rFreq)
{
    return int(rFreq/iSampleRate * LEN_PSD_AV_EACH_BLOCK_RSI);
}

_REAL InputPSD::CalcTotalPower(CVector<_REAL> &vecrData, int iStartBin, int iEndBin)
{
    if (iStartBin < 0) iStartBin = 0;
    if (iEndBin > LEN_PSD_AV_EACH_BLOCK_RSI/2)
        iEndBin = LEN_PSD_AV_EACH_BLOCK_RSI/2;

    _REAL rSigPower = _REAL(0.0);
    for (int i=iStartBin; i<=iEndBin; i++)
    {
        _REAL rPSD = pow(_REAL(10.0), vecrData[i]/_REAL(10.0));
        // The factor of 2 below is needed because half of the power is in the negative frequencies
        rSigPower += rPSD * _REAL(2.0);
    }

    return rSigPower;
}
