#ifndef AUDIOSUPERFRAMEDECODER_H
#define AUDIOSUPERFRAMEDECODER_H

#include "aacsuperframe.h"
#include "../sourcedecoders/audioframe.h"
#include "../util/Modul.h"
#include "../Parameter.h"
#include "../TextMessage.h"

class AudioSuperframeDecoder: public CReceiverModul<_BINARY, AudioFrame>
{
public:
    AudioSuperframeDecoder();
    virtual ~AudioSuperframeDecoder();
    unsigned getNumFrames() { if(pAudioSuperFrame==nullptr) return 0; return pAudioSuperFrame->getNumFrames();}
    AudioFrame& getFrame(unsigned i);
protected:
    virtual void InitInternal(CParameter& Parameters);
    virtual void ProcessDataInternal(CParameter& Parameters);

    AudioSuperFrame* pAudioSuperFrame;
};

#endif // AUDIOSUPERFRAMEDECODER_H
