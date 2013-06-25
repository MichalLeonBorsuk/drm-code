#include <drmdeclib.h>
#include <stdio.h>
#include <stdlib.h>

#undef DRMDEC_EXTERN
#define DRMDEC_EXTERN

DRMDEC_EXTERN unsigned int DrmdecOpen(hDrmDecoder *drmDecoder,
                                        int          audioCoding,
                                        int          coderField,
                                        int          coderSamplingRate,
                                        int          audioMode,
                                        int          SBROnOffFlag,
                                        int          lengthOfAudioSuperFrame,
                                        int          lengthHigherProtected)
{
  return 0;
}


  DRMDEC_EXTERN unsigned int DrmdecDecode(hDrmDecoder    drmDecoder,
                                          unsigned char *inbuffer,
                                          unsigned int   inputBufferSize,
                                          float         *outbuffer,
                                          int           *nSamples,
                                          int           *frameStatus)
{
  return 0;
}


  DRMDEC_EXTERN unsigned int DrmdecGetFrameLength(hDrmDecoder  drmDecoder,
                                                  int         *frameLength,
                                                  int         *sensitiveLength)
{
  return 0;
}


  DRMDEC_EXTERN unsigned int DrmdecClose(hDrmDecoder drmDecoder)
{
  return 0;
}


DRMDEC_EXTERN void DrmdecGetLibInfo(DrmdecLibInfo *libInfo)
{
}

DRMDEC_EXTERN unsigned int DrmdecSetParameter(hDrmDecoder  drmDecoder,
                                                unsigned int decParameter,
                                                int          iValue,
                                                float        fValue)
{
  return 0;
}

