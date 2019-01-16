#include "speexresampler.h"
#include <cstring>
#define RESAMPLING_QUALITY 6 /* 0-10 : 0=fast/bad 10=slow/good */

SpeexResampler::SpeexResampler() :
    rRatio(1.0), iInputBlockSize(0), iOutputBlockSize(0),
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
    vecfInput.Init(0);
    vecfOutput.Init(0);
    rRatio = 1.0;
}

void SpeexResampler::Resample(CVector<_REAL>& rInput, CVector<_REAL>& rOutput)
{
    if (rRatio == 1.0)
    {
        memcpy(&rOutput[0], &rInput[0], sizeof(_REAL) * iOutputBlockSize);
    }
    else
    {
        int i;
        if (rOutput.Size() != iOutputBlockSize)
            qDebug("SpeexResampler::Resample(): rOutput.Size(%i) != iOutputBlockSize(%i)", (int)rOutput.Size(), iOutputBlockSize);

        int iInputSize = GetFreeInputSize();
        for (i = 0; i < iInputSize; i++)
            vecfInput[i+iInputBuffered] = rInput[i];

        int input_frames_used = 0;
        int output_frames_gen = 0;
        int input_frames = iInputBuffered + iInputSize;

        if (resampler != nullptr)
        {
            spx_uint32_t in_len = input_frames;
            spx_uint32_t out_len = iOutputBlockSize;
            int err = speex_resampler_process_float(
                resampler,
                0,
                &vecfInput[0],
                &in_len,
                &vecfOutput[0],
                &out_len);
            if (err != RESAMPLER_ERR_SUCCESS)
                qDebug("SpeexResampler::Init(): libspeexdsp error: %s", speex_resampler_strerror(err));
            input_frames_used = err != RESAMPLER_ERR_SUCCESS ? 0 : in_len;
            output_frames_gen = err != RESAMPLER_ERR_SUCCESS ? 0 : out_len;
        }

        if (output_frames_gen != iOutputBlockSize)
            qDebug("SpeexResampler::Resample(): output_frames_gen(%i) != iOutputBlockSize(%i)", output_frames_gen, iOutputBlockSize);

        for (i = 0; i < iOutputBlockSize; i++)
            rOutput[i] = vecfOutput[i];

        iInputBuffered = input_frames - input_frames_used;
        for (i = 0; i < iInputBuffered; i++)
            vecfInput[i] = vecfInput[i+input_frames_used];
    }
}

int SpeexResampler::GetMaxInputSize() const
{
    return iMaxInputSize != 0 ? iMaxInputSize : iInputBlockSize;
}

int SpeexResampler::GetFreeInputSize() const
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
            qDebug("SpeexResampler::Init(): libspeexdsp error: %s", speex_resampler_strerror(err));
    }
}

void SpeexResampler::Init(const int iNewInputBlockSize, const _REAL rNewRatio)
{
    Free();
    if (!iNewInputBlockSize)
        return;
    if(int(rNewRatio)==0)
        return;
    iInputBlockSize = iNewInputBlockSize;
    iOutputBlockSize = int(iNewInputBlockSize * rNewRatio);
    rRatio = _REAL(iOutputBlockSize) / iInputBlockSize;
    iInputBuffered = 0;
    iMaxInputSize = 0;
    if (int(rRatio) != 1)
    {
        int err;
        resampler = speex_resampler_init(1, spx_uint32_t(iInputBlockSize), spx_uint32_t(iOutputBlockSize), RESAMPLING_QUALITY, &err);
        if (!resampler)
            qDebug("SpeexResampler::Init(): libspeexdsp error: %s", speex_resampler_strerror(err));
        vecfInput.Init(iInputBlockSize);
        vecfOutput.Init(iOutputBlockSize);
    }
}

void SpeexResampler::Init(const int iNewOutputBlockSize, const int iInputSamplerate, const int iOutputSamplerate)
{
    rRatio = _REAL(iOutputSamplerate) / iInputSamplerate;
    iInputBlockSize = int(iNewOutputBlockSize / rRatio);
    iOutputBlockSize = iNewOutputBlockSize;
    if (rRatio != 1.0)
    {
        const int iNewMaxInputSize = iInputBlockSize * 2;
        const int iInputSize = vecfInput.Size();
        if (iInputSize < iNewMaxInputSize)
        {
            vecfInput.Enlarge(iNewMaxInputSize - iInputSize);
            iMaxInputSize = iNewMaxInputSize;
        }
        vecfOutput.Init(iOutputBlockSize);
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
        if (err != RESAMPLER_ERR_SUCCESS)
            qDebug("SpeexResampler::Init(): libspeexdsp error: %s", speex_resampler_strerror(err));
    }
    else
    {
        Free();
    }
}
