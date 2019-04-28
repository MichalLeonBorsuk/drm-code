#ifndef CRECEIVEDATA_H
#define CRECEIVEDATA_H

#include "util/Modul.h"
#include "sound/soundinterface.h"
#include "util/Utilities.h"
#ifdef QT_MULTIMEDIA_LIB
# include <QAudioInput>
# include <QIODevice>
#else
# ifdef QT_CORE_LIB
  class QIODevice;
# endif
#endif

/* Number of FFT blocks used for averaging. See next definition
   ("NUM_SMPLS_4_INPUT_SPECTRUM") for how to set the parameters */
#define NUM_AV_BLOCKS_PSD			16
#define LEN_PSD_AV_EACH_BLOCK		512

  /* Length of vector for input spectrum. We use approx. 0.2 sec
     of sampled data for spectrum calculation, this is 2^13 = 8192 to
     make the FFT work more efficient. Make sure that this number is not smaller
     than the symbol lenght in 48 khz domain of longest mode (which is mode A/B:
     1280) */
#define NUM_SMPLS_4_INPUT_SPECTRUM (NUM_AV_BLOCKS_PSD * LEN_PSD_AV_EACH_BLOCK)

/* same but for the rpsd tag */
#define NUM_AV_BLOCKS_PSD_RSI	150
#define LEN_PSD_AV_EACH_BLOCK_RSI		256
#define PSD_OVERLAP_RSI	128

/* The RSI output needs 400ms with a 50% overlap, so this needs more space
   I think the RSCI spec is slightly wrong - using 150 windows consumes just over 400ms, 149 would be exact */
#define INPUT_DATA_VECTOR_SIZE (NUM_AV_BLOCKS_PSD_RSI * (LEN_PSD_AV_EACH_BLOCK_RSI-PSD_OVERLAP_RSI)+PSD_OVERLAP_RSI)

enum EInChanSel {CS_LEFT_CHAN, CS_RIGHT_CHAN, CS_MIX_CHAN, CS_SUB_CHAN, CS_IQ_POS,
                   CS_IQ_NEG, CS_IQ_POS_ZERO, CS_IQ_NEG_ZERO, CS_IQ_POS_SPLIT, CS_IQ_NEG_SPLIT
                  };

class CReceiveData : public CReceiverModul<_REAL, _REAL>
{
public:
    CReceiveData() :
#ifdef QT_MULTIMEDIA_LIB
        pIODevice(nullptr),
#endif
        pSound(nullptr),
        vecrInpData(INPUT_DATA_VECTOR_SIZE, 0.0),
        bFippedSpectrum(false), eInChanSelection(CS_MIX_CHAN), iPhase(0)
    {}
    virtual ~CReceiveData();

    _REAL ConvertFrequency(_REAL rFrequency, bool bInvert=false) const;

    void GetInputSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    void GetInputPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
                     const int iLenPSDAvEachBlock = LEN_PSD_AV_EACH_BLOCK,
                     const int iNumAvBlocksPSD = NUM_AV_BLOCKS_PSD,
                     const int iPSDOverlap = 0);

    void SetFlippedSpectrum(const bool bNewF) {
        bFippedSpectrum = bNewF;
    }

    bool GetFlippedSpectrum() {
        return bFippedSpectrum;
    }

    void ClearInputData() {
        mutexInpData.Lock();
        vecrInpData.Init(INPUT_DATA_VECTOR_SIZE, 0.0);
        mutexInpData.Unlock();
    }

    void SetSoundInterface(std::string);
    std::string GetSoundInterface() { return soundDevice; }
    void Enumerate(std::vector<string>& names, std::vector<string>& descriptions);
    void Stop();
#ifdef QT_MULTIMEDIA_LIB
    std::string GetSoundInterfaceVersion() { return "QtMultimedia"; }
#else
    std::string GetSoundInterfaceVersion() { return pSound->GetVersion(); }
#endif
    void SetInChanSel(const EInChanSel eNS) {
        eInChanSelection = eNS;
    }
    EInChanSel GetInChanSel() {
        return eInChanSelection;
    }

protected:
    CSignalLevelMeter		SignalLevelMeter;

#ifdef QT_MULTIMEDIA_LIB
    QAudioInput*            pAudioInput;
    QIODevice*              pIODevice;
#endif
    CSoundInInterface*		pSound;
    CVector<_SAMPLE>		vecsSoundBuffer;
    std::string             soundDevice;

    /* Access to vecrInpData buffer must be done
       inside mutexInpData mutex */
    CShiftRegister<_REAL>	vecrInpData;
    CMutex                  mutexInpData;

    int					iSampleRate;
    bool			bFippedSpectrum;

    int					iUpscaleRatio;
    std::vector<float>		vecf_B, vecf_YL, vecf_YR, vecf_ZL, vecf_ZR;

    EInChanSel			eInChanSelection;

    CVector<_REAL>		vecrReHist;
    CVector<_REAL>		vecrImHist;
    _COMPLEX			cCurExp;
    _COMPLEX			cExpStep;
    int					iPhase;

    _REAL HilbertFilt(const _REAL rRe, const _REAL rIm);

    /* OPH: counter to count symbols within a frame in order to generate */
    /* RSCI output */
    int							iFreeSymbolCounter;

    virtual void InitInternal(CParameter& Parameters);
    virtual void ProcessDataInternal(CParameter& Parameters);

    void PutPSD(CParameter& Parameters);
    void CalculatePSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
                      const int iLenPSDAvEachBlock = LEN_PSD_AV_EACH_BLOCK,
                      const int iNumAvBlocksPSD = NUM_AV_BLOCKS_PSD,
                      const int iPSDOverlap = 0);

    void CalculateSigStrengthCorrection(CParameter &Parameters, CVector<_REAL> &vecrPSD);
    void CalculatePSDInterferenceTag(CParameter &Parameters, CVector<_REAL> &vecrPSD);

    int FreqToBin(_REAL rFreq);
    _REAL CalcTotalPower(CVector<_REAL> &vecrData, int iStartBin, int iEndBin);

    void InterpFIR_2X(const int channels, _SAMPLE* X, std::vector<float>& Z, std::vector<float>& Y, std::vector<float>& B);
};


#endif // CRECEIVEDATA_H
