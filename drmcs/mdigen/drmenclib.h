/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):  Julian Cable, Ollie Haffenden, Andrew Murphy
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
/******************************** DRM Audio Encoder ***************************

                       (C) copyright Coding Technologies (2002-2004)
                               All Rights Reserved

  This software module was developed by Coding Technologies.
  This is company confidential information and the property of CT, and can 
  not be reproduced or disclosed in any form without written authorization 
  of CT.

  $Id: drmenclib.h,v 1.4 2004/04/14 12:25:04 boe Exp $
  Initial author:       R. Boehm
  contents/description: DRM encoder library

*******************************************************************************/

#ifndef _DRMENCLIB_H
#define _DRMENCLIB_H

#ifndef WIN32
#define DRMENCSTATIC
#endif

#ifndef DRMENCSTATIC
  #ifdef DRMENCDLL_EXPORTS
    #define DRMENC_EXTERN extern __declspec( dllexport )
  #else
    #define DRMENC_EXTERN extern __declspec( dllimport )
  #endif
#else
  #define DRMENC_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef void* hDrmEncoder;

  typedef struct {
    char date[20];
    char versionNo[20];
    int  libID;
  } DrmencLibInfo;

  typedef enum {
    AUDIO_CODING_AAC = 0,
    AUDIO_CODING_CELP,
    AUDIO_CODING_HVXC,
    AUDIO_CODING_RES       /* reserved */
  } AudioCodingType;

  typedef enum {           /* values for AUDIO_CODING_AAC */
    AUDIO_MODE_MONO = 0,
    AUDIO_MODE_PARAMETRIC_STEREO,
    AUDIO_MODE_STEREO,
    AUDIO_MODE_RES         /* reserved */
  } AudioModeType;

  /* error codes */
  typedef enum {
    DRMENC_OK = 0,
    DRMENC_NOT_INITIALIZED,
    DRMENC_INIT_ERROR,
    DRMENC_UNIMPLEMENTED,
    DRMENC_INTERNAL_ERROR,
    DRMENC_OUT_OF_MEMORY,
    DRMENC_USE_ERROR,
    DRMENC_NO_LICENSE,
    DRMENC_WRONG_HANDLE
  } DrmencError;


  /*****************************************************************************

  functionname: DrmencOpen
  description:  open a DRM encoder
  returns:      an error code
  input:        audioCoding              audio coding (SDC value)
                coderField               coder field  (SDC value)
                coderSamplingRate        sampling rate in Hz (8000, 12000, 16000
                                         or 24000) (translated SDC value)
                audioMode                audio mode   (SDC value)
                stereoInputFlag          audio input: mono (0) or stereo (1)         
                SBROnOffFlag             SBR flag     (SDC value)
                lengthOfAudioSuperFrame  length of audio super frame in bytes
                                         (lenHPP + lenLPP - lenText)
                lengthHigherProtected    length of higher protected part in bytes
  output:       drmEncoder               DRM encoder handle

  *****************************************************************************/

  DRMENC_EXTERN unsigned int DrmencOpen(hDrmEncoder *drmEncoder,
                                        int          audioCoding,
                                        int          coderField,
                                        int          coderSamplingRate,
                                        int          audioMode,
                                        int          stereoInputFlag,
                                        int          SBROnOffFlag,
                                        int          lengthOfAudioSuperFrame,
                                        int          lengthHigherProtected);


  /*****************************************************************************

  functionname: DrmencEncode
  description:  encode one audio frame
  returns:      an error code
  input:        drmEncoder               DRM encoder handle
                inbuffer                 pointer to audio samples
                nSamples                 number of samples
  output:       outbuffer                pointer to bitstream buffer
                nOutputBytes             number of bytes in bitstream buffer

  remarks: - the input sampling rate is always 48000 Hz
           - the number of samples in inbuffer is signaled in nSamples and needs
             to match the number of samples needed to encode one audio frame:

           AAC:

             nSamples | coderSamplingRate | input mode
             ---------+-------------------+----------------------------------
             1920     |       24000       | mono   (stereoInputFlag == 0)
             3840     |       24000       | stereo (stereoInputFlag == 1)
             3840     |       12000       | mono   (stereoInputFlag == 0)
             7680     |       12000       | stereo (stereoInputFlag == 1)

           - every Nth frame: one audio super frame will be returned in outbuffer
             and nOutputBytes is set to the length of the audio super frame
             N=10 for coderSamplingRate==24000, N=5 for coderSamplingRate==12000

  *****************************************************************************/

  DRMENC_EXTERN unsigned int DrmencEncode(hDrmEncoder    drmEncoder,
                                          float         *inbuffer,
                                          int            nSamples,
                                          unsigned char *outbuffer,
                                          unsigned int  *nOutputBytes);


  /*****************************************************************************

  functionname: DrmencGetFrameLength
  description:  get the frame length of the previously encoded audio frame
  returns:      an error code
  input:        drmEncoder          DRM encoder handle
  output:       frameLength         frame length in bits if not NULL
                sensitiveLength     sensitive length in bits if not NULL

  remarks:      - only available for AUDIO_CODING_AAC

  *****************************************************************************/

  DRMENC_EXTERN unsigned int DrmencGetFrameLength(hDrmEncoder  drmEncoder,
                                                  int         *frameLength,
                                                  int         *sensitiveLength);


  /*****************************************************************************

  functionname: DrmencClose
  description:  close a DRM encoder
  returns:      an error code
  input:        drmEncoder               DRM encoder handle
  output:

  *****************************************************************************/

  DRMENC_EXTERN unsigned int DrmencClose(hDrmEncoder drmEncoder);


  /*****************************************************************************

  functionname: DrmencGetLibInfo
  description:  get DRM encoder library info
  returns:
  input:
  output:       libInfo                  library info

  *****************************************************************************/

  DRMENC_EXTERN void DrmencGetLibInfo(DrmencLibInfo *libInfo);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _DRMENCLIB_H */
