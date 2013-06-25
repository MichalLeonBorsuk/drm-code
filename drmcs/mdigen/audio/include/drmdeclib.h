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
/******************************** DRM Audio Decoder ***************************

                       (C) copyright Coding Technologies (2001-2004)
                               All Rights Reserved

  This software module was developed by Coding Technologies.
  This is company confidential information and the property of CT, and can 
  not be reproduced or disclosed in any form without written authorization 
  of CT.

  $Id: drmdeclib.h 126 2006-02-09 11:33:34Z julianc $
  Initial author:       R. Boehm
  contents/description: DRM decoder library

*******************************************************************************/

#ifndef _DRMDECLIB_H
#define _DRMDECLIB_H

#ifndef WIN32
#define DRMDECSTATIC
#endif

#ifndef DRMDECSTATIC
  #ifdef DRMDECDLL_EXPORTS
    #define DRMDEC_EXTERN extern __declspec( dllexport )
  #else
    #define DRMDEC_EXTERN extern __declspec( dllimport )
  #endif
#else
  #define DRMDEC_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef void* hDrmDecoder;

  typedef struct {
    char date[20];
    char versionNo[20];
    int  libID;
  } DrmdecLibInfo;

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
    DRMDEC_OK = 0,
    DRMDEC_PARTLY_OK,
    DRMDEC_NOT_INITIALIZED,
    DRMDEC_INIT_ERROR,
    DRMDEC_UNIMPLEMENTED,
    DRMDEC_INTERNAL_ERROR,
    DRMDEC_OUT_OF_MEMORY,
    DRMDEC_USE_ERROR
  } DrmdecError;

  /* additional decoder parameter */
  typedef enum {
    DRMDEC_PARAM_LIMITER = 0,
    DRMDEC_PARAM_HVXC_SPEED,   /* only HVXC */
    DRMDEC_PARAM_HVXC_PITCH,   /* only HVXC */
    DRMDEC_PARAM_MAX           /* number of available additional decoder parameter */
  } DrmdecParameterType;


  /*****************************************************************************

  functionname: DrmdecOpen
  description:  open a DRM decoder
  returns:      an error code
  input:        audioCoding              audio coding from SDC
                coderField               coder field  from SDC
                coderSamplingRate        sampling rate in Hz (8000, 12000, 16000
                                         or 24000) derived from SDC
                audioMode                audio mode   from SDC
                SBROnOffFlag             SBR flag     from SDC
                lengthOfAudioSuperFrame  length of audio super frame in bytes
                                         (lenHPP + lenLPP - lenText)
                lengthHigherProtected    length of higher protected part in bytes
  output:       drmDecoder               DRM decoder handle

  *****************************************************************************/

  DRMDEC_EXTERN unsigned int DrmdecOpen(hDrmDecoder *drmDecoder,
                                        int          audioCoding,
                                        int          coderField,
                                        int          coderSamplingRate,
                                        int          audioMode,
                                        int          SBROnOffFlag,
                                        int          lengthOfAudioSuperFrame,
                                        int          lengthHigherProtected);

  /*****************************************************************************

  functionname: DrmdecDecode
  description:  decode one audio frame
  returns:      an error code
  input:        drmDecoder               DRM decoder handle
                inbuffer                 pointer to bitstream buffer
                inputBufferSize          number of bytes in bitstream buffer
  output:       outbuffer                pointer to audio samples
                nSamples                 number of samples
                frameStatus              frame status according to RSCI reas TAG

  remarks: - the ouput sampling rate is always 48000 Hz
           - the output mode is always stereo

             coderSamplingRate | nSamples | N (number of audio frames
                               |          |    per audio super frame)
             ------------------+----------+----------------------------------
                   24000       |   3840   | 10
                   12000       |   7680   | 5

           - the first call and than every Nth call: one audio super frame is
             in inbuffer and inputBufferSize is set to the length of the audio
             super frame

  *****************************************************************************/

  DRMDEC_EXTERN unsigned int DrmdecDecode(hDrmDecoder    drmDecoder,
                                          unsigned char *inbuffer,
                                          unsigned int   inputBufferSize,
                                          float         *outbuffer,
                                          int           *nSamples,
                                          int           *frameStatus);

  /*****************************************************************************

  functionname: DrmdecGetFrameLength
  description:  get the frame length of the previously decoded audio frame
  returns:      an error code
  input:        drmDecoder          DRM encoder handle
  output:       frameLength         frame length in bits if not NULL
                sensitiveLength     sensitive length in bits if not NULL

  remarks:      - only available for AUDIO_CODING_AAC

  *****************************************************************************/

  DRMDEC_EXTERN unsigned int DrmdecGetFrameLength(hDrmDecoder  drmDecoder,
                                                  int         *frameLength,
                                                  int         *sensitiveLength);

  /*****************************************************************************

  functionname: DrmdecClose
  description:  close a DRM decoder
  returns:      an error code
  input:        drmDecoder               DRM decoder handle
  output:

  *****************************************************************************/

  DRMDEC_EXTERN unsigned int DrmdecClose(hDrmDecoder drmDecoder);


  /*****************************************************************************

  functionname: DrmdecGetLibInfo
  description:  get DRM decoder library info
  returns:
  input:
  output:       libInfo                  library info

  *****************************************************************************/

  DRMDEC_EXTERN void DrmdecGetLibInfo(DrmdecLibInfo *libInfo);


  /*****************************************************************************

  functionname: DrmdecSetParameter
  description:  set DRM decoder parameter
  returns:      an error code
  input:        drmDecoder               DRM decoder handle
                decParameter             decoder parameter to change
                iValue                   new value for int   parameter
                fValue                   new value for float parameter
  output:

  *****************************************************************************/

  DRMDEC_EXTERN unsigned int DrmdecSetParameter(hDrmDecoder  drmDecoder,
                                                unsigned int decParameter,
                                                int          iValue,
                                                float        fValue);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _DRMDECLIB_H */
