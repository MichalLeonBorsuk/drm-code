#include <au_channel.h>
#include <stdio.h>
#include <stdlib.h>

AuChanError AuChannelOpen ( hAudioChannel* audioChannel,
                              const char* filename,
                              AuChanMode mode,
                              AuChanType* type,
                              AuChanInfo* info)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}
                              
AuChanError AuChannelSeek ( hAudioChannel audioChannel, int nSamples )
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}
                              
void AuChannelClose (hAudioChannel audioChannel)
{
}

AuChanError AuChannelWriteLong( hAudioChannel audioChannel,
                                  const long* samples,
                                  int nBytes,
                                  int* written)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}

AuChanError AuChannelWriteShort( hAudioChannel audioChannel,
                                   const short* samples,
                                   int nSamples,
                                   int* written)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}

AuChanError AuChannelWriteFloat( hAudioChannel audioChannel,
                                   const float* samples,
                                   int nSamples,
                                   int* written)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}

AuChanError AuChannelWriteBytes( hAudioChannel audioChannel,
                                   const char* bytes,
                                   int nBytes,
                                   int* written)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}

AuChanError AuChannelReadShort( hAudioChannel audioChannel,
                                  short* samples,
                                  int nSamples,
                                  int* written)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}

AuChanError AuChannelReadFloat( hAudioChannel audioChannel,
                                  float* samples,
                                  int nSamples,
                                  int* read)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}

AuChanError AuChannelReadBytes( hAudioChannel audioChannel,
                                   char* bytes,
                                   int nBytes,
                                   int* read)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}
  
AuChanError AuChannelSetLevel( hAudioChannel audioChannel,
                                 int volume)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}
  
AuChanError AuChannelParseFormatString(const char* format,
                                         AuChanInfo* info,
                                         AuChanType* type)
{
  return AU_CHAN_UNIMPLEMENTED_FEATURE;
}

void AuChannelGetLibInfo (AuChanLibInfo* libInfo)
{
}
