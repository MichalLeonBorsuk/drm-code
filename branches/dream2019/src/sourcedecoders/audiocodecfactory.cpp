#include "audiocodecfactory.h"
#include "null_codec.h"
#include "aac_codec.h"
#include "opus_codec.h"
#ifdef HAVE_LIBFDK_AAC
# include "fdk_aac_codec.h"
#endif

AudioCodecFactory::AudioCodecFactory()
{

}

AudioCodecFactory::~AudioCodecFactory()
{

}

vector<CAudioCodec*> AudioCodecFactory::CodecList;

int AudioCodecFactory::RefCount = 0;

void
AudioCodecFactory::InitCodecList()
{
    if (CodecList.size() == 0)
    {
        /* Null codec, MUST be the first */
        CodecList.push_back(new NullCodec);

        /* AAC */
#ifdef HAVE_LIBFDK_AAC
        CodecList.push_back(new FdkAacCodec);
#endif
        CodecList.push_back(new AacCodec);

        /* Opus */
        CodecList.push_back(new OpusCodec);
    }
    RefCount ++;
}

void
AudioCodecFactory::UnrefCodecList()
{
    RefCount --;
    if (!RefCount)
    {
        while (CodecList.size() != 0)
        {
            delete CodecList.back();
            CodecList.pop_back();
        }
    }
}

CAudioCodec*
AudioCodecFactory::GetDecoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr)
{
    const int size = int(CodecList.size());
    for (int i = 1; i < size; i++)
        if (CodecList[unsigned(i)]->CanDecode(eAudioCoding))
            return CodecList[unsigned(i)];
    /* Fallback to null codec */
    return bCanReturnNullPtr ? nullptr : CodecList[0]; // ie the null codec
}

CAudioCodec*
AudioCodecFactory::GetEncoder(CAudioParam::EAudCod eAudioCoding, bool bCanReturnNullPtr)
{
    const int size = CodecList.size();
    for (int i = 1; i < size; i++)
        if (CodecList[i]->CanEncode(eAudioCoding))
            return CodecList[i];
    /* Fallback to null codec */
    return bCanReturnNullPtr ? nullptr : CodecList[0]; // ie the null codec
}
