#ifndef SPEEXRESAMPLER_H
#define SPEEXRESAMPLER_H

#include <speex/speex_resampler.h>
#include "../util/Vector.h"

class SpeexResampler
{
public:
    SpeexResampler();
    virtual ~SpeexResampler();
    void Init(int iNewInputBlockSize, _REAL rNewRatio);
    void Init(int iNewOutputBlockSize, int iInputSamplerate, int iOutputSamplerate);
    void Resample(CVector<_REAL>& rInput, CVector<_REAL>& rOutput);
    int GetFreeInputSize() const;
    int GetMaxInputSize() const;
    void Reset();
protected:
    _REAL					rRatio;
    int						iInputBlockSize;
    int						iOutputBlockSize;
    CShiftRegister<_REAL>	vecrIntBuff;
    int						iHistorySize;
    SpeexResamplerState*	resampler;
    CVector<float>			vecfInput;
    CVector<float>			vecfOutput;
    int						iInputBuffered;
    int						iMaxInputSize;
    void Free();
};

#endif // SPEEXRESAMPLER_H
