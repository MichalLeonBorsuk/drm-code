#include <drmenclib.h>

#undef DRMENC_EXTERN
#define DRMENC_EXTERN

DRMENC_EXTERN  DrmencError DrmencOpen(hDrmEncoder *drmEncoder,
                                        int          audioCoding,
                                        int          coderField,
                                        int          coderSamplingRate,
                                        int          audioMode,
                                        int          stereoInputFlag,
                                        int          SBROnOffFlag,
                                        int          lengthOfAudioSuperFrame,
                                        int          lengthHigherProtected)
{
  return DRMENC_NO_LICENSE;
}

DRMENC_EXTERN DrmencError DrmencEncode(hDrmEncoder    drmEncoder,
                                          float         *inbuffer,
                                          int            nSamples,
                                          unsigned char *outbuffer,
                                          unsigned int  *nOutputBytes)
{
  return DRMENC_NO_LICENSE;
}

DRMENC_EXTERN DrmencError DrmencGetFrameLength(hDrmEncoder  drmEncoder,
                                                  int         *frameLength,
                                                  int         *sensitiveLength)
{
  return DRMENC_NO_LICENSE;
}



DRMENC_EXTERN DrmencError DrmencClose(hDrmEncoder drmEncoder)
{
  return DRMENC_NO_LICENSE;
}

DRMENC_EXTERN void DrmencGetLibInfo(DrmencLibInfo *libInfo)
{
}
