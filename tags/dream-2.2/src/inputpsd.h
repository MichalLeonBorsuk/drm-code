#ifndef INPUTPSD_H
#define INPUTPSD_H
#include "util/Vector.h"
#include "matlib/Matlib.h"

class CParameter;

class InputPSD
{
public:
    InputPSD();
    void PutPSD(CParameter& Parameters);
    void SetSampleRate(int sr) {
        iSampleRate = sr;
    }
    void setNegativeFrequency(bool b) { bNegativeFreq = b; }
    void setOffsetFrequency(bool b) { bOffsetFreq = b; }
    void CalculateLinearPSD(const CShiftRegister<_REAL>& vecrInpData,
                             int iLenPSDAvEachBlock, int iNumAvBlocksPSD,
                             int iPSDOverlap);
    void PSD2LogPSD(int iLenPSDAvEachBlock, int iNumAvBlocksPSD, CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);

private:
    void CalculateSigStrengthCorrection(CParameter &Parameters, CVector<_REAL> &vecrPSD);
    void CalculatePSDInterferenceTag(CParameter &Parameters, CVector<_REAL> &vecrPSD);
    _REAL CalcTotalPower(CVector<_REAL> &vecrData, int iStartBin, int iEndBin);
    int FreqToBin(_REAL rFreq);

    int iSampleRate;
    bool bNegativeFreq;
    bool bOffsetFreq;
    CRealVector vecrAvSqMagSpect;
};

#endif // INPUTPSD_H
