#include "speexresampler.h"
#include <cstring>
#include <iostream>
#define RESAMPLING_QUALITY 6 /* 0-10 : 0=fast/bad 10=slow/good */

SpeexResampler::SpeexResampler() :
    iInputBlockSize(0), iOutputBlockSize(0),
    resampler(nullptr), iInputBuffered(0), iMaxInputSize(0)
{
}

SpeexResampler::~SpeexResampler()
{
    Free();
}

void SpeexResampler::Free()
{
    if (resampler != nullptr)
    {
        speex_resampler_destroy(resampler);
        resampler = nullptr;
    }
    vecfInput.resize(0);
    vecfOutput.resize(0);
}

/* this function is only called when the input and output sample rates are different */
void SpeexResampler::Resample(CVector<_REAL>& rInput, CVector<_REAL>& rOutput)
{
    if (rOutput.Size() != int(iOutputBlockSize))
        cerr << "SpeexResampler::Resample(): rOutput.Size(" << rOutput.Size() << ") != iOutputBlockSize(" << iOutputBlockSize << ")" << endl;

    size_t iInputSize = GetFreeInputSize();
    for (size_t i = 0; i < iInputSize; i++)
        vecfInput[i+iInputBuffered] = float(rInput[int(i)]);

    spx_uint32_t input_frames_used = spx_uint32_t(rInput.size());
    spx_uint32_t output_frames_gen = spx_uint32_t(rOutput.size());
    size_t input_frames = iInputBuffered + iInputSize;

    if (resampler != nullptr)
    {
        int err = speex_resampler_process_float(
            resampler,
            0,
            &vecfInput[0],
            &input_frames_used,
            &vecfOutput[0],
            &output_frames_gen);
        if (err != RESAMPLER_ERR_SUCCESS) {
            cerr << "SpeexResampler::Init(): libspeexdsp error: " << speex_resampler_strerror(err) << endl;
            input_frames_used = 0;
            output_frames_gen = 0;
        }
    }

    if (output_frames_gen != iOutputBlockSize)
        cerr << "SpeexResampler::Resample(): output_frames_gen(" << output_frames_gen << ") != iOutputBlockSize(" << iOutputBlockSize << ")" << endl;

    for (size_t i = 0; i < iOutputBlockSize; i++)
        rOutput[int(i)] = _REAL(vecfOutput[i]);

    iInputBuffered = input_frames - input_frames_used;
    for (size_t i = 0; i < iInputBuffered; i++)
        vecfInput[i] = vecfInput[i+input_frames_used];
}


size_t SpeexResampler::GetMaxInputSize() const
{
    return iMaxInputSize != 0 ? iMaxInputSize : iInputBlockSize;
}

size_t SpeexResampler::GetFreeInputSize() const
{
    return GetMaxInputSize() - iInputBuffered;
}

void SpeexResampler::Reset()
{
    iInputBuffered = 0;
    if (resampler != nullptr)
    {
        int err = speex_resampler_reset_mem(resampler);
        if (err != RESAMPLER_ERR_SUCCESS)
            cerr << "SpeexResampler::Init(): libspeexdsp error: " << speex_resampler_strerror(err) << endl;
    }
}

void SpeexResampler::Init(const int iNewInputBlockSize, const _REAL rNewRatio)
{
    Free();
    if (iNewInputBlockSize==0)
        return;
    if(rNewRatio<0.1) {
        cerr << "resampler initialised with too great a compression ratio" << endl;
        return;
    }
    iInputBlockSize = size_t(iNewInputBlockSize);
    iOutputBlockSize = size_t(iNewInputBlockSize * rNewRatio);
    iInputBuffered = 0;
    iMaxInputSize = 0;
    int err;
    resampler = speex_resampler_init(1, spx_uint32_t(iInputBlockSize), spx_uint32_t(iOutputBlockSize), RESAMPLING_QUALITY, &err);
    if (resampler == nullptr)
        cerr << "SpeexResampler::Init(): libspeexdsp error: " << speex_resampler_strerror(err)<< endl;
    vecfInput.resize(size_t(iInputBlockSize));
    vecfOutput.resize(size_t(iOutputBlockSize));
}

void SpeexResampler::Init(int iNewOutputBlockSize, int iInputSamplerate, int iOutputSamplerate)
{
    iInputBlockSize = size_t((iOutputSamplerate * iNewOutputBlockSize) / iInputSamplerate);
    iOutputBlockSize = size_t(iNewOutputBlockSize);
    const size_t iNewMaxInputSize = unsigned(iInputBlockSize) * 2;
    iMaxInputSize = iNewMaxInputSize;
    int err = RESAMPLER_ERR_SUCCESS;
    if (resampler == nullptr)
    {
        resampler = speex_resampler_init(1, spx_uint32_t(iInputSamplerate), spx_uint32_t(iOutputSamplerate), RESAMPLING_QUALITY, &err);
        iInputBuffered = 0;
    }
    else
    {
        err = speex_resampler_set_rate(resampler, spx_uint32_t(iInputSamplerate), spx_uint32_t(iOutputSamplerate));
    }
    if (err == RESAMPLER_ERR_SUCCESS) {
        vecfInput.resize(size_t(iInputBlockSize));
        vecfOutput.resize(size_t(iOutputBlockSize));
    }
    else {
        cerr << "SpeexResampler::Init(): libspeexdsp error: " << speex_resampler_strerror(err) << endl;
        Free();
    }
}
