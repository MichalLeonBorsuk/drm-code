#ifndef AUDIOCODECFACTORY_H
#define AUDIOCODECFACTORY_H

#include "../SDC/audioparam.h"
class CAudioCodec;

class AudioCodecFactory
{
public:
    AudioCodecFactory();
    virtual ~AudioCodecFactory();
    static void InitCodecList();
    static void UnrefCodecList();
    static CAudioCodec* GetDecoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr=false);
    static CAudioCodec* GetEncoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr=false);
private:
    static vector<CAudioCodec*> CodecList;
    static int RefCount;
};

#endif // AUDIOCODECFACTORY_H
